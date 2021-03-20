/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 20 мар. 2021 г.
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

#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/resource/BuiltinLoader.h>

namespace lsp
{
    namespace core
    {
        // Static variable
        Resources *Resources::pRoot  = NULL;

        Resources::Resources(const void *data, size_t size, const resource::raw_resource_t *entries, size_t count)
        {
            pNext       = pRoot;
            pData       = data;
            nSize       = size;
            vEnt        = entries;
            nEnt        = count;

            pRoot       = this;
        }

        Resources::~Resources()
        {
            pNext       = NULL;
        }

        resource::ILoader *Resources::loader()
        {
            // Create loader
            resource::BuiltinLoader *ldr = new resource::BuiltinLoader();
            if (ldr == NULL)
                return NULL;

            // Initialize loader
            status_t res = ldr->init(pData, nSize, vEnt, nEnt, LSP_RESOURCE_BUFSZ);
            if (res == STATUS_OK)
                return ldr;

            delete ldr;
            return NULL;
        }
    }
}


