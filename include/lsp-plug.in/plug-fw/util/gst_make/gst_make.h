/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_GST_MAKE_GST_MAKE_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_GST_MAKE_GST_MAKE_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

namespace lsp
{
    namespace gst_make
    {
        /**
         * Command-line arguments
         */
        typedef struct cmdline_t
        {
            const char                 *manifest;       // Manifest file
            const char                 *template_file;  // Template file
            const char                 *destination;    // Destination directory
        } cmdline_t;

        /**
         * The main function of the utility
         * @param argc number of command line arguments
         * @param argv list of command line arguments
         * @return status of operation
         */
        status_t main(int argc, const char **argv);

    } /* namespace gst_make */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_GST_MAKE_GST_MAKE_H_ */
