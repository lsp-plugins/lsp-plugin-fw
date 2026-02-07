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
#include <lsp-plug.in/ipc/Process.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/util/launcher/plugin_metadata.h>
#include <lsp-plug.in/plug-fw/util/launcher/ui.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace launcher
    {
        status_t load_package_info(meta::package_t **package, resource::ILoader *loader)
        {
            // Load package information
            status_t res;
            io::IInStream *is = loader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
            if (is == NULL)
            {
                lsp_error("No manifest.json found in resources");
                return STATUS_BAD_STATE;
            }
            lsp_finally {
                is->close();
                delete is;
            };

            res = meta::load_manifest(package, is);
            if (res != STATUS_OK)
                lsp_error("Error while reading manifest file, error: %d", int(res));

            return res;
        }

        status_t launch_plugin(const meta::package_t *package, const meta::plugin_t *launch)
        {
            // Check that we need to launch plugin
            if (launch == NULL)
                return STATUS_OK;

            LSPString program;
            if (!program.set_ascii(package->artifact))
                return STATUS_NO_MEM;
            if (!program.append('-'))
                return STATUS_NO_MEM;
            if (!program.append_ascii(launch->uid))
                return STATUS_NO_MEM;
            program.replace_all('_', '-');

            lsp_info("Launching plugin uid='%s', executable='%s', name='%s'...", launch->uid, program.get_native(), launch->description);

            ipc::Process proc;
            proc.set_command(&program);
            status_t res = proc.launch();
            if (res != STATUS_OK)
            {
                lsp_error("Error launching process '%s': code=%d", program.get_native(), res);
                return res;
            }

            if ((res = proc.wait()) != STATUS_OK)
                return res;
            int exit_code = STATUS_OK;
            if ((res = proc.exit_code(&exit_code)) != STATUS_OK)
                return res;
            return status_t(exit_code);
        }

        status_t launch_ui(resource::ILoader *loader, meta::package_t *package, const meta::plugin_t **launch)
        {
            status_t res;
            UI ui(loader, package, launch);
            lsp_finally { ui.destroy(); };

            if ((res = ui.init(NULL)) != STATUS_OK)
            {
                lsp_error("Could not initialize UI");
                return STATUS_FAILED;
            }

            // Launch the main display loop
            if ((res = ui.main_loop()) != STATUS_OK)
                lsp_error("Failed executing UI, code=%d", int(res));

            return res;
        }

        int execute(int argc, const char **argv)
        {
            status_t res;

            // Create the resource loader
            resource::ILoader *loader = core::create_resource_loader();
            if (loader == NULL)
            {
                lsp_error("No resource loader available");
                return STATUS_NO_DATA;
            }
            lsp_finally { delete loader; };

            // Load package
            meta::package_t *package = NULL;
            if ((res = load_package_info(&package, loader)) != STATUS_OK)
            {
                lsp_error("No package info");
                return res;
            }
            lsp_finally {
                if (package != NULL)
                    meta::free_manifest(package);
            };

            // Create UI
            const meta::plugin_t *launch = NULL;
            if ((res = launch_ui(loader, package, &launch)) != STATUS_OK)
                return res;

            if (res == STATUS_OK)
                res = launch_plugin(package, launch);

            return res;
        }
    } /* namespace launcher */
} /* namespace lsp */


