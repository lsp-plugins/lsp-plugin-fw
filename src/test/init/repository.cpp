/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 янв. 2021 г.
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

#include <lsp-plug.in/test-fw/init.h>
#include <lsp-plug.in/io/Path.h>

#ifdef LSP_IDE_DEBUG
namespace lsp
{
    namespace resource
    {
        // Resource merge
        status_t build_repository(const char *dst, const char * const *paths, size_t npaths);
    }
}
#endif /* LSP_IDE_DEBUG */

INIT_BEGIN(repository)

    INIT_FUNC
    {
    #ifdef LSP_IDE_DEBUG
        static const char *paths[]=
        {
            "",
            "modules/*"
        };

        io::Path resdir;
        resdir.set(tempdir(), "resources");
        lsp::resource::build_repository(resdir.as_utf8(), paths, sizeof(paths)/sizeof(const char *));
    #endif /* LSP_IDE_DEBUG */
    }
INIT_END





