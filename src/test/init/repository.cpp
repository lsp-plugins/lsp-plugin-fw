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
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/util/repository/repository.h>

#include <private/test/repository.h>

namespace lsp
{
    namespace test
    {
        static void remove_dir(const io::Path *path)
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

        void make_repository(const io::Path *path)
        {
            repository::cmdline_t cmd;

            static const char *paths[]=
            {
                "",
                "modules/*",
                NULL
            };

            static const char *vars[]=
            {
                "ARTIFACT_ID=test",
                "ARTIFACT_DESC=Test Case",
                "ARTIFACT_VERSION=0.0.0-devel",
                NULL
            };

            cmd.strict      = false;
            cmd.dst_dir     = path->as_utf8();
            cmd.local_dir   = NULL;
            cmd.manifest    = "res/manifest.json";
            cmd.checksums   = NULL;

            for (const char **p = paths; *p != NULL; ++p)
                cmd.paths.add(const_cast<char *>(*p));
            for (const char **p = vars; *p != NULL; ++p)
                cmd.vars.add(const_cast<char *>(*p));

            remove_dir(path);
            lsp::repository::make_repository(&cmd);
        }
    } /* namespace test */
} /* namespace lsp */

#ifdef LSP_NO_BUILTIN_RESOURCES

INIT_BEGIN(repository)

    INIT_FUNC
    {
        io::Path resdir;
        resdir.set(tempdir(), "resources");
        make_repository(&resdir);
    }
INIT_END

#endif /* LSP_NO_BUILTIN_RESOURCES */





