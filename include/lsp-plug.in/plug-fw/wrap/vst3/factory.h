/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 27 дек. 2023 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        /**
         * Plugin factory. Scans plugin metadata at initialization stage and prepares
         * coresponding VST3 data structures.
         */
        class PluginFactory: public Steinberg::IPluginFactory3
        {
            protected:
                volatile uatomic_t                      nRefCounter;
                resource::ILoader                      *pLoader;
                Steinberg::PFactoryInfo                 sFactoryInfo;
                lltl::darray<Steinberg::PClassInfo>     vClassInfo;
                lltl::darray<Steinberg::PClassInfo2>    vClassInfo2;
                lltl::darray<Steinberg::PClassInfoW>    vClassInfoW;

            protected:
                void fill_factory_info(const meta::package_t *manifest);
                status_t fill_plugin_info(const meta::package_t *manifest);
                status_t create_class_info(const meta::package_t *manifest, const meta::plugin_t *meta);
                status_t create_class_info2(const meta::package_t *manifest, const meta::plugin_t *meta);
                status_t create_class_infow(const meta::package_t *manifest, const meta::plugin_t *meta);
                status_t make_plugin_categories(LSPString *dst, const meta::plugin_t *meta);

            public:
                PluginFactory();
                PluginFactory(const PluginFactory &) = delete;
                PluginFactory(PluginFactory &&) = delete;
                virtual ~PluginFactory();

                PluginFactory & operator = (const PluginFactory &) = delete;
                PluginFactory & operator = (PluginFactory &&) = delete;

                status_t    init();
                void        destroy();

            public: // FUnknown
                virtual Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;

                virtual Steinberg::uint32 PLUGIN_API addRef() override;

                virtual Steinberg::uint32 PLUGIN_API release() override;

            public: // IPluginFactory
                virtual Steinberg::tresult PLUGIN_API getFactoryInfo(Steinberg::PFactoryInfo *info) override;

                virtual Steinberg::int32 PLUGIN_API countClasses() override;

                virtual Steinberg::tresult PLUGIN_API getClassInfo(Steinberg::int32 index, Steinberg::PClassInfo *info) override;

                virtual Steinberg::tresult PLUGIN_API createInstance(Steinberg::FIDString cid, Steinberg::FIDString _iid, void **obj) override;

            public: // IPluginFactory2

                virtual Steinberg::tresult PLUGIN_API getClassInfo2(Steinberg::int32 index, Steinberg::PClassInfo2 *info) override;

            public: // IPluginFactory3

                virtual Steinberg::tresult PLUGIN_API getClassInfoUnicode(Steinberg::int32 index, Steinberg::PClassInfoW *info) override;

                virtual Steinberg::tresult PLUGIN_API setHostContext(Steinberg::FUnknown *context) override;
        };

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_FACTORY_H_ */
