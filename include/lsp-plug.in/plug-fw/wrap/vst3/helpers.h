/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 28 дек. 2023 г.
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

        /**
         * Perform safe acquire of Steinberg::FUnknown object
         *
         * @tparam V type to which cast the original pointer
         * @param ptr pointer to acquire
         * @return pointer to the object
         */
        template <class T>
        inline T *safe_acquire(T *ptr)
        {
            if (ptr == NULL)
                return NULL;

            ptr->addRef();
            return ptr;
        }

        /**
         * Perform safe release of Steinberg::FUnknown object
         * @tparam T type of the class derived from Steinberg::FUnknown
         * @param ptr field that stores the pointer, is automatically reset to NULL.
         */
        template <class T>
        inline void safe_release(T * &ptr)
        {
            if (ptr == NULL)
                return;

            ptr->release();
            ptr = NULL;
        }

        /**
         * Implement the cast operation for the queryInterface call.
         * @tparam V destination type of pointer to cast
         * @param ptr original pointer to cast
         * @param obj destination to store pointer
         * @return Steinberg::kResultOk
         */
        template <class T, class V>
        inline Steinberg::tresult cast_interface(V *ptr, void **obj)
        {
            ptr->addRef();
            *obj = static_cast<T *>(ptr);
            return Steinberg::kResultOk;
        }

        /**
         * Return the value when it is not possible to implement queryInterface call.
         * @param obj destination to store pointer
         * @return Steinberg::kNoInterface
         */
        inline Steinberg::tresult no_interface(void **obj)
        {
            *obj = NULL;
            return Steinberg::kNoInterface;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_HELPERS_H_ */
