/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 янв. 2024 г.
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

#ifndef PLUG_IN_PLUG_FW_WRAP_VST3_UI_WRAPPER_H_
#define PLUG_IN_PLUG_FW_WRAP_VST3_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/ui.h>

#include <steinberg/vst3.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/string_buf.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/sync.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ui_ports.h>

namespace lsp
{
    namespace vst3
    {
        class PluginFactory;

        #include <steinberg/vst3/base/WarningsPush.h>
        class UIWrapper:
            public ui::IWrapper,
            public IPortChangeHandler,
            public Steinberg::IDependent,
            public Steinberg::Vst::IComponent,
            public Steinberg::Vst::IConnectionPoint,
            public Steinberg::Vst::IEditController,
            public Steinberg::Vst::IEditController2
        {
            protected:
                volatile uatomic_t                  nRefCounter;            // Reference counter
                PluginFactory                      *pFactory;               // Reference to the factory
                const meta::package_t              *pPackage;
                Steinberg::FUnknown                *pHostContext;           // Host context
                Steinberg::Vst::IHostApplication   *pHostApplication;       // Host application
                Steinberg::Vst::IConnectionPoint   *pPeerConnection;        // Peer connection
                Steinberg::Vst::IComponentHandler  *pComponentHandler;      // Component handler
                Steinberg::Vst::IComponentHandler2 *pComponentHandler2;     // Component handler (version 2)
                Steinberg::Vst::IComponentHandler3 *pComponentHandler3;     // Component handler (version 3)

                lltl::parray<meta::port_t>          vGenMetadata;           // Generated metadata
                vst3::string_buf                    sNotifyBuf;             // Notify buffer

            protected:
                vst3::UIPort                       *create_port(const meta::port_t *port, const char *postfix);

            public:
                explicit UIWrapper(PluginFactory *factory, ui::Module *ui, resource::ILoader *loader, const meta::package_t *package);
                UIWrapper(const UIWrapper &) = delete;
                UIWrapper(UIWrapper &&) = delete;
                virtual ~UIWrapper() override;

                UIWrapper & operator = (const UIWrapper &) = delete;
                UIWrapper & operator = (UIWrapper &&) = delete;

                virtual status_t                    init(void *root_widget) override;
                virtual void                        destroy() override;

            public: // ui::Wrapper
                virtual core::KVTStorage           *kvt_lock() override;
                virtual core::KVTStorage           *kvt_trylock() override;
                virtual bool                        kvt_release() override;
                virtual void                        dump_state_request() override;
                virtual const meta::package_t      *package() const override;
                virtual status_t                    play_file(const char *file, wsize_t position, bool release) override;
                virtual float                       ui_scaling_factor(float scaling) override;
                virtual void                        main_iteration() override;
                virtual bool                        accept_window_size(size_t width, size_t height) override;

            public: // vst3::IPortChangeHandler
                virtual void                        port_write(ui::IPort *port, size_t flags) override;

            public: // Steinberg::FUnknown
                virtual Steinberg::tresult          PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;
                virtual Steinberg::uint32           PLUGIN_API addRef() override;
                virtual Steinberg::uint32           PLUGIN_API release() override;

            public: // Steinberg::IDependent
                virtual void                        PLUGIN_API update(FUnknown* changedUnknown, Steinberg::int32 message) override;

            public: // Steinberg::IPluginBase
                virtual Steinberg::tresult          PLUGIN_API initialize(Steinberg::FUnknown *context) override;
                virtual Steinberg::tresult          PLUGIN_API terminate() override;

            public: // Steinberg::Vst::IComponent
                virtual Steinberg::tresult          PLUGIN_API getControllerClassId(Steinberg::TUID classId) override;
                virtual Steinberg::tresult          PLUGIN_API setIoMode(Steinberg::Vst::IoMode mode) override;
                virtual Steinberg::int32            PLUGIN_API getBusCount(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir) override;
                virtual Steinberg::tresult          PLUGIN_API getBusInfo(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::BusInfo & bus /*out*/) override;
                virtual Steinberg::tresult          PLUGIN_API getRoutingInfo(Steinberg::Vst::RoutingInfo & inInfo, Steinberg::Vst::RoutingInfo & outInfo /*out*/) override;
                virtual Steinberg::tresult          PLUGIN_API activateBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::TBool state) override;
                virtual Steinberg::tresult          PLUGIN_API setActive(Steinberg::TBool state) override;
                virtual Steinberg::tresult          PLUGIN_API setState(Steinberg::IBStream *state) override;
                virtual Steinberg::tresult          PLUGIN_API getState(Steinberg::IBStream *state) override;

            public: // Steinberg::vst::IConnectionPoint
                virtual Steinberg::tresult          PLUGIN_API connect(Steinberg::Vst::IConnectionPoint *other) override;
                virtual Steinberg::tresult          PLUGIN_API disconnect(Steinberg::Vst::IConnectionPoint *other) override;
                virtual Steinberg::tresult          PLUGIN_API notify(Steinberg::Vst::IMessage *message) override;

            public: // Steinberg::vst::IEditController
                virtual Steinberg::tresult          PLUGIN_API setComponentState(Steinberg::IBStream *state) override;
                virtual Steinberg::int32            PLUGIN_API getParameterCount() override;
                virtual Steinberg::tresult          PLUGIN_API getParameterInfo(Steinberg::int32 paramIndex, Steinberg::Vst::ParameterInfo & info /*out*/) override;
                virtual Steinberg::tresult          PLUGIN_API getParamStringByValue(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized /*in*/, Steinberg::Vst::String128 string /*out*/) override;
                virtual Steinberg::tresult          PLUGIN_API getParamValueByString(Steinberg::Vst::ParamID id, Steinberg::Vst::TChar *string /*in*/, Steinberg::Vst::ParamValue & valueNormalized /*out*/) override;
                virtual Steinberg::Vst::ParamValue  PLUGIN_API normalizedParamToPlain(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized) override;
                virtual Steinberg::Vst::ParamValue  PLUGIN_API plainParamToNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue plainValue) override;
                virtual Steinberg::Vst::ParamValue  PLUGIN_API getParamNormalized(Steinberg::Vst::ParamID id) override;
                virtual Steinberg::tresult          PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) override;
                virtual Steinberg::tresult          PLUGIN_API setComponentHandler(Steinberg::Vst::IComponentHandler *handler) override;
                virtual Steinberg::IPlugView *      PLUGIN_API createView(Steinberg::FIDString name) override;

            public: // Steinberg::vst::IEditController2
                virtual Steinberg::tresult          PLUGIN_API setKnobMode(Steinberg::Vst::KnobMode mode) override;
                virtual Steinberg::tresult          PLUGIN_API openHelp(Steinberg::TBool onlyCheck) override;
                virtual Steinberg::tresult          PLUGIN_API openAboutBox(Steinberg::TBool onlyCheck) override;
        };
        #include <steinberg/vst3/base/WarningsPop.h>

    } /* namespace vst3 */
} /* namespace lsp */




#endif /* PLUG_IN_PLUG_FW_WRAP_VST3_UI_WRAPPER_H_ */
