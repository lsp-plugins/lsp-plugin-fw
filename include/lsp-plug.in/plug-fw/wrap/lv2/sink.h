/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_SINK_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_SINK_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/lv2.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/stdlib/stdlib.h>

namespace lsp
{
    namespace lv2
    {
        typedef struct lv2_sink
        {
            uint8_t    *buf;
            size_t      size;
            size_t      cap;
            status_t    res;
            LV2_Atom    stub;

            static LV2_Atom_Forge_Ref sink(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
            {
                lv2_sink *_this = reinterpret_cast<lv2_sink *>(handle);
                if (_this->res != STATUS_OK)
                    return 0;

                size_t offset   = _this->size;
                size_t capacity = offset + size;
                if (capacity > _this->cap)
                {
                    capacity       += (capacity >> 1);
                    uint8_t *ptr    = reinterpret_cast<uint8_t *>(::realloc(_this->buf, capacity));
                    if (ptr == NULL) {
                        _this->res      = STATUS_NO_MEM;
                        return 0;
                    }
                    _this->buf      = ptr;
                    _this->cap      = capacity;
                }

                ::memcpy(&_this->buf[offset], buf, size);
                _this->size    += size;

                // From LV2 spec:
                // Note that 0 is an invalid reference, so if you are using a buffer offset be
                // sure to offset it such that 0 is never a valid reference.  You will get
                // confusing errors otherwise.
                // So we will add the sizeof(LV2_Atom) for this case
                return offset + sizeof(LV2_Atom);
            }

            static LV2_Atom *deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
            {
                lv2_sink *_this = reinterpret_cast<lv2_sink *>(handle);
                if (_this->res == STATUS_OK)
                {
                    // From LV2 spec:
                    // Note that 0 is an invalid reference, so if you are using a buffer offset be
                    // sure to offset it such that 0 is never a valid reference.  You will get
                    // confusing errors otherwise.
                    // So we will subtract the sizeof(LV2_Atom) for this case
                    return reinterpret_cast<LV2_Atom *>(&_this->buf[ref - sizeof(LV2_Atom)]);
                }

                return &_this->stub;
            }

            explicit lv2_sink(size_t capacity = 0x100)
            {
                buf         = NULL;
                size        = 0;
                cap         = capacity;
                res         = STATUS_OK;
                stub.size   = 0;
                stub.type   = 0;

                if (cap > 0)
                {
                    buf     = reinterpret_cast<uint8_t *>(::malloc(capacity));
                    if (buf == NULL)
                        res     = STATUS_NO_MEM;
                }
            }

            ~lv2_sink()
            {
                if (buf != NULL)
                {
                    ::free(buf);
                    buf     = NULL;
                }
                size        = 0;
                cap         = 0;
            }
        } lv2_sink;

    } /* namespace lv2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_SINK_H_ */
