/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 янв. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/ipc/IExecutor.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/core/KVTDispatcher.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>
#include <lsp-plug.in/plug-fw/plug.h>

#include <steinberg/vst3.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/sync.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/string_buf.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ports.h>

namespace lsp
{
    namespace vst3
    {
        class PluginFactory;

        #include <steinberg/vst3/base/WarningsPush.h>
        class Wrapper:
            public plug::IWrapper,
            public IDataSync,
            public Steinberg::IDependent,
            public Steinberg::Vst::IComponent,
            public Steinberg::Vst::IConnectionPoint,
            public Steinberg::Vst::IAudioProcessor,
            public Steinberg::Vst::IProcessContextRequirements
        {
            protected:
                typedef struct audio_bus_t
                {
                    Steinberg::char16              *sName;      // Bus name
                    meta::port_group_type_t         nType;      // Type of the group (MONO, STEREO, etc)
                    uint32_t                        nPorts;     // Bus ports
                    Steinberg::Vst::BusType         nBusType;   // Bus type
                    Steinberg::Vst::SpeakerArrangement nCurrArr;// Current bus arrangement
                    Steinberg::Vst::SpeakerArrangement nMinArr; // Minimum allowed bus arrangement
                    Steinberg::Vst::SpeakerArrangement nFullArr;// Maximum allowed bus arrangement
                    bool                            bActive;    // Bus is active
                    vst3::AudioPort                *vPorts[];   // List of ports related to the audio bus
                } audio_bus_t;

                typedef struct event_bus_t
                {
                    Steinberg::char16              *sName;      // Bus name
                    uint32_t                        nPorts;     // Bus ports
                    bool                            bActive;    // Bus is active
                    vst3::MidiPort                 *vPorts[];   // List of ports related to the event bus
                } event_bus_t;

                class VST3KVTListener: public core::KVTListener
                {
                    private:
                        vst3::Wrapper              *pWrapper;

                    public:
                        explicit VST3KVTListener(vst3::Wrapper *wrapper) { pWrapper = wrapper; }

                    public:
                        virtual void created(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending);
                        virtual void changed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *oval, const core::kvt_param_t *nval, size_t pending);
                        virtual void removed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending);
                };

            protected:
                volatile uatomic_t                  nRefCounter;            // Reference counter
                PluginFactory                      *pFactory;               // Reference to the factory
                const meta::package_t              *pPackage;               // Package information
                Steinberg::FUnknown                *pHostContext;           // Host context
                Steinberg::Vst::IHostApplication   *pHostApplication;       // Host application
                Steinberg::Vst::IConnectionPoint   *pPeerConnection;        // Peer connection
                ipc::IExecutor                     *pExecutor;              // Offline task executor
                lltl::parray<plug::IPort>           vAllPorts;              // All possible plugin ports
                lltl::parray<audio_bus_t>           vAudioIn;               // Input audio busses
                lltl::parray<audio_bus_t>           vAudioOut;              // Output audio busses
                lltl::parray<vst3::ParameterPort>   vParams;                // Non-virtual parameter ports
                lltl::parray<vst3::Port>            vAllParams;             // All parameter ports
                lltl::parray<vst3::MeterPort>       vMeters;                // Meter ports
                lltl::parray<plug::IPort>           vMeshes;                // Mesh ports
                lltl::parray<plug::IPort>           vFBuffers;              // Frame buffer ports
                lltl::parray<plug::IPort>           vStreams;               // Streaming ports
                lltl::pphash<char, vst3::Port>      vParamMapping;          // Virtual input port mapping
                lltl::parray<meta::port_t>          vGenMetadata;           // Generated metadata for virtual ports
                event_bus_t                        *pEventsIn;              // Input event bus
                event_bus_t                        *pEventsOut;             // Output event bus
                core::SamplePlayer                 *pSamplePlayer;          // Sample player
                wssize_t                            nPlayPosition;          // Sample playback position
                wssize_t                            nPlayLength;            // Sample playback length
                plug::position_t                    sUIPosition;            // Position notified to UI


                vst3::string_buf                    sRxNotifyBuf;           // Notify buffer for notify() processing
                vst3::string_buf                    sTxNotifyBuf;           // Notify buffer for sync_data()
                core::KVTStorage                    sKVT;                   // KVT storage
                ipc::Mutex                          sKVTMutex;              // KVT storage access mutex
                VST3KVTListener                     sKVTListener;           // KVT state listener
                core::KVTDispatcher                *pKVTDispatcher;         // KVT dispatcher
                uint8_t                            *pOscPacket;             // OSC packet data

                volatile uatomic_t                  nPositionLock;          // Position lock
                uatomic_t                           nUICounterReq;          // UI counter request
                uatomic_t                           nUICounterResp;         // Actual number of UIs
                uatomic_t                           nDirtyReq;              // Dirty state request
                uatomic_t                           nDirtyResp;             // Dirty state response
                uatomic_t                           nDumpReq;               // State dump request
                uatomic_t                           nDumpResp;              // State dump response
                uint32_t                            nMaxSamplesPerBlock;    // Maximum samples per block
                bool                                bUpdateSettings;        // Indicator that settings should be updated
                bool                                bMidiMapping;           // Midi mapping is used

                uint32_t                            nLatency;               // Current latency of the plugin

            protected:
                static audio_bus_t         *alloc_audio_bus(const char *name, size_t ports);
                static void                 free_audio_bus(audio_bus_t *bus);
                static audio_bus_t         *create_audio_bus(const meta::port_group_t *meta,
                                                             lltl::parray<plug::IPort> *ins,
                                                             lltl::parray<plug::IPort> *outs);
                static audio_bus_t         *create_audio_bus(plug::IPort *port);
                static void                 bind_bus_buffers(lltl::parray<audio_bus_t> *busses, Steinberg::Vst::AudioBusBuffers *buffers, size_t num_buffers, size_t num_samples);
                static void                 advance_bus_buffers(lltl::parray<audio_bus_t> *busses, size_t samples);
                static void                 update_port_activity(audio_bus_t *bus);

                static event_bus_t         *alloc_event_bus(const char *name, size_t ports);
                static void                 free_event_bus(event_bus_t *bus);

                static plug::IPort         *find_port(const char *id, lltl::parray<plug::IPort> *list);
                static ssize_t              compare_audio_ports_by_speaker(const vst3::AudioPort *a, const vst3::AudioPort *b);

                static ssize_t              compare_in_param_ports(const vst3::ParameterPort *a, const vst3::ParameterPort *b);

                void                        receive_raw_osc_event(osc::parse_frame_t *frame);

            protected:
                void                        create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix);
                status_t                    create_ports(lltl::parray<plug::IPort> *plugin_ports, const meta::plugin_t *meta);
                bool                        create_busses(const meta::plugin_t *meta);
                void                        sync_position(Steinberg::Vst::ProcessContext *pctx);
                size_t                      prepare_block(int32_t frame, Steinberg::Vst::ProcessData *pdata);
                vst3::ParameterPort        *input_parameter(Steinberg::Vst::ParamID id);
                status_t                    save_kvt_parameters_v1(Steinberg::IBStream *os, core::KVTStorage *kvt);
                status_t                    save_state(Steinberg::IBStream *os);
                status_t                    load_state(Steinberg::IBStream *is);
                void                        toggle_ui_state();
                void                        process_input_events(Steinberg::Vst::IEventList *events);
                void                        process_output_events(Steinberg::Vst::IEventList *events);
                void                        report_latency();
                void                        report_state_change();
                void                        report_music_position();
                void                        transmit_kvt_changes();
                void                        transmit_meter_values();
                void                        transmit_mesh_states();
                void                        transmit_frame_buffers();
                void                        transmit_streams();
                void                        transmit_play_position();

            public:
                explicit Wrapper(PluginFactory *factory, plug::Module *plugin, resource::ILoader *loader, const meta::package_t *package);
                Wrapper(const IWrapper &) = delete;
                Wrapper(IWrapper &&) = delete;
                virtual ~Wrapper() override;

                Wrapper & operator = (const Wrapper &) = delete;
                Wrapper & operator = (Wrapper &&) = delete;

            public: // plug::IWrapper
                virtual ipc::IExecutor         *executor() override;
                virtual core::KVTStorage       *kvt_lock() override;
                virtual core::KVTStorage       *kvt_trylock() override;
                virtual bool                    kvt_release() override;
                virtual void                    state_changed() override;
                virtual void                    request_settings_update() override;
                virtual const meta::package_t  *package() const override;

            public: // vst3::IDataSync
                virtual void                    sync_data() override;

            public: // Steinberg::FUnknown
                virtual Steinberg::tresult  PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;
                virtual Steinberg::uint32   PLUGIN_API addRef() override;
                virtual Steinberg::uint32   PLUGIN_API release() override;

            public: // Steinberg::IDependent
                virtual void                PLUGIN_API update(FUnknown* changedUnknown, Steinberg::int32 message) override;

            public: // Steinberg::IPluginBase
                virtual Steinberg::tresult  PLUGIN_API initialize(Steinberg::FUnknown *context) override;
                virtual Steinberg::tresult  PLUGIN_API terminate() override;

            public: // Steinberg::Vst::IComponent
                virtual Steinberg::tresult  PLUGIN_API getControllerClassId(Steinberg::TUID classId) override;
                virtual Steinberg::tresult  PLUGIN_API setIoMode(Steinberg::Vst::IoMode mode) override;
                virtual Steinberg::int32    PLUGIN_API getBusCount(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir) override;
                virtual Steinberg::tresult  PLUGIN_API getBusInfo(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::BusInfo & bus /*out*/) override;
                virtual Steinberg::tresult  PLUGIN_API getRoutingInfo(Steinberg::Vst::RoutingInfo & inInfo, Steinberg::Vst::RoutingInfo & outInfo /*out*/) override;
                virtual Steinberg::tresult  PLUGIN_API activateBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::TBool state) override;
                virtual Steinberg::tresult  PLUGIN_API setActive(Steinberg::TBool state) override;
                virtual Steinberg::tresult  PLUGIN_API setState(Steinberg::IBStream *state) override;
                virtual Steinberg::tresult  PLUGIN_API getState(Steinberg::IBStream *state) override;

            public: // Steinberg::vst::IConnectionPoint
                virtual Steinberg::tresult  PLUGIN_API connect(Steinberg::Vst::IConnectionPoint *other) override;
                virtual Steinberg::tresult  PLUGIN_API disconnect(Steinberg::Vst::IConnectionPoint *other) override;
                virtual Steinberg::tresult  PLUGIN_API notify(Steinberg::Vst::IMessage *message) override;

            public: // Steinberg::vst::IAudioProcessor
                virtual Steinberg::tresult  PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement *inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement *outputs, Steinberg::int32 numOuts) override;
                virtual Steinberg::tresult  PLUGIN_API getBusArrangement(Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::SpeakerArrangement &arr) override;
                virtual Steinberg::tresult  PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
                virtual Steinberg::uint32   PLUGIN_API getLatencySamples() override;
                virtual Steinberg::tresult  PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup & setup) override;
                virtual Steinberg::tresult  PLUGIN_API setProcessing(Steinberg::TBool state) override;
                virtual Steinberg::tresult  PLUGIN_API process(Steinberg::Vst::ProcessData & data) override;
                virtual Steinberg::uint32   PLUGIN_API getTailSamples() override;

            public: // Steinberg::Vst::IProcessContextRequirements
                virtual Steinberg::uint32   PLUGIN_API getProcessContextRequirements() override;
        };
        #include <steinberg/vst3/base/WarningsPop.h>

    } /* namespace vst3 */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_WRAPPER_H_ */
