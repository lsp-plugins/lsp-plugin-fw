/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 февр. 2026 г.
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

#include <lsp-plug.in/plug-fw/util/launcher/config.h>
#include <lsp-plug.in/fmt/json/Parser.h>
#include <lsp-plug.in/fmt/json/Serializer.h>
#include <lsp-plug.in/runtime/system.h>

namespace lsp
{
    namespace launcher
    {
        static status_t get_user_config_file(io::Path & path)
        {
            io::Path tmp;
            LSP_STATUS_ASSERT(system::get_user_config_path(&tmp));
            LSP_STATUS_ASSERT(tmp.append_child("lsp-plugins"));
            LSP_STATUS_ASSERT(tmp.append_child("launcher.json"));

            tmp.swap(&path);

            return STATUS_OK;
        }

        static status_t read_favourites(lltl::phashset<char> & favourites, json::Parser & p)
        {
            json::event_t ev;
            status_t res;
            if ((res = p.read_next(&ev)) != STATUS_OK)
                return res;

            if (ev.type != json::JE_ARRAY_START)
                return STATUS_CORRUPTED_FILE;

            while ((res = p.read_next(&ev)) == STATUS_OK)
            {
                if (ev.type == json::JE_STRING)
                {
                    // Clone identifier
                    char * uid = ev.sValue.clone_utf8();
                    if (uid == NULL)
                        return STATUS_NO_MEM;
                    lsp_finally {
                        if (uid != NULL)
                            free(uid);
                    };

                    // Add identifier to favourites
                    if (favourites.create(uid))
                        uid = NULL;
                    else if (!favourites.contains(uid))
                        return STATUS_NO_MEM;
                }
                else if (ev.type == json::JE_ARRAY_END)
                    break;
            }

            return res;
        }

        static status_t read_config(config_t & config, json::Parser & p)
        {
            config_t tmp;
            init_config(tmp);
            lsp_finally { free_config(tmp); };

            json::event_t ev;
            status_t res;
            if ((res = p.read_next(&ev)) != STATUS_OK)
                return res;

            if (ev.type != json::JE_OBJECT_START)
                return STATUS_CORRUPTED_FILE;

            while ((res = p.read_next(&ev)) == STATUS_OK)
            {
                if (ev.type == json::JE_PROPERTY)
                {
                    if (ev.sValue.equals_ascii("width"))
                        res = p.read_int(&tmp.nWidth);
                    else if (ev.sValue.equals_ascii("height"))
                        res = p.read_int(&tmp.nHeight);
                    else if (ev.sValue.equals_ascii("favourites"))
                        res = read_favourites(config.vFavourites, p);
                    else
                        res = p.skip_current();
                }
                else if (ev.type == json::JE_OBJECT_END)
                {
                    // Ensure that end of object occurred
                    if ((res = p.read_next(&ev)) == STATUS_EOF)
                        res = STATUS_OK;
                    else
                        res = STATUS_CORRUPTED_FILE;
                    break;
                }
            }

            // Commit config
            if (res == STATUS_OK)
            {
                lsp::swap(config.nWidth, tmp.nWidth);
                lsp::swap(config.nHeight, tmp.nHeight);
                config.vFavourites.swap(tmp.vFavourites);
            }

            return res;
        }

        static status_t write_config(const config_t & config, json::Serializer & s)
        {
            // Form sorted list of favourites
            lltl::parray<char> items;
            for (lltl::iterator<const char> it = config.vFavourites.values(); it; ++it)
            {
                const char *id = it.get();
                if (id == NULL)
                    continue;
                if (!items.add(const_cast<char * >(it.get())))
                    return STATUS_NO_MEM;
            }
            items.qsort();

            // Write configuration
            LSP_STATUS_ASSERT(s.start_object());
            {
                LSP_STATUS_ASSERT(s.prop_int("width", uint32_t(config.nWidth)));
                LSP_STATUS_ASSERT(s.prop_int("height", uint32_t(config.nHeight)));
                LSP_STATUS_ASSERT(s.write_property("favourites"));

                s.start_array();
                {
                    for (lltl::iterator<char> it = items.values(); it; ++it)
                        LSP_STATUS_ASSERT(s.write_string(it.get()));
                }
                s.end_array();
            }
            LSP_STATUS_ASSERT(s.end_object());

            return STATUS_OK;
        }

        static void init_json_flags(json::serial_flags_t & flags)
        {
            json::init_serial_flags(&flags);

            flags.padding       = 4;
            flags.separator     = true;
            flags.multiline     = true;
        }

        void init_config(config_t & config)
        {
            config.nWidth       = 400;
            config.nHeight      = 648;
        }

        void free_config(config_t & config)
        {
            for (lltl::iterator<char> it = config.vFavourites.values(); it; ++it)
            {
                char * id = it.get();
                if (id != NULL)
                    free(id);
            }
            config.vFavourites.flush();
        }

        status_t load_config(config_t & config, const io::Path & path)
        {
            json::Parser p;
            status_t res = p.open(&path, json::JSON_LEGACY);
            if (res == STATUS_OK)
                res     = read_config(config, p);
            res = update_status(res, p.close());
            return res;
        }

        status_t load_config(config_t & config, const LSPString & path)
        {
            json::Parser p;
            status_t res = p.open(&path, json::JSON_LEGACY);
            if (res == STATUS_OK)
                res     = read_config(config, p);
            res = update_status(res, p.close());
            return res;
        }

        status_t load_config(config_t & config, const char *path)
        {
            json::Parser p;
            status_t res = p.open(path, json::JSON_LEGACY);
            if (res == STATUS_OK)
                res     = read_config(config, p);
            res = update_status(res, p.close());
            return res;
        }

        status_t load_config(config_t & config)
        {
            io::Path path;
            LSP_STATUS_ASSERT(get_user_config_file(path));
            return load_config(config, path);
        }

        status_t save_config(const config_t & config, const io::Path & path)
        {
            json::Serializer s;
            json::serial_flags_t flags;
            init_json_flags(flags);

            status_t res = s.open(&path, &flags);
            if (res == STATUS_OK)
                res     = write_config(config, s);
            res = update_status(res, s.close());
            return res;
        }

        status_t save_config(const config_t & config, const LSPString & path)
        {
            json::Serializer s;
            json::serial_flags_t flags;
            init_json_flags(flags);

            status_t res = s.open(&path, &flags);
            if (res == STATUS_OK)
                res     = write_config(config, s);
            res = update_status(res, s.close());
            return res;
        }

        status_t save_config(const config_t & config, const char *path)
        {
            json::Serializer s;
            json::serial_flags_t flags;
            init_json_flags(flags);

            status_t res = s.open(path, &flags);
            if (res == STATUS_OK)
                res     = write_config(config, s);
            res = update_status(res, s.close());
            return res;
        }

        status_t save_config(const config_t & config)
        {
            io::Path path;
            LSP_STATUS_ASSERT(get_user_config_file(path));
            return save_config(config, path);
        }

    } /* namespace launcher */
} /* namespace lsp */
