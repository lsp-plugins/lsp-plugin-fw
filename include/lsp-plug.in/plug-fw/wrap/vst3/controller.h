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
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/state.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/protocol/osc.h>

#include <steinberg/vst3.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/string_buf.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/sync.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ctl_ports.h>

#ifdef WITH_UI_FEATURE
    #include <lsp-plug.in/plug-fw/wrap/vst3/ui_wrapper.h>
#endif /* WITH_UI_FEATURE */

namespace lsp
{
    namespace vst3
    {
        class PluginFactory;
        class UIWrapper;

        #include <steinberg/vst3/base/WarningsPush.h>
        class Controller:
            public IDataSync,
            public CtlPortChangeHandler,
            public Steinberg::IDependent,
            public Steinberg::Vst::IConnectionPoint,
            public Steinberg::Vst::IEditController,
            public Steinberg::Vst::IEditController2,
            public Steinberg::Vst::IMidiMapping
        {
            private:
                friend class PluginView;

            protected:
                uatomic_t                           nRefCounter;            // Reference counter
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
                uint8_t                            *pOscPacket;             // OSC packet data
                lltl::parray<vst3::CtlPort>         vPorts;                 // All possible ports
                lltl::parray<vst3::CtlParamPort>    vPlainParams;           // Input parameters (non-virtual) sorted according to the metadata order
                lltl::parray<vst3::CtlParamPort>    vParams;                // Input parameters (non-virtual) sorted by unique parameter ID
                lltl::parray<vst3::CtlMeterPort>    vMeters;                // Meters

            #ifdef WITH_UI_FEATURE
                ipc::Mutex                          sWrappersLock;          // Lock of wrappers
                lltl::parray<vst3::UIWrapper>       vWrappers;              // UI wrappers
            #endif /* WITH_UI_FEATURE */

                lltl::parray<meta::port_t>          vGenMetadata;           // Generated metadata
                lltl::state<core::ShmState>         sShmState;              // Shared memory state
                vst3::string_buf                    sRxNotifyBuf;           // Notify buffer on receive
                vst3::string_buf                    sTxNotifyBuf;           // Notify buffer on receive
                core::KVTStorage                    sKVT;                   // KVT storage
                ipc::Mutex                          sKVTMutex;              // KVT storage access mutex
                uint32_t                            nLatency;               // Plugin latency
                float                               fScalingFactor;         // Scaling factor
                bool                                bMidiMapping;           // Use MIDI mapping
                bool                                bMsgWorkaround;         // Workaround of message exchange for bogus hosts

            protected:
                vst3::CtlPort                      *create_port(const meta::port_t *port, const char *postfix);
                vst3::CtlParamPort                 *find_param(Steinberg::Vst::ParamID param_id);
                void                                receive_raw_osc_packet(const void *data, size_t size);
                void                                parse_raw_osc_event(osc::parse_frame_t *frame);
                status_t                            load_state(Steinberg::IBStream *is);
                void                                send_kvt_state();

        #ifdef WITH_UI_FEATURE
            protected:
                ui::Module                         *create_ui();
        #endif /* WITH_UI_FEATURE */

            protected:
                static ssize_t                      compare_param_ports(const vst3::CtlParamPort *a, const vst3::CtlParamPort *b);
                static ssize_t                      compare_ports_by_id(const vst3::CtlPort *a, const vst3::CtlPort *b);
                static void                         shm_state_deleter(core::ShmState *state);

            public:
                explicit Controller(PluginFactory *factory, resource::ILoader *loader, const meta::package_t *package, const meta::plugin_t *meta);
                Controller(const Controller &) = delete;
                Controller(Controller &&) = delete;
                virtual ~Controller() override;

                Controller & operator = (const Controller &) = delete;
                Controller & operator = (Controller &&) = delete;

                status_t                            init();
                void                                destroy();

        #ifdef VST_USE_RUNLOOP_IFACE
            public:
                Steinberg::Linux::IRunLoop         *acquire_run_loop();
        #endif /* VST_USE_RUNLOOP_IFACE */

            public:
                inline ipc::Mutex                  &kvt_mutex();
                inline core::KVTStorage            *kvt_storage();
                inline const meta::package_t       *package() const;
                status_t                            play_file(const char *file, wsize_t position, bool release);
                vst3::CtlPort                      *port_by_id(const char *id);
                void                                dump_state_request();
                const core::ShmState               *shm_state();

        #ifdef WITH_UI_FEATURE
            public: // UI-related stuff
                status_t                            detach_ui_wrapper(UIWrapper *wrapper);
        #endif /* WITH_UI_FEATURE */

            public: // vst3::IDataSync
                virtual void                        sync_data() override;

            public: // vst3::CtlPortChangeHandler
                virtual void                        port_write(vst3::CtlPort *port, size_t flags) override;

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

            public: // Steinberg::Vst::IMidiMapping
                virtual Steinberg::tresult  PLUGIN_API getMidiControllerAssignment(Steinberg::int32 busIndex, Steinberg::int16 channel, Steinberg::Vst::CtrlNumber midiControllerNumber, Steinberg::Vst::ParamID & id) override;
        };
        #include <steinberg/vst3/base/WarningsPop.h>

    } /* namespace vst3 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_CONTROLLER_H_ */
