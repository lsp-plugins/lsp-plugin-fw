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

#include <lsp-plug.in/plug-fw/meta/registry.h>

namespace lsp
{
    namespace meta
    {
        const Registry * Registry::pRoot = NULL;

        Registry::Registry(const plugin_t *meta) noexcept:
            pPlugin(meta),
            pNext(pRoot)
        {
            pRoot = this;
        }

        Registry::Registry(const plugin_t & meta) noexcept:
            pPlugin(&meta),
            pNext(pRoot)
        {
            pRoot = this;
        }

    } /* namespace meta */
} /* namespace lsp */



