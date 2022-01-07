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

#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/ipc/Library.h>
#include <lsp-plug.in/resource/BuiltinLoader.h>
#include <lsp-plug.in/resource/DirLoader.h>
#include <lsp-plug.in/resource/PrefixLoader.h>
#include <lsp-plug.in/runtime/system.h>

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

        resource::ILoader *create_builtin_loader()
        {
            resource::ILoader *loader = NULL;

            // Check that we have built-in resources
            core::Resources *r = core::Resources::root();
            if (r != NULL)
                loader = r->loader();

            if (loader != NULL)
                lsp_trace("Using built-in resource loader");
            else
                lsp_trace("No builtin resources are present");

            return loader;
        }

        static resource::ILoader *create_directory_loader()
        {
            io::Path fpath;
            LSPString path;
            status_t res;

            if ((res = system::get_env_var("LSP_RESOURCE_PATH", &path)) == STATUS_OK)
            {
                // Nothing
            }
            else if ((res = ipc::Library::get_self_file(&fpath)) == STATUS_OK)
            {
                if ((res = fpath.get_parent(&path)) != STATUS_OK)
                    lsp_warn("Could not obtain binary path");
            }
            else if ((res = system::get_current_dir(&path)) != STATUS_OK)
                lsp_warn("Could not obtain current directory");

            // Create resource loader
            if (res != STATUS_OK)
            {
                lsp_warn("Could not obtain directory with resources");
                return NULL;
            }

            resource::DirLoader *dldr = new resource::DirLoader();
            if (dldr == NULL)
            {
                lsp_warn("Failed to allocate directory loader");
                return NULL;
            }

            if ((res = dldr->set_path(&path)) != STATUS_OK)
            {
                lsp_warn("Failed to initialize directory loader, error=%d", int(res));
                delete dldr;
            }
            dldr->set_enforce(true);

            lsp_trace("Using builtin resource path: %s", path.get_native());

            return dldr;
        }

        LSP_SYMBOL_HIDDEN
        resource::ILoader *create_resource_loader()
        {
            // Get bultin resource loader.
            status_t res;
            resource::ILoader *loader = create_builtin_loader();
            if (loader == NULL)
                loader = create_directory_loader();

            // Get prefix loader for all operations with resources
            resource::PrefixLoader *pldr = new resource::PrefixLoader();
            if (pldr == NULL)
            {
                lsp_warn("Error creating prefix loader");
                if (loader != NULL)
                    delete loader;
                return NULL;
            }

            // Add loader to prefix loader with 'builtin://' prefix
            if (loader != NULL)
            {
                if ((res = pldr->add_prefix(LSP_BUILTIN_PREFIX, loader)) != STATUS_OK)
                {
                    lsp_warn("Error setting loader to prefix '%s', error=%d", LSP_BUILTIN_PREFIX, int(res));
                    delete loader;
                }
            }

            return pldr;
        }
    }
}


