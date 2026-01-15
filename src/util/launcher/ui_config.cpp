/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 янв. 2026 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/fmt/config/PullParser.h>
#include <lsp-plug.in/fmt/config/Serializer.h>
#include <lsp-plug.in/lltl/hash_index.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/ui/const.h>
#include <lsp-plug.in/plug-fw/util/launcher/ui_config.h>

namespace lsp
{
    namespace launcher
    {
        static const char *config_separator = "-------------------------------------------------------------------------------";

        static status_t get_user_config_file(io::Path & path)
        {
            io::Path tmp;
            status_t res    = system::get_user_config_path(&tmp);
            if (res == STATUS_OK)
                res             = tmp.append_child("lsp-plugins");
            if (res == STATUS_OK)
                res             = tmp.append_child("lsp-plugins.cfg");
            if (res == STATUS_OK)
                tmp.swap(&path);

            return res;
        }

        static void swap(ui_config_t & a, ui_config_t & b)
        {
            lsp::swap(a.nWidth, b.nWidth);
            lsp::swap(a.nHeight, b.nHeight);
            lsp::swap(a.fUIScaling, b.fUIScaling);
            lsp::swap(a.fFontScaling, b.fFontScaling);
            a.sSchema.swap(b.sSchema);
        }

        static void drop_parameters(lltl::parray<config::param_t> & params)
        {
            for (lltl::iterator<config::param_t> it = params.values(); it; ++it)
            {
                config::param_t * const param = it.get();
                delete param;
            }
            params.flush();
        }

        status_t init_config(ui_config_t & config)
        {
            LSPString schema;
            if (!schema.set_ascii(LSP_BUILTIN_PREFIX DEFAULT_SCHEMA_PATH))
                return STATUS_NO_MEM;

            config.nWidth       = 480;
            config.nHeight      = 640;
            config.fUIScaling   = 100.0f;
            config.fFontScaling = 100.0f;
            config.sSchema.swap(schema);

            return STATUS_OK;
        }

        status_t load_config(ui_config_t & config)
        {
            ui_config_t tmp;
            status_t res = init_config(tmp);
            if (res != STATUS_OK)
                return res;

            // Open config parser
            io::Path path;
            if ((res = get_user_config_file(path)) != STATUS_OK)
                return res;

            config::PullParser parser;
            if ((res = parser.open(&path)) != STATUS_OK)
                return res;
            lsp_finally { parser.close(); };

            // Read config
            config::param_t param;
            while ((res = parser.next(&param)) == STATUS_OK)
            {
                const LSPString & id = param.name;

                if (id.equals_ascii(UI_VISUAL_SCHEMA_FILE_ID))
                {
                    const char *path = param.get_string();
                    if ((path != NULL) && (!tmp.sSchema.set_utf8(path)))
                        return STATUS_NO_MEM;
                }
                else if (id.equals_ascii(UI_FONT_SCALING_PORT_ID))
                    tmp.fFontScaling    = lsp_limit(param.to_f32(), 25.0f, 400.0f);
                else if (id.equals_ascii(UI_SCALING_PORT_ID))
                    tmp.fUIScaling      = lsp_limit(param.to_f32(), 25.0f, 400.0f);
                else if (id.equals_ascii(UI_LAUNCHER_WIDTH_ID))
                {
                    if (param.is_numeric())
                        tmp.nWidth          = lsp_max(param.to_u32(), uint32_t(240));
                }
                else if (id.equals_ascii(UI_LAUNCHER_HEIGHT_ID))
                {
                    if (param.is_numeric())
                        tmp.nHeight         = lsp_max(param.to_u32(), uint32_t(320));
                }
            }

            if (res != STATUS_EOF)
                return res;

            // Commit result
            swap(tmp, config);

            return STATUS_OK;
        }

        static status_t load_full_config(
            lltl::parray<config::param_t> & params,
            lltl::hash_index<LSPString, config::param_t> & index,
            const io::Path & path)
        {
            status_t res;
            config::PullParser parser;
            config::param_t param;

            if ((res = parser.open(&path)) != STATUS_OK)
                return res;
            lsp_finally { parser.close(); };

            while ((res = parser.next(&param)) == STATUS_OK)
            {
                // Add new value to hash
                config::param_t *value = new config::param_t();
                if (value == NULL)
                    return STATUS_NO_MEM;
                lsp_finally { delete value; };
                if (!value->copy(&param))
                    return STATUS_NO_MEM;

                // Put data to the mapping
                if (!params.add(value))
                    return STATUS_NO_MEM;
                if (!index.put(&value->name, value, &value))
                    return STATUS_NO_MEM;
                if (value != NULL)
                {
                    lsp_warn("Duplicate entry '%s' in configuration file", param.name.get_utf8());
                    continue;
                }
            }

            if (res != STATUS_EOF)
                return res;

            return STATUS_OK;
        }

        static status_t save_full_config(
            lltl::parray<config::param_t> & params,
            const io::Path & path)
        {
            status_t res;
            io::Path parent;

            // Create parent directory
            if ((res = path.get_parent(&parent)) != STATUS_OK)
                return res;
            if ((res = parent.mkdir(true)) != STATUS_OK)
            {
                if (res != STATUS_ALREADY_EXISTS)
                    return res;
            }

            // Create configuration serializer
            config::Serializer s;
            if ((res = s.open(&path)) != STATUS_OK)
                return res;
            lsp_finally { s.close(); };

            for (lltl::iterator<config::param_t> it = params.values(); it; ++it)
            {
                if ((res = s.write(it.get())) != STATUS_OK)
                    return res;
            }

            // Write footer
            if ((res = s.write_comment(config_separator)) != STATUS_OK)
                return res;

            return s.close();
        }

        static status_t get_or_create_param(
            config::param_t * & result,
            lltl::parray<config::param_t> & params,
            lltl::hash_index<LSPString, config::param_t> & index,
            const char * id)
        {
            LSPString name;
            if (!name.set_ascii(id))
                return STATUS_NO_MEM;

            // Check that parameter exists
            config::param_t * param = index.get(&name);
            if (param != NULL)
            {
                result = param;
                return STATUS_OK;
            }

            // Create new parameter
            param = new config::param_t();
            if (param == NULL)
                return STATUS_NO_MEM;
            lsp_finally { delete param; };

            param->name.swap(&name);
            if (!params.add(param))
                return STATUS_NO_MEM;
            if (!index.create(&param->name, param))
                return STATUS_NO_MEM;

            result = release_ptr(param);
            return STATUS_OK;
        }

        inline status_t set_value(
            lltl::parray<config::param_t> & params,
            lltl::hash_index<LSPString, config::param_t> & index,
            const char * id,
            float value)
        {
            config::param_t *p = NULL;
            status_t res = get_or_create_param(p, params, index, id);
            if (res == STATUS_OK)
                p->set_f32(value);

            return res;
        }

        inline status_t set_value(
            lltl::parray<config::param_t> & params,
            lltl::hash_index<LSPString, config::param_t> & index,
            const char * id,
            uint32_t value)
        {
            config::param_t *p = NULL;
            status_t res = get_or_create_param(p, params, index, id);
            if (res == STATUS_OK)
                p->set_i32(value);

            return res;
        }

        inline status_t set_value(
            lltl::parray<config::param_t> & params,
            lltl::hash_index<LSPString, config::param_t> & index,
            const char * id,
            const LSPString & value)
        {
            config::param_t *p = NULL;
            status_t res = get_or_create_param(p, params, index, id);
            if (res == STATUS_OK)
                res = (p->set_string(value)) ? STATUS_OK : STATUS_NO_MEM;

            return res;
        }

        status_t save_config(const ui_config_t & config)
        {
            // Obtain the location of configuration file
            status_t res;
            io::Path path;
            if ((res = get_user_config_file(path)) != STATUS_OK)
                return res;

            // Load full config
            lltl::parray<config::param_t> params;
            lltl::hash_index<LSPString, config::param_t> index;
            lsp_finally {
                drop_parameters(params);
                index.flush();
            };
            if ((res = load_full_config(params, index, path)) != STATUS_OK)
            {
                if (res != STATUS_NOT_FOUND)
                    return res;
            }

            // Update config
            if ((res = set_value(params, index, UI_VISUAL_SCHEMA_FILE_ID, config.sSchema)) != STATUS_OK)
                return res;
            if ((res = set_value(params, index, UI_FONT_SCALING_PORT_ID, config.fFontScaling)) != STATUS_OK)
                return res;
            if ((res = set_value(params, index, UI_SCALING_PORT_ID, config.fUIScaling)) != STATUS_OK)
                return res;
            if ((res = set_value(params, index, UI_LAUNCHER_WIDTH_ID, config.nWidth)) != STATUS_OK)
                return res;
            if ((res = set_value(params, index, UI_LAUNCHER_HEIGHT_ID, config.nHeight)) != STATUS_OK)
                return res;

            // Save full config
            return save_full_config(params, path);
        }

        void destroy_config(ui_config_t & config)
        {
            config.sSchema.truncate();
        }

    } /* namespace launcher */
} /* namespace lsp */
