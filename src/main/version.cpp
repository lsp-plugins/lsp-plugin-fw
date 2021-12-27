/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 дек. 2021 г.
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

#include <lsp-plug.in/common/types.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifndef LSP_IDE_DEBUG
    static const ::lsp::version_t module_version =
    {
        PLUGIN_PACKAGE_MAJOR,
        PLUGIN_PACKAGE_MINOR,
        PLUGIN_PACKAGE_MICRO,
        PLUGIN_PACKAGE_BRANCH
    };

    LSP_CSYMBOL_EXPORT
    LSP_DEF_VERSION_FUNC_HEADER
    {
        return &module_version;
    }
#endif /* LSP_IDE_DEBUG */

#ifdef __cplusplus
}
#endif /* __cplusplus */


