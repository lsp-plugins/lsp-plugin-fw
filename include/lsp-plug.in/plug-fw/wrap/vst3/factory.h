/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 дек. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/ipc/IExecutor.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/ptrset.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>

#include <steinberg/vst3.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/data.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/sync.h>

namespace lsp
{
    namespace vst3
    {
        class Wrapper;

        /**
         * Plugin factory. Scans plugin metadata at initialization stage and prepares
         * coresponding VST3 data structures.
         */
        class PluginFactory:
            public Steinberg::IPluginFactory3,
            public ipc::IRunnable
        {
            protected:
                volatile uatomic_t                      nRefCounter;    // Reference counter
                uatomic_t                               nRefExecutor;   // Number of references to the executor
                ipc::Mutex                              sMutex;         // Mutex for managing factory state
                ipc::Mutex                              sDataMutex;     // Mutex for managing data synchronization primitives
                resource::ILoader                      *pLoader;        // Resource loader
                ipc::IExecutor                         *pExecutor;      // Offline task executor
                ipc::Thread                            *pDataSync;      // Synchronization thread
                meta::package_t                        *pPackage;       // Package manifest
                volatile IDataSync                     *pActiveSync;    // Active data sync
                lltl::ptrset<IDataSync>                 vDataSync;      // List of objects for synchronization
                lltl::parray<IUISync>                   vUISync;        // List of UI for synchronization

            #ifdef VST_USE_RUNLOOP_IFACE
                Steinberg::Linux::IRunLoop             *pRunLoop;       // Run loop interface
                Steinberg::Linux::ITimerHandler        *pTimer;         // Timer handler
            #endif /* VST_USE_RUNLOOP_IFACE */

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

            public:
                PluginFactory();
                PluginFactory(const PluginFactory &) = delete;
                PluginFactory(PluginFactory &&) = delete;
                virtual ~PluginFactory();

                PluginFactory & operator = (const PluginFactory &) = delete;
                PluginFactory & operator = (PluginFactory &&) = delete;

                status_t    init();
                void        destroy();

            public:
                ipc::IExecutor                         *acquire_executor();
                void                                    release_executor();
                status_t                                register_data_sync(IDataSync *sync);
                status_t                                unregister_data_sync(IDataSync *sync);
                status_t                                register_ui_sync(IUISync *sync);
                status_t                                unregister_ui_sync(IUISync *sync);

            public: // ipc::IRunnable
                virtual status_t                        run();

            public: // FUnknown
                virtual Steinberg::tresult PLUGIN_API   queryInterface(const Steinberg::TUID _iid, void **obj) override;
                virtual Steinberg::uint32 PLUGIN_API    addRef() override;
                virtual Steinberg::uint32 PLUGIN_API    release() override;

            public: // IPluginFactory
                virtual Steinberg::tresult PLUGIN_API   getFactoryInfo(Steinberg::PFactoryInfo *info) override;
                virtual Steinberg::int32 PLUGIN_API     countClasses() override;
                virtual Steinberg::tresult PLUGIN_API   getClassInfo(Steinberg::int32 index, Steinberg::PClassInfo *info) override;
                virtual Steinberg::tresult PLUGIN_API   createInstance(Steinberg::FIDString cid, Steinberg::FIDString _iid, void **obj) override;

            public: // IPluginFactory2
                virtual Steinberg::tresult PLUGIN_API   getClassInfo2(Steinberg::int32 index, Steinberg::PClassInfo2 *info) override;

            public: // IPluginFactory3
                virtual Steinberg::tresult PLUGIN_API   getClassInfoUnicode(Steinberg::int32 index, Steinberg::PClassInfoW *info) override;
                virtual Steinberg::tresult PLUGIN_API   setHostContext(Steinberg::FUnknown *context) override;
        };

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_FACTORY_H_ */
