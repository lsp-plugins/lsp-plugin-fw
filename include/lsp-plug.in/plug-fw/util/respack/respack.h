/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/io/PathPattern.h>

namespace lsp
{
    namespace respack
    {
        /**
         * Command-line arguments
         */
        typedef struct cmdline_t
        {
            const char         *src_dir;            // Source directory
            const char         *dst_file;           // Destination file
            const char         *checksums;          // Output checksums file
            lltl::parray<io::PathPattern> exclude;
        } cmdline_t;

        /**
         * Parse command-line arguments
         * @param cmd command-line arguments to store
         * @param argc number of command line arguments
         * @param argv list of command line arguments
         * @return status of operation
         */
        status_t parse_cmdline(cmdline_t *cmd, int argc, const char **argv);

        /**
         * Pack directory with resources and generate the output C++ file for compilation.
         * @param cmd command-line arguments
         * @return status of operation
         */
        status_t pack_resources(const cmdline_t *cmd);

        /**
         *
         */
        void free_cmdline(cmdline_t *cmd);

        /**
         * Execute the code of utility
         * @param argc number of command line arguments
         * @param argv list of command line arguments
         * @return status of operation
         */
        status_t main(int argc, const char **argv);

    } /* namespace respack */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_RESPACK_RESPACK_H_ */
