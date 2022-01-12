/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 янв. 2021 г.
 *
 * lsp-plugin-fw is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugin-fw is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugin-fw. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/fmt/json/dom.h>
#include <lsp-plug.in/fmt/xml/PullParser.h>
#include <lsp-plug.in/io/Dir.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/io/PathPattern.h>
#include <lsp-plug.in/io/OutFileStream.h>
#include <lsp-plug.in/io/OutSequence.h>
#include <lsp-plug.in/io/InSequence.h>
#include <lsp-plug.in/io/InFileStream.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/util/common/checksum.h>
#include <lsp-plug.in/plug-fw/util/repository/repository.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/runtime/LSPString.h>


namespace lsp
{
    namespace repository
    {
        typedef struct context_t
        {
            bool                                use_checksums;  // Use checksums
            io::Path                            base;           // Destination directory
            lltl::pphash<LSPString, LSPString>  schema;         // XML files (schemas)
            lltl::pphash<LSPString, LSPString>  ui;             // XML files (UI)
            lltl::pphash<LSPString, LSPString>  preset;         // Preset files
            lltl::pphash<LSPString, json::Node> i18n;           // INternationalization data
            lltl::pphash<LSPString, LSPString>  other;          // Other files
            lltl::pphash<LSPString, LSPString>  local;          // Local files
            lltl::pphash<LSPString, LSPString>  fonts;          // Fonts
            util::checksum_list_t               checksums;      // Checksums
        } context_t;

        typedef struct xml_node_t
        {
            LSPString name;             // Name of XML node
            size_t    attributes;       // Number of attributes
            size_t    children;         // Number of children
        } xml_node_t;

        /**
         * File handler function
         * @param ctx context
         * @param relative relative path to the file
         * @param full full path to the file
         * @return status of operation
         */
        typedef status_t (* file_handler_t)(context_t *ctx, const LSPString *relative, const LSPString *full);

        void drop_paths(lltl::parray<LSPString> *paths)
        {
            for (size_t i=0, n=paths->size(); i<n; ++i)
            {
                LSPString *item = paths->uget(i);
                if (item != NULL)
                    delete item;
            }
            paths->flush();
        }

        void destroy_context(context_t *ctx)
        {
            lltl::parray<LSPString> paths;
            lltl::parray<json::Node> i18n;

            // Drop themes
            ctx->schema.values(&paths);
            ctx->schema.flush();
            drop_paths(&paths);

            // Drop UI files
            ctx->ui.values(&paths);
            ctx->ui.flush();
            drop_paths(&paths);

            // Drop preset files
            ctx->preset.values(&paths);
            ctx->preset.flush();
            drop_paths(&paths);

            // Drop internationalization
            ctx->i18n.values(&i18n);
            ctx->i18n.flush();
            for (size_t i=0, n=i18n.size(); i<n; ++i)
            {
                json::Node *node = i18n.get(i);
                if (node != NULL)
                    delete node;
            }
            i18n.flush();

            // Drop other files
            ctx->other.values(&paths);
            ctx->other.flush();
            drop_paths(&paths);

            // Drop local files
            ctx->local.values(&paths);
            ctx->local.flush();
            drop_paths(&paths);

            // Drop fonts
            ctx->fonts.values(&paths);
            ctx->fonts.flush();
            drop_paths(&paths);

            // Drop checksums
            util::drop_checksums(&ctx->checksums);
        }

        void destroy_nodes(lltl::parray<xml_node_t> *stack)
        {
            for (size_t i=0, n=stack->size(); i<n; ++i)
            {
                xml_node_t *node = stack->uget(i);
                if (node != NULL)
                    delete node;
            }
            stack->flush();
        }

        bool is_xml_file(const io::Path *path)
        {
            LSPString ext;
            if (path->get_ext(&ext) != STATUS_OK)
                return false;
            return ext.equals_ascii_nocase("xml");
        }

        status_t add_unique_file(lltl::pphash<LSPString, LSPString> *dst, const LSPString *relative, const LSPString *full)
        {
            LSPString *path = dst->get(relative);
            if (path != NULL)
            {
                fprintf(stderr,
                        "Resource file '%s' conflicts with '%s', can not proceed",
                        full->get_native(), path->get_native()
                );
                return STATUS_DUPLICATED;
            }

            if ((path = full->clone()) == NULL)
                return STATUS_NO_MEM;

            if (!dst->create(relative, path))
            {
                delete path;
                return STATUS_NO_MEM;
            }

            return STATUS_OK;
        }

        status_t merge_i18n(LSPString *path, json::Node *dst, const json::Node *src, const LSPString *full)
        {
            status_t res;
            lltl::parray<LSPString> fields;
            json::Object sjo = src;
            json::Object djo = dst;

            // Obtain the whole list of fields
            if ((res = sjo.fields(&fields)) != STATUS_OK)
                return res;
            fields.qsort();

            for (size_t i=0, n=fields.size(); i<n; ++i)
            {
                const LSPString *field = fields.uget(i);
                if (field == NULL)
                {
                    fprintf(stderr, "  file '%s': corrupted JSON object\n", full->get_native());
                    return STATUS_BAD_STATE;
                }

                // Form the path of property
                size_t len = path->length();
                if (len > 0)
                {
                    if (!path->append_ascii("->"))
                        return STATUS_NO_MEM;
                }
                if (!path->append(field))
                    return STATUS_NO_MEM;

                json::Node sjn = sjo.get(field);
                if (sjn.is_object())
                {
                    json::Node obj = djo.get(field);
                    if (!obj.is_object())
                    {
                        obj = json::Object::build();
                        if ((res = djo.set(field, &obj)) != STATUS_OK)
                            return res;
                    }
                    if ((res = merge_i18n(path, &obj, &sjn, full)) != STATUS_OK)
                        return res;
                }
                else if (sjn.is_string())
                {
                    if (djo.contains(field))
                    {
                        fprintf(stderr, "  file '%s': overrided property '%s'\n", full->get_native(), path->get_native());
                        return STATUS_CORRUPTED;
                    }
                    if ((res = djo.set(field, sjn)) != STATUS_OK)
                        return res;
                }
                else
                {
                    fprintf(stderr, "  file '%s': unsupported object type '%s'\n", full->get_native(), sjn.stype());
                    return STATUS_BAD_STATE;
                }

                // Truncate the path
                path->set_length(len);
            }

            return STATUS_OK;
        }

        status_t write_xml_tag(io::IOutSequence *os, const LSPString *s)
        {
            LSPString tmp;
            status_t res;

            for (size_t i=0, n=s->length(); i<n; ++i)
            {
                lsp_wchar_t c = s->at(i);

                // Emit character data
                if ((c >= 0) && (c < 0x80))
                {
                    if ((c < 0x20) && (c != '\t'))
                    {
                        if (tmp.fmt_ascii("&#x%02x;", int(c)) < 0)
                            return STATUS_NO_MEM;
                        res = os->write(&tmp);
                    }
                    else
                        res = os->write(c);
                }
                else
                {
                    if (tmp.fmt_ascii("&#x%04x;", int(c)) < 0)
                        return STATUS_NO_MEM;
                    res = os->write(&tmp);
                }

                // Analyze result
                if (res != STATUS_OK)
                    return res;
            }

            return STATUS_OK;
        }

        status_t write_xml_string(io::IOutSequence *os, const LSPString *s)
        {
            LSPString tmp;
            status_t res;

            for (size_t i=0, n=s->length(); i<n; ++i)
            {
                lsp_wchar_t c = s->at(i);

                // Emit character data
                if ((c >= 0) && (c < 0x80))
                {
                    if ((c < 0x20) && (c != '\t'))
                    {
                        if (tmp.fmt_ascii("&#x%02x;", int(c)) < 0)
                            return STATUS_NO_MEM;
                        res = os->write(&tmp);
                    }
                    else
                    {
                        switch (c)
                        {
                            case '\"': res = os->write_ascii("&quot;"); break;
                            //case '\'': res = os->write_ascii("&apos;"); break; // Don't need this at this moment
                            case '<': res = os->write_ascii("&lt;"); break;
                            case '>': res = os->write_ascii("&gt;"); break;
                            case '&': res = os->write_ascii("&amp;"); break;
                            default: res = os->write(c); break;
                        }

                    }
                }
                else
                {
                    if (tmp.fmt_ascii("&#x%04x;", int(c)) < 0)
                        return STATUS_NO_MEM;
                    res = os->write(&tmp);
                }

                // Analyze result
                if (res != STATUS_OK)
                    return res;
            }

            return STATUS_OK;
        }

        status_t do_xml_processing(xml::PullParser *p, io::IOutSequence *os)
        {
            status_t res;
            lltl::parray<xml_node_t> stack;

            do
            {
                // Get next XML element
                if ((res = p->read_next()) < 0)
                {
                    res = -res;
                    break;
                }

                if (res == xml::XT_END_DOCUMENT)
                {
                    res = STATUS_OK;
                    break;
                }

                xml_node_t *curr = (stack.is_empty()) ? NULL : stack.last();

                switch (res)
                {
                    // Skip characters and comments
                    case xml::XT_CHARACTERS:
                    case xml::XT_COMMENT:
                    case xml::XT_DTD:
                    case xml::XT_START_DOCUMENT:
                        res = STATUS_OK;
                        break;

                    case xml::XT_START_ELEMENT:
                    {
                        if (curr != NULL)
                        {
                            // If we meet the first child, we need to end the tag definition
                            if ((curr->children++) <= 0)
                            {
                                if ((res = os->write('>')) != STATUS_OK)
                                    break;
                            }
                        }

                        // Create new node
                        if ((curr = new xml_node_t) == NULL)
                        {
                            res = STATUS_NO_MEM;
                            break;
                        }
                        curr->attributes    = 0;
                        curr->children      = 0;

                        // Add to stack and initialize name
                        if ((!stack.push(curr)) || (!curr->name.set(p->name())))
                        {
                            delete curr;
                            res = STATUS_NO_MEM;
                            break;
                        }

                        // Emit tag name
                        res = os->write_ascii("<");
                        if (res == STATUS_OK)
                            res = write_xml_tag(os, p->name());
                        break;
                    }

                    case xml::XT_END_ELEMENT:
                    {
                        // Check that we are in valid state
                        if (curr == NULL)
                        {
                            res = STATUS_BAD_STATE;
                            break;
                        }

                        // If there were some children, then use long form of tag
                        if (curr->children > 0)
                        {
                            res = os->write_ascii("</");
                            if (res == STATUS_OK)
                                res = write_xml_tag(os, &curr->name);
                            if (res == STATUS_OK)
                                res = os->write('>');
                        }
                        else
                            res = os->write_ascii("/>"); // Use short form

                        if (res != STATUS_OK)
                            break;

                        // Destroy the current node and pop the stack
                        if (!stack.pop())
                        {
                            res = STATUS_NO_MEM;
                            break;
                        }
                        delete curr;

                        break;
                    }

                    case xml::XT_ATTRIBUTE:
                    {
                        // Check that we are in valid state
                        if (curr == NULL)
                        {
                            res = STATUS_BAD_STATE;
                            break;
                        }

                        ++curr->attributes;

                        // Emit the attribute
                        res = os->write(' ');
                        if (res == STATUS_OK)
                            res = write_xml_tag(os, p->name());
                        if (res == STATUS_OK)
                            res = os->write_ascii("=\"");
                        if (res == STATUS_OK)
                            res = write_xml_string(os, p->value());
                        if (res == STATUS_OK)
                            res = os->write('\"');
                        break;
                    }

                    default:
                        res = STATUS_CORRUPTED;
                        break;
                }
            } while (res == STATUS_OK);

            // Destroy stack
            destroy_nodes(&stack);

            return res;
        }

        status_t preprocess_xml(const LSPString *src, const io::Path *dst)
        {
            status_t res, res2;
            xml::PullParser p;
            io::OutFileStream ofs;
            io::OutSequence os;

            // Open XML file
            if ((res = p.open(src)) == STATUS_OK)
            {
                if ((res = ofs.open(dst, io::File::FM_WRITE_NEW)) == STATUS_OK)
                {
                    if ((res = os.wrap(&ofs, false, "UTF-8") == STATUS_OK))
                    {
                        // Do XML processing
                        res = do_xml_processing(&p, &os);
                    }

                    // Close output sequence
                    res2 = os.close();
                    if (res == STATUS_OK)
                        res = res2;
                }

                // Close output file
                res2 = ofs.close();
                if (res == STATUS_OK)
                    res = res2;
            }

            // Close XML file
            res2 = p.close();
            if (res == STATUS_OK)
                res = res2;

            return res;
        }

        status_t schema_handler(context_t *ctx, const LSPString *relative, const LSPString *full)
        {
            return add_unique_file(&ctx->schema, relative, full);
        }

        status_t ui_handler(context_t *ctx, const LSPString *relative, const LSPString *full)
        {
            return add_unique_file(&ctx->ui, relative, full);
        }

        status_t preset_handler(context_t *ctx, const LSPString *relative, const LSPString *full)
        {
            return add_unique_file(&ctx->preset, relative, full);
        }

        status_t other_handler(context_t *ctx, const LSPString *relative, const LSPString *full)
        {
            return add_unique_file(&ctx->other, relative, full);
        }

        status_t font_handler(context_t *ctx, const LSPString *relative, const LSPString *full)
        {
            return add_unique_file(&ctx->fonts, relative, full);
        }

        status_t local_handler(context_t *ctx, const LSPString *relative, const LSPString *full)
        {
            return add_unique_file(&ctx->local, relative, full);
        }

        status_t i18n_handler(context_t *ctx, const LSPString *relative, const LSPString *full)
        {
            status_t res;
            LSPString path;
            json::Node *node = new json::Node();
            if (node == NULL)
                return STATUS_NO_MEM;

            if ((res = json::dom_load(full, node, json::JSON_LEGACY, "UTF-8")) != STATUS_OK)
            {
                fprintf(stderr, "  file '%s': failed to load, error code=%d\n", full->get_native(), int(res));
                delete node;
                return res;
            }

            if (!node->is_object())
            {
                fprintf(stderr, "  file '%s': not a JSON object\n", full->get_native());
                delete node;
                return STATUS_CORRUPTED;
            }

            // Check that node previously existed
            json::Node *dst = ctx->i18n.get(relative);
            if (dst == NULL)
            {
                // Create new entry
                res = (ctx->i18n.create(relative, node)) ? STATUS_OK : STATUS_NO_MEM;
                if (res != STATUS_OK)
                    delete node;
            }
            else
            {
                // Merge node and delete just loaded data
                res = merge_i18n(&path, dst, node, full);
                delete node;
            }

            return res;
        }

        status_t scan_files(
            const io::Path *base, const LSPString *child, const io::PathPattern *pattern,
            context_t *ctx, file_handler_t handler
        )
        {
            status_t res;
            io::Path path, rel, full;
            io::Dir fd;
            io::fattr_t fa;
            LSPString item, *subdir;
            lltl::parray<LSPString> subdirs;

            // Open directory
            if ((res = path.set(base, child)) != STATUS_OK)
                return res;
            printf("Scan files: %s", path.as_native());
            if ((res = fd.open(&path)) != STATUS_OK)
                return STATUS_OK;

            while (true)
            {
                // Read directory entry
                if ((res = fd.reads(&item, &fa)) != STATUS_OK)
                {
                    // Close directory
                    fd.close();

                    // Analyze status
                    if (res != STATUS_EOF)
                    {
                        drop_paths(&subdirs);
                        fprintf(stderr, "Could not read directory entry for %s\n", path.as_native());
                        return res;
                    }
                    break;
                }

                // This should be a directory and not dots
                if (fa.type == io::fattr_t::FT_DIRECTORY)
                {
                    // Add child directory to list
                    if (!io::Path::is_dots(&item))
                    {
                        // Build directory paths
                        if ((res = rel.set(child, &item)) != STATUS_OK)
                            return res;

                        // Create a copy
                        subdir = rel.as_string()->clone();
                        if (subdir == NULL)
                        {
                            drop_paths(&subdirs);
                            return res;
                        }

                        // Add to list
                        if (!subdirs.add(subdir))
                        {
                            delete subdir;
                            drop_paths(&subdirs);
                            return res;
                        }

//                        printf("  found  dir: %s\n", rel.as_native());
                    }
                }
                else if (pattern->test(&item))
                {
                    // Build file paths
                    if ((res = rel.set(child, &item)) != STATUS_OK)
                        return res;
                    if ((res = full.set(base, &rel)) != STATUS_OK)
                        return res;

                    // Handle the file
                    //printf("  found file: %s\n", rel.as_native());
                    if ((res = handler(ctx, rel.as_string(), full.as_string())) != STATUS_OK)
                        return res;
                }
            }

            // Process each sub-directory
            for (size_t i=0, n=subdirs.size(); i<n; ++i)
            {
                subdir = subdirs.uget(i);
                if ((res = scan_files(base, subdir, pattern, ctx, handler)) != STATUS_OK)
                {
                    drop_paths(&subdirs);
                    return res;
                }
            }

            // Drop sub-directories
            drop_paths(&subdirs);

            return STATUS_OK;
        }

        status_t scan_files(
            const io::Path *base, const LSPString *child, const char *pattern,
            context_t *ctx, file_handler_t handler
        )
        {
            status_t res;
            io::PathPattern pat;

            if ((res = pat.set(pattern)) != STATUS_OK)
                return res;

            return scan_files(base, child, &pat, ctx, handler);
        }

        status_t scan_resources(const LSPString *path, context_t *ctx)
        {
            status_t res;
            LSPString child;

            // Compute the base path
            io::Path base, full;
            if ((res = base.set(path, "res/main")) != STATUS_OK)
                return res;

            // Lookup for specific resources
            io::Dir dir;
            if ((res = dir.open(&base)) != STATUS_OK)
                return STATUS_OK;

            while ((res = dir.read(&child)) == STATUS_OK)
            {
                if (io::Path::is_dots(&child))
                    continue;

                if (child.equals_ascii("schema"))
                    res = scan_files(&base, &child, "*.xml", ctx, schema_handler);
                else if (child.equals_ascii("ui"))
                    res = scan_files(&base, &child, "*.xml", ctx, ui_handler);
                else if (child.equals_ascii("preset"))
                    res = scan_files(&base, &child, "*.preset", ctx, preset_handler);
                else if (child.equals_ascii("i18n"))
                    res = scan_files(&base, &child, "*.json", ctx, i18n_handler);
                else if (child.equals_ascii("fonts"))
                    res = scan_files(&base, &child, "*", ctx, font_handler);
                else
                    res = scan_files(&base, &child, "*", ctx, other_handler);

                if (res != STATUS_OK)
                    return res;
            }
            if (res == STATUS_EOF)
                res     = STATUS_OK;

            ssize_t xres = dir.close();
            return (res != STATUS_OK) ? res : xres;
        }

        status_t scan_local_files(const char *path, context_t *ctx)
        {
            status_t res;
            io::Path base;
            LSPString child;

            if (path == NULL)
                return STATUS_OK;

            if ((res = base.set(path)) != STATUS_OK)
                return res;

            return scan_files(&base, &child, "*", ctx, local_handler);
        }

        status_t lookup_path(const char *path, context_t *ctx)
        {
            status_t res;
            io::Dir fd;
            io::Path dir, resdir;
            io::PathPattern pat;
            io::fattr_t fa;
            LSPString pattern, item, resource, *respath;
            lltl::parray<LSPString> matched;

            // Prepare search structures
            if ((res = dir.set(path)) != STATUS_OK)
                return res;
            if ((res = dir.canonicalize()) != STATUS_OK)
                return res;
            if ((res = dir.get_last(&pattern)) != STATUS_OK)
                return res;

            // Check if there is no pattern
            if (pattern.length() <= 0)
            {
                printf("Scanning directory '%s' for resources\n", dir.as_native());
                if ((res = scan_resources(dir.as_string(), ctx)) != STATUS_OK)
                    return res;
                return STATUS_OK;
            }

            // Update path and pattern
            if ((res = dir.remove_last()) != STATUS_OK)
                return res;
            if ((res = pat.set(&pattern)) != STATUS_OK)
                return res;

            // Scan directory for entries
            if ((res = fd.open(&dir)) != STATUS_OK)
                return res;

            while (true)
            {
                // Read directory entry
                if ((res = fd.reads(&item, &fa)) != STATUS_OK)
                {
                    // Close directory
                    fd.close();

                    // Analyze status
                    if (res != STATUS_EOF)
                    {
                        drop_paths(&matched);
                        fprintf(stderr, "Could not read directory entry for %s\n", dir.as_native());
                        return res;
                    }
                    break;
                }

                // This should be a directory and not dots
                if ((fa.type != io::fattr_t::FT_DIRECTORY) ||
                    (io::Path::is_dots(&item)))
                    continue;

                // Match the directory to the pattern
                if (!pat.test(&item))
                    continue;

                // We found a child resource directory, add to list
                if ((res = resdir.set(&dir, &item)) != STATUS_OK)
                {
                    drop_paths(&matched);
                    return res;
                }

                // Create a copy
                respath = resdir.as_string()->clone();
                if (respath == NULL)
                {
                    drop_paths(&matched);
                    return res;
                }

                // Add to list
                if (!matched.add(respath))
                {
                    delete respath;
                    drop_paths(&matched);
                    return res;
                }
            }

            // Now we have the full list of matched directories
            for (size_t i=0, n=matched.size(); i<n; ++i)
            {
                respath = matched.uget(i);
                printf("Scanning directory '%s' for resources\n", respath->get_native());
                if ((res = scan_resources(respath, ctx)) != STATUS_OK)
                {
                    drop_paths(&matched);
                    return res;
                }
            }

            drop_paths(&matched);

            return STATUS_OK;
        }

        status_t export_files(bool strict, context_t *ctx, lltl::pphash<LSPString, LSPString> *files)
        {
            io::Path df;
            status_t res = STATUS_OK;
            wssize_t nbytes;
            lltl::parray<LSPString> flist;

            if (!files->keys(&flist))
                return STATUS_NO_MEM;

            flist.qsort();

            for (size_t i=0, n=flist.size(); i<n; ++i)
            {
                const LSPString *name = flist.uget(i);
                const LSPString *source = files->get(name);

                if ((name == NULL) || (source == NULL))
                    return STATUS_BAD_STATE;
                if ((res = df.set(&ctx->base, name)) != STATUS_OK)
                    return res;

                printf("  copying %s -> %s\n", source->get_native(), df.as_native());

                if ((res = update_status(res, df.mkparent(true))) != STATUS_OK)
                {
                    fprintf(stderr, "Could not create directory for file: %s\n", df.as_native());
                    if (!strict)
                        continue;
                    return res;
                }

                if (is_xml_file(&df))
                {
                    // Preprocess XML file
                    if ((res = update_status(res, preprocess_xml(source, &df))) != STATUS_OK)
                    {
                        fprintf(stderr, "Error preprocessing XML file '%s', error: %d\n", df.as_native(), int(res));
                        if (!strict)
                            continue;
                        return res;
                    }
                }
                else
                {
                    // Copy file
                    if ((nbytes = io::File::copy(source, &df)) < 0)
                    {
                        fprintf(stderr, "Could not create file: %s, error: %d\n", df.as_native(), int(-nbytes));
                        if (!strict)
                        {
                            res = update_status(res, -nbytes);
                            continue;
                        }
                        return -nbytes;
                    }
                }

                // Compute checksum of the output file
                if (ctx->use_checksums)
                {
                    if ((res = util::add_checksum(&ctx->checksums, &ctx->base, &df)) != STATUS_OK)
                    {
                        fprintf(stderr, "Could not compute file checksum: %s, error: %d\n", df.as_native(), int(res));
                        return res;
                    }
                }
            }

            return res;
        }

        status_t export_i18n(bool strict, context_t *ctx, lltl::pphash<LSPString, json::Node> *files)
        {
            io::Path df;
            status_t res = STATUS_OK;
            lltl::parray<LSPString> flist;
            json::serial_flags_t settings;

            if (!files->keys(&flist))
                return STATUS_NO_MEM;

            flist.qsort();

            json::init_serial_flags(&settings);
            settings.version    = json::JSON_LEGACY;
            settings.ident      = '\t';
            settings.padding    = 0;
            settings.separator  = false;
            settings.multiline  = false;

            for (size_t i=0, n=flist.size(); i<n; ++i)
            {
                const LSPString *name = flist.uget(i);
                const json::Node *node = files->get(name);

                if ((name == NULL) || (node == NULL))
                    return STATUS_BAD_STATE;
                if ((res = df.set(&ctx->base, name)) != STATUS_OK)
                    return res;

                printf("  writing i18n file %s\n", df.as_native());

                if ((res = df.mkparent(true)) != STATUS_OK)
                {
                    fprintf(stderr, "Could not create directory for file: %s\n", df.as_native());
                    if (!strict)
                        continue;
                    return res;
                }

                // Save file
                if ((res = json::dom_save(&df, node, &settings, "UTF-8")) != STATUS_OK)
                {
                    fprintf(stderr, "Could not write file: %s\n", df.as_native());
                    if (!strict)
                        continue;
                    return res;
                }

                // Compute checksum
                if (ctx->use_checksums)
                {
                    if ((res = util::add_checksum(&ctx->checksums, &ctx->base, &df)) != STATUS_OK)
                    {
                        fprintf(stderr, "Could not compute file checksum: %s, error: %d\n", df.as_native(), int(res));
                        if (!strict)
                            continue;
                        return res;
                    }
                }
            }

            return STATUS_OK;
        }

        status_t process_line(LSPString *out, LSPString *in, lltl::pphash<LSPString, LSPString> *vars)
        {
            ssize_t first = 0, last = in->length();
            LSPString key;

            out->clear();

            while (first < last)
            {
                // Find the '$' match
                ssize_t idx1 = in->index_of(first, '$');
                if (idx1 < 0)
                    break;
                if ((++idx1) >= last)
                    break;

                // Sequence should match '${'
                if (in->char_at(idx1) != '{')
                {
                    if (!out->append('$'))
                        return STATUS_NO_MEM;
                    ++first;
                    continue;
                }

                // Find the enclosing bracket
                ssize_t idx2 = in->index_of(++idx1, '}');
                if (idx2 < 0)
                    break;

                // Append variable value
                if (!key.set(in, idx1, idx2))
                    return STATUS_NO_MEM;
                if (!out->append(in, first, idx1 - 2))
                    return STATUS_NO_MEM;

                const LSPString *value = vars->get(&key);
                if (value == NULL)
                {
                    if (!out->append(in, idx1, idx2))
                        return STATUS_NO_MEM;
                }
                else if (!out->append(value))
                    return STATUS_NO_MEM;

                // Update index of sequence
                first = idx2 + 1;
            }

            if (first < last)
            {
                if (!out->append(in, first, last))
                    return STATUS_NO_MEM;
            }

            return STATUS_OK;
        }

        status_t make_vars(lltl::pphash<LSPString, LSPString> *vars, const cmdline_t *cmd)
        {
            LSPString k, v;

            for (size_t i=0, n=cmd->vars.size(); i<n; ++i)
            {
                v.clear();

                const char *def = cmd->vars.uget(i);
                const char *value = strchr(def, '=');

                if (value)
                {
                    if (!k.set_native(def, value - def))
                        return STATUS_NO_MEM;
                    if (!v.set_native(value + 1))
                        return STATUS_NO_MEM;
                }
                else if (!k.set_native(def))
                    return STATUS_NO_MEM;

                LSPString *xv = v.clone();
                if (value == NULL)
                    return STATUS_NO_MEM;
                if (!vars->create(&k, xv))
                {
                    delete xv;
                    fprintf(stderr, "Duplicate variable: %s\n", k.get_native());
                    return STATUS_DUPLICATED;
                }
            }

            return STATUS_OK;
        }

        void destroy_vars(lltl::pphash<LSPString, LSPString> *vars)
        {
            lltl::parray<LSPString> v;
            if (vars->values(&v))
            {
                for (size_t i=0, n=v.size(); i<n; ++i)
                {
                    LSPString *value = v.uget(i);
                    delete value;
                }
                v.flush();
            }
            vars->flush();
        }

        status_t process_manifest_file(context_t *ctx, const cmdline_t *cmd)
        {
            if (!cmd->manifest)
                return STATUS_OK;

            status_t res;
            LSPString fname;
            io::Path src, dst;

            // Initialize paths
            if ((res = src.set(cmd->manifest)) != STATUS_OK)
                return res;
            if ((res = src.get_last(&fname)) != STATUS_OK)
                return res;
            if ((res = dst.set(cmd->dst_dir, &fname)) != STATUS_OK)
                return res;

            io::InFileStream ifs;
            io::OutFileStream ofs;
            io::InSequence is;
            io::OutSequence os;

            // Form the list of variables for substitution
            lltl::pphash<LSPString, LSPString> vars;
            if ((res = make_vars(&vars, cmd)) != STATUS_OK)
            {
                destroy_vars(&vars);
                return res;
            }

            // Process the manifest file
            if ((res = ifs.open(&src)) == STATUS_OK)
            {
                if ((res = is.wrap(&ifs, WRAP_NONE, "utf-8")) == STATUS_OK)
                {
                    if ((res = ofs.open(&dst, io::File::FM_WRITE_NEW)) == STATUS_OK)
                    {
                        if ((res = os.wrap(&ofs, WRAP_NONE, "utf-8")) == STATUS_OK)
                        {
                            printf("  writing manifest: %s\n", dst.as_native());

                            LSPString in_line, out_line;

                            while (true)
                            {
                                if ((res = is.read_line(&in_line, true)) != STATUS_OK)
                                {
                                    if (res == STATUS_EOF)
                                        res = STATUS_OK;
                                    else
                                        fprintf(stderr, "Error reading manifest template: error code=%d", int(res));
                                    break;
                                }

                                if ((res = process_line(&out_line, &in_line, &vars)) != STATUS_OK)
                                {
                                    fprintf(stderr, "Error processing manifest file: error code=%d", int(res));
                                    break;
                                }

                                if ((res = os.write(&out_line)) != STATUS_OK)
                                {
                                    fprintf(stderr, "Error writing manifest file: error code=%d", int(res));
                                    break;
                                }
                            }

                            res = update_status(res, os.close());
                        }
                        res = update_status(res, ofs.close());
                    }
                    res = update_status(res, is.close());
                }

                res = update_status(res, ifs.close());
            }

            destroy_vars(&vars);

            return res;
        }

        status_t make_repository(const cmdline_t *cmd)
        {
            context_t ctx;
            status_t res;
            system::time_t ts, te;

            system::get_time(&ts);

            // Parse basic parameters
            ctx.use_checksums = cmd->checksums != NULL;
            if ((res = ctx.base.set_native(cmd->dst_dir)) != STATUS_OK)
            {
                fprintf(stderr, "Could not parse path: %s, error code=%d\n", cmd->dst_dir, int(res));
                return res;
            }

            // Scan local files
            if ((res = scan_local_files(cmd->local_dir, &ctx)) != STATUS_OK)
            {
                // Destroy context and return error
                destroy_context(&ctx);
                return res;
            }

            // Scan for resource path
            for (size_t i=0, n=cmd->paths.size(); i<n; ++i)
            {
                if ((res = lookup_path(cmd->paths.uget(i), &ctx)) != STATUS_OK)
                {
                    // Destroy context and return error
                    destroy_context(&ctx);
                    return res;
                }
            }

            // Export all resources
            printf("Generating resource tree\n");

            bool strict = cmd->strict;
            if ((res == STATUS_OK) || (!strict))
                res = update_status(res, export_files(strict, &ctx, &ctx.schema));
            if ((res == STATUS_OK) || (!strict))
                res = update_status(res, export_files(strict, &ctx, &ctx.ui));
            if ((res == STATUS_OK) || (!strict))
                res = update_status(res, export_files(strict, &ctx, &ctx.preset));
            if ((res == STATUS_OK) || (!strict))
                res = update_status(res, export_files(strict, &ctx, &ctx.other));
            if ((res == STATUS_OK) || (!strict))
                res = update_status(res, export_i18n(strict, &ctx, &ctx.i18n));
            if ((res == STATUS_OK) || (!strict))
                res = update_status(res, export_files(strict, &ctx, &ctx.local));
            if ((res == STATUS_OK) || (!strict))
                res = update_status(res, export_files(strict, &ctx, &ctx.fonts));
            if ((res == STATUS_OK) || (!strict))
                res = process_manifest_file(&ctx, cmd);

            // Export checksums if specified
            if (cmd->checksums)
            {
                io::Path path;
                if ((res = path.set_native(cmd->checksums)) != STATUS_OK)
                    return res;
                printf("Writing checksums to file: %s\n", path.as_native());
                if ((res = util::save_checksums(&ctx.checksums, &path)) != STATUS_OK)
                {
                    fprintf(stderr, "Could not write checksum file: %s\n", path.as_native());
                    return res;
                }
            }

            // Destroy context
            destroy_context(&ctx);

            system::get_time(&te);
            printf("Execution time: %.2f s\n", (te.seconds + te.nanos * 1e-9) - (ts.seconds + ts.nanos * 1e-9));

            return res;
        }

        status_t parse_cmdline(cmdline_t *cfg, int argc, const char **argv)
        {
            cfg->strict     = true;
            cfg->dst_dir    = NULL;
            cfg->local_dir  = NULL;
            cfg->manifest   = NULL;
            cfg->checksums  = NULL;

            // Parse arguments
            int i = 1;
            bool strict_set = false;

            while (i < argc)
            {
                const char *arg = argv[i++];
                if ((!::strcmp(arg, "--help")) || (!::strcmp(arg, "-h")))
                {
                    printf("Usage: %s [parameters] [resource-directories]\n\n", argv[0]);
                    printf("Available parameters:\n");
                    printf("  -c, --checksums <file>        Write file checksums to the specified file\n");
                    printf("  -d, --define <key>=<value>    Define variable for manifest\n");
                    printf("  -h, --help                    Show help\n");
                    printf("  -l, --local <dir>             The local resource directory\n");
                    printf("  -m, --manifest <file>         Manifest file\n");
                    printf("  -ns, --no-strict              Disable strict processing\n");
                    printf("  -o, --output <dir>            The name of the destination directory to\n");
                    printf("                                place resources\n");
                    printf("\n");

                    return STATUS_CANCELLED;
                }
                else if ((!::strcmp(arg, "--output")) || (!::strcmp(arg, "-o")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified directory name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->dst_dir)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->dst_dir = argv[i++];
                }
                else if ((!::strcmp(arg, "--local")) || (!::strcmp(arg, "-l")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified directory name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->local_dir)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->local_dir = argv[i++];
                }
                else if ((!::strcmp(arg, "--manifest")) || (!::strcmp(arg, "-m")))
                {
                    if (cfg->manifest)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->manifest = argv[i++];
                }
                else if ((!::strcmp(arg, "--strict")) || (!::strcmp(arg, "-s")))
                {
                    if (strict_set)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->strict = true;
                }
                else if ((!::strcmp(arg, "--no-strict")) || (!::strcmp(arg, "-ns")))
                {
                    if (strict_set)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->strict = false;
                }
                else if ((!::strcmp(arg, "--define")) || (!::strcmp(arg, "-d")))
                {
                    if (!cfg->vars.add(const_cast<char *>(arg)))
                        return STATUS_NO_MEM;
                }
                else if ((!::strcmp(arg, "--checksums")) || (!::strcmp(arg, "-c")))
                {
                    if (cfg->checksums)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->checksums = argv[i++];
                }
                else
                {
                    if (!cfg->paths.add(const_cast<char *>(arg)))
                        return STATUS_NO_MEM;
                }
            }

            // Validate mandatory arguments
            if (cfg->dst_dir == NULL)
            {
                fprintf(stderr, "Output directory name is required\n");
                return STATUS_BAD_ARGUMENTS;
            }

            return STATUS_OK;
        }

        int main(int argc, const char **argv)
        {
            // Parse command-line arguments
            cmdline_t cmd;
            status_t res = parse_cmdline(&cmd, argc, argv);
            if (res != STATUS_OK)
                return res;

            // Launch the processing
            res = make_repository(&cmd);
            if (res != lsp::STATUS_OK)
                fprintf(stderr, "Error while generating resource files, code=%d\n", int(res));

            return res;
        }
    } /* namespace repository */
} /* namespace lsp */

