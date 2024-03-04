/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 нояб. 2021 г.
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
#include <lsp-plug.in/runtime/system.h>

#include <ladspa/ladspa.h>

extern const LADSPA_Descriptor *ladspa_descriptor(unsigned long index);

MTEST_BEGIN("", ladspa)

    MTEST_MAIN
    {
        // Pass the path to resource directory
        io::Path resdir;
        resdir.set(tempdir(), "resources");
        system::set_env_var(LSP_RESOURCE_PATH_VAR, resdir.as_string());

        // Fetch LADSPA descriptors
        for (size_t i=0; ; ++i)
        {
            const LADSPA_Descriptor *ld = ladspa_descriptor(i);
            if (ld == NULL)
                break;

            printf("------------------------------------------------------------\n");
            printf("Name:       %s\n", ld->Name);
            printf("Label:      %s\n", ld->Label);
            printf("UniqueID:   %ld\n", (long)ld->UniqueID);
            printf("Properties: %x\n", ld->Properties);
            printf("Maker:      %s\n", ld->Maker);
            printf("Copyright:  %s\n", ld->Copyright);
            printf("Ports:      %ld\n", (long)ld->PortCount);
        }
    }

MTEST_END
