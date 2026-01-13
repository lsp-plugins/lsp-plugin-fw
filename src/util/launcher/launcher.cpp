/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 13 янв. 2026 г.
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

#include <lsp-plug.in/plug-fw/util/launcher/launcher.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/util/launcher/plugin_metadata.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace launcher
    {

        status_t create_ui(tk::Display *dpy)
        {
            return STATUS_OK;
        }

        int execute(int argc, const char **argv)
        {
            // Create the resource loader
            resource::ILoader *loader = core::create_resource_loader();
            if (loader == NULL)
            {
                lsp_error("No resource loader available");
                return STATUS_NO_DATA;
            }
            lsp_finally { delete loader; };

            // Read plugin metadata from JSON
            plugin_registry_t registry;
            status_t res = read_plugin_metadata(registry, loader, LSP_BUILTIN_PREFIX "loader/plugins.json");
            if (res != STATUS_OK)
            {
                lsp_error("Error obtaining plugin metadata");
                return STATUS_NO_DATA;
            }
            lsp_finally { destroy_plugin_metadata(registry); };

//            // Initialize display settings
//            tk::display_settings_t settings;
//            resource::Environment env;
//
//            settings.resources      = loader;
//            settings.environment    = &env;
//
//            tk::Display *dpy = new tk::Display();
//            if (dpy == NULL)
//            {
//                lsp_error("Can not initialize UI graphics subsystem");
//                return STATUS_NO_MEM;
//            }
//
//            lsp_finally {
//                dpy->destroy();
//                delete dpy;
//            };
//
//            res = create_ui(dpy);
//            if (res != STATUS_OK)
//            {
//
//            }
//
//            status_t res = dpy->main();

            return res;
        }
    } /* namespace launcher */
} /* namespace lsp */


