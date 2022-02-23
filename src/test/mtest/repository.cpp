/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 дек. 2021 г.
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
#include <lsp-plug.in/plug-fw/util/repository/repository.h>


MTEST_BEGIN("", repository)

    MTEST_MAIN
    {
        io::Path resdir;
        io::Path outfile;

        MTEST_ASSERT(resdir.set(tempdir(), "test-resources") == STATUS_OK)
        int narg = 0;
        const char *varg[20];

        varg[narg++] = full_name();
        varg[narg++] = "-o";
        varg[narg++] = resdir.as_native();
        varg[narg++] = "-l";
        varg[narg++] = "res/local";
        varg[narg++] = "modules/*";

        status_t res = lsp::repository::main(narg, varg);
        MTEST_ASSERT(res == STATUS_OK);
    }

MTEST_END


