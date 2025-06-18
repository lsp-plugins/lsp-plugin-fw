/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/core/config.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>
#include <lsp-plug.in/plug-fw/core/ShmClient.h>
#include <lsp-plug.in/plug-fw/wrap/jack/factory.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/ipc/IExecutor.h>
#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/dsp-units/units.h>

#include <jack/jack.h>

namespace lsp
{
    namespace jack
    {
        class Port;
        class DataPort;
        class AudioBufferPort;
        class StringPort;
        class Factory;

        /**
         * Wrapper for the plugin module
         */
        class Wrapper: public plug::IWrapper
        {
            private:
                friend class    UIWrapper;

            protected:
                enum state_t
                {
                    S_CREATED,
                    S_INITIALIZED,
                    S_CONNECTED,
                    S_CONN_LOST,
                    S_DISCONNECTED
                };

                typedef struct audio_send_t
                {
                    const char                 *sID;
                    size_t                      nChannels;
                    bool                        bPublish;
                    core::AudioSend            *pSend;
                    jack::StringPort           *pName;
                    char                        sLastName[MAX_SHM_SEGMENT_NAME_BYTES];
                    jack::AudioBufferPort      *vChannels[];
                } audio_send_t;

                typedef struct audio_return_t
                {
                    const char                 *sID;
                    size_t                      nChannels;
                    core::AudioReturn          *pReturn;
                    jack::StringPort           *pName;
                    jack::AudioBufferPort      *vChannels[];
                } audio_return_t;

            private:
                jack::Factory                  *pFactory;           // Factory for shared resources
                jack_client_t                  *pClient;            // JACK connection client
                state_t                         nState;             // Connection state to JACK server
                bool                            bUpdateSettings;    // Plugin settings are required to be updated
                ssize_t                         nLatency;           // The actual latency of device
                ipc::IExecutor                 *pExecutor;          // Off-line task executor
                core::KVTStorage                sKVT;               // Key-value tree
                ipc::Mutex                      sKVTMutex;          // Key-value tree mutex

                uatomic_t                       nPosition;          // Position counter
                volatile bool                   bUIActive;          // UI activity flag

                uatomic_t                       nQueryDrawReq;      // QueryDraw request
                uatomic_t                       nQueryDrawResp;     // QueryDraw response
                uatomic_t                       nDumpReq;           // Dump state to file request
                uatomic_t                       nDumpResp;          // Dump state to file response

                core::SamplePlayer             *pSamplePlayer;      // Sample player
                core::ShmClient                *pShmClient;         // Shared memory client

                lltl::parray<jack::Port>        vAllPorts;          // All ports
                lltl::parray<jack::Port>        vParams;            // All input parameters
                lltl::parray<jack::Port>        vSortedPorts;       // Alphabetically-sorted ports
                lltl::parray<jack::DataPort>    vDataPorts;         // Data ports (audio, MIDI)
                lltl::parray<jack::AudioBufferPort> vAudioBuffers;  // Audio buffers
                lltl::parray<meta::port_t>      vGenMetadata;       // Generated metadata for virtual ports

                meta::package_t                *pPackage;           // Package descriptor

            protected:
                void            create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix);
                int             sync_position(jack_transport_state_t state, const jack_position_t *pos);
                int             latency_callback(jack_latency_callback_mode_t mode);
                int             run(size_t samples);

                status_t        import_settings(config::PullParser *parser);
                status_t        import_settings_work(config::PullParser *parser);

            protected:
                static int      process(jack_nframes_t nframes, void *arg);
                static int      sync_buffer_size(jack_nframes_t nframes, void *arg);
                static int      sync_sample_rate(jack_nframes_t nframes, void *arg);
                static int      jack_sync(jack_transport_state_t state, jack_position_t *pos, void *arg);
                static int      latency_callback(jack_latency_callback_mode_t mode, void *arg);
                static void     shutdown(void *arg);
                static bool     set_port_value(jack::Port *port, const config::param_t *param, size_t flags, const io::Path *base);

            public:
                explicit Wrapper(jack::Factory *factory, plug::Module *plugin, resource::ILoader *loader);
                Wrapper(const Wrapper &) = delete;
                Wrapper(Wrapper &&) = delete;
                virtual ~Wrapper() override;

                Wrapper & operator = (const Wrapper &) = delete;
                Wrapper & operator = (Wrapper &&) = delete;

                status_t                            init();
                void                                destroy();

            public: // plug::IWrapper
                virtual ipc::IExecutor             *executor() override;
                virtual void                        query_display_draw() override;
                virtual core::KVTStorage           *kvt_lock() override;
                virtual core::KVTStorage           *kvt_trylock() override;
                virtual bool                        kvt_release() override;
                virtual const meta::package_t      *package() const override;
                virtual void                        request_settings_update() override;
                virtual meta::plugin_format_t       plugin_format() const override;
                virtual const core::ShmState       *shm_state() override;

            public:
                inline jack_client_t               *client();
                inline bool                         initialized() const;
                inline bool                         connected() const;
                inline bool                         disconnected() const;
                inline bool                         connection_lost() const;

                inline core::SamplePlayer          *sample_player();

                status_t                            connect();
                void                                set_routing(const lltl::darray<connection_t> *routing);
                status_t                            disconnect();

                jack::Port                         *port_by_id(const char *id);
                jack::Port                         *port_by_idx(size_t index);

                status_t                            import_settings(const char *path);
                status_t                            import_settings(const LSPString *path);
                status_t                            import_settings(const io::Path *path);
                status_t                            import_settings(io::IInSequence *is);

                bool                                set_ui_active(bool active);

                // Inline display interface
                plug::canvas_data_t                *render_inline_display(size_t width, size_t height);

                inline bool                         test_display_draw();
        };
    } /* namespace jack */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_WRAPPER_H_ */
