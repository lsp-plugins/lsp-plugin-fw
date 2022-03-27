/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_IMPL_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/jack/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/jack/ports.h>

namespace lsp
{
    namespace jack
    {
        Wrapper::Wrapper(plug::Module *plugin, resource::ILoader *loader): IWrapper(plugin, loader)
        {
            pClient         = NULL;
            nState          = S_CREATED;
            bUpdateSettings = true;
            nLatency        = 0;
            pExecutor       = NULL;

            nPosition       = 0;
            bUIActive       = false;

            nQueryDrawReq   = 0;
            nQueryDrawResp  = 0;
            nDumpReq        = 0;
            nDumpResp       = 0;

            pPackage        = NULL;
        }

        Wrapper::~Wrapper()
        {
            pClient         = NULL;
            nState          = S_CREATED;
            nLatency        = 0;
            pExecutor       = NULL;
            nQueryDrawReq   = 0;
            nQueryDrawResp  = 0;
            nDumpReq        = 0;
            nDumpResp       = 0;
        }

        static ssize_t cmp_port_identifiers(const jack::Port *pa, const jack::Port *pb)
        {
            const meta::port_t *a = pa->metadata();
            const meta::port_t *b = pb->metadata();
            return strcmp(a->id, b->id);
        }

        status_t Wrapper::init()
        {
            status_t res;

            // Load package information
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
            if (pPlugin != NULL)
                pPlugin->init(this, plugin_ports.array());

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
            if (jack_set_buffer_size_callback(pClient, sync_buffer_size, this))
            {
                lsp_error("Could not setup buffer size callback");
                nState = S_CONN_LOST;
                return STATUS_DISCONNECTED;
            }
            size_t buf_size             = jack_get_buffer_size(pClient);

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
            if (jack_set_sample_rate_callback(pClient, sync_sample_rate, this))
            {
                lsp_error("Could not setup sample rate callback");
                nState = S_CONN_LOST;
                return STATUS_DISCONNECTED;
            }
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

            // Prepare ports
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                // Get port
                jack::Port *port = vAllPorts.uget(i);
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
            uatomic_t dump_req  = nDumpReq;
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
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                jack::Port *port = vAllPorts.uget(i);
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
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                Port *p = vAllPorts.uget(i);
//                lsp_trace("destroy port id=%s", p->metadata()->id);
                p->destroy();
                delete p;
            }
            vAllPorts.flush();
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

        void Wrapper::create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix)
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
                #ifdef LSP_DEBUG
                    const char *src_id = jp->metadata()->id;
                    for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
                    {
                        jack::Port *p = vAllPorts.uget(i);
                        if (!strcmp(src_id, p->metadata()->id))
                        {
                            lsp_error("ERROR: port %s already defined", src_id);
                        }
                    }
                #endif /* LSP_DEBUG */

                vAllPorts.add(jp);
                plugin_ports->add(jp);
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
            jack::Wrapper *_this        = static_cast<jack::Wrapper *>(arg);

            for (size_t i=0, n=_this->vDataPorts.size(); i<n; ++i)
            {
                jack::DataPort *p = _this->vDataPorts.uget(i);
                if (p != NULL)
                    p->set_buffer_size(nframes);
            }

            return 0;
        }

        int Wrapper::sync_sample_rate(jack_nframes_t nframes, void *arg)
        {
            jack::Wrapper *_this        = static_cast<jack::Wrapper *>(arg);
            _this->pPlugin->set_sample_rate(nframes);
            _this->sPosition.sampleRate = nframes;
            _this->bUpdateSettings      = true;
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

        jack::Port *Wrapper::port_by_idx(size_t index)
        {
            return vAllPorts.get(index);
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

        status_t Wrapper::import_settings(config::PullParser *parser)
        {
            status_t res;
            config::param_t param;
            core::KVTStorage *kvt = kvt_lock();

            while ((res = parser->next(&param)) == STATUS_OK)
            {
                if ((param.name.starts_with('/')) && (kvt != NULL)) // KVT
                {
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
                        jack::Port *p = vAllPorts.uget(i);
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

            // Release KVT
            if (kvt != NULL)
            {
                kvt->gc();
                kvt_release();
            }

            return (res == STATUS_EOF) ? STATUS_OK : res;
        }

        bool Wrapper::set_port_value(jack::Port *port, const config::param_t *param, size_t flags, const io::Path *base)
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
                {
                    if (meta::is_discrete_unit(p->unit))
                    {
                        if (meta::is_bool_unit(p->unit))
                            port->update_value((param->to_bool()) ? 1.0f : 0.0f);
                        else
                            port->update_value(param->to_int());
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

                        port->set_value(v);
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

                    path_t *bpath = (meta::is_path_port(port->metadata())) ? port->buffer<path_t>() : NULL;
                    if (bpath != NULL)
                        bpath->submit(value, flags);
                    break;
                }
                default:
                    return false;
            }
            return true;
        }

        bool Wrapper::test_display_draw()
        {
            uatomic_t last      = nQueryDrawReq;
            bool result         = last != nQueryDrawResp;
            nQueryDrawResp      = last;
            return result;
        }

        jack_client_t *Wrapper::client()
        {
            return pClient;
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
    } /* namespace jack */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_IMPL_WRAPPER_H_ */
