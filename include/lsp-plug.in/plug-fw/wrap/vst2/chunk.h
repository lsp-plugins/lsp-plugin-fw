/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 7 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_CHUNK_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_CHUNK_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/stdlib/stdlib.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace vst2
    {
        /**
         * VST plugin state chunk generator
         */
        typedef struct chunk_t
        {
            uint8_t    *data;
            size_t      offset;
            size_t      capacity;
            status_t    res;

            explicit chunk_t()
            {
                data        = NULL;
                offset      = 0;
                capacity    = 0;
                res         = STATUS_OK;
            }

            ~chunk_t()
            {
                if (data != NULL)
                {
                    ::free(data);
                    data    = NULL;
                }
                offset      = 0;
                capacity    = 0;
            }

            void clear()
            {
                offset      = 0;
                res         = STATUS_OK;
            }

            bool ensure_capacity(size_t count)
            {
                size_t cap      = offset + count;
                if (cap <= capacity)
                    return true;

                cap            += (cap >> 1);
                uint8_t *ptr    = reinterpret_cast<uint8_t *>(::realloc(data, cap));
                if (ptr == NULL)
                {
                    res             = STATUS_NO_MEM;
                    return false;
                }

                data            = ptr;
                capacity        = cap;
                return true;
            }

            size_t write(const void *bytes, size_t count)
            {
                if (res != STATUS_OK)
                    return 0;

                if (!ensure_capacity(count))
                    return 0;

                size_t off      = offset;
                ::memcpy(&data[offset], bytes, count);
                offset         += count;
                return off;
            }

            template <class T>
                size_t write(T value)
                {
                    if (res != STATUS_OK)
                        return 0;

                    if (!ensure_capacity(sizeof(value)))
                        return 0;

                    size_t off      = offset;
                    IF_UNALIGNED_MEMORY_SAFE(
                        *reinterpret_cast<T *>(&data[off])  = CPU_TO_BE(value);
                    )
                    IF_UNALIGNED_MEMORY_UNSAFE(
                        value           = CPU_TO_BE(value);
                        ::memcpy(&data[off], &value, sizeof(value));
                    )
                    offset         += sizeof(value);
                    return off;
                }

            template <class T>
                bool write_at(size_t position, T value)
                {
                    if (res != STATUS_OK)
                        return false;

                    if ((offset - position) < sizeof(T))
                    {
                        res = STATUS_OVERFLOW;
                        return false;
                    }

                    IF_UNALIGNED_MEMORY_SAFE(
                        *reinterpret_cast<T *>(&data[position])  = CPU_TO_BE(value);
                    )
                    IF_UNALIGNED_MEMORY_UNSAFE(
                        value           = CPU_TO_BE(value);
                        ::memcpy(&data[position], &value, sizeof(value));
                    )
                    return true;
                }

            inline size_t write_string(const char *str)
            {
                if (res != STATUS_OK)
                    return 0;

                size_t slen     = ::strlen(str) + 1;
                if (!ensure_capacity(slen))
                    return 0;

                size_t off      = offset;
                ::memcpy(&data[offset], str, slen);
                offset         += slen;
                return off;
            }

            inline size_t write_byte(int b)
            {
                if (res != STATUS_OK)
                    return 0;

                if (!ensure_capacity(sizeof(uint8_t)))
                    return 0;

                size_t off      = offset;
                data[offset++]  = uint8_t(b);
                return off;
            }

            template <class T>
                inline T *fetch(size_t offset)
                {
                    return reinterpret_cast<T *>(&data[offset]);
                }
        } chunk_t;
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_CHUNK_H_ */
