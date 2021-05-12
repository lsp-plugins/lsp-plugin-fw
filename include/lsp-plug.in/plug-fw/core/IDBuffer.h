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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_IDBUFFER_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_IDBUFFER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace core
    {
        /**
         * Inline display buffer: buffer that helps to cache data for plotting
         * for the InlineDisplay feature
         */
        class IDBuffer
        {
            private:
                IDBuffer & operator = (const IDBuffer *);

            public:
                size_t      lines;
                size_t      items;
                float      *v[];

            private:
                explicit IDBuffer();

            public:
                static IDBuffer        *create(size_t lines, size_t items);
                static IDBuffer        *reuse(IDBuffer *buf, size_t lines, size_t items);
                void                    destroy();
                IDBuffer               *resize(size_t lines, size_t items);
        };
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_IDBUFFER_H_ */
