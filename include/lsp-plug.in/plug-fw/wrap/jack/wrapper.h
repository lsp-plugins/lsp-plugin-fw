/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/ipc/IExecutor.h>
#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/ipc/Mutex.h>

#include <jack/jack.h>

namespace lsp
{
    namespace jack
    {
        class Port;
        class DataPort;

        /**
         * Wrapper for the plugin module
         */
        class Wrapper: public plug::IWrapper
        {
            private:
                Wrapper & operator = (const Wrapper &);

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

            private:
                jack_client_t                  *pClient;            // JACK connection client
                state_t                         nState;             // Connection state to JACK server
                bool                            bUpdateSettings;    // Plugin settings are required to be updated
                ssize_t                         nLatency;           // The actual latency of device
                ipc::IExecutor                 *pExecutor;          // Off-line task executor
                plug::ICanvas                  *pCanvas;            // Inline display featured canvas
                core::KVTStorage                sKVT;               // Key-value tree
                ipc::Mutex                      sKVTMutex;          // Key-value tree mutex

                atomic_t                        nPosition;          // Position counter
                plug::position_t                sPosition;          // Actual time position

                volatile atomic_t               nQueryDrawReq;      // QueryDraw request
                atomic_t                        nQueryDrawResp;     // QueryDraw response
                volatile atomic_t               nDumpReq;           // Dump state to file request
                atomic_t                        nDumpResp;          // Dump state to file response

                lltl::parray<jack::Port>        vPorts;             // All ports
                lltl::parray<jack::Port>        vSortedPorts;       // Alphabetically-sorted ports
                lltl::parray<jack::DataPort>    vDataPorts;         // Data ports (audio, MIDI)
                lltl::parray<meta::port_t>      vGenMetadata;       // Generated metadata for virtual ports

                meta::package_t                *pPackage;           // Package descriptor

//                size_t                  nCounter;

            protected:
                void            create_port(const meta::port_t *port, const char *postfix);
                int             sync_position(jack_transport_state_t state, const jack_position_t *pos);
                int             latency_callback(jack_latency_callback_mode_t mode);
                int             run(size_t samples);

            protected:
                static int      process(jack_nframes_t nframes, void *arg);
                static int      sync_buffer_size(jack_nframes_t nframes, void *arg);
                static int      jack_sync(jack_transport_state_t state, jack_position_t *pos, void *arg);
                static int      latency_callback(jack_latency_callback_mode_t mode, void *arg);
                static void     shutdown(void *arg);

            public:
                explicit Wrapper(plug::Module *plugin, resource::ILoader *loader): IWrapper(plugin, loader)
                {
                    pClient         = NULL;
                    nState          = S_CREATED;
                    bUpdateSettings = true;
                    nLatency        = 0;
                    pExecutor       = NULL;
                    pCanvas         = NULL;

                    nPosition       = 0;
                    plug::position_t::init(&sPosition);

                    nQueryDrawReq   = 0;
                    nQueryDrawResp  = 0;
                    nDumpReq        = 0;
                    nDumpResp       = 0;

                    pPackage        = NULL;
//                    nCounter        = 0;
                }

                virtual ~Wrapper()
                {
                    pClient         = NULL;
                    nState          = S_CREATED;
                    nLatency        = 0;
                    pExecutor       = NULL;
                    pCanvas         = NULL;
                    nQueryDrawReq   = 0;
                    nQueryDrawResp  = 0;
                    nDumpReq        = 0;
                    nDumpResp       = 0;
                }

                status_t                            init();

                void                                destroy();

            public:
                virtual ipc::IExecutor             *executor();

                virtual void                        query_display_draw()    { atomic_add(&nQueryDrawReq, 1);    }

                virtual const plug::position_t     *position()  { return &sPosition;                }

                virtual plug::ICanvas              *create_canvas(plug::ICanvas *&cv, size_t width, size_t height);

                virtual core::KVTStorage           *kvt_lock();

                virtual core::KVTStorage           *kvt_trylock();

                virtual bool                        kvt_release();

                virtual const meta::package_t      *package() const;

            public:
                inline jack_client_t               *client()                { return pClient;                   }
                inline bool                         initialized() const     { return nState != S_CREATED;       }
                inline bool                         connected() const       { return nState == S_CONNECTED;     }
                inline bool                         disconnected() const    { return (nState == S_DISCONNECTED) || (nState == S_INITIALIZED); }
                inline bool                         connection_lost() const { return nState == S_CONN_LOST;     }

                status_t                            connect();
                status_t                            disconnect();

                jack::Port                         *port_by_id(const char *id);
                jack::Port                         *port_by_idx(size_t index);

//            public:
//                bool transfer_dsp_to_ui();
//
//                // Inline display interface
//                canvas_data_t *render_inline_display(size_t width, size_t height);
//                virtual void query_display_draw()
//                {
//                    nQueryDraw++;
//                }
//
//                virtual ICanvas *create_canvas(ICanvas *&cv, size_t width, size_t height);
//
//                inline bool test_display_draw()
//                {
//                    atomic_t last       = nQueryDraw;
//                    bool result         = last != nQueryDrawLast;
//                    nQueryDrawLast      = last;
//                    return result;
//                }
        };
    }
}

#include <lsp-plug.in/plug-fw/wrap/jack/ports.h>

namespace lsp
{
    namespace jack
    {
        static ssize_t cmp_port_identifiers(const jack::Port *pa, const jack::Port *pb)
        {
            const meta::port_t *a = pa->metadata();
            const meta::port_t *b = pb->metadata();
            return strcmp(a->id, b->id);
        }

        status_t Wrapper::init()
        {
            // Load package information
            io::IInStream *is = pLoader->read_stream("manifest.json");
            if (is == NULL)
            {
                lsp_error("No manifest.json found in resources");
                return STATUS_BAD_STATE;
            }

            status_t res    = meta::load_manifest(&pPackage, is);
            is->close();
            delete is;

            if (res != STATUS_OK)
            {
                lsp_error("Error while reading manifest file");
                return res;
            }

            // Obtain plugin metadata
            const meta::plugin_t *meta = pPlugin->metadata();
            if (meta == NULL)
                return STATUS_BAD_STATE;

            // Create ports
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(port, NULL);

            // Generate list of sorted ports by identifier
            if (!vSortedPorts.add(vPorts))
                return STATUS_NO_MEM;
            vSortedPorts.qsort(cmp_port_identifiers);

            // Initialize plugin and UI
            if (pPlugin != NULL)
                pPlugin->init(this);

            // Update state, mark initialized
            nState      = S_INITIALIZED;

            return STATUS_OK;
        }

        status_t Wrapper::connect()
        {
            // Ensure that client identifier is not longer than jack_client_name_size()
            size_t max_client_size  = jack_client_name_size();
            char *client_name       = static_cast<char *>(alloca(max_client_size));
            strncpy(client_name, pPlugin->metadata()->uid, max_client_size);
            client_name[max_client_size-1] = '\0';

            // Check connection state
            switch (nState)
            {
                case S_CREATED:
                    lsp_error("connect() on uninitialized JACK wrapper");
                    return STATUS_BAD_STATE;
                case S_CONNECTED:
                    return STATUS_OK;

                case S_INITIALIZED:
                case S_DISCONNECTED:
                    // OK, valid states
                    break;

                case S_CONN_LOST:
                    lsp_error("connect() from CONNECTION_LOST state, need to perform disconnect() first");
                    return STATUS_BAD_STATE;

                default:
                    lsp_error("connect() from invalid state");
                    return STATUS_BAD_STATE;
            }

            // Get JACK client
            jack_status_t jack_status;
            pClient     = jack_client_open(client_name, JackNoStartServer, &jack_status);
            if (pClient == NULL)
            {
                lsp_warn("Could not connect to JACK (status=0x%08x)", int(jack_status));
                nState = S_DISCONNECTED;
                return STATUS_DISCONNECTED;
            }

            // Set-up shutdown handler
            jack_on_shutdown(pClient, shutdown, this);

            // Determine size of buffer
            size_t buf_size             = jack_get_buffer_size(pClient);
            if (jack_set_buffer_size_callback(pClient, sync_buffer_size, this))
            {
                lsp_error("Could not setup buffer size callback");
                nState = S_CONN_LOST;
                return STATUS_DISCONNECTED;
            }

            // Connect data ports
            for (size_t i=0, n=vDataPorts.size(); i<n; ++i)
            {
                jack::DataPort *dp = vDataPorts.uget(i);
                if (dp == NULL)
                    continue;

                dp->connect();
                dp->set_buffer_size(buf_size);
            }

            // Set plugin sample rate and call for settings update
            jack_nframes_t sr           = jack_get_sample_rate(pClient);
            pPlugin->set_sample_rate(sr);
            sPosition.sampleRate        = sr;
            bUpdateSettings             = true;

            // Add processing callback
            if (jack_set_process_callback(pClient, process, this))
            {
                lsp_error("Could not initialize JACK client");
                nState = S_CONN_LOST;
                return STATUS_DISCONNECTED;
            }

            // Setup position synchronization callback
            if (jack_set_sync_callback(pClient, jack_sync, this))
            {
                lsp_error("Could not bind position sync callback");
                nState = S_CONN_LOST;
                return STATUS_DISCONNECTED;
            }

            // Set sync timeout for handler
            if (jack_set_sync_timeout(pClient, 100000)) // 100 msec timeout
            {
                lsp_error("Could not setup sync timeout");
                nState = S_CONN_LOST;
                return STATUS_DISCONNECTED;
            }

            // Now we ready for processing
            if (pPlugin != NULL)
                pPlugin->activate();

            // Activate JACK client
            if (jack_activate(pClient))
            {
                lsp_error("Could not activate JACK client");
                nState  = S_CONN_LOST;
                return STATUS_DISCONNECTED;
            }

            nState = S_CONNECTED;
            return STATUS_OK;
        }

        int Wrapper::run(size_t samples)
        {
            // Prepare ports
            for (size_t i=0, n=vPorts.size(); i<n; ++i)
            {
                // Get port
                jack::Port *port = vPorts.uget(i);
                if (port == NULL)
                    continue;

                // Pre-process data in port
                if (port->pre_process(samples))
                {
                    lsp_trace("port changed: %s", port->metadata()->id);
                    bUpdateSettings = true;
                }
            }

            // Check that input parameters have changed
            if (bUpdateSettings)
            {
                lsp_trace("updating settings");
                pPlugin->update_settings();
                bUpdateSettings = false;
            }

            // Need to dump state?
            atomic_t dump_req   = nDumpReq;
            if (dump_req != nDumpResp)
            {
                dump_plugin_state();
                nDumpResp           = dump_req;
            }

            // Call the main processing unit
            pPlugin->process(samples);

            // Report latency if changed
            ssize_t latency = pPlugin->latency();
            if (latency != nLatency)
            {
                jack_recompute_total_latencies(pClient);
                nLatency = latency;
            }

            // Post-process ALL ports
            for (size_t i=0, n=vPorts.size(); i<n; ++i)
            {
                jack::Port *port = vPorts.uget(i);
                if (port != NULL)
                    port->post_process(samples);
            }
            return 0;
        }

        status_t Wrapper::disconnect()
        {
            // Check connection state
            switch (nState)
            {
                case S_CREATED:
                case S_DISCONNECTED:
                case S_INITIALIZED:
                    // OK, valid state
                    return STATUS_OK;

                case S_CONNECTED:
                case S_CONN_LOST:
                    // OK, perform disconnect
                    break;

                default:
                    lsp_error("disconnect() from invalid state");
                    return STATUS_BAD_STATE;
            }

            // Try to deactivate application
            if (pClient != NULL)
                jack_deactivate(pClient);

            // Deactivate plugin
            if (pPlugin != NULL)
                pPlugin->deactivate();

            // Try to disconnect all data ports
            for (size_t i=0, n=vDataPorts.size(); i<n; ++i)
            {
                jack::DataPort *dp = vDataPorts.uget(i);
                if (dp != NULL)
                    dp->disconnect();
            }

            // Destroy jack client
            if (pClient != NULL)
                jack_client_close(pClient);

            nState      = S_DISCONNECTED;
            pClient     = NULL;

            return STATUS_OK;
        }

        void Wrapper::destroy()
        {
            // Disconnect
            disconnect();

            // Destroy ports
            for (size_t i=0, n=vPorts.size(); i<n; ++i)
            {
                Port *p = vPorts.uget(i);
//                lsp_trace("destroy port id=%s", p->metadata()->id);
                p->destroy();
                delete p;
            }
            vPorts.flush();
            vSortedPorts.flush();

            // Cleanup generated metadata
            for (size_t i=0, n=vGenMetadata.size(); i<n; ++i)
            {
                meta::port_t *port = vGenMetadata.uget(i);
//                lsp_trace("destroy generated port metadata %p", port);
                meta::drop_port_metadata(port);
            }

            // Clear all other port containers
            vDataPorts.flush();

            // Forget the plugin instance
            pPlugin     = NULL;

            // Drop canvas
            if (pCanvas != NULL)
            {
                pCanvas->destroy();
                delete pCanvas;
                pCanvas     = NULL;
            }

            // Destroy executor service
            if (pExecutor != NULL)
            {
                pExecutor->shutdown();
                delete pExecutor;
                pExecutor   = NULL;
            }

            // Destroy package
            meta::free_manifest(pPackage);
            pPackage    = NULL;
        }

        void Wrapper::create_port(const meta::port_t *port, const char *postfix)
        {
            jack::Port *jp  = NULL;

            switch (port->role)
            {
                case meta::R_MESH:
                    jp      = new jack::MeshPort(port, this);
                    break;

                case meta::R_FBUFFER:
                    jp      = new jack::FrameBufferPort(port, this);
                    break;

                case meta::R_STREAM:
                    jp      = new jack::StreamPort(port, this);
                    break;

                case meta::R_MIDI:
                case meta::R_AUDIO:
                {
                    jack::DataPort *jdp = new jack::DataPort(port, this);
                    vDataPorts.add(jdp);
                    jp      = jdp;
                    break;
                }

                case meta::R_OSC:
                    jp      = new jack::OscPort(port, this);
                    break;

                case meta::R_PATH:
                    jp      = new jack::PathPort(port, this);
                    break;

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                    jp      = new jack::ControlPort(port, this);
                    break;

                case meta::R_METER:
                    jp      = new jack::MeterPort(port, this);
                    break;

                case meta::R_PORT_SET:
                {
                    LSPString postfix_str;
                    jack::PortGroup     *pg      = new jack::PortGroup(port, this);
                    pg->init();
                    vPorts.add(pg);
                    pPlugin->add_port(pg);

                    for (size_t row=0; row<pg->rows(); ++row)
                    {
                        // Generate postfix
                        postfix_str.fmt_ascii("%s_%d", (postfix != NULL) ? postfix : "", int(row));
                        const char *port_post   = postfix_str.get_ascii();

                        // Clone port metadata
                        meta::port_t *cm        = clone_port_metadata(port->members, port_post);
                        if (cm != NULL)
                        {
                            vGenMetadata.add(cm);

                            for (; cm->id != NULL; ++cm)
                            {
                                if (meta::is_growing_port(cm))
                                    cm->start    = cm->min + ((cm->max - cm->min) * row) / float(pg->rows());
                                else if (meta::is_lowering_port(cm))
                                    cm->start    = cm->max - ((cm->max - cm->min) * row) / float(pg->rows());

                                create_port(cm, port_post);
                            }
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            if (jp != NULL)
            {
                jp->init();
                #ifdef LSP_DEBUG
                    const char *src_id = jp->metadata()->id;
                    for (size_t i=0, n=vPorts.size(); i<n; ++i)
                    {
                        jack::Port *p = vPorts.uget(i);
                        if (!strcmp(src_id, p->metadata()->id))
                        {
                            lsp_error("ERROR: port %s already defined", src_id);
                        }
                    }
                #endif /* LSP_DEBUG */

                vPorts.add(jp);
                pPlugin->add_port(jp);
            }
        }

        const meta::package_t *Wrapper::package() const
        {
            return pPackage;
        }

        int Wrapper::latency_callback(jack_latency_callback_mode_t mode)
        {
            if (mode == JackCaptureLatency)
            {
                ssize_t latency = pPlugin->latency();

                for (size_t i=0, n=vDataPorts.size(); i < n; ++i)
                {
                    jack::DataPort *dp = vDataPorts.uget(i);
                    if (dp == NULL)
                        continue;
                    dp->report_latency(latency);
                }
            }

            return 0;
        }

        int Wrapper::sync_position(jack_transport_state_t state, const jack_position_t *pos)
        {
            plug::position_t npos   = sPosition;

            // Update settings
            npos.speed          = (state == JackTransportRolling) ? 1.0f : 0.0f;
            npos.frame          = pos->frame;

            if (pos->valid & JackPositionBBT)
            {
                npos.numerator      = pos->beats_per_bar;
                npos.denominator    = pos->beat_type;
                npos.beatsPerMinute = pos->beats_per_minute;
                npos.tick           = pos->tick;
                npos.ticksPerBeat   = pos->ticks_per_beat;
            }

    //        lsp_trace("numerator = %.3f, denominator = %.3f, bpm = %.3f, tick = %.3f, tpb = %.3f",
    //                float(npos.numerator), float(npos.denominator), float(npos.beatsPerMinute),
    //                float(npos.tick), float(npos.ticksPerBeat));

            // Call plugin for position update
            if (pPlugin->set_position(&npos))
                bUpdateSettings = true;

            // Update current position and increment version counter
            sPosition = npos;
            atomic_add(&nPosition, 1);

            return 0;
        }

        ipc::IExecutor *Wrapper::executor()
        {
            lsp_trace("executor = %p", reinterpret_cast<void *>(pExecutor));
            if (pExecutor != NULL)
                return pExecutor;

            lsp_trace("Creating native executor service");
            ipc::NativeExecutor *exec = new ipc::NativeExecutor();
            if (exec == NULL)
                return NULL;
            if (exec->start() != STATUS_OK)
            {
                delete exec;
                return NULL;
            }
            return pExecutor = exec;
        }

        core::KVTStorage *Wrapper::kvt_lock()
        {
            return (sKVTMutex.lock()) ? &sKVT : NULL;
        }

        core::KVTStorage *Wrapper::kvt_trylock()
        {
            return (sKVTMutex.try_lock()) ? &sKVT : NULL;
        }

        bool Wrapper::kvt_release()
        {
            return sKVTMutex.unlock();
        }

        int Wrapper::process(jack_nframes_t nframes, void *arg)
        {
            dsp::context_t ctx;
            int result;

            // Call the plugin for processing
            dsp::start(&ctx);
            jack::Wrapper *_this    = static_cast<jack::Wrapper *>(arg);
            result                  = _this->run(nframes);
            dsp::finish(&ctx);

            return result;
        }

        int Wrapper::sync_buffer_size(jack_nframes_t nframes, void *arg)
        {
            jack::Wrapper *_this    = static_cast<jack::Wrapper *>(arg);

            for (size_t i=0, n=_this->vDataPorts.size(); i<n; ++i)
            {
                jack::DataPort *p = _this->vDataPorts.uget(i);
                if (p != NULL)
                    p->set_buffer_size(nframes);
            }

            return 0;
        }

        int Wrapper::jack_sync(jack_transport_state_t state, jack_position_t *pos, void *arg)
        {
            dsp::context_t ctx;
            int result;

            // Call the plugin for processing
            dsp::start(&ctx);
            jack::Wrapper *_this    = static_cast<jack::Wrapper *>(arg);
            result                  = _this->sync_position(state, pos);
            dsp::finish(&ctx);

            return result;
        }

        int Wrapper::latency_callback(jack_latency_callback_mode_t mode, void *arg)
        {
            jack::Wrapper *_this    = static_cast<jack::Wrapper *>(arg);
            return _this->latency_callback(mode);
        }

        void Wrapper::shutdown(void *arg)
        {
            // Reset the client state
            jack::Wrapper *_this    = static_cast<jack::Wrapper *>(arg);
            _this->nState           = S_CONN_LOST;
            lsp_warn("JACK NOTIFICATION: shutdown");
        }

        plug::ICanvas *Wrapper::create_canvas(plug::ICanvas *&cv, size_t width, size_t height)
        {
            return NULL; // TODO
        }

        jack::Port *Wrapper::port_by_idx(size_t index)
        {
            return vPorts.get(index);
        }

        jack::Port *Wrapper::port_by_id(const char *id)
        {
            ssize_t first = 0, last = vSortedPorts.size() - 1;
            while (first <= last)
            {
                ssize_t middle = (first + last) >> 1;
                jack::Port *p = vSortedPorts.uget(middle);
                const meta::port_t *meta = p->metadata();
                int cmp = strcmp(id, meta->id);

                if (cmp < 0)
                    last    = middle - 1;
                else if (cmp > 0)
                    first   = middle + 1;
                else
                    return p;
            }

            return NULL;
        }
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_WRAPPER_H_ */
