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

#include <lsp-plug.in/test-fw/mtest.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/util/pluglist_gen/pluglist_gen.h>
#include <lsp-plug.in/plug-fw/util/launcher/launcher.h>
#include <lsp-plug.in/plug-fw/util/launcher/plugin_metadata.h>
#include <lsp-plug.in/runtime/system.h>

static const char * launcher_argv[] =
{
    NULL
};

MTEST_BEGIN("", launcher)

    MTEST_MAIN
    {
        // Pass the path to resource directory
        io::Path resources;
        resources.set(tempdir(), "resources");
        system::set_env_var(LSP_RESOURCE_PATH_VAR, resources.as_string());

        // Call the pluglist_gen tool
        MTEST_ASSERT(resources.append_child("loader") == STATUS_OK);
        MTEST_ASSERT(resources.mkdir(true) == STATUS_OK);

        MTEST_ASSERT(resources.append_child("plugins.json") == STATUS_OK);

        pluglist_gen::cmdline_t cmd;
        cmd.json_out = resources.as_native();
        cmd.php_out = NULL;

        MTEST_ASSERT(lsp::pluglist_gen::generate(&cmd) == 0);

        MTEST_ASSERT(launcher::execute(0, launcher_argv) == 0);
    }

MTEST_END




