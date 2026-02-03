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
#include <lsp-plug.in/plug-fw/util/launcher/ui.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace launcher
    {
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

            // Create UI
            UI ui(loader);
            lsp_finally { ui.destroy(); };

            status_t res = ui.init(NULL);
            if (res != STATUS_OK)
            {
                lsp_error("Could not initialize UI");
                return STATUS_FAILED;
            }

            // Launch the main display loop
            return ui.main_loop();
        }
    } /* namespace launcher */
} /* namespace lsp */


