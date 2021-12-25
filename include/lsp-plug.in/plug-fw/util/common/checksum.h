/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 17 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_CHECKSUM_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_CHECKSUM_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/io/Path.h>

namespace lsp
{
    namespace util
    {
        typedef struct checksum_t
        {
            uint64_t ck1;
            uint64_t ck2;
            uint64_t ck3;
        } checksum_t;

        /**
         * Ensure that one checksum matches another checksum
         * @param c1 checksum 1
         * @param c2 checksum 2
         * @return true if checsums match
         */
        bool match_checksum(const checksum_t *c1, const checksum_t *c2);

        /**
         * Compute checksum of the file
         * @param dst destination checksum to store data
         * @param file path to the file
         * @return status of operation
         */
        status_t calc_checksum(checksum_t *dst, const io::Path *file);

    } /* namespace util */
} /* namespace lsp */



#endif /* INCLUDE_LSP_PLUG_IN_PLUG_FW_UTIL_CHECKSUM_H_ */
