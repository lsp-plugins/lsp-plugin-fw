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
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/stdlib/stdio.h>

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

        static bool match_checksum(const checksum_t *c1, const checksum_t *c2)
        {
            return (c1->ck1 == c2->ck1) &&
                   (c1->ck2 == c2->ck2) &&
                   (c1->ck3 == c2->ck3);
        }

        static status_t calc_checksum(checksum_t *dst, const io::Path *file)
        {
            uint8_t buf[0x400];

            dst->ck1   = 0;
            dst->ck2   = 0;
            dst->ck3   = 0xffffffff;

            FILE *in = fopen(file->as_native(), "r");
            if (in == NULL)
                return STATUS_OK;

            while (true)
            {
                int nread = fread(buf, sizeof(uint8_t), sizeof(buf), in);
                if (nread <= 0)
                    break;

                for (int i=0; i<nread; ++i)
                {
                    // Compute checksums
                    dst->ck1 = dst->ck1 + buf[i];
                    dst->ck2 = ((dst->ck2 << 9) | (dst->ck2 >> 55)) ^ buf[i];
                    dst->ck3 = ((dst->ck3 << 17) | (dst->ck3 >> 47)) ^ uint16_t(0xa5ff - buf[i]);
                }
            }

            fclose(in);

            return STATUS_OK;
        }
    } /* namespace util */
} /* namespace lsp */



#endif /* INCLUDE_LSP_PLUG_IN_PLUG_FW_UTIL_CHECKSUM_H_ */
