/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 февр. 2024 г.
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
#include <lsp-plug.in/plug-fw/util/vst3_modinfo/vst3_modinfo.h>
#include <lsp-plug.in/runtime/system.h>

MTEST_BEGIN("", vst3_modinfo)

    MTEST_MAIN
    {
        // Pass the path to resource directory
        io::Path resdir;
        resdir.set(tempdir(), "resources");
        system::set_env_var(LSP_RESOURCE_PATH_VAR, resdir.as_string());

        // Call the tool
        io::Path modinfo;
        MTEST_ASSERT(modinfo.fmt("%s/mtest-%s-vst3-moduleinfo.json", tempdir(), full_name()) > 0);

        io::Path plist;
        MTEST_ASSERT(plist.fmt("%s/mtest-%s-vst3-info.plist", tempdir(), full_name()) > 0);

        io::Path pkginfo;
        MTEST_ASSERT(pkginfo.fmt("%s/mtest-%s-vst3-PkgInfo", tempdir(), full_name()) > 0);

        const char *data[]=
        {
            "vst3_modinfo",
            "-m",
            modinfo.as_native(),
            "-i",
            plist.as_native(),
            "-p",
            pkginfo.as_native()
        };

        printf("Writing moduleinfo.json file to %s...\n", modinfo.as_native());
        printf("Writing info.plist file to %s...\n", plist.as_native());
        printf("Writing PkgInfo file to %s...\n", pkginfo.as_native());

        MTEST_ASSERT(lsp::vst3_modinfo::main(sizeof(data)/sizeof(const char *), data) == 0);
    }

MTEST_END



