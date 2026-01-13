/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 13 янв. 2026 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/util/launcher/plugin_metadata.h>
#include <lsp-plug.in/fmt/json/Parser.h>

namespace lsp
{
    namespace launcher
    {
        static const char * const category_ordering[] =
        {
            "equalizers",
            "dynamics",
            "mb_dynamics",
            "convolution",
            "delays",
            "analyzers",
            "mb_processing",
            "samplers",
            "effects",
            "generators",
            "utilities",
            NULL
        };

        typedef struct raw_bundle_t
        {
            LSPString       id;
            LSPString       name;
            LSPString       description;
            LSPString       group_id;
        } raw_bundle_t;

        typedef struct raw_plugin_t
        {
            LSPString       id;
            LSPString       name;
            LSPString       description;
            LSPString       version;
            LSPString       acronym;
            LSPString       bundle_id;
        } raw_plugin_t;

        typedef struct raw_content_t
        {
            package_t       package;
            lltl::parray<raw_bundle_t> bundles;
            lltl::parray<raw_plugin_t> plugins;
        } raw_content_t;

        static void destroy_raw_content(raw_content_t & metadata)
        {
            for (lltl::iterator<raw_bundle_t> it = metadata.bundles.values(); it; ++it)
                delete it.get();
            metadata.bundles.flush();

            for (lltl::iterator<raw_plugin_t> it = metadata.plugins.values(); it; ++it)
                delete it.get();
            metadata.plugins.flush();
        }

        static status_t parse_package(package_t & package, json::Parser & parser)
        {
            json::event_t ev;

            status_t res = parser.read_next(&ev);
            if (res != STATUS_OK)
                return res;
            if (ev.type != json::JE_OBJECT_START)
                return STATUS_BAD_FORMAT;

            while ((res = parser.read_next(&ev)) == STATUS_OK)
            {
                if (ev.type == json::JE_PROPERTY)
                {
                    if (ev.sValue.equals_ascii("artifact"))
                        res = parser.read_string(&package.artifact);
                    else if (ev.sValue.equals_ascii("name"))
                        res = parser.read_string(&package.name);
                    else if (ev.sValue.equals_ascii("version"))
                        res = parser.read_string(&package.version);
                    else
                        res = parser.skip_current();

                    if (res != STATUS_OK)
                        break;
                }
                else if (ev.type != json::JE_OBJECT_END)
                    return STATUS_BAD_FORMAT;
                else
                    break;
            }

            return res;
        }

        static status_t parse_bundle(raw_bundle_t & bundle, json::Parser & parser)
        {
            json::event_t ev;
            status_t res;

            while ((res = parser.read_next(&ev)) == STATUS_OK)
            {
                if (ev.type == json::JE_PROPERTY)
                {
                    if (ev.sValue.equals_ascii("id"))
                        res         = parser.read_string(&bundle.id);
                    else if (ev.sValue.equals_ascii("name"))
                        res         = parser.read_string(&bundle.name);
                    else if (ev.sValue.equals_ascii("description"))
                        res         = parser.read_string(&bundle.description);
                    else if (ev.sValue.equals_ascii("group"))
                        res         = parser.read_string(&bundle.group_id);
                    else
                        res         = parser.skip_current();

                    if (res != STATUS_OK)
                        break;
                }
                else if (ev.type != json::JE_OBJECT_END)
                    return STATUS_BAD_FORMAT;
                else
                    break;
            }

            return res;
        }

        static status_t parse_plugin(raw_plugin_t & plugin, json::Parser & parser)
        {
            json::event_t ev;
            status_t res;

            while ((res = parser.read_next(&ev)) == STATUS_OK)
            {
                if (ev.type == json::JE_PROPERTY)
                {
                    if (ev.sValue.equals_ascii("id"))
                        res         = parser.read_string(&plugin.id);
                    else if (ev.sValue.equals_ascii("name"))
                        res         = parser.read_string(&plugin.name);
                    else if (ev.sValue.equals_ascii("description"))
                        res         = parser.read_string(&plugin.description);
                    else if (ev.sValue.equals_ascii("version"))
                        res         = parser.read_string(&plugin.version);
                    else if (ev.sValue.equals_ascii("acronym"))
                        res         = parser.read_string(&plugin.acronym);
                    else if (ev.sValue.equals_ascii("bundle"))
                        res         = parser.read_string(&plugin.bundle_id);
                    else
                        res         = parser.skip_current();

                    if (res != STATUS_OK)
                        break;
                }
                else if (ev.type != json::JE_OBJECT_END)
                    return STATUS_BAD_FORMAT;
                else
                    break;
            }

            return res;
        }

        static status_t parse_bundles(lltl::parray<raw_bundle_t> & list, json::Parser & parser)
        {
            json::event_t ev;

            status_t res = parser.read_next(&ev);
            if (res != STATUS_OK)
                return res;
            if (ev.type != json::JE_ARRAY_START)
                return STATUS_BAD_FORMAT;

            while ((res = parser.read_next(&ev)) == STATUS_OK)
            {
                if (ev.type == json::JE_OBJECT_START)
                {
                    raw_bundle_t * const bundle = new raw_bundle_t;
                    if (bundle == NULL)
                        return STATUS_NO_MEM;

                    if (!list.append(bundle))
                    {
                        delete bundle;
                        return STATUS_NO_MEM;
                    }

                    res = parse_bundle(*bundle, parser);

                    if (res != STATUS_OK)
                        break;
                }
                else if (ev.type != json::JE_ARRAY_END)
                    return STATUS_BAD_FORMAT;
                else
                    break;
            }

            return res;
        }

        static status_t parse_plugins(lltl::parray<raw_plugin_t> & list, json::Parser & parser)
        {
            json::event_t ev;

            status_t res = parser.read_next(&ev);
            if (res != STATUS_OK)
                return res;
            if (ev.type != json::JE_ARRAY_START)
                return STATUS_BAD_FORMAT;

            while ((res = parser.read_next(&ev)) == STATUS_OK)
            {
                if (ev.type == json::JE_OBJECT_START)
                {
                    raw_plugin_t * const plugin = new raw_plugin_t;
                    if (plugin == NULL)
                        return STATUS_NO_MEM;

                    if (!list.append(plugin))
                    {
                        delete plugin;
                        return STATUS_NO_MEM;
                    }

                    res = parse_plugin(*plugin, parser);

                    if (res != STATUS_OK)
                        break;
                }
                else if (ev.type != json::JE_ARRAY_END)
                    return STATUS_BAD_FORMAT;
                else
                    break;
            }

            return res;
        }

        static status_t read_json_plugin_metadata(raw_content_t & out, resource::ILoader * loader, const char * file)
        {
            if (file == NULL)
                return STATUS_NOT_FOUND;

            // Open parser
            json::Parser parser;
            lsp_finally { parser.close(); };

            status_t res;
            if (loader != NULL)
            {
                io::IInStream *in = loader->read_stream(file);
                if (in == NULL)
                    return STATUS_NOT_FOUND;
                res = parser.wrap(in, json::JSON_VERSION5, WRAP_CLOSE | WRAP_DELETE);
            }
            else
                res = parser.open(file, json::JSON_VERSION5);

            if (res != STATUS_OK)
                return res;

            json::event_t ev;
            size_t level = 0;;
            while ((res = parser.read_next(&ev)) == STATUS_OK)
            {
                switch (ev.type)
                {
                    case json::JE_OBJECT_START:
                        ++level;
                        break;

                    case json::JE_OBJECT_END:
                        --level;
                        break;

                    case json::JE_PROPERTY:
                        if (level != 1)
                            res = parser.skip_current();

                        if (ev.sValue.equals_ascii("package"))
                            res     = parse_package(out.package, parser);
                        else if (ev.sValue.equals_ascii("bundles"))
                            res     = parse_bundles(out.bundles, parser);
                        else if (ev.sValue.equals_ascii("plugins"))
                            res     = parse_plugins(out.plugins, parser);
                        else
                            res     = parser.skip_current();

                        break;

                    default:
                        res     = STATUS_BAD_FORMAT;
                        break;
                }

                if (res != STATUS_OK)
                    break;
            }

            return (res == STATUS_EOF) ? STATUS_OK : res;
        }

        static const category_t *map_category(lltl::parray<category_t> & categories, LSPString & id)
        {
            // Check if category already exits
            for (lltl::iterator<category_t> it= categories.values(); it; ++it)
            {
                const category_t * cat = it.get();
                if ((cat != NULL) && (cat->id.equals(&id)))
                    return cat;
            }

            // Create new category
            category_t *cat = new category_t;
            if (cat == NULL)
                return cat;

            if (!categories.append(cat))
            {
                delete cat;
                return NULL;
            }

            // Commit category identifier
            cat->id.swap(id);
            cat->priority = 0;

            for (const char * const * it = category_ordering; *it != NULL; ++it)
            {
                if (cat->id.equals_ascii(*it))
                    break;
                ++cat->priority;
            }

            return cat;
        }

        static const bundle_t *find_bundle(lltl::parray<bundle_t> & bundles, const LSPString & id)
        {
            // Check if category already exits
            for (lltl::iterator<bundle_t> it= bundles.values(); it; ++it)
            {
                const bundle_t * bundle = it.get();
                if ((bundle != NULL) && (bundle->id.equals(&id)))
                    return bundle;
            }

            return NULL;
        }

        static status_t transform_plugin_metadata(plugin_registry_t & out, raw_content_t & raw)
        {
            // Reserve space
            if (!out.bundles.reserve(raw.bundles.size()))
                return STATUS_NO_MEM;
            if (!out.plugins.reserve(raw.plugins.size()))
                return STATUS_NO_MEM;

            // Package
            out.package.artifact.swap(raw.package.artifact);
            out.package.name.swap(raw.package.name);
            out.package.version.swap(raw.package.version);

            // Categories and bundles
            for (lltl::iterator<raw_bundle_t> it = raw.bundles.values(); it; ++it)
            {
                raw_bundle_t * const rbundle = it.get();
                const category_t * const category = map_category(out.categories, rbundle->group_id);
                if (category == NULL)
                    return STATUS_BAD_STATE;

                bundle_t * const bundle = new bundle_t;
                if (bundle == NULL)
                    return STATUS_NO_MEM;
                if (!out.bundles.add(bundle))
                {
                    delete bundle;
                    return STATUS_NO_MEM;
                }

                // Copy bundle contents
                bundle->id.swap(rbundle->id);
                bundle->name.swap(rbundle->name);
                bundle->description.swap(rbundle->description);
                bundle->category = category;
            }

            // Plugins
            for (lltl::iterator<raw_plugin_t> it = raw.plugins.values(); it; ++it)
            {
                raw_plugin_t * const rplugin = it.get();
                const bundle_t * const bundle = find_bundle(out.bundles, rplugin->bundle_id);
                if (bundle == NULL)
                    return STATUS_NO_MEM;

                plugin_t * const plugin = new plugin_t;
                if (plugin == NULL)
                    return STATUS_NO_MEM;
                if (!out.plugins.add(plugin))
                {
                    delete plugin;
                    return STATUS_NO_MEM;
                }

                // Copy bundle contents
                plugin->id.swap(rplugin->id);
                plugin->name.swap(rplugin->name);
                plugin->description.swap(rplugin->description);
                plugin->version.swap(rplugin->version);
                plugin->acronym.swap(rplugin->acronym);
                plugin->bundle = bundle;
            }

            return STATUS_OK;
        }

        status_t read_builtin_plugin_metadata(raw_content_t & out)
        {
            return STATUS_NOT_FOUND;
        }

        void destroy_plugin_metadata(plugin_registry_t & metadata)
        {
            for (lltl::iterator<category_t> it = metadata.categories.values(); it; ++it)
                delete it.get();
            metadata.categories.flush();

            for (lltl::iterator<bundle_t> it = metadata.bundles.values(); it; ++it)
                delete it.get();
            metadata.bundles.flush();

            for (lltl::iterator<plugin_t> it = metadata.plugins.values(); it; ++it)
                delete it.get();
            metadata.plugins.flush();
        }

        status_t read_plugin_metadata(plugin_registry_t & metadata, resource::ILoader * loader, const char * file)
        {
            raw_content_t raw_out;
            lsp_finally { destroy_raw_content(raw_out); };

            status_t res = read_json_plugin_metadata(raw_out, loader, file);
            if (res != STATUS_OK)
                res     = read_builtin_plugin_metadata(raw_out);

            if (res == STATUS_OK)
                res         = transform_plugin_metadata(metadata, raw_out);

            return res;
        }


    } /* namespace launcher */
} /* namespace lsp */


