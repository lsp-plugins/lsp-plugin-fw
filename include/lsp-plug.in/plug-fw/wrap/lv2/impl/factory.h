/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 сент. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_IMPL_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_IMPL_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/factory.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace lv2
    {
        Factory::Factory()
        {
            nReferences     = 1;

            // Obtain the resource loader
            lsp_trace("Obtaining resource loader...");
            pLoader = core::create_resource_loader();
            if (pLoader == NULL)
                return;

            // Obtain the manifest
            lsp_trace("Obtaining manifest...");
            status_t res = load_manifest(&pPackage, pLoader);
            if (res != STATUS_OK)
                lsp_trace("No manifest file found");
        }

        Factory::~Factory()
        {
        }

        size_t Factory::acquire()
        {
            return atomic_add(&nReferences, 1) + 1;
        }

        size_t Factory::release()
        {
            atomic_t ref_count = atomic_add(&nReferences, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        status_t Factory::create_plugin(plug::Module **module, const char *lv2_uri)
        {
            // Lookup plugin identifier among all registered plugin factories
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Check plugin identifier
                    if (!::strcmp(meta->uids.lv2, lv2_uri))
                    {
                        // Instantiate the plugin and return
                        plug::Module *m = f->create(meta);
                        if (m != NULL)
                        {
                            *module = m;
                            return STATUS_OK;
                        }

                        fprintf(stderr, "Plugin instantiation error: %s\n", lv2_uri);
                        return STATUS_NO_MEM;
                    }
                }
            }

            // No plugin has been found
            return STATUS_NOT_FOUND;
        }

        core::Catalog *Factory::acquire_catalog()
        {
            return sCatalogManager.acquire();
        }

        void Factory::release_catalog(core::Catalog *catalog)
        {
            sCatalogManager.release(catalog);
        }

        const meta::package_t *Factory::manifest() const
        {
            return pPackage;
        }

        resource::ILoader *Factory::resources()
        {
            return pLoader;
        }

    } /* namespace lv2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_IMPL_FACTORY_H_ */
