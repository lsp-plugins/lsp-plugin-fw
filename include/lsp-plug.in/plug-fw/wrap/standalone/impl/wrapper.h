/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 31 янв. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_IMPL_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>
#include <lsp-plug.in/plug-fw/wrap/standalone/ports.h>
#include <lsp-plug.in/plug-fw/wrap/standalone/wrapper.h>

namespace lsp
{
    namespace standalone
    {
        const audio::callbacks_t Wrapper::callbacks =
        {
            Wrapper::on_connected,
            Wrapper::on_activated,
            Wrapper::on_io_changed,
            Wrapper::on_process,
            Wrapper::on_deactivated,
            Wrapper::on_connection_lost,
            Wrapper::on_disconnected
        };

        Wrapper::Wrapper(
                standalone::Factory *factory,
            plug::Module *plugin,
            resource::ILoader *loader,
            const wrapper_info_t *info,
            lltl::parray<core::AudioBackendInfo> *backends):
            IWrapper(plugin, loader)
        {
            pFactory        = factory;
            pBackend        = NULL;
            nCurrentBackend = 0;
            sClientName     = (info->client_name != NULL) ? strdup(info->client_name) : NULL;
            nState          = S_CREATED;
            bUpdateSettings = true;
            nLatency        = 0;
            pExecutor       = NULL;

            atomic_store(&nPosition, 0);
            bUIActive       = false;

            atomic_store(&nQueryDrawReq, 0);
            nQueryDrawResp  = 0;
            atomic_store(&nDumpReq, 0);
            nDumpResp       = 0;

            atomic_init(nLockMeters);

            pSamplePlayer   = NULL;
            pShmClient      = NULL;

            pPackage        = NULL;

            vAudioBackends.swap(backends);
        }

        Wrapper::~Wrapper()
        {
            pBackend        = NULL;
            if (sClientName != NULL)
            {
                free(sClientName);
                sClientName     = NULL;
            }
            nState          = S_CREATED;
            nLatency        = 0;
            pExecutor       = NULL;
            nQueryDrawResp  = 0;
            nDumpResp       = 0;
            pSamplePlayer   = NULL;
            pShmClient      = NULL;
        }

        static ssize_t cmp_port_identifiers(const standalone::Port *pa, const standalone::Port *pb)
        {
            const meta::port_t *a = pa->metadata();
            const meta::port_t *b = pb->metadata();
            return strcmp(a->id, b->id);
        }

        status_t Wrapper::init()
        {
        #ifdef LSP_TRACE
            lsp_trace("Begin plugin wrapper initialization");
            const system::time_millis_t start = system::get_time_millis();
            lsp_finally {
                const system::time_millis_t end = system::get_time_millis();
                lsp_trace("Plugin wrapper initialization time: %d ms", int(end - start));
            };
        #endif /* LSP_TRACE */

            // Load package information
            status_t res;
            io::IInStream *is = resources()->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
            if (is == NULL)
            {
                lsp_error("No manifest.json found in resources");
                return STATUS_BAD_STATE;
            }

            res = meta::load_manifest(&pPackage, is);
            is->close();
            delete is;

            if (res != STATUS_OK)
            {
                lsp_error("Error while reading manifest file, error: %d", int(res));
                return res;
            }

            // Obtain plugin metadata
            if (pPlugin == NULL)
                return STATUS_BAD_STATE;

            const meta::plugin_t *meta = pPlugin->metadata();
            if (meta == NULL)
                return STATUS_BAD_STATE;

            // Create ports
            lsp_trace("Creating ports for %s - %s", meta->name, meta->description);
            lltl::parray<plug::IPort> plugin_ports;
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(&plugin_ports, port, NULL);

            // Generate list of sorted ports by identifier
            if (!vSortedPorts.add(vAllPorts))
                return STATUS_NO_MEM;
            vSortedPorts.qsort(cmp_port_identifiers);

            // Initialize plugin and UI
            pPlugin->init(this, plugin_ports.array());

            // Create sample player if required
            if (meta->extensions & meta::E_FILE_PREVIEW)
            {
                pSamplePlayer       = new core::SamplePlayer(meta);
                if (pSamplePlayer == NULL)
                    return STATUS_NO_MEM;
                pSamplePlayer->init(this, plugin_ports.array(), plugin_ports.size());
            }

            // Create shared memory client
            if ((vAudioBuffers.size() > 0) || (meta->extensions & meta::E_SHM_TRACKING))
            {
                lsp_trace("Creating shared memory client");
                pShmClient          = new core::ShmClient();
                if (pShmClient == NULL)
                    return STATUS_NO_MEM;
                pShmClient->init(this, pFactory, plugin_ports.array(), plugin_ports.size());
            }

            // Update state, mark initialized
            nState      = S_INITIALIZED;

            return STATUS_OK;
        }

        void Wrapper::set_routing(const lltl::darray<connection_t> *routing)
        {
            if (pBackend == NULL)
                return;

            if (routing->size() <= 0)
                return;

            for (size_t i=0, n=routing->size(); i<n; ++i)
            {
                const connection_t *conn = routing->uget(i);
                if (conn == NULL)
                    continue;

                size_t self_ports = 0;
                const char *src = conn->src;
                const char *dst = conn->dst;

                // Check the source port
                if (strchr(src, ':') == NULL)
                {
                    ++self_ports;
                    standalone::Port *p         = port_by_id(src);
                    const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                    if ((meta == NULL) || !(meta::is_audio_out_port(meta) || meta::is_midi_out_port(meta)))
                    {
                        fprintf(stderr, "  %s -> %s: invalid port '%s', should be AUDIO OUT or MIDI OUT\n", src, dst, src);
                        continue;
                    }
                    standalone::DataPort *dp    = static_cast<standalone::DataPort *>(p);
                    src     = dp->system_name();
                }

                // Check the destination port
                if (strchr(dst, ':') == NULL)
                {
                    ++self_ports;
                    standalone::Port *p         = port_by_id(dst);
                    const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                    if ((meta == NULL) || !(meta::is_audio_in_port(meta) || meta::is_midi_in_port(meta)))
                    {
                        fprintf(stderr, "  %s -> %s: invalid port '%s', should be AUDIO IN or MIDI IN\n", src, dst, dst);
                        continue;
                    }
                    standalone::DataPort *dp    = static_cast<standalone::DataPort *>(p);
                    dst     = dp->system_name();
                }

                // At least one self port should be defined
                if (self_ports <= 0)
                {
                    fprintf(stderr, "  %s -> %s: at least one port should belong to the plugin\n", src, dst);
                    continue;
                }

                // Perform the connection
                status_t res = pBackend->connect_ports(pBackend, src, dst);
                switch (res)
                {
                    case STATUS_OK:
                        fprintf(stderr, "  %s -> %s: OK\n", src, dst);
                        break;
                    case STATUS_ALREADY_BOUND:
                        fprintf(stderr, "  %s -> %s: connection already has been estimated\n", src, dst);
                        break;
                    default:
                        fprintf(stderr, "  %s -> %s: error, code=%d\n", src, dst, int(res));
                        break;
                }
            }
        }

        status_t Wrapper::connect()
        {
            // Check connection state
            switch (nState)
            {
                case S_CREATED:
                    lsp_error("connect() on uninitialized audio backend wrapper");
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

            // Create audio backend if needed
            status_t res;
            if (pBackend == NULL)
            {
                const core::AudioBackendInfo * const info = vAudioBackends.get(nCurrentBackend);
                if (info == NULL)
                    return STATUS_NOT_FOUND;

                if ((res = core::create_audio_backend(&pBackend, &sBackendLibrary, info)) != STATUS_OK)
                    return res;
            }

            // Obtain the connection to the backend
            audio::connection_params_t params;
            params.client_name      = (sClientName != NULL) ? sClientName : pPlugin->metadata()->uid;
            params.url              = NULL;

            // Establish connection to the backend
            if ((res = pBackend->connect(pBackend, &params, &callbacks, this)) != STATUS_OK)
            {
                lsp_warn("Could not connect to backend, code=%d", int(res));
                nState = S_DISCONNECTED;
                return res;
            }

            nState = S_CONNECTED;
            return STATUS_OK;
        }

        status_t Wrapper::on_connected(void *user_data, const audio::io_parameters_t *params)
        {
            standalone::Wrapper * const self    = static_cast<standalone::Wrapper *>(user_data);

            // Connect data ports
            for (size_t i=0, n=self->vDataPorts.size(); i<n; ++i)
            {
                standalone::DataPort * const dp = self->vDataPorts.uget(i);
                if (dp == NULL)
                    continue;

                dp->connect();
                dp->set_buffer_size(params->max_buffer_size);
            }

            for (size_t i=0, n=self->vAudioBuffers.size(); i<n; ++i)
            {
                standalone::AudioBufferPort * const ap = self->vAudioBuffers.uget(i);
                if (ap == NULL)
                    continue;
                ap->set_buffer_size(params->max_buffer_size);
            }

            // Set sample rate
            lsp_info("Sample rate set to %d Hz", int(params->sample_rate));
            self->pPlugin->set_sample_rate(params->sample_rate);
            if (self->pSamplePlayer != NULL)
                self->pSamplePlayer->set_sample_rate(params->sample_rate);
            self->sPosition.sampleRate        = params->sample_rate;
            self->bUpdateSettings             = true;

            return STATUS_OK;
        }

        status_t Wrapper::on_activated(void *user_data)
        {
            standalone::Wrapper * const self    = static_cast<standalone::Wrapper *>(user_data);

            // Now we ready for processing
            if (self->pPlugin != NULL)
                self->pPlugin->activate();

            return STATUS_OK;
        }

        status_t Wrapper::on_io_changed(void *user_data, const audio::io_parameters_t *params)
        {
            standalone::Wrapper * const self    = static_cast<standalone::Wrapper *>(user_data);

            // Update buffer size
            for (size_t i=0, n=self->vDataPorts.size(); i<n; ++i)
            {
                standalone::DataPort *p = self->vDataPorts.uget(i);
                if (p != NULL)
                    p->set_buffer_size(params->max_buffer_size);
            }

            for (size_t i=0, n=self->vAudioBuffers.size(); i<n; ++i)
            {
                standalone::AudioBufferPort *p = self->vAudioBuffers.uget(i);
                if (p != NULL)
                    p->set_buffer_size(params->max_buffer_size);
            }

            if (self->pShmClient != NULL)
                self->pShmClient->set_buffer_size(params->max_buffer_size);

            // Update sample rate
            if (self->sPosition.sampleRate != params->sample_rate)
            {
                lsp_info("Sample rate changed to %d Hz", int(params->sample_rate));

                self->pPlugin->set_sample_rate(params->sample_rate);
                if (self->pSamplePlayer != NULL)
                    self->pSamplePlayer->set_sample_rate(params->sample_rate);
                if (self->pShmClient != NULL)
                    self->pShmClient->set_sample_rate(params->sample_rate);
                self->sPosition.sampleRate = params->sample_rate;
                self->bUpdateSettings      = true;
            }

            return STATUS_OK;
        }

        status_t Wrapper::on_process(void *user_data, const audio::io_position_t *position, uint32_t frames)
        {
            // Call the plugin for processing
            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            // Call the run() method
            standalone::Wrapper * const self    = static_cast<standalone::Wrapper *>(user_data);
            return self->run(position, frames);
        }

        status_t Wrapper::on_deactivated(void *user_data)
        {
            standalone::Wrapper * const self    = static_cast<standalone::Wrapper *>(user_data);

            // Deactivate plugin
            if (self->pPlugin != NULL)
                self->pPlugin->deactivate();

            // Disconnect all data ports
            for (size_t i=0, n=self->vDataPorts.size(); i<n; ++i)
            {
                standalone::DataPort * const dp     = self->vDataPorts.uget(i);
                if (dp != NULL)
                    dp->disconnect();
            }

            return STATUS_OK;
        }

        void Wrapper::on_connection_lost(void *user_data)
        {
            // Reset the client state
            standalone::Wrapper * const self    = static_cast<standalone::Wrapper *>(user_data);
            self->nState            = S_CONN_LOST;
            lsp_warn("AUDIO BACKEND NOTIFICATION: shutdown");
        }

        void Wrapper::on_disconnected(void *user_data)
        {
        }

        status_t Wrapper::run(const audio::io_position_t *position, size_t samples)
        {
            // Activate UI if present
            bool ui_active = bUIActive;
            if (ui_active != pPlugin->ui_active())
            {
                lsp_trace("UI ACTIVE: %s", (ui_active) ? "true" : "false");
                if (ui_active)
                    pPlugin->activate_ui();
                else
                    pPlugin->deactivate_ui();
            }

            // Set-up audio position
            plug::position_t npos   = sPosition;
            npos.speed          = position->speed;
            npos.frame          = position->frame;
            npos.numerator      = position->numerator;
            npos.denominator    = position->denominator;
            npos.beatsPerMinute = position->beats_per_minute;
            npos.tick           = position->tick;
            npos.ticksPerBeat   = position->ticks_per_beat;

//            lsp_trace("numerator = %.3f, denominator = %.3f, bpm = %.3f, tick = %.3f, tpb = %.3f",
//                    float(npos.numerator), float(npos.denominator), float(npos.beatsPerMinute),
//                    float(npos.tick), float(npos.ticksPerBeat));

            // Call plugin for position update
            if (pPlugin->set_position(&npos))
                bUpdateSettings = true;

            sPosition = npos;
            atomic_add(&nPosition, 1);

            // Pre-process data ports
            for (size_t i=0, n=vDataPorts.size(); i<n; ++i)
            {
                standalone::DataPort * const dp = vDataPorts.uget(i);
                if (dp != NULL)
                    dp->before_process(samples);
            }

            // Check that input ports have been changed
            for (size_t i=0, n=vParams.size(); i<n; ++i)
            {
                // Get port
                standalone::Port * const port   = vParams.uget(i);
                if (port == NULL)
                    continue;

                // Pre-process data in port
                if (port->sync())
                {
                    lsp_trace("port changed: %s", port->metadata()->id);
                    bUpdateSettings = true;
                }
            }

            // Check that input parameters have changed
            if (bUpdateSettings)
            {
                lsp_trace("updating settings");
                if (pShmClient != NULL)
                    pShmClient->update_settings();
                pPlugin->update_settings();
                bUpdateSettings = false;
            }

            // Need to dump state?
            uatomic_t dump_req  = atomic_load(&nDumpReq);
            if (dump_req != nDumpResp)
            {
                dump_plugin_state();
                nDumpResp           = dump_req;
            }

            if (pShmClient != NULL)
            {
                pShmClient->begin(samples);
                pShmClient->pre_process(samples);
            }

            // Call the main processing unit
            pPlugin->process(samples);

            // Launch the sample player
            if (pSamplePlayer != NULL)
                pSamplePlayer->process(samples);

            if (pShmClient != NULL)
            {
                pShmClient->post_process(samples);
                pShmClient->end();
            }

            // Report latency if changed
            ssize_t latency = pPlugin->latency();
            if (latency != nLatency)
            {
                lsp_trace("Plugin latency changed from %d to %d", int(nLatency), int(latency));

                nLatency = latency;
                pBackend->set_latency(pBackend, nLatency);
            }

            // Post-process data ports
            for (size_t i=0, n=vDataPorts.size(); i<n; ++i)
            {
                standalone::DataPort * const dp = vDataPorts.uget(i);
                if (dp != NULL)
                    dp->after_process(samples);
            }

            // Commit meters
            if (lock_meters())
            {
                lsp_finally { unlock_meters(); };
                for (size_t i=0, n=vMeters.size(); i<n; ++i)
                {
                    standalone::MeterPort * const mp    = vMeters.uget(i);
                    if (mp != NULL)
                        mp->commit();
                }
            }

            return STATUS_OK;
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
            if (pBackend != NULL)
                pBackend->disconnect(pBackend);

            nState      = S_DISCONNECTED;

            return STATUS_OK;
        }

        void Wrapper::destroy()
        {
            // Disconnect
            disconnect();

            // Destroy backend
            if (pBackend != NULL)
            {
                pBackend->destroy(pBackend);
                sBackendLibrary.close();
            }

            // Destroy sample player
            if (pSamplePlayer != NULL)
            {
                pSamplePlayer->destroy();
                delete pSamplePlayer;
                pSamplePlayer   = NULL;
            }

            // Release catalog
            if (pShmClient != NULL)
            {
                lsp_trace("Destroying shared memory client");
                pShmClient->destroy();
                delete pShmClient;
                pShmClient      = NULL;
            }

            // Destroy ports
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                Port *p = vAllPorts.uget(i);
//                lsp_trace("destroy port id=%s", p->metadata()->id);
                p->destroy();
                delete p;
            }
            vParams.flush();
            vMeters.flush();
            vAllPorts.flush();
            vSortedPorts.flush();

            // Cleanup generated metadata
            for (size_t i=0, n=vGenMetadata.size(); i<n; ++i)
            {
                meta::port_t *port = vGenMetadata.uget(i);
//                lsp_trace("destroy generated port metadata %p", port);
                meta::drop_port_metadata(port);
            }
            vGenMetadata.flush();

            // Clear all other port containers
            vDataPorts.flush();

            // Forget the plugin instance
            pPlugin     = NULL;

            // Destroy executor service
            if (pExecutor != NULL)
            {
                pExecutor->shutdown();
                delete pExecutor;
                pExecutor   = NULL;
            }

            // Destroy audio backend information
            core::free_audio_backends(&vAudioBackends);

            // Destroy package
            meta::free_manifest(pPackage);
            pPackage    = NULL;
        }

        void Wrapper::create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix)
        {
            standalone::Port *jp    = NULL;

            switch (port->role)
            {
                case meta::R_MESH:
                    jp      = new standalone::MeshPort(port, this);
                    break;

                case meta::R_FBUFFER:
                    jp      = new standalone::FrameBufferPort(port, this);
                    break;

                case meta::R_STREAM:
                    jp      = new standalone::StreamPort(port, this);
                    break;

                case meta::R_MIDI_IN:
                case meta::R_MIDI_OUT:
                {
                    standalone::MidiPort * const jmp = new standalone::MidiPort(port, this);
                    vDataPorts.add(jmp);
                    jp      = jmp;
                    break;
                }
                case meta::R_AUDIO_IN:
                case meta::R_AUDIO_OUT:
                {
                    standalone::AudioPort * const jap = new standalone::AudioPort(port, this);
                    vDataPorts.add(jap);
                    jp      = jap;
                    break;
                }

                case meta::R_AUDIO_SEND:
                case meta::R_AUDIO_RETURN:
                {
                    standalone::AudioBufferPort * const jbp = new standalone::AudioBufferPort(port, this);
                    vAudioBuffers.add(jbp);
                    jp      = jbp;
                    break;
                }

                case meta::R_OSC_IN:
                case meta::R_OSC_OUT:
                    jp      = new standalone::OscPort(port, this);
                    break;

                case meta::R_PATH:
                    jp      = new standalone::PathPort(port, this);
                    vParams.add(jp);
                    break;

                case meta::R_STRING:
                case meta::R_SEND_NAME:
                case meta::R_RETURN_NAME:
                    jp      = new standalone::StringPort(port, this);
                    vParams.add(jp);
                    break;

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                    jp      = new standalone::ControlPort(port, this);
                    vParams.add(jp);
                    break;

                case meta::R_METER:
                {
                    standalone::MeterPort * const jmp = new standalone::MeterPort(port, this);
                    vMeters.add(jmp);
                    jp      = jmp;
                    break;
                }

                case meta::R_PORT_SET:
                {
                    LSPString postfix_str;
                    standalone::PortGroup * const pg  = new standalone::PortGroup(port, this);
                    pg->init();
                    vParams.add(pg);
                    vAllPorts.add(pg);
                    plugin_ports->add(pg);

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

                                create_port(plugin_ports, cm, port_post);
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
                vAllPorts.add(jp);
                plugin_ports->add(jp);
            }
        }

        const meta::package_t *Wrapper::package() const
        {
            return pPackage;
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

        plug::canvas_data_t *Wrapper::render_inline_display(size_t width, size_t height)
        {
            // Allocate canvas for drawing
            plug::ICanvas *canvas = create_canvas(width, height);
            if (canvas == NULL)
                return NULL;

            // Call plugin for rendering and return canvas data
            bool res = pPlugin->inline_display(canvas, width, height);
            canvas->sync();

            return (res) ? canvas->data() : NULL;
        }

        standalone::Port *Wrapper::port_by_idx(size_t index)
        {
            return vAllPorts.get(index);
        }

        standalone::Port *Wrapper::port_by_id(const char *id)
        {
            ssize_t first = 0, last = vSortedPorts.size() - 1;
            while (first <= last)
            {
                ssize_t middle = (first + last) >> 1;
                standalone::Port * const p  = vSortedPorts.uget(middle);
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

        status_t Wrapper::import_settings(const char *file)
        {
            config::PullParser parser;
            status_t res = parser.open(file);
            if (res == STATUS_OK)
                res = import_settings(&parser);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t Wrapper::import_settings(const LSPString *file)
        {
            config::PullParser parser;
            status_t res = parser.open(file);
            if (res == STATUS_OK)
                res = import_settings(&parser);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t Wrapper::import_settings(const io::Path *file)
        {
            config::PullParser parser;
            status_t res = parser.open(file);
            if (res == STATUS_OK)
                res = import_settings(&parser);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t Wrapper::import_settings(io::IInSequence *is)
        {
            config::PullParser parser;
            status_t res = parser.wrap(is);
            if (res == STATUS_OK)
                res = import_settings(&parser);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t Wrapper::import_settings_work(config::PullParser *parser)
        {
            status_t res;
            config::param_t param;

            // Lock KVT
            core::KVTStorage *kvt = kvt_lock();
            lsp_finally {
                if (kvt != NULL)
                {
                    kvt->gc();
                    kvt_release();
                }
            };

            // Reset all ports to default values
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                standalone::Port * const p = vAllPorts.uget(i);
                if (p == NULL)
                    continue;
                p->set_default();
            }

            // Process the configuration file
            while ((res = parser->next(&param)) == STATUS_OK)
            {
                if (param.name.starts_with('/')) // KVT
                {
                    // Do nothing if there is no KVT
                    if (kvt == NULL)
                    {
                        lsp_warn("Could not apply KVT parameter %s because there is no KVT", param.name.get_utf8());
                        continue;
                    }

                    core::kvt_param_t kp;

                    switch (param.type())
                    {
                        case config::SF_TYPE_I32:
                            kp.type         = core::KVT_INT32;
                            kp.i32          = param.v.i32;
                            break;
                        case config::SF_TYPE_U32:
                            kp.type         = core::KVT_UINT32;
                            kp.u32          = param.v.u32;
                            break;
                        case config::SF_TYPE_I64:
                            kp.type         = core::KVT_INT64;
                            kp.i64          = param.v.i64;
                            break;
                        case config::SF_TYPE_U64:
                            kp.type         = core::KVT_UINT64;
                            kp.u64          = param.v.u64;
                            break;
                        case config::SF_TYPE_F32:
                            kp.type         = core::KVT_FLOAT32;
                            kp.f32          = param.v.f32;
                            break;
                        case config::SF_TYPE_F64:
                            kp.type         = core::KVT_FLOAT64;
                            kp.f64          = param.v.f64;
                            break;
                        case config::SF_TYPE_BOOL:
                            kp.type         = core::KVT_FLOAT32;
                            kp.f32          = (param.v.bval) ? 1.0f : 0.0f;
                            break;
                        case config::SF_TYPE_STR:
                            kp.type         = core::KVT_STRING;
                            kp.str          = param.v.str;
                            break;
                        case config::SF_TYPE_BLOB:
                            kp.type         = core::KVT_BLOB;
                            kp.blob.size    = param.v.blob.length;
                            kp.blob.ctype   = param.v.blob.ctype;
                            kp.blob.data    = NULL;
                            if (param.v.blob.data != NULL)
                            {
                                // Allocate memory
                                size_t src_left = strlen(param.v.blob.data);
                                size_t dst_left = 0x10 + param.v.blob.length;
                                void *blob      = ::malloc(dst_left);
                                if (blob != NULL)
                                {
                                    kp.blob.data    = blob;

                                    // Decode
                                    size_t n = dsp::base64_dec(blob, &dst_left, param.v.blob.data, &src_left);
                                    if ((n != param.v.blob.length) || (src_left != 0))
                                    {
                                        ::free(blob);
                                        kp.type         = core::KVT_ANY;
                                        kp.blob.data    = NULL;
                                    }
                                }
                                else
                                    kp.type         = core::KVT_ANY;
                            }
                            break;
                        default:
                            kp.type         = core::KVT_ANY;
                            break;
                    }

                    if (kp.type != core::KVT_ANY)
                    {
                        const char *id = param.name.get_utf8();
                        kvt->put(id, &kp, core::KVT_RX);
                    }

                    // Free previously allocated data
                    if ((kp.type == core::KVT_BLOB) && (kp.blob.data != NULL))
                        free(const_cast<void *>(kp.blob.data));
                }
                else
                {
                    for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
                    {
                        standalone::Port * const p  = vAllPorts.uget(i);
                        if (p == NULL)
                            continue;
                        const meta::port_t *meta = p->metadata();
                        if ((meta != NULL) && (param.name.equals_ascii(meta->id)))
                        {
                            set_port_value(p, &param, plug::PF_STATE_IMPORT, NULL);
                            break;
                        }
                    }
                }
            }

            return (res == STATUS_EOF) ? STATUS_OK : res;
        }

        status_t Wrapper::import_settings(config::PullParser *parser)
        {
            // Notify plugin that state is about to load
            pPlugin->before_state_load();

            // Import settings
            status_t res = import_settings_work(parser);

            // Notify plugin that state has been just loaded
            if (res == STATUS_OK)
                pPlugin->state_loaded();

            return res;
        }

        bool Wrapper::set_port_value(standalone::Port *port, const config::param_t *param, size_t flags, const io::Path *base)
        {
            // Get metadata
            const meta::port_t *p = (port != NULL) ? port->metadata() : NULL;
            if (p == NULL)
                return false;

            // Check that it's a control port
            if (!meta::is_in_port(p))
                return false;

            // Apply changes
            switch (p->role)
            {
                case meta::R_PORT_SET:
                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    if (meta::is_discrete_unit(p->unit))
                    {
                        if (meta::is_bool_unit(p->unit))
                            port->commit_value((param->to_bool()) ? 1.0f : 0.0f);
                        else
                            port->commit_value(param->to_int());
                    }
                    else
                    {
                        float v = param->to_float();

                        // Decode decibels to values
                        if ((meta::is_decibel_unit(p->unit)) && (param->is_decibel()))
                        {
                            if ((p->unit == meta::U_GAIN_AMP) || (p->unit == meta::U_GAIN_POW))
                            {
                                if (v < -250.0f)
                                    v       = 0.0f;
                                else if (v > 250.0f)
                                    v       = (p->unit == meta::U_GAIN_AMP) ? dspu::db_to_gain(250.0f) : dspu::db_to_power(250.0f);
                                else
                                    v       = (p->unit == meta::U_GAIN_AMP) ? dspu::db_to_gain(v) : dspu::db_to_power(v);
                            }
                        }

                        port->commit_value(v);
                    }
                    break;
                }
                case meta::R_PATH:
                {
                    // Check type of argument
                    if (!param->is_string())
                        return false;

                    const char *value = param->v.str;
                    size_t len      = ::strlen(value);
                    io::Path path;

                    if (core::parse_relative_path(&path, base, value, len))
                    {
                        // Update value and it's length
                        value   = path.as_utf8();
                        len     = strlen(value);
                    }

                    path_t *bpath = port->buffer<path_t>();
                    if (bpath != NULL)
                        bpath->submit(value, flags);
                    break;
                }
                case meta::R_STRING:
                case meta::R_SEND_NAME:
                case meta::R_RETURN_NAME:
                {
                    // Check type of argument
                    if (!param->is_string())
                        return false;

                    // Submit string to the port
                    const char *value = param->v.str;
                    standalone::StringPort * const sp = static_cast<standalone::StringPort *>(port);
                    plug::string_t *str = sp->data();
                    if (str != NULL)
                        str->submit(value, false);

                    break;
                }
                default:
                    return false;
            }
            return true;
        }

        bool Wrapper::lock_meters()
        {
            for (size_t i=0; i<10; ++i)
            {
                if (atomic_trylock(nLockMeters))
                    return true;
            }

            return false;
        }

        bool Wrapper::lock_meters_soft()
        {
            for (size_t i=0; i<10; ++i)
            {
                if (atomic_trylock(nLockMeters))
                    return true;
                if (i & 1)
                    ipc::Thread::yield();
            }

            return false;
        }

        void Wrapper::unlock_meters()
        {
            atomic_unlock(nLockMeters);
        }

        bool Wrapper::test_display_draw()
        {
            uatomic_t last      = atomic_load(&nQueryDrawReq);
            bool result         = last != nQueryDrawResp;
            nQueryDrawResp      = last;
            return result;
        }

        audio::backend_t *Wrapper::backend()
        {
            return pBackend;
        }

        bool Wrapper::initialized() const
        {
            return nState != S_CREATED;
        }

        bool Wrapper::connected() const
        {
            return nState == S_CONNECTED;
        }

        bool Wrapper::disconnected() const
        {
            return (nState == S_DISCONNECTED) || (nState == S_INITIALIZED);
        }

        bool Wrapper::connection_lost() const
        {
            return nState == S_CONN_LOST;
        }

        void Wrapper::query_display_draw()
        {
            atomic_add(&nQueryDrawReq, 1);
        }

        bool Wrapper::set_ui_active(bool active)
        {
            bool prev = bUIActive;
            bUIActive = active;
            return prev;
        }

        core::SamplePlayer *Wrapper::sample_player()
        {
            return pSamplePlayer;
        }

        void Wrapper::request_settings_update()
        {
            bUpdateSettings     = true;
        }

        meta::plugin_format_t Wrapper::plugin_format() const
        {
            return meta::PLUGIN_JACK;
        }

        const core::ShmState *Wrapper::shm_state()
        {
            return (pShmClient != NULL) ? pShmClient->state() : NULL;
        }

    } /* namespace standalone */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_IMPL_WRAPPER_H_ */
