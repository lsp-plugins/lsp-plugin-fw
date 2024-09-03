/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 сент. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/core/CatalogManager.h>
#include <lsp-plug.in/plug-fw/core/ICatalogFactory.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>

#include <clap/clap.h>

namespace lsp
{
    namespace clap
    {
        class Wrapper;

        class Factory: public core::ICatalogFactory
        {
            private:
                atomic_t                    nReferences;
                lltl::darray<clap_plugin_descriptor_t> vDescriptors;
                resource::ILoader          *pLoader;
                meta::package_t            *pManifest;
                core::CatalogManager        sCatalogManager;    // Catalog management

            private:
                static ssize_t              cmp_descriptors(const clap_plugin_descriptor_t *d1, const clap_plugin_descriptor_t *d2);
                static meta::package_t     *load_manifest(resource::ILoader *loader);
                static const char * const  *make_feature_list(const meta::plugin_t *meta);
                static void                 drop_descriptor(clap_plugin_descriptor_t *d);

            private:
                void                        generate_descriptors();
                void                        drop_descriptors();

            public:
                Factory();
                Factory(const Factory &) = delete;
                Factory(Factory &&) = delete;
                virtual ~Factory() override;

                Factory & operator = (const Factory &) = delete;
                Factory & operator = (Factory &&) = delete;

            public: // core::ICatalogFactory
                virtual core::Catalog      *acquire_catalog() override;
                virtual void                release_catalog(core::Catalog *catalog) override;

            public:
                size_t                      acquire();
                size_t                      release();

            public:
                const meta::package_t      *manifest() const;
                resource::ILoader          *resources();

                size_t                      descriptors_count() const;
                const clap_plugin_descriptor_t *descriptor(size_t index) const;
                Wrapper                    *instantiate(const clap_host_t *host, const clap_plugin_descriptor_t *descriptor) const;
        };

    } /* namespace clap */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_FACTORY_H_ */
