/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 авг. 2024 г.
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

#include <lsp-plug.in/plug-fw/core/ShmState.h>
#include <lsp-plug.in/common/alloc.h>

namespace lsp
{
    namespace core
    {
        ShmState::ShmState(ShmRecord *items, char *strings, size_t count)
        {
            vItems      = items;
            nItems      = count;
            vStrings    = strings;
        }

        ShmState::~ShmState()
        {
            nItems      = 0;
            if (vItems != NULL)
            {
                free(vItems);
                vItems      = NULL;
            }

            if (vStrings != NULL)
            {
                free(vStrings);
                vStrings    = NULL;
            }
        }

        size_t ShmState::size() const
        {
            return nItems;
        }

        const ShmRecord *ShmState::get(size_t index) const
        {
            return (index < nItems) ? &vItems[index] : NULL;
        }

    } /* namespace core */
} /* namespace lsp */




