/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/KVTDispatcher.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>
#include <lsp-plug.in/plug-fw/core/ShmClient.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/chunk.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/defs.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ports.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/factory.h>

#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/lltl/parray.h>

namespace lsp
{
    namespace vst2
    {
        class UIWrapper;

        class Wrapper: public plug::IWrapper
        {
            private:
                friend class UIWrapper;

            private:
                AEffect                            *pEffect;
                vst2::Factory                      *pFactory;
                audioMasterCallback                 pMaster;
                ipc::IExecutor                     *pExecutor;
                vst2::chunk_t                       sChunk;
                bool                                bUpdateSettings;
                bool                                bStateManage;   // State management barrier

            #ifdef WITH_UI_FEATURE
                UIWrapper                          *pUIWrapper;     // UI wrapper
                uatomic_t                           nUIReq;         // UI change request
                uatomic_t                           nUIResp;        // UI change response
            #endif /* WITH_UI_FEATURE */

                uint32_t                            nLatency;
                uatomic_t                           nDumpReq;
                uatomic_t                           nDumpResp;
                vst2::Port                         *pBypass;
                core::SamplePlayer                 *pSamplePlayer;  // Sample player
                core::ShmClient                    *pShmClient;     // Shared memory client

                lltl::parray<vst2::AudioPort>       vAudioPorts;    // List of audio ports
                lltl::parray<vst2::AudioBufferPort> vAudioBuffers;  // Audio buffer ports
                lltl::parray<vst2::MidiInputPort>   vMidiIn;        // Input MIDI ports
                lltl::parray<vst2::MidiOutputPort>  vMidiOut;       // Output MIDI ports
                lltl::parray<vst2::ParameterPort>   vExtParams;     // List of controllable external parameters
                lltl::parray<vst2::Port>            vParams;        // List of controllable parameters
                lltl::parray<vst2::Port>            vPorts;         // List of all created VST ports
                lltl::parray<vst2::Port>            vSortedPorts;   // List of all created VST ports ordered by unique id
                lltl::parray<vst2::Port>            vProxyPorts;    // List of all created VST proxy ports
                lltl::parray<meta::port_t>          vGenMetadata;   // Generated metadata

                core::KVTStorage                    sKVT;
                ipc::Mutex                          sKVTMutex;

            private:
                vst2::Port                 *create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix);

            protected:
                status_t                    check_vst_bank_header(const fxBank *bank, size_t size);
                status_t                    check_vst_program_header(const fxProgram *prog, size_t size);
                void                        deserialize_v1(const fxBank *bank);
                void                        deserialize_v2_v3(const uint8_t *data, size_t bytes);
                void                        deserialize_new_chunk_format(const uint8_t *data, size_t bytes);
                void                        sync_position();
                status_t                    serialize_port_data();
                bool                        check_parameters_updated();
                void                        apply_settings_update();
                void                        report_latency();

            public:
                Wrapper(
                    plug::Module *plugin,
                    vst2::Factory *factory,
                    AEffect *effect,
                    audioMasterCallback callback
                );
                Wrapper(const Wrapper &) = delete;
                Wrapper(Wrapper &&) = delete;
                virtual ~Wrapper() override;

                Wrapper & operator = (const Wrapper &) = delete;
                Wrapper & operator = (Wrapper &&) = delete;

                status_t                        init();
                void                            destroy();

            public:
                inline vst2::ParameterPort     *parameter_port(size_t index);
                vst2::Port                     *find_by_id(const char *id);

                inline void                     open();
                void                            run(float** inputs, float** outputs, size_t samples);
                void                            run_legacy(float** inputs, float** outputs, size_t samples);
                void                            process_events(const VstEvents *e);
                inline void                     set_sample_rate(float sr);
                inline void                     set_block_size(size_t size);
                inline void                     mains_changed(VstIntPtr value);
                inline bool                     has_bypass() const;
                inline void                     set_bypass(bool bypass);

                size_t                          serialize_state(const void **dst, bool program);
                void                            deserialize_state(const void *data, size_t size);
                void                            request_state_dump();

                inline core::SamplePlayer      *sample_player();

        #ifdef WITH_UI_FEATURE
            public: // UI-related stuff
                inline void                     set_ui_wrapper(UIWrapper *ui);
                inline UIWrapper               *ui_wrapper();
        #endif /* WITH_UI_FEATURE */

            public: // core::IWrapper
                virtual ipc::IExecutor         *executor() override;
                virtual core::KVTStorage       *kvt_lock() override;
                virtual core::KVTStorage       *kvt_trylock() override;
                virtual bool                    kvt_release() override;
                virtual const meta::package_t  *package() const override;
                virtual void                    request_settings_update() override;
                virtual void                    state_changed() override;
                virtual meta::plugin_format_t   plugin_format() const override;
                virtual const core::ShmState   *shm_state() override;
        };
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_WRAPPER_H_ */
