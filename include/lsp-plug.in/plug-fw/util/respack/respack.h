/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_RESPACK_RESPACK_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_RESPACK_RESPACK_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/common/status.h>

namespace lsp
{
    namespace respack
    {
        /**
         * Pack directory with resources and generate the output C++ file for compilation.
         * @param destfile destination file
         * @param dir source directory to perform packing
         * @return status of operation
         */
        status_t pack_resources(const char *destfile, const char *dir);

        /**
         * Execute the code of utility
         * @param argc number of command line arguments
         * @param argv list of command line arguments
         * @return status of operation
         */
        status_t main(int argc, const char **argv);
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_RESPACK_RESPACK_H_ */
