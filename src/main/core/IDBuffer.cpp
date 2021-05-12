/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 мая 2021 г.
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

#include <lsp-plug.in/plug-fw/core/IDBuffer.h>
#include <lsp-plug.in/stdlib/stdlib.h>
#include <lsp-plug.in/common/alloc.h>

namespace lsp
{
    namespace core
    {
        IDBuffer *IDBuffer::create(size_t lines, size_t items)
        {
            size_t b_size       = align_size(sizeof(IDBuffer) + lines * sizeof(float *), 64);
            size_t v_size       = align_size(sizeof(float) * items, 64);
            size_t alloc        = b_size + v_size*lines + 64; // Additional 64-byte alignment for buffes
            uint8_t *ptr        = reinterpret_cast<uint8_t *>(malloc(alloc));
            if (ptr == NULL)
                return NULL;
            IDBuffer *r         = reinterpret_cast<IDBuffer *>(ptr);
            ptr                 = align_ptr(ptr + b_size, 64);

            for (size_t i=0; i<lines; ++i)
            {
                r->v[i]             = reinterpret_cast<float *>(ptr);
                ptr                += v_size;
            }

            r->lines            = lines;
            r->items            = items;
            return r;
        }

        IDBuffer *IDBuffer::resize(size_t l, size_t i)
        {
            if ((lines == l) && (items == i))
                return this;

            free(this);
            return create(l, i);
        }

        IDBuffer *IDBuffer::reuse(IDBuffer *buf, size_t lines, size_t items)
        {
            return (buf != NULL) ? buf->resize(lines, items) : create(lines, items);
        }

        void IDBuffer::detroy()
        {
            free(this);
        }
    }
}



