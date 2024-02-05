/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 февр. 2024 г.
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
#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_CONTROLLER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_CONTROLLER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/protocol/osc.h>

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

        #include <steinberg/vst3/base/WarningsPush.h>
        class Controller:
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
                const meta::plugin_t               *pUIMetadata;            // UI metadata
                Steinberg::FUnknown                *pHostContext;           // Host context
                Steinberg::Vst::IHostApplication   *pHostApplication;       // Host application
                Steinberg::Vst::IConnectionPoint   *pPeerConnection;        // Peer connection
                Steinberg::Vst::IComponentHandler  *pComponentHandler;      // Component handler
                Steinberg::Vst::IComponentHandler2 *pComponentHandler2;     // Component handler (version 2)
                Steinberg::Vst::IComponentHandler3 *pComponentHandler3;     // Component handler (version 3)
                lltl::parray<ui::IPort>             vPorts;                 // All possible ports
                lltl::parray<vst3::UIParameterPort> vParams;                // Input parameters (non-virtual) sorted by unique parameter ID
                lltl::parray<vst3::UIMeterPort>     vMeters;                // Meters
                lltl::pphash<char, vst3::UIPort>    vParamMapping;          // Parameter mapping

                lltl::parray<meta::port_t>          vGenMetadata;           // Generated metadata
                vst3::string_buf                    sNotifyBuf;             // Notify buffer
                core::KVTStorage                    sKVT;                   // KVT storage
                ipc::Mutex                          sKVTMutex;              // KVT storage access mutex
                uint32_t                            nLatency;               // Plugin latency
                float                               fScalingFactor;         // Scaling factor

            protected:
                vst3::UIPort                       *create_port(const meta::port_t *port, const char *postfix);
                vst3::UIParameterPort              *find_param(Steinberg::Vst::ParamID param_id);
                void                                receive_raw_osc_packet(const void *data, size_t size);
                void                                parse_raw_osc_event(osc::parse_frame_t *frame);
                status_t                            load_state(Steinberg::IBStream *is);
                void                                position_updated(const plug::position_t *pos);
                void                                dump_state_request();

            protected:
                static ssize_t                      compare_param_ports(const vst3::UIParameterPort *a, const vst3::UIParameterPort *b);
                static ssize_t                      compare_ports_by_id(const ui::IPort *a, const ui::IPort *b);

            public:
                explicit Controller(PluginFactory *factory, resource::ILoader *loader, const meta::package_t *package, const meta::plugin_t *meta);
                Controller(const Controller &) = delete;
                Controller(Controller &&) = delete;
                virtual ~Controller() override;

                Controller & operator = (const Controller &) = delete;
                Controller & operator = (Controller &&) = delete;

                status_t                            init();
                void                                destroy();

            public:
            #ifdef VST_USE_RUNLOOP_IFACE
                Steinberg::Linux::IRunLoop         *acquire_run_loop();
            #endif /* VST_USE_RUNLOOP_IFACE */

            public:
                inline ipc::Mutex                  &kvt_mutex();
                inline core::KVTStorage            &kvt_storage();
                inline const meta::package_t       *package() const;
                status_t                            play_file(const char *file, wsize_t position, bool release);
                ui::IPort                          *port_by_id(const char *id);

            public: // vst3::IPortChangeHandler
                virtual void                        port_write(ui::IPort *port, size_t flags) override;

            public: // Steinberg::FUnknown
                virtual Steinberg::tresult          PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;
                virtual Steinberg::uint32           PLUGIN_API addRef() override;
                virtual Steinberg::uint32           PLUGIN_API release() override;

            public: // Steinberg::IDependent
                virtual void                        PLUGIN_API update(FUnknown *changedUnknown, Steinberg::int32 message) override;

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



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_CONTROLLER_H_ */
