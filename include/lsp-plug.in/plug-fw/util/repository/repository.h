/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_REPOSITORY_REPOSITORY_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_REPOSITORY_REPOSITORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/lltl/parray.h>

namespace lsp
{
    namespace repository
    {
        /**
         * Command-line arguments
         */
        typedef struct cmdline_t
        {
            bool                strict;     // Strict mode
            const char         *dst_dir;    // Destination directory
            const char         *local_dir;  // Local directory
            lltl::parray<char>  paths;      // Additional resource paths
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
         * Generate resources
         * @param cmd command-line arguments
         * @return status of operation
         */
        status_t make_repository(const cmdline_t *cmd);

        /**
         * Execute the tool
         * @param args number of command line arguments
         * @param argv list of command line arguments
         * @return status of operation
         */
        status_t main(int argc, const char **argv);

    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_REPOSITORY_REPOSITORY_H_ */
