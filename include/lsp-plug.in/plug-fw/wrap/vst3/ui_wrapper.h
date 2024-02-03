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
#include <lsp-plug.in/plug-fw/wrap/vst3/plugview.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ui_ports.h>

namespace lsp
{
    namespace vst3
    {
        class PluginFactory;
        class PluginView;

        #include <steinberg/vst3/base/WarningsPush.h>
        class UIWrapper:
            public ui::IWrapper,
            public vst3::IUISync,
            public IPortChangeHandler,
            public Steinberg::IDependent,
            public Steinberg::Vst::IConnectionPoint,
            public Steinberg::Vst::IEditController,
            public Steinberg::Vst::IEditController2
        {
            private:
                friend class PluginView;

            protected:
                volatile uatomic_t                  nRefCounter;            // Reference counter
                PluginFactory                      *pFactory;               // Reference to the factory
                resource::ILoader                  *pLoader;                // Resource loader
                const meta::package_t              *pPackage;               // Package metadata
                Steinberg::FUnknown                *pHostContext;           // Host context
                PluginView                         *pPluginView;            // Plugin view
                Steinberg::Vst::IHostApplication   *pHostApplication;       // Host application
                Steinberg::Vst::IConnectionPoint   *pPeerConnection;        // Peer connection
                Steinberg::Vst::IComponentHandler  *pComponentHandler;      // Component handler
                Steinberg::Vst::IComponentHandler2 *pComponentHandler2;     // Component handler (version 2)
                Steinberg::Vst::IComponentHandler3 *pComponentHandler3;     // Component handler (version 3)
                lltl::parray<vst3::UIParameterPort> vParams;                // Input parameters (non-virtual) sorted by unique parameter ID
                lltl::parray<vst3::UIMeterPort>     vMeters;                // Meters
                lltl::pphash<char, vst3::UIPort>    vParamMapping;          // Parameter mapping

                lltl::parray<meta::port_t>          vGenMetadata;           // Generated metadata
                vst3::string_buf                    sNotifyBuf;             // Notify buffer
                core::KVTStorage                    sKVT;                   // KVT storage
                ipc::Mutex                          sKVTMutex;              // KVT storage access mutex
                uint32_t                            nLatency;               // Plugin latency
                float                               fScalingFactor;         // Scaling factor
                bool                                bUIInitialized;         // UI initialized flag

            protected:
                static status_t                     slot_ui_resize(tk::Widget *sender, void *ptr, void *data);
                static status_t                     slot_ui_show(tk::Widget *sender, void *ptr, void *data);
                static status_t                     slot_ui_realized(tk::Widget *sender, void *ptr, void *data);
                static status_t                     slot_ui_close(tk::Widget *sender, void *ptr, void *data);
                static status_t                     slot_display_idle(tk::Widget *sender, void *ptr, void *data);

            protected:
                vst3::UIPort                       *create_port(const meta::port_t *port, const char *postfix);
                vst3::UIParameterPort              *find_param(Steinberg::Vst::ParamID param_id);
                void                                receive_raw_osc_packet(const void *data, size_t size);
                void                                parse_raw_osc_event(osc::parse_frame_t *frame);
                status_t                            load_state(Steinberg::IBStream *is);
                status_t                            initialize_ui();
                bool                                start_event_loop();
                void                                stop_event_loop();

            protected:
                static ssize_t                      compare_param_ports(const vst3::UIParameterPort *a, const vst3::UIParameterPort *b);

            public:
                explicit UIWrapper(PluginFactory *factory, ui::Module *ui, resource::ILoader *loader, const meta::package_t *package);
                UIWrapper(const UIWrapper &) = delete;
                UIWrapper(UIWrapper &&) = delete;
                virtual ~UIWrapper() override;

                UIWrapper & operator = (const UIWrapper &) = delete;
                UIWrapper & operator = (UIWrapper &&) = delete;

                virtual status_t                    init(void *root_widget) override;
                virtual void                        destroy() override;

            public:
                status_t                            detach_ui(PluginView *view);
                void                                set_scaling_factor(float factor);

            public: // ui::Wrapper
                virtual core::KVTStorage           *kvt_lock() override;
                virtual core::KVTStorage           *kvt_trylock() override;
                virtual bool                        kvt_release() override;
                virtual void                        dump_state_request() override;
                virtual const meta::package_t      *package() const override;
                virtual status_t                    play_file(const char *file, wsize_t position, bool release) override;
                virtual float                       ui_scaling_factor(float scaling) override;
                virtual bool                        accept_window_size(tk::Window *wnd, size_t width, size_t height) override;

            public: // vst3::IUISync
                virtual void                        sync_ui() override;

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

            public: // Steinberg::vst::IConnectionPoint
                virtual Steinberg::tresult          PLUGIN_API connect(Steinberg::Vst::IConnectionPoint *other) override;
                virtual Steinberg::tresult          PLUGIN_API disconnect(Steinberg::Vst::IConnectionPoint *other) override;
                virtual Steinberg::tresult          PLUGIN_API notify(Steinberg::Vst::IMessage *message) override;

            public: // Steinberg::vst::IEditController
                virtual Steinberg::tresult          PLUGIN_API setState(Steinberg::IBStream *state) override;
                virtual Steinberg::tresult          PLUGIN_API getState(Steinberg::IBStream *state) override;
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
