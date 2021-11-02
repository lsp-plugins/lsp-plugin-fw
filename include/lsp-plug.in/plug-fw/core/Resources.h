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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_RESOURCES_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_RESOURCES_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/resource/types.h>
#include <lsp-plug.in/resource/ILoader.h>

#define LSP_RESOURCE_BUFSZ      0x100000

namespace lsp
{
    namespace core
    {
        /**
         * Resource factory
         */
        class Resources
        {
            protected:
                static Resources       *pRoot;
                Resources              *pNext;

                const void             *pData;
                size_t                  nSize;
                const resource::raw_resource_t *vEnt;
                size_t                  nEnt;

            public:
                /**
                 * Constructor
                 * @param data the location of compressed resource data
                 * @param size size of the compressed resource data
                 * @param entries the list of all resource entries
                 * @param count the number of resource entries
                 */
                explicit Resources(const void *data, size_t size, const resource::raw_resource_t *entries, size_t count);
                virtual ~Resources();

            public:
                /**
                 * Get root item of the resource chain
                 * @return root item of the resource chain
                 */
                static inline Resources *root()     { return pRoot; }

            public:
                /**
                 * Get next resource in the chain
                 * @return next resource in the chain
                 */
                Resources *next()                   { return pNext; }

                /**
                 * Create resource loader
                 * @return instantiated resource loader
                 */
                virtual resource::ILoader          *loader();
        };


        /**
         * Create resource loader for plugin
         * @return resource loader for plugin, should be deleted after use
         */
        LSP_SYMBOL_HIDDEN
        resource::ILoader *create_resource_loader();

        /**
         * Create builtin resource loader for plugin
         * @return resource loader for plugin, should be deleted after use
         */
        LSP_SYMBOL_HIDDEN
        resource::ILoader *create_builtin_loader();
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_RESOURCES_H_ */
