/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 28 дек. 2023 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_HELPERS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_HELPERS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/stdlib/string.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        inline bool read_octet(char & dst, const char *str)
        {
            uint8_t c1 = str[0];
            uint8_t c2 = str[1];

            if ((c1 >= '0') && (c1 <= '9'))
                c1     -= '0';
            else if ((c1 >= 'a') && (c1 <= 'f'))
                c1      = c1 + 10 - 'a';
            else if ((c1 >= 'A') && (c1 <= 'F'))
                c1      = c1 + 10 - 'A';
            else
                return false;

            if ((c2 >= '0') && (c2 <= '9'))
                c2     -= '0';
            else if ((c2 >= 'a') && (c2 <= 'f'))
                c2      = c2 + 10 - 'a';
            else if ((c2 >= 'A') && (c2 <= 'F'))
                c2      = c2 + 10 - 'A';
            else
                return false;

            dst     = (c1 << 4) | c2;

            return true;
        }

        /**
         * Parse TUID from string to the TUID data structure
         *
         * @param tuid TUID data structure
         * @param str string to parse (different forms of TUID available)
         * @return status of operation
         */
        inline status_t parse_tuid(Steinberg::TUID & tuid, const char *str)
        {
            size_t len = strlen(str);

            if (len == 16)
            {
                uint32_t v[4];
                v[0]    = CPU_TO_BE(*reinterpret_cast<const uint32_t *>(&str[0]));
                v[1]    = CPU_TO_BE(*reinterpret_cast<const uint32_t *>(&str[4]));
                v[2]    = CPU_TO_BE(*reinterpret_cast<const uint32_t *>(&str[8]));
                v[3]    = CPU_TO_BE(*reinterpret_cast<const uint32_t *>(&str[12]));

                const Steinberg::TUID uid = INLINE_UID(v[0], v[1], v[2], v[3]);

                memcpy(tuid, uid, sizeof(Steinberg::TUID));

                return STATUS_OK;
            }
            else if (len == 32)
            {
                for (size_t i=0; i<16; ++i)
                {
                    if (!read_octet(tuid[i], &str[i*2]))
                        return STATUS_INVALID_UID;
                }

                return STATUS_OK;
            }

            return STATUS_INVALID_UID;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_HELPERS_H_ */
