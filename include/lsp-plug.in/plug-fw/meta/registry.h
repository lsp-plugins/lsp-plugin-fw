/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 4 февр. 2026 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_META_REGISTRY_H_
#define LSP_PLUG_IN_PLUG_FW_META_REGISTRY_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/common/types.h>

#include <lsp-plug.in/plug-fw/meta/types.h>

#define LSP_PLUG_IN_META_GEN_UNIQUE_NAME2(prefix, counter)   meta_registry ## counter
#define LSP_PLUG_IN_META_GEN_UNIQUE_NAME1(prefix, counter)   LSP_PLUG_IN_META_GEN_UNIQUE_NAME2(prefix, counter)
#define LSP_PLUG_IN_META_GEN_UNIQUE_NAME(prefix)             LSP_PLUG_IN_META_GEN_UNIQUE_NAME1(prefix, __COUNTER__)

#define LSP_REGISTER_METADATA(metadata) \
    static ::lsp::meta::Registry LSP_PLUG_IN_META_GEN_UNIQUE_NAME(meta_registry) (metadata);

namespace lsp
{
    namespace meta
    {
        class Registry
        {
            private:
                static const Registry * pRoot;

                const plugin_t * const pPlugin;
                const Registry * const pNext;

            public:
                explicit Registry(const plugin_t *meta) noexcept;
                explicit Registry(const plugin_t & meta) noexcept;
                Registry(const Registry &) = delete;
                Registry(Registry &&) = delete;

                Registry & operator = (const Registry &) = delete;
                Registry & operator = (Registry &&) = delete;

            public:
                inline const Registry *next() const noexcept    { return pNext;     }
                inline const plugin_t *plugin() const noexcept  { return pPlugin;   }
                static inline const Registry *root() noexcept   { return pRoot;     }
        };
    } /* namespace meta */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_META_REGISTRY_H_ */
