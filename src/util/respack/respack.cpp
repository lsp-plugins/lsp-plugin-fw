/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 дек. 2021 г.
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
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/io/Dir.h>
#include <lsp-plug.in/io/IOutStream.h>
#include <lsp-plug.in/io/InFileStream.h>
#include <lsp-plug.in/resource/Compressor.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/util/common/checksum.h>
#include <lsp-plug.in/plug-fw/util/respack/respack.h>
#include <lsp-plug.in/runtime/system.h>

namespace lsp
{
    namespace respack
    {
        class OutFileStream: public io::IOutStream
        {
            protected:
                FILE       *pOut;
                size_t      nTotal;

            public:
                explicit OutFileStream(FILE *fd)
                {
                    pOut        = fd;
                    nTotal      = 0;
                }

                virtual ~OutFileStream()
                {
                    pOut        = NULL;
                    nTotal      = 0;
                }

            protected:
                void    out_byte(uint8_t b)
                {
                    if (nTotal > 0)
                        fputs(", ", pOut);
                    if (!(nTotal & 0xf))
                        fputs("\n\t\t", pOut);

                    fprintf(pOut, "0x%02x", b);
                    ++nTotal;
                }

            public:
                virtual ssize_t write(const void *buf, size_t count)
                {
                    const uint8_t *p = reinterpret_cast<const uint8_t *>(buf);

                    for (size_t i=0; i<count; ++i)
                        out_byte(p[i]);

                    return count;
                }

                inline size_t   total() const { return nTotal; }
        };

        class state_t
        {
            public:
                const cmdline_t        *cfg;
                lltl::pphash<LSPString, lltl::parray<io::Path> > ext;   // Files grouped by extension
                resource::Compressor    c;                              // Resource compressor
                FILE                   *fd;                             // File descriptor
                OutFileStream          *os;                             // Output stream
                wssize_t                in_bytes;                       // Overall size of input data


            public:
                explicit inline state_t(const cmdline_t *cmd)
                {
                    cfg         = cmd;
                    fd          = NULL;
                    os          = NULL;
                    in_bytes    = 0;
                }

                ~state_t()
                {
                }
        };

        void drop_context(state_t *ctx)
        {
            lltl::parray<lltl::parray<io::Path> > vp;
            ctx->ext.values(&vp);

            // Destroy compressor
            ctx->c.close();
            if (ctx->os != NULL)
            {
                ctx->os->close();
                delete ctx->os;
                ctx->os     = NULL;
            }

            // Close file descriptor
            if (ctx->fd != NULL)
            {
                fclose(ctx->fd);
                ctx->fd     = NULL;
            }

            // Destroy items
            for (size_t i=0, n=vp.size(); i<n; ++i)
            {
                lltl::parray<io::Path> *pa = vp.uget(i);
                if (pa == NULL)
                    continue;

                for (size_t j=0, m=pa->size(); j<m; ++j)
                {
                    io::Path *p = pa->uget(j);
                    if (p != NULL)
                        delete p;
                }

                pa->flush();
                delete pa;
            }

            vp.flush();
            ctx->ext.flush();

            // Drop context object
            delete ctx;
        }

        status_t add_file(state_t *ctx, const io::Path *file)
        {
            LSPString ext;
            status_t res;

            // Get extension
            if ((res = file->get_ext(&ext)) != STATUS_OK)
                return res;
            ext.tolower();

            // Create the list of files
            lltl::parray<io::Path> *pa = ctx->ext.get(&ext);
            if (pa == NULL)
            {
                pa  = new lltl::parray<io::Path>();
                if (pa == NULL)
                    return STATUS_NO_MEM;
                if (!ctx->ext.create(&ext, pa))
                    return STATUS_NO_MEM;
            }

            // Add the path to list
            io::Path *item = file->clone();
            if (item == NULL)
                return STATUS_NO_MEM;

            if (pa->add(item))
                return STATUS_OK;

            delete item;
            return STATUS_NO_MEM;
        }

        bool pattern_matches(const lltl::parray<io::PathPattern> *list, const io::Path *path)
        {
            lltl::parray<io::PathPattern> *vlist = const_cast<lltl::parray<io::PathPattern> *>(list);
            for (lltl::iterator<io::PathPattern> it = vlist->values(); it; ++it)
            {
                io::PathPattern *pp = it.get();
                if (pp == NULL)
                    continue;

                if (pp->test(path))
                    return true;
            }
            return false;
        }

        status_t scan_directory(state_t *ctx, const io::Path *base, const io::Path *p)
        {
            io::Dir d;
            io::Path child;
            LSPString item;
            status_t res;

            if ((res = d.open(p)) != STATUS_OK)
                return res;

            while ((res = d.read(&item)) == STATUS_OK)
            {
                // Skip dots
                if (io::Path::is_dots(&item))
                    continue;

                // Set path
                if ((res = child.set(p, &item)) != STATUS_OK)
                    return res;

                if (child.is_dir())
                {
                    if ((res = scan_directory(ctx, base, &child)) != STATUS_OK)
                        return res;
                }
                else
                {
                    if ((res = child.remove_base(base)) != STATUS_OK)
                        return res;

                    // Check that exclude matches
                    if (!pattern_matches(&ctx->cfg->exclude, &child))
                    {
                        printf("  found file: %s\n", child.as_native());
                        if ((res = add_file(ctx, &child)) != STATUS_OK)
                            return res;
                    }
                }
            }

            // Close directory
            if (res != STATUS_EOF)
                d.close();
            else
                res = d.close();

            return res;
        }

        status_t create_resource_file(state_t *ctx, const char *dst)
        {
            FILE *fd;

            // Open file
            if ((fd = fopen(dst, "w+")) == NULL)
            {
                fprintf(stderr, "  Could not open file: %s\n", dst);
                return STATUS_IO_ERROR;
            }

            // Wrap file descriptor
            OutFileStream *os = new OutFileStream(fd);
            if (os == NULL)
                return STATUS_NO_MEM;

            // Save stream to context
            ctx->os     = os;
            ctx->fd     = fd;

            // Write some data
            fprintf(fd, "/*\n");
            fprintf(fd, " * Resource file, automatically generated, do not edit\n");
            fprintf(fd, " */\n\n");

            fprintf(fd, "#include <lsp-plug.in/common/types.h>\n");
            fprintf(fd, "#include <lsp-plug.in/resource/types.h>\n");
            fprintf(fd, "#include <lsp-plug.in/plug-fw/core/Resources.h>\n");
            fprintf(fd, "\n");

            // Anonimous namespace start
            fprintf(fd, "namespace\n");
            fprintf(fd, "{\n\n");
            fprintf(fd, "\tusing namespace lsp::resource;\n");
            fprintf(fd, "\tusing namespace lsp::core;\n\n");
            fprintf(fd, "\tstatic const uint8_t data[] =\n");
            fprintf(fd, "\t{");

            // Initialize compressor
            return ctx->c.init(LSP_RESOURCE_BUFSZ, ctx->os);
        }

        status_t compress_data(state_t *ctx, const io::Path *base)
        {
            io::Path path;
            io::InFileStream ifs;
            status_t res;
            wssize_t bytes;
            lltl::parray<LSPString> vk;
            if (!ctx->ext.keys(&vk))
                return STATUS_NO_MEM;
            vk.qsort();

            for (size_t i=0, n=vk.size(); i<n; ++i)
            {
                // Fetch the sorted list of files
                const LSPString *ext = vk.uget(i);
                if (ext == NULL)
                    return STATUS_CORRUPTED;

                lltl::parray<io::Path> *vp = ctx->ext.get(ext);
                if (vp == NULL)
                    return STATUS_CORRUPTED;

                vp->qsort();

                // Compress the data
                for (size_t j=0, m=vp->size(); j<m; ++j)
                {
                    io::Path *p = vp->uget(j);
                    if (p == NULL)
                        return STATUS_CORRUPTED;
                    if ((res = path.set(base, p)) != STATUS_OK)
                        return res;

                    // Open the source file
                    if ((res = ifs.open(&path)) != STATUS_OK)
                    {
                        fprintf(stderr, "Could not open file: %s\n", path.as_native());
                        return res;
                    }

                    // Compress the file
                    printf("  compressing file: %s\n", path.as_native());
                    if ((bytes = ctx->c.create_file(p, &ifs)) < 0)
                    {
                        fprintf(stderr, "Error compressing file: %s, code=%d\n",
                                path.as_native(), int(-bytes));
                        return -bytes;
                    }
                    ctx->in_bytes  += bytes;

                    // Close source file
                    if ((res = ifs.close()) != STATUS_OK)
                        return res;
                }

                // Flush the compressor
                if ((res = ctx->c.flush()) != STATUS_OK)
                    return res;
            }

            // Output statistics
            printf("Input size: %ld, compressed size: %ld, compression ratio: %.2f\n",
                    long(ctx->in_bytes), long(ctx->os->total()),
                    double(ctx->in_bytes) / double(ctx->os->total())
            );

            return STATUS_OK;
        }

        status_t write_entries(state_t *ctx)
        {
            FILE *fd = ctx->fd;

            // End of data[] array
            fprintf(fd, "\n\t};\n\n");

            // Start the entries description
            fprintf(fd, "\tstatic const raw_resource_t entries[] =\n");
            fprintf(fd, "\t{\n");

            const resource::raw_resource_t *vent = ctx->c.entries();
            for (size_t i=0, n=ctx->c.num_entires(); i<n; ++i)
            {
                const resource::raw_resource_t *r = &vent[i];

                if (i > 0)
                    fprintf(fd, ",\n");
                fprintf(fd, "\t\t/* %4d */ { %s, \"%s\", %ld, %ld, %ld, %ld }",
                        int(i),
                        (r->type == resource::RES_DIR) ? "RES_DIR" : "RES_FILE",
                        r->name, long(r->parent), long(r->segment), long(r->offset),
                        long(r->length)
                );
            }

            // End of entries[] array
            fprintf(fd, "\n\t};\n\n");

            // Emit resource factory
            fprintf(fd, "\tResources builtin(data, %ld, entries, %ld);\n\n",
                    long(ctx->os->total()),
                    long(ctx->c.num_entires())
                );

            // End of anonymous namespace
            fprintf(fd, "}\n");
            fflush(fd);
            fclose(fd);
            ctx->fd = NULL;

            return STATUS_OK;
        }

        status_t validate_checksums(const cmdline_t *cfg, state_t *ctx)
        {
            status_t res;
            util::checksum_t *cks;
            io::Path file, cksum, src_dir;

            if ((cfg->checksums == NULL) || (cfg->dst_file == NULL))
                return STATUS_OK;

            if ((res = file.set_native(cfg->dst_file)) != STATUS_OK)
                return STATUS_OK;
            if ((res = cksum.set_native(cfg->checksums)) != STATUS_OK)
                return STATUS_OK;
            if ((res = src_dir.set_native(cfg->src_dir)) != STATUS_OK)
                return STATUS_OK;

            if ((!file.is_reg()) || (!cksum.is_reg()))
                return STATUS_OK;

            // Now read checksum file
            util::checksum_list_t ck;
            if ((res = util::read_checksums(&ck, &cksum)) != STATUS_OK)
                return STATUS_OK; // Need to regenerate file

            // Now we need to validate checksums with files
            lltl::parray<lltl::parray<io::Path> > files;
            if (!ctx->ext.values(&files))
                return STATUS_OK;

            // Validate checksum for generated file
            cks = ck.get(file.as_string());
            if ((!cks) || (!util::match_checksum(cks, &file)))
            {
                util::drop_checksums(&ck);
                return STATUS_OK;
            }
            ck.remove(file.as_string(), NULL);
            free(cks);
            cks = NULL;

            // Validate checksum for other files
            for (size_t i=0, n=files.size(); i<n; ++i)
            {
                lltl::parray<io::Path> *list = files.uget(i);
                for (size_t j=0, m=list->size(); j<m; ++j)
                {
                    // File should be present and match the checksum
                    io::Path *fname = list->uget(j);
                    if ((res = file.set(&src_dir, fname)) != STATUS_OK)
                    {
                        util::drop_checksums(&ck);
                        return res;
                    }

                    cks = ck.get(fname->as_string());
                    if ((!cks) || (!util::match_checksum(cks, &file)))
                    {
                        util::drop_checksums(&ck);
                        return STATUS_OK;
                    }

                    // Checksums matched, remove item from list
                    ck.remove(fname->as_string(), NULL);
                    free(cks);
                    cks = NULL;
                }
            }

            // We need to ensure that the list is empty now. If it is true, then all checksums matched.
            res = (ck.size() > 0) ? STATUS_OK : STATUS_SKIP;
            util::drop_checksums(&ck);

            return res;
        }

        status_t write_checksums(const cmdline_t *cfg, state_t *ctx)
        {
            status_t res;
            io::Path file, cksum, src_dir;

            if ((cfg->checksums == NULL) || (cfg->dst_file == NULL))
                return STATUS_OK;

            if ((res = file.set_native(cfg->dst_file)) != STATUS_OK)
                return res;
            if ((res = cksum.set_native(cfg->checksums)) != STATUS_OK)
                return res;
            if ((res = src_dir.set_native(cfg->src_dir)) != STATUS_OK)
                return res;

            if (!file.is_reg())
                return STATUS_BAD_FORMAT;

            util::checksum_list_t ck;

            // Now we need to generate checksum list
            lltl::parray<lltl::parray<io::Path> > files;
            if (!ctx->ext.values(&files))
                return STATUS_OK;

            // Add checksum of target file
            if ((res = util::add_checksum(&ck, NULL, &file)) != STATUS_OK)
            {
                util::drop_checksums(&ck);
                return res;
            }

            // Add checksum to listed files
            for (size_t i=0, n=files.size(); i<n; ++i)
            {
                lltl::parray<io::Path> *list = files.uget(i);
                for (size_t j=0, m=list->size(); j<m; ++j)
                {
                    // File should be present and match the checksum
                    io::Path *fname = list->uget(j);
                    if ((res = file.set(&src_dir, fname)) != STATUS_OK)
                    {
                        util::drop_checksums(&ck);
                        return res;
                    }

                    if ((res = util::add_checksum(&ck, &src_dir, &file)) != STATUS_OK)
                    {
                        util::drop_checksums(&ck);
                        return res;
                    }
                }
            }

            // Write checksums and exit
            res = util::save_checksums(&ck, &cksum);
            util::drop_checksums(&ck);

            return res;
        }

        status_t pack_resources(const cmdline_t *cfg)
        {
            status_t res;
            io::Path path;
            system::time_t ts, te;

            system::get_time(&ts);
            state_t *ctx = new state_t(cfg);
            if (ctx == NULL)
            {
                fprintf(stderr, "Out of memory\n");
                return STATUS_NO_MEM;
            }
            lsp_finally {
                drop_context(ctx);
            };

            printf("Packing resources to %s from %s\n", cfg->dst_file, cfg->src_dir);

            // Scan the source directory for files
            res     = path.set_native(cfg->src_dir);
            if (res == STATUS_OK)
                res     = scan_directory(ctx, &path, &path);

            // If we have checksums, we need to ensure that
            bool skip_file_creation = false;
            if (res == STATUS_OK)
            {
                res = validate_checksums(cfg, ctx);
                if (res == STATUS_SKIP)
                {
                    skip_file_creation = true;
                    res = STATUS_OK;
                }
            }

            // Skip file creation
            if ((res == STATUS_OK) && (!skip_file_creation))
            {
                res     = create_resource_file(ctx, cfg->dst_file);
                if (res == STATUS_OK)
                    res     = compress_data(ctx, &path);
                if (res == STATUS_OK)
                    res     = write_entries(ctx);

                // Need to write checksums?
                if ((res == STATUS_OK) && (cfg->checksums != NULL))
                    res     = write_checksums(cfg, ctx);
            }

            system::get_time(&te);
            printf("Execution time: %.2f s\n", (te.seconds + te.nanos * 1e-9) - (ts.seconds + ts.nanos * 1e-9));

            return res;
        }

        status_t add_pattern(lltl::parray<io::PathPattern> *list, const LSPString *pattern)
        {
            io::PathPattern *pp = new io::PathPattern();
            if (pp == NULL)
            {
                fprintf(stderr, "Out of memory\n");
                return STATUS_NO_MEM;
            }
            lsp_finally {
                if (pp != NULL)
                    delete pp;
            };

            status_t res = pp->set(pattern, io::PathPattern::FULL_PATH);
            if (res != STATUS_OK)
            {
                fprintf(stderr, "Failed to parse pattern: %s, code=%d\n", pattern->get_native(), int(res));
                return res;
            }

            if (!list->add(pp))
            {
                fprintf(stderr, "Out of memory\n");
                return STATUS_NO_MEM;
            }

            pp = NULL;

            return STATUS_OK;
        }

        status_t parse_pattern(lltl::parray<io::PathPattern> *list, const char *pattern)
        {
            LSPString tmp;
            const char *split;

            while ((split = strchr(pattern, ':')) != NULL)
            {
                if (!tmp.set_native(pattern, split - pattern))
                {
                    fprintf(stderr, "Out of memory\n");
                    return STATUS_NO_MEM;
                }

                status_t res = add_pattern(list, &tmp);
                if (res != STATUS_OK)
                    return res;

                pattern = split + 1;
            }

            if (!tmp.set_native(pattern))
            {
                fprintf(stderr, "Out of memory\n");
                return STATUS_NO_MEM;
            }

            return add_pattern(list, &tmp);
        }

        status_t parse_cmdline(cmdline_t *cfg, int argc, const char **argv)
        {
            cfg->dst_file   = NULL;
            cfg->src_dir    = NULL;
            cfg->checksums  = NULL;

            // Parse arguments
            int i = 1;

            while (i < argc)
            {
                const char *arg = argv[i++];
                if ((!::strcmp(arg, "--help")) || (!::strcmp(arg, "-h")))
                {
                    printf("Usage: %s [parameters] [resource-directories]\n\n", argv[0]);
                    printf("Available parameters:\n");
                    printf("  -c, --checksums <file>        Write file checksums to the specified file\n");
                    printf("  -h, --help                    Show help\n");
                    printf("  -i, --input <dir>             The local resource directory\n");
                    printf("  -o, --output <file>           The name of the destination file to generate resources\n");
                    printf("  -x, --exclude <file>[:<file>] Specify the exclude files that should be not packed\n");
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
                    else if (cfg->dst_file)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->dst_file = argv[i++];
                }
                else if ((!::strcmp(arg, "--input")) || (!::strcmp(arg, "-i")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified directory name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->src_dir)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->src_dir = argv[i++];
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
                else if ((!::strcmp(arg, "--exclude")) || (!::strcmp(arg, "-x")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified exclude name/list for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }

                    status_t res = parse_pattern(&cfg->exclude, argv[i++]);
                    if (res != STATUS_OK)
                        return res;
                }
            }

            // Validate mandatory arguments
            if (cfg->src_dir == NULL)
            {
                fprintf(stderr, "Input directory name is required\n");
                return STATUS_BAD_ARGUMENTS;
            }
            if (cfg->dst_file == NULL)
            {
                fprintf(stderr, "Output file name is required\n");
                return STATUS_BAD_ARGUMENTS;
            }

            return STATUS_OK;
        }

        void free_cmdline(cmdline_t *cmd)
        {
            for (lltl::iterator<io::PathPattern> it = cmd->exclude.values(); it; ++it)
            {
                io::PathPattern *pp = it.get();
                if (pp != NULL)
                    delete pp;
            }
            cmd->exclude.flush();
        }

        int main(int argc, const char **argv)
        {
            cmdline_t cmd;
            lsp::status_t res;

            lsp_finally {
                free_cmdline(&cmd);
            };

            if ((res = parse_cmdline(&cmd, argc, argv)) != STATUS_OK)
                return res;

            if ((res = pack_resources(&cmd)) != lsp::STATUS_OK)
                fprintf(stderr, "Error while generating resource file, code=%d\n", int(res));

            return res;
        }
    } /* namespace respack */
} /* namespace lsp */



