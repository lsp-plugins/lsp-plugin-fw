/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace plug
    {
        Factory *Factory::pRoot = NULL;

        Factory::Factory()
        {
            pNext       = pRoot;
            pRoot       = this;
            pFunc       = NULL;
            vList       = NULL;
            nItems      = 0;
        }

        Factory::Factory(factory_func_t func, const meta::plugin_t **list, size_t items)
        {
            pNext       = pRoot;
            pRoot       = this;
            pFunc       = func;
            vList       = list;
            nItems      = items;
        }

        Factory::~Factory()
        {
            pNext       = NULL;
        }

        const meta::plugin_t *Factory::enumerate(size_t index) const
        {
            return (vList != NULL) && (index < nItems) ? vList[index] : NULL;
        }

        Module *Factory::create(const meta::plugin_t *meta) const
        {
            if (vList == NULL)
                return NULL;

            for (size_t i=0; i<nItems; ++i)
            {
                if (meta == vList[i])
                    return pFunc(meta);
            }

            return NULL;
        }
    }
}


