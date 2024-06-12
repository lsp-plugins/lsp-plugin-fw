/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 дек. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/wrap/clap/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/clap/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/clap/ports.h>
#include <lsp-plug.in/plug-fw/wrap/clap/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace clap
    {
        class UIWrapper;

        /**
         * CLAP plugin wrapper interface
         */
        class Wrapper: public plug::IWrapper
        {
            protected:
                typedef struct audio_group_t
                {
                    meta::port_group_type_t nType;      // Type of the group (MONO, STEREO, etc)
                    size_t                  nFlags;     // Flags to return to the CLAP host
                    ssize_t                 nInPlace;   // CLAP host optimizations: in-place pair
                    const char             *sName;      // Pointer to the group name
                    size_t                  nPorts;     // Number of ports in the group
                    AudioPort              *vPorts[];   // List of ports in the audio port group
                } audio_group_t;

            protected:
                const clap_host_t              *pHost;              // Host interface
                const meta::package_t          *pPackage;           // Package metadata
                const meta::plugin_t           *pUIMetadata;        // UI metadata
                void                           *pUIFactory;         // UI factory
                clap::UIWrapper                *pUIWrapper;         // UI wrapper
                uatomic_t                       nUIReq;             // UI change request
                uatomic_t                       nUIResp;            // UI change response
                clap::HostExtensions           *pExt;               // CLAP Extensions
                ipc::IExecutor                 *pExecutor;          // Executor service
                ssize_t                         nLatency;           // The actual plugin latency
                ssize_t                         nTailSize;          // Tail size
                uatomic_t                       nDumpReq;           // State dump request counter
                uatomic_t                       nDumpResp;          // State dump response counter

                lltl::parray<audio_group_t>     vAudioIn;           // Input audio ports
                lltl::parray<audio_group_t>     vAudioOut;          // Output audio ports
                lltl::parray<ParameterPort>     vParamPorts;        // List of parameters sorted by clap_id
                lltl::parray<MidiInputPort>     vMidiIn;            // Midi input ports
                lltl::parray<MidiOutputPort>    vMidiOut;           // Midi output ports
                lltl::parray<clap::Port>        vAllPorts;          // List of all available ports
                lltl::parray<clap::StringPort>  vStringPorts;       // List of string ports
                lltl::parray<clap::Port>        vSortedPorts;       // List of ports sorted by metadata identifier
                lltl::parray<meta::port_t>      vGenMetadata;       // Generated metadata for virtual ports

                core::KVTStorage                sKVT;               // KVT storage
                ipc::Mutex                      sKVTMutex;          // KVT storage access mutex

                bool                            bLatencyChanged;    // Flag that indicates that the plugin restart was requested
                bool                            bUpdateSettings;    // Trigger settings update for the nearest run
                bool                            bStateManage;       // State management barrier
                core::SamplePlayer             *pSamplePlayer;      // Sample player

            protected:
                static audio_group_t *alloc_audio_group(size_t ports);
                static audio_group_t *create_audio_group(
                    const meta::port_group_t *meta,
                    lltl::parray<plug::IPort> * ins,
                    lltl::parray<plug::IPort> * outs);
                static audio_group_t *create_audio_group(plug::IPort *port);
                static void     destroy_audio_group(audio_group_t *grp);
                static plug::IPort *find_port(const char *id, lltl::parray<plug::IPort> *list);
                static ssize_t  compare_ports_by_clap_id(const ParameterPort *a, const ParameterPort *b);
                static ssize_t  compare_ports_by_id(const clap::Port *a, const clap::Port *b);
                static status_t read_value(const clap_istream_t *is, const char *name, core::kvt_param_t *p);
                static void     destroy_value(core::kvt_param_t *p);

            protected:
                void            lookup_ui_factory();
                void            create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix);
                status_t        create_ports(lltl::parray<plug::IPort> *plugin_ports, const meta::plugin_t *meta);
                status_t        generate_audio_port_groups(const meta::plugin_t *meta);
                clap::ParameterPort  *find_param(clap_id param_id);
                size_t          prepare_block(size_t *ev_index, size_t offset, const clap_process_t *process);
                void            generate_output_events(size_t offset, const clap_process_t *process);

            public:
                explicit Wrapper(
                    plug::Module *module,
                    const meta::package_t *package,
                    resource::ILoader *loader,
                    const clap_host_t *host);
                virtual ~Wrapper() override;

            public:
                // CLAP public API functions
                status_t        init();
                void            destroy();
                status_t        activate(double sample_rate, size_t min_frames_count, size_t max_frames_count);
                void            deactivate();
                status_t        start_processing();
                void            stop_processing();
                void            reset();
                clap_process_status     process(const clap_process_t *process);
                const void     *get_extension(const char *id);
                void            on_main_thread();

            public:
                // CLAP parameter extension
                size_t          params_count() const;
                status_t        param_info(clap_param_info_t *info, size_t index);
                status_t        get_param_value(double *value, clap_id param_id);
                status_t        format_param_value(char *buffer, size_t buf_size, clap_id param_id, double value);
                status_t        parse_param_value(double *value, clap_id param_id, const char *text);
                void            flush_param_events(const clap_input_events_t *in, const clap_output_events_t *out);

            public:
                // CLAP latency extension
                size_t          latency();

            public:
                // CLAP audio port extension
                size_t          audio_ports_count(bool is_input) const;
                status_t        audio_port_info(clap_audio_port_info_t *info, size_t index, bool is_input) const;

            public:
                // CLAP note port extension
                size_t          has_note_ports() const;
                size_t          note_ports_count(bool is_input) const;
                status_t        note_port_info(clap_note_port_info_t *info, size_t index, bool is_input) const;

            public:
                // CLAP state extension
                status_t        save_state(const clap_ostream_t *os);
                status_t        save_state_work(const clap_ostream_t *os);
                status_t        load_state(const clap_istream_t *is);
                status_t        load_state_work(const clap_istream_t *is);

            public:
                // CLAP tail extension
                uint32_t        tail_size() const;

            public: // plug::IWrapper methods
                virtual ipc::IExecutor         *executor() override;
                virtual core::KVTStorage       *kvt_lock() override;
                virtual core::KVTStorage       *kvt_trylock() override;
                virtual bool                    kvt_release() override;
                virtual const meta::package_t  *package() const override;
                virtual void                    state_changed() override;
                virtual void                    request_settings_update() override;
                virtual meta::plugin_format_t   plugin_format() const override;

            public:
                // Miscellaneous functions
                clap::Port                     *find_by_id(const char *id);
                inline core::SamplePlayer      *sample_player();
                void                            request_state_dump();
                inline UIWrapper               *ui_wrapper();
                UIWrapper                      *create_ui();
                void                            destroy_ui();
                inline HostExtensions          *extensions();
                bool                            ui_provided();
                void                            ui_visibility_changed();
        };
    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_WRAPPER_H_ */
