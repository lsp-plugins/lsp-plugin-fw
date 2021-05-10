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
#include <lsp-plug.in/io/Dir.h>

#ifdef LSP_IDE_DEBUG
namespace lsp
{
    namespace resource
    {
        // Resource merge
        status_t build_repository(const char *dst, const char *local_dir, const char * const *paths, size_t npaths);
    }
}
#endif /* LSP_IDE_DEBUG */

INIT_BEGIN(repository)

    void remove_dir(const io::Path *path)
    {
        io::Path child;
        LSPString item;
        io::Dir dir;

        if (dir.open(path) == STATUS_OK)
        {
            while (dir.read(&item) == STATUS_OK)
            {
                if (io::Path::is_dots(&item))
                    continue;
                if (child.set(path, &item) != STATUS_OK)
                    continue;

                if (child.is_dir())
                    remove_dir(&child);
                else
                {
                    printf("  removing: %s\n", child.as_native());
                    child.remove();
                }
            }
        }

        printf("  removing: %s\n", path->as_native());
        path->remove();
    }


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

        remove_dir(&resdir);
        lsp::resource::build_repository(resdir.as_utf8(), "res/local", paths, sizeof(paths)/sizeof(const char *));
    #endif /* LSP_IDE_DEBUG */
    }
INIT_END





