/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 февр. 2024 г.
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
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/modinfo.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

namespace lsp
{
    namespace vst3_modinfo
    {
        int main(int argc, const char **argv)
        {
            if (argc < 2)
            {
                fprintf(stderr, "Required destination moduleinfo.json file\n");
                return STATUS_BAD_ARGUMENTS;
            }

            // Create resource loader
            resource::ILoader *loader   = core::create_resource_loader();
            if (loader == NULL)
            {
                fprintf(stderr, "No resource loader available\n");
                return STATUS_BAD_STATE;
            }

            status_t res;
            meta::package_t *manifest = NULL;
            {
                // Obtain the manifest file descriptor
                io::IInStream *is = loader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
                if (is == NULL)
                {
                    fprintf(stderr, "No manifest file found\n");
                    return STATUS_BAD_STATE;
                }
                lsp_finally {
                    is->close();
                    delete is;
                };

                // Read manifest data
                if ((res = meta::load_manifest(&manifest, is)) != STATUS_OK)
                {
                    fprintf(stderr, "Error loading manifest file, error=%d\n", int(res));
                    return res;
                }
            }
            if (manifest == NULL)
            {
                fprintf(stderr, "No manifest has been loaded\n");
                return STATUS_BAD_STATE;
            }
            lsp_finally {
                meta::free_manifest(manifest);
            };

            // Write the moduleinfo.json file
            io::Path path;
            if ((res = path.set_native(argv[1])) != STATUS_OK)
            {
                fprintf(stderr, "Error parsing manifest path, error=%d\n", int(res));
                return res;
            }

            // Make moduleinfo file
            if ((res = vst3::make_moduleinfo(&path, manifest)) != STATUS_OK)
            {
                fprintf(stderr, "Error creating moduleinfo.json file, error=%d\n", int(res));
                return res;
            }

            // All seems to be OK
            return res;
        }
    } /* namespace vst3_modinfo */
} /* namespace lsp */


