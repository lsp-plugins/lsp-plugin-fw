/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 янв. 2021 г.
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

#include <lsp-plug.in/test-fw/mtest.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/io/IInStream.h>
#include <lsp-plug.in/io/OutFileStream.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>


MTEST_BEGIN("", unpack)

    status_t unpack_resources(const io::Path *out, const io::Path *path, resource::ILoader *loader)
    {
        resource::resource_t *list = NULL;
        io::Path child, sub;

        ssize_t n = loader->enumerate(path, &list);
        if (n < 0)
        {
            printf("error enumerating path %s: %d\n", path->as_native(), int(-n));
            return -n;
        }

        for (ssize_t i=0; i<n; ++i)
        {
            MTEST_ASSERT(child.set(out, list[i].name) == STATUS_OK);
            MTEST_ASSERT(sub.set(path, list[i].name) == STATUS_OK);

            if (list[i].type == resource::RES_DIR)
            {
                printf("Found subdirectory: %s\n", sub.as_native());
                status_t res = unpack_resources(&child, &sub, loader);
                MTEST_ASSERT(res == STATUS_OK);
            }
            else
            {
                printf("Unpacking resource: %s -> %s\n", sub.as_native(), child.as_native());

                // Create destination path
                MTEST_ASSERT(out->mkdir(true) == STATUS_OK);

                io::OutFileStream ofs;
                io::IInStream *ifs = loader->read_stream(&sub);
                MTEST_ASSERT(ifs != NULL);
                MTEST_ASSERT(ofs.open(&child, io::File::FM_WRITE_NEW) == STATUS_OK);

                wssize_t written = ifs->sink(&ofs);
                MTEST_ASSERT(written >= 0);
                printf("  unpacked size: %ld\n", long(written));

                MTEST_ASSERT(ifs->close() == STATUS_OK);
                MTEST_ASSERT(ofs.close() == STATUS_OK);
                delete ifs;
            }
        }
        free(list);

        return STATUS_OK;
    }

    MTEST_MAIN
    {
        io::Path outdir;

        MTEST_ASSERT(outdir.set(tempdir(), full_name()) == STATUS_OK)

        core::Resources *r = core::Resources::root();
        if (r == NULL)
        {
            lsp_warn("No builtin resources present");
            return;
        }

        resource::ILoader *loader = r->loader();
        MTEST_ASSERT(loader != NULL);

        io::Path path;
        MTEST_ASSERT(path.set("") == STATUS_OK);
        MTEST_ASSERT(unpack_resources(&outdir, &path, loader) == STATUS_OK);
    }

MTEST_END



