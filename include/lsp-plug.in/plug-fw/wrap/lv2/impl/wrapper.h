/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 28 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_IMPL_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/wrapper.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/types.h>

#define LSP_LEGACY_KVT_URI          LSP_LV2_BASE_URI "ui/lv2"

namespace lsp
{
    namespace lv2
    {
        //---------------------------------------------------------------------
        void Wrapper::LV2KVTListener::created(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending)
        {
            pWrapper->state_changed();
        }

        void Wrapper::LV2KVTListener::changed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *oval, const core::kvt_param_t *nval, size_t pending)
        {
            pWrapper->state_changed();
        }

        void Wrapper::LV2KVTListener::removed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending)
        {
            pWrapper->state_changed();
        }

        //---------------------------------------------------------------------
        ssize_t Wrapper::compare_ports_by_urid(const lv2::Port *a, const lv2::Port *b)
        {
            return ssize_t(a->get_urid()) - ssize_t(b->get_urid());
        }

        Wrapper::Wrapper(plug::Module *plugin, resource::ILoader *loader, lv2::Extensions *ext):
            plug::IWrapper(plugin, loader),
            sKVTListener(this)
        {
            pPlugin         = plugin;
            pExt            = ext;
            pExecutor       = NULL;
            pAtomIn         = NULL;
            pAtomOut        = NULL;
            pLatency        = NULL;
            nPatchReqs      = 0;
            nStateReqs      = 0;
            nSyncTime       = 0;
            nSyncSamples    = 0;
            nClients        = 0;
            nDirectClients  = 0;
            sSurface.data   = NULL;
            sSurface.width  = 0;
            sSurface.height = 0;
            sSurface.stride = 0;

            bQueueDraw      = false;
            bUpdateSettings = true;
            fSampleRate     = DEFAULT_SAMPLE_RATE;
            pOscPacket      = reinterpret_cast<uint8_t *>(::malloc(OSC_PACKET_MAX));
            nStateMode      = SM_LOADING;
            nDumpReq        = 0;
            nDumpResp       = 0;
            pPackage        = NULL;
            pKVTDispatcher  = NULL;

            pSamplePlayer   = NULL;
            nPlayPosition   = 0;
            nPlayLength     = 0;
        }

        Wrapper::~Wrapper()
        {
            do_destroy();

            pPlugin         = NULL;
            pExt            = NULL;
            pExecutor       = NULL;
            pAtomIn         = NULL;
            pAtomOut        = NULL;
            pLatency        = NULL;
            nPatchReqs      = 0;
            nStateReqs      = 0;
            nSyncTime       = 0;
            nSyncSamples    = 0;
            nClients        = 0;
            nDirectClients  = 0;
            sSurface.data   = NULL;
            sSurface.width  = 0;
            sSurface.height = 0;
            sSurface.stride = 0;
            pPackage        = NULL;

            pKVTDispatcher  = NULL;
            pSamplePlayer   = NULL;
        }

        lv2::Port *Wrapper::port_by_urid(LV2_URID urid)
        {
            // Try to find the corresponding port
            ssize_t first = 0, last = vPluginPorts.size() - 1;
            while (first <= last)
            {
                size_t center   = size_t(first + last) >> 1;
                lv2::Port *p    = vPluginPorts.uget(center);
                if (urid == p->get_urid())
                    return p;
                else if (urid < p->get_urid())
                    last    = center - 1;
                else
                    first   = center + 1;
            }
            return NULL;
        }

        lv2::Port *Wrapper::port(const char *id)
        {
            for (size_t i=0, n = vPluginPorts.size(); i<n; ++i)
            {
                lv2::Port *p = vPluginPorts.uget(i);
                if (p == NULL)
                    continue;
                const meta::port_t *meta = p->metadata();
                if (meta == NULL)
                    continue;
                if (!strcmp(meta->id, id))
                    return p;
            }
            return NULL;
        }

        status_t Wrapper::init(float srate)
        {
            // Update sample rate
            fSampleRate = srate;

            // Get plugin metadata
            const meta::plugin_t *m  = pPlugin->metadata();
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
                lsp_error("Error while reading manifest file");
                return res;
            }

            // Create ports
            lsp_trace("Creaing ports");
            lltl::parray<plug::IPort> plugin_ports;
            for (const meta::port_t *meta = m->ports ; meta->id != NULL; ++meta)
                create_port(&plugin_ports, meta, NULL, false);

            // Sort port lists
            vPluginPorts.qsort(compare_ports_by_urid);
            vMeshPorts.qsort(compare_ports_by_urid);
            vStreamPorts.qsort(compare_ports_by_urid);
            vFrameBufferPorts.qsort(compare_ports_by_urid);

            // Need to create and start KVT dispatcher?
            lsp_trace("Plugin extensions=0x%x", int(m->extensions));
            if (m->extensions & meta::E_KVT_SYNC)
            {
                lsp_trace("Binding KVT listener");
                sKVT.bind(&sKVTListener);
                lsp_trace("Creating KVT dispatcher thread...");
                pKVTDispatcher         = new core::KVTDispatcher(&sKVT, &sKVTMutex);
                lsp_trace("Starting KVT dispatcher thread...");
                pKVTDispatcher->start();
            }

            // Initialize plugin
            lsp_trace("Initializing plugin");
            pPlugin->init(this, plugin_ports.array());
            pPlugin->set_sample_rate(srate);
            bUpdateSettings     = true;

            // Create sample player if required
            if (m->extensions & meta::E_FILE_PREVIEW)
            {
                pSamplePlayer       = new core::SamplePlayer(m);
                if (pSamplePlayer == NULL)
                    return STATUS_NO_MEM;
                pSamplePlayer->init(this, plugin_ports.array(), plugin_ports.size());
                pSamplePlayer->set_sample_rate(srate);
            }

            // Update refresh rate
            nSyncSamples        = srate / pExt->ui_refresh_rate();
            nClients            = 0;

            return STATUS_OK;
        }

        void Wrapper::destroy()
        {
            do_destroy();
        }

        void Wrapper::do_destroy()
        {
            // Destroy sample player
            if (pSamplePlayer != NULL)
            {
                pSamplePlayer->destroy();
                delete pSamplePlayer;
                pSamplePlayer = NULL;
            }

            // Stop KVT dispatcher
            if (pKVTDispatcher != NULL)
            {
                lsp_trace("Stopping KVT dispatcher thread...");
                pKVTDispatcher->cancel();
                pKVTDispatcher->join();
                delete pKVTDispatcher;

                sKVT.unbind(&sKVTListener);
                pKVTDispatcher          = NULL;
            }

            // Drop surface
            sSurface.data           = NULL;
            sSurface.width          = 0;
            sSurface.height         = 0;
            sSurface.stride         = 0;

            // Shutdown and delete executor if exists
            if (pExecutor != NULL)
            {
                pExecutor->shutdown();
                delete pExecutor;
                pExecutor   = NULL;
            }

            // Drop plugin
            if (pPlugin != NULL)
            {
                pPlugin->destroy();
                delete pPlugin;

                pPlugin     = NULL;
            }

            // Cleanup ports
            for (size_t i=0; i < vAllPorts.size(); ++i)
            {
                lsp_trace("destroy port id=%s", vAllPorts[i]->metadata()->id);
                delete vAllPorts[i];
            }

            // Cleanup generated metadata
            for (size_t i=0; i<vGenMetadata.size(); ++i)
            {
                lsp_trace("destroy generated port metadata %p", vGenMetadata[i]);
                drop_port_metadata(vGenMetadata[i]);
            }

            // Destroy manifest
            if (pPackage != NULL)
            {
                meta::free_manifest(pPackage);
                pPackage        = NULL;
            }

            vAllPorts.flush();
            vExtPorts.flush();
            vMeshPorts.flush();
            vStreamPorts.flush();
            vMidiPorts.flush();
            vOscPorts.flush();
            vFrameBufferPorts.flush();
            vPluginPorts.flush();
            vGenMetadata.flush();

            // Delete temporary buffer for OSC serialization
            if (pOscPacket != NULL)
            {
                ::free(pOscPacket);
                pOscPacket = NULL;
            }

            // Drop extensions
            if (pExt != NULL)
            {
                delete pExt;
                pExt        = NULL;
            }

            // Drop loader
            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader     = NULL;
            }
        }

        lv2::Port *Wrapper::create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *p, const char *postfix, bool virt)
        {
            lv2::Port *result = NULL;

            switch (p->role)
            {
                case meta::R_MESH:
                    if (pExt->atom_supported())
                    {
                        result = new lv2::MeshPort(p, pExt);
                        vMeshPorts.add(result);
                    }
                    else
                        result = new lv2::Port(p, pExt, false);

                    vPluginPorts.add(result);
                    plugin_ports->add(result);
                    break;
                case meta::R_STREAM:
                    if (pExt->atom_supported())
                    {
                        result = new lv2::StreamPort(p, pExt);
                        vStreamPorts.add(result);
                    }
                    else
                        result = new lv2::Port(p, pExt, false);
                    vPluginPorts.add(result);
                    plugin_ports->add(result);
                    break;
                case meta::R_FBUFFER:
                    if (pExt->atom_supported())
                    {
                        result = new lv2::FrameBufferPort(p, pExt);
                        vFrameBufferPorts.add(result);
                    }
                    else
                        result = new lv2::Port(p, pExt, false);
                    vPluginPorts.add(result);
                    plugin_ports->add(result);
                    break;
                case meta::R_PATH:
                    if (pExt->atom_supported())
                        result      = new lv2::PathPort(p, pExt);
                    else
                        result      = new lv2::Port(p, pExt, false);
                    vPluginPorts.add(result);
                    plugin_ports->add(result);
                    break;

                case meta::R_MIDI_IN:
                case meta::R_MIDI_OUT:
                    if (pExt->atom_supported())
                    {
                        result = new lv2::MidiPort(p, pExt);
                        vMidiPorts.add(result);
                    }
                    else
                        result = new lv2::Port(p, pExt, false);
                    plugin_ports->add(result);
                    break;
                case meta::R_OSC_IN:
                case meta::R_OSC_OUT:
                    if (pExt->atom_supported())
                    {
                        result = new lv2::OscPort(p, pExt);
                        vOscPorts.add(result);
                    }
                    else
                        result = new lv2::Port(p, pExt, false);
                    plugin_ports->add(result);
                    break;


                case meta::R_AUDIO_IN:
                case meta::R_AUDIO_OUT:
                    result = new lv2::AudioPort(p, pExt);

                    vPluginPorts.add(result);
                    vAudioPorts.add(static_cast<lv2::AudioPort *>(result));
                    plugin_ports->add(result);

                    if (postfix == NULL)
                    {
                        result->set_id(vExtPorts.size());
                        vExtPorts.add(result);
                        lsp_trace("Added external port id=%s, external_id=%d", result->metadata()->id, int(vExtPorts.size() - 1));
                    }
                    break;
                case meta::R_CONTROL:
                case meta::R_METER:
                    if (meta::is_out_port(p))
                        result      = new lv2::OutputPort(p, pExt);
                    else
                        result      = new lv2::InputPort(p, pExt, virt);

                    vPluginPorts.add(result);
                    plugin_ports->add(result);

                    if (postfix == NULL)
                    {
                        result->set_id(vExtPorts.size());
                        vExtPorts.add(result);
                        lsp_trace("Added external port id=%s, external_id=%d", result->metadata()->id, int(vExtPorts.size() - 1));
                    }
                    break;

                case meta::R_BYPASS:
                    if (meta::is_out_port(p))
                        result      = new lv2::Port(p, pExt, false);
                    else
                        result      = new lv2::BypassPort(p, pExt);

                    vPluginPorts.add(result);
                    plugin_ports->add(result);

                    if (postfix == NULL)
                    {
                        result->set_id(vExtPorts.size());
                        vExtPorts.add(result);
                        lsp_trace("Added bypass port id=%s, external_id=%d", result->metadata()->id, int(vExtPorts.size() - 1));
                    }
                    break;

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];
                    lv2::PortGroup   *pg    = new lv2::PortGroup(p, pExt, virt);

                    // Add Port Set immediately
                    vPluginPorts.add(pg);
                    vAllPorts.add(pg);
                    plugin_ports->add(pg);

                    // Generate nested ports
                    for (size_t row=0; row<pg->rows(); ++row)
                    {
                        // Generate postfix
                        snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                        // Clone port metadata
                        meta::port_t *cm        = meta::clone_port_metadata(p->members, postfix_buf);
                        if (cm == NULL)
                            continue;

                        vGenMetadata.add(cm);

                        size_t col          = 0;
                        for (; cm->id != NULL; ++cm, ++col)
                        {
                            if (meta::is_growing_port(cm))
                                cm->start    = cm->min + ((cm->max - cm->min) * row) / float(pg->rows());
                            else if (meta::is_lowering_port(cm))
                                cm->start    = cm->max - ((cm->max - cm->min) * row) / float(pg->rows());

                            // Recursively generate new ports associated with the port set
                            create_port(plugin_ports, cm, postfix_buf, true);
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            // Register created port for garbage collection
            if (result != NULL)
                vAllPorts.add(result);

            return result;
        }

        void Wrapper::connect(size_t id, void *data)
        {
            size_t ports_count  = vExtPorts.size();
            if (id < ports_count)
            {
                lv2::Port *p        = vExtPorts.get(id);
                if (p != NULL)
                    p->bind(data);
            }
            else
            {
                switch (id - ports_count)
                {
                    case 0: pAtomIn     = data; break;
                    case 1: pAtomOut    = data; break;
                    case 2: pLatency    = reinterpret_cast<float *>(data); break;
                    default:
                        lsp_warn("Unknown port number: %d", int(id));
                        break;
                }
            }
        }

        void Wrapper::run(size_t samples)
        {
            // Activate/deactivate the UI
            ssize_t clients = nClients + nDirectClients;
            if (clients > 0)
            {
                if (!pPlugin->ui_active())
                    pPlugin->activate_ui();
            }
            else if (pPlugin->ui_active())
                pPlugin->deactivate_ui();

            // First pre-process transport ports
            clear_midi_ports();
            receive_atoms(samples);

            // Pre-rocess regular ports
            size_t smode            = nStateMode;
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                // Get port
                lv2::Port *port = vAllPorts.uget(i);
                if (port == NULL)
                    continue;

                // Pre-process data in port
                if (port->pre_process(samples))
                {
                    lsp_trace("port changed: %s, value=%f", port->metadata()->id, port->value());
                    bUpdateSettings = true;
                    if ((smode != SM_LOADING) && (port->is_virtual()))
                        change_state_atomic(SM_SYNC, SM_CHANGED);
                }
            }

            // Commit state
            if (smode == SM_LOADING)
                change_state_atomic(SM_LOADING, SM_SYNC);

            // Check that input parameters have changed
            if (bUpdateSettings)
            {
                pPlugin->update_settings();
                bUpdateSettings     = false;
            }

            // Need to dump state?
            uatomic_t dump_req  = nDumpReq;
            if (dump_req != nDumpResp)
            {
                dump_plugin_state();
                nDumpResp           = dump_req;
            }

            // Call the main processing unit (split data buffers into chunks not greater than MaxBlockLength)
            size_t n_audio_ports = vAudioPorts.size();
            for (size_t off=0; off < samples; )
            {
                size_t to_process = lsp_min(samples - off, pExt->nMaxBlockLength);

                // Sanitize input data
                for (size_t i=0; i<n_audio_ports; ++i)
                {
                    lv2::AudioPort *port = vAudioPorts.uget(i);
                    if (port != NULL)
                        port->sanitize_before(off, to_process);
                }
                // Process samples
                pPlugin->process(to_process);
                if (pSamplePlayer != NULL)
                    pSamplePlayer->process(to_process);
                // Sanitize output data
                for (size_t i=0; i<n_audio_ports; ++i)
                {
                    lv2::AudioPort *port = vAudioPorts.uget(i);
                    if (port != NULL)
                        port->sanitize_after(off, to_process);
                }

                off += to_process;
            }

            // Transmit atoms (if possible)
            transmit_atoms(samples);
            clear_midi_ports();

            // Post-process regular ports for changes
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                // Get port
                lv2::Port *port = vAllPorts.uget(i);
                if (port != NULL)
                    port->post_process(samples);
            }

            // Transmit latency (if possible)
            if (pLatency != NULL)
                *pLatency   = pPlugin->latency();
        }

        void Wrapper::clear_midi_ports()
        {
            // Clear all MIDI ports
            for (size_t i=0, n=vMidiPorts.size(); i<n; ++i)
            {
                lv2::Port *p        = vMidiPorts.uget(i);
                if (!meta::is_midi_port(p->metadata()))
                    continue;
                plug::midi_t *midi  = p->buffer<plug::midi_t>();
                if (midi != NULL)
                    midi->clear();
            }
        }

        void Wrapper::receive_midi_event(const LV2_Atom_Event *ev)
        {
            // Are there any MIDI input ports in plugin?
            if (vMidiPorts.size() <= 0)
                return;

            // Decode MIDI event
            midi::event_t me;
            const uint8_t *bytes        = reinterpret_cast<const uint8_t*>(ev + 1);

            // Debug
            #ifdef LSP_TRACE
                #define TRACE_KEY(x)    case midi::MIDI_MSG_ ## x: evt_type = #x; break;
                lsp_trace("midi dump: %02x %02x %02x", int(bytes[0]), int(bytes[1]), int(bytes[2]));

                char tmp_evt_type[32];
                const char *evt_type = NULL;
                switch (me.type)
                {
                    TRACE_KEY(NOTE_OFF)
                    TRACE_KEY(NOTE_ON)
                    TRACE_KEY(NOTE_PRESSURE)
                    TRACE_KEY(NOTE_CONTROLLER)
                    TRACE_KEY(PROGRAM_CHANGE)
                    TRACE_KEY(CHANNEL_PRESSURE)
                    TRACE_KEY(PITCH_BEND)
                    TRACE_KEY(SYSTEM_EXCLUSIVE)
                    TRACE_KEY(MTC_QUARTER)
                    TRACE_KEY(SONG_POS)
                    TRACE_KEY(SONG_SELECT)
                    TRACE_KEY(TUNE_REQUEST)
                    TRACE_KEY(END_EXCLUSIVE)
                    TRACE_KEY(CLOCK)
                    TRACE_KEY(START)
                    TRACE_KEY(CONTINUE)
                    TRACE_KEY(STOP)
                    TRACE_KEY(ACTIVE_SENSING)
                    TRACE_KEY(RESET)
                    default:
                        snprintf(tmp_evt_type, sizeof(tmp_evt_type), "UNKNOWN(0x%02x)", int(me.type));
                        evt_type = tmp_evt_type;
                        break;
                }

                lsp_trace("MIDI Event: type=%s, timestamp=%ld", evt_type, (long)(me.timestamp));

                #undef TRACE_KEY

            #endif /* LSP_TRACE */

            if (midi::decode(&me, bytes) <= 0)
            {
                lsp_warn("Could not decode MIDI message");
                return;
            }

            // Put the event to the queue
            me.timestamp      = uint32_t(ev->time.frames);

            // For each MIDI port: add event to the queue
            for (size_t i=0, n=vMidiPorts.size(); i<n; ++i)
            {
                lv2::Port *midi = vMidiPorts.uget(i);
                if (!meta::is_midi_in_port(midi->metadata()))
                    continue;
                plug::midi_t *buf   = midi->buffer<plug::midi_t>();
                if (buf == NULL)
                    continue;
                if (!buf->push(me))
                    lsp_warn("MIDI event queue overflow");
            }
        }

        void Wrapper::receive_raw_osc_event(osc::parse_frame_t *frame)
        {
            osc::parse_token_t token;
            status_t res = osc::parse_token(frame, &token);
            if (res != STATUS_OK)
                return;

            if (token == osc::PT_BUNDLE)
            {
                osc::parse_frame_t child;
                uint64_t time_tag;
                status_t res = osc::parse_begin_bundle(&child, frame, &time_tag);
                if (res != STATUS_OK)
                    return;
                receive_raw_osc_event(&child); // Perform recursive call
                osc::parse_end(&child);
            }
            else if (token == osc::PT_MESSAGE)
            {
                const void *msg_start;
                size_t msg_size;
                const char *msg_addr;

                // Perform address lookup and routing
                status_t res = osc::parse_raw_message(frame, &msg_start, &msg_size, &msg_addr);
                if (res != STATUS_OK)
                    return;

                lsp_trace("Received OSC message of %d bytes, address=%s", int(msg_size), msg_addr);
                osc::dump_packet(msg_start, msg_size);

                if (::strstr(msg_addr, "/KVT/") == msg_addr)
                    pKVTDispatcher->submit(msg_start, msg_size);
                else
                {
                    for (size_t i=0, n=vOscPorts.size(); i<n; ++i)
                    {
                        lv2::Port *p = vOscPorts.uget(i);
                        if (!meta::is_osc_in_port(p->metadata()))
                            continue;

                        // Submit message to the buffer
                        core::osc_buffer_t *buf = p->buffer<core::osc_buffer_t>();
                        if (buf != NULL)
                            buf->submit(msg_start, msg_size);
                    }
                }
            }
        }

        void Wrapper::receive_atom_object(const LV2_Atom_Event *ev)
        {
                // Analyze object type
            const LV2_Atom_Object *obj = reinterpret_cast<const LV2_Atom_Object*>(&ev->body);
    //        lsp_trace("obj->body.otype (%d) = %s", int(obj->body.otype), pExt->unmap_urid(obj->body.otype));
    //        lsp_trace("obj->body.id (%d) = %s", int(obj->body.id), pExt->unmap_urid(obj->body.id));

            if (obj->body.otype == pExt->uridPatchGet) // PatchGet request
            {
                lsp_trace("triggered patch request");
                #ifdef LSP_TRACE
                for (
                    LV2_Atom_Property_Body *body = lv2_atom_object_begin(&obj->body) ;
                    !lv2_atom_object_is_end(&obj->body, obj->atom.size, body) ;
                    body = lv2_atom_object_next(body)
                )
                {
                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));
                }
                #endif /* LSP_TRACE */

                // Increment the number of patch requests
                nPatchReqs  ++;
            }
            else if (obj->body.otype == pExt->uridPatchSet) // PatchSet request
            {
                // Parse atom body
                const LV2_Atom_URID    *key     = NULL;
                const LV2_Atom         *value   = NULL;

                for (
                    LV2_Atom_Property_Body *body = lv2_atom_object_begin(&obj->body) ;
                    !lv2_atom_object_is_end(&obj->body, obj->atom.size, body) ;
                    body = lv2_atom_object_next(body)
                )
                {
                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    if ((body->key  == pExt->uridPatchProperty) && (body->value.type == pExt->uridAtomUrid))
                    {
                        key     = reinterpret_cast<const LV2_Atom_URID *>(&body->value);
                        lsp_trace("body->value.body (%d) = %s", int(key->body), pExt->unmap_urid(key->body));
                    }
                    else if (body->key   == pExt->uridPatchValue)
                        value   = &body->value;

                    if ((key != NULL) && (value != NULL))
                    {
                        lv2::Port *p    = port_by_urid(key->body);
                        if ((p != NULL) && (p->get_type_urid() == value->type))
                        {
                            lsp_trace("forwarding patch message to port %s", p->metadata()->id);
                            if (p->deserialize(value, 0)) // No flags for simple PATCH message
                            {
                                // Change state if it is a virtual port
                                if (p->is_virtual())
                                    state_changed();
                            }
                        }

                        key     = NULL;
                        value   = NULL;
                    }
                }
            }
            else if (obj->body.otype == pExt->uridTimePosition) // Time position notification
            {
                plug::position_t pos    = sPosition;

                pos.sampleRate          = fSampleRate;
                pos.ticksPerBeat        = DEFAULT_TICKS_PER_BEAT;

                for (
                    LV2_Atom_Property_Body *body = lv2_atom_object_begin(&obj->body) ;
                    !lv2_atom_object_is_end(&obj->body, obj->atom.size, body) ;
                    body = lv2_atom_object_next(body)
                )
                {
    //                lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
    //                lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    if ((body->key == pExt->uridTimeFrame) && (body->value.type == pExt->forge.Long))
                        pos.frame           = (reinterpret_cast<LV2_Atom_Long *>(&body->value))->body;
                    else if ((body->key == pExt->uridTimeSpeed) && (body->value.type == pExt->forge.Float))
                        pos.speed           = (reinterpret_cast<LV2_Atom_Float *>(&body->value))->body;
                    else if ((body->key == pExt->uridTimeBeatsPerMinute) && (body->value.type == pExt->forge.Float))
                        pos.beatsPerMinute  = (reinterpret_cast<LV2_Atom_Float *>(&body->value))->body;
                    else if ((body->key == pExt->uridTimeBeatUnit) && (body->value.type == pExt->forge.Int))
                        pos.numerator       = (reinterpret_cast<LV2_Atom_Int *>(&body->value))->body;
                    else if ((body->key == pExt->uridTimeBeatsPerBar) && (body->value.type == pExt->forge.Float))
                        pos.denominator     = (reinterpret_cast<LV2_Atom_Float *>(&body->value))->body;
                    else if ((body->key == pExt->uridTimeBarBeat) && (body->value.type == pExt->forge.Float))
                        pos.tick            = (reinterpret_cast<LV2_Atom_Float *>(&body->value))->body * pos.ticksPerBeat;
                }
//                lsp_trace("triggered timePosition event\n"
//                      "  frame      = %lld\n"
//                      "  speed      = %f\n"
//                      "  bpm        = %f\n"
//                      "  numerator  = %f\n"
//                      "  denominator= %f\n"
//                      "  tick       = %f\n"
//                      "  tpb        = %f\n",
//                      (long long)(pos.frame),
//                      pos.speed,
//                      pos.beatsPerMinute,
//                      pos.denominator,
//                      pos.numerator,
//                      pos.tick,
//                      pos.ticksPerBeat);

                // Call plugin callback and update position
                bUpdateSettings = pPlugin->set_position(&pos);
                sPosition = pos;
            }
            else if (obj->body.otype == pExt->uridUINotification)
            {
                if (obj->body.id == pExt->uridConnectUI)
                {
                    nClients    ++;
                    nStateReqs  ++;
                    lsp_trace("UI has connected, current number of clients=%d", int(nClients));
                    if (pKVTDispatcher != NULL)
                        pKVTDispatcher->connect_client();

                    // Notify all ports that UI has connected to backend
                    for (size_t i=0, n = vAllPorts.size(); i<n; ++i)
                    {
                        lv2::Port *p = vAllPorts.get(i);
                        if (p != NULL)
                            p->ui_connected();
                    }
                }
                else if (obj->body.id == pExt->uridDisconnectUI)
                {
                    nClients    --;
                    if (pKVTDispatcher != NULL)
                        pKVTDispatcher->disconnect_client();
                    lsp_trace("UI has disconnected, current number of clients=%d", int(nClients));
                }
                else if (obj->body.id == pExt->uridDumpState)
                {
                    lsp_trace("Received DUMP_STATE event");
                    atomic_add(&nDumpReq, 1);
                }
            }
            else if (obj->body.otype == pExt->uridPlayRequestType)
            {
                if (pSamplePlayer != NULL)
                {
                    char *file          = pSamplePlayer->requested_file_name(); // Destination buffer to store the file name
                    wsize_t position    = 0;
                    bool release        = false;
                    file[0]             = '\0';

                    for (
                        LV2_Atom_Property_Body *body = lv2_atom_object_begin(&obj->body) ;
                        !lv2_atom_object_is_end(&obj->body, obj->atom.size, body) ;
                        body = lv2_atom_object_next(body)
                    )
                    {
        //                lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
        //                lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                        if ((body->key == pExt->uridPlayRequestFileName) && (body->value.type == pExt->forge.Path))
                        {
                            // Read the path string
                            const LV2_Atom *atom = &body->value;
                            lv2_set_string(file, PATH_MAX, reinterpret_cast<const char *>(atom + 1), atom->size);
                        }
                        else if ((body->key == pExt->uridPlayRequestPosition) && (body->value.type == pExt->forge.Long))
                            position            = (reinterpret_cast<LV2_Atom_Long *>(&body->value))->body;
                        else if ((body->key == pExt->uridPlayRequestRelease) && (body->value.type == pExt->forge.Bool))
                            release             = (reinterpret_cast<LV2_Atom_Long *>(&body->value))->body != 0;
                    }

                    // Submit the playback request and make the play position out-of-sync
                    pSamplePlayer->play_sample(position, release);
                }
            }
            else
            {
                lsp_trace(
                    "Unknown object: \n"
                    "  ev->body.type (%d) = %s\n"
                    "  obj->body.otype (%d) = %s\n"
                    "  obj->body.id (%d) = %s",
                    int(ev->body.type), pExt->unmap_urid(ev->body.type),
                    int(obj->body.otype), pExt->unmap_urid(obj->body.otype),
                    int(obj->body.id), pExt->unmap_urid(obj->body.id));
            }
        }

        void Wrapper::receive_atoms(size_t samples)
        {
            // Update synchronization
            if (nSyncTime <= 0)
            {
                size_t n_ports  = vMeshPorts.size();
                for (size_t i=0; i<n_ports; ++i)
                {
                    plug::mesh_t *mesh  = vMeshPorts[i]->buffer<plug::mesh_t>();
    //                lsp_trace("Mesh id=%s is waiting=%s", vMeshPorts[i]->metadata()->id, (mesh->isWaiting()) ? "true" : "false");
                    if ((mesh != NULL) && (mesh->isWaiting()))
                        mesh->cleanup();
                }
            }

            // Get sequence
            if (pAtomIn == NULL)
                return;

            const LV2_Atom_Sequence *seq = reinterpret_cast<const LV2_Atom_Sequence *>(pAtomIn);

    //        lsp_trace("pSequence->atom.type (%d) = %s", int(pSequence->atom.type), pExt->unmap_urid(pSequence->atom.type));
    //        lsp_trace("pSequence->atom.size = %d", int(pSequence->atom.size));

            for (
                const LV2_Atom_Event* ev = lv2_atom_sequence_begin(&seq->body);
                !lv2_atom_sequence_is_end(&seq->body, seq->atom.size, ev);
                ev = lv2_atom_sequence_next(ev)
            )
            {
    //            lsp_trace("ev->body.type (%d) = %s", int(ev->body.type), pExt->unmap_urid(ev->body.type));
                if (ev->body.type == pExt->uridMidiEventType)
                    receive_midi_event(ev);
                else if (ev->body.type == pExt->uridOscRawPacket)
                {
                    osc::parser_t parser;
                    osc::parser_frame_t root;
                    status_t res = osc::parse_begin(&root, &parser, &ev[1], ev->body.size);
                    if (res == STATUS_OK)
                    {
                        receive_raw_osc_event(&root);
                        osc::parse_end(&root);
                        osc::parse_destroy(&parser);
                    }
                }
                else if ((ev->body.type == pExt->uridObject) || (ev->body.type == pExt->uridBlank))
                    receive_atom_object(ev);
            }
        }

        void Wrapper::transmit_time_position_to_clients()
        {
            LV2_Atom_Forge_Frame    frame;

            pExt->forge_frame_time(0); // Event header
            pExt->forge_object(&frame, 0, pExt->uridTimePosition);

            pExt->forge_key(pExt->uridTimeFrame);
            pExt->forge_long(int64_t(sPosition.frame));

            pExt->forge_key(pExt->uridTimeFrameRate);
            pExt->forge_float(fSampleRate);

            pExt->forge_key(pExt->uridTimeSpeed);
            pExt->forge_float(sPosition.speed);

            pExt->forge_key(pExt->uridTimeBarBeat);
            pExt->forge_float(sPosition.tick / sPosition.ticksPerBeat);

            pExt->forge_key(pExt->uridTimeBar);
            pExt->forge_long(0);

            pExt->forge_key(pExt->uridTimeBeatUnit);
            pExt->forge_int(int(sPosition.denominator));

            pExt->forge_key(pExt->uridTimeBeatUnit);
            pExt->forge_float(sPosition.numerator);

            pExt->forge_key(pExt->uridTimeBeatsPerMinute);
            pExt->forge_float(sPosition.beatsPerMinute);

            pExt->forge_pop(&frame);
        }

        void Wrapper::transmit_play_position_to_clients()
        {
            if (pSamplePlayer == NULL)
                return;

            wssize_t position   = pSamplePlayer->position();
            wssize_t length     = pSamplePlayer->sample_length();

            if ((nPlayPosition == position) && (nPlayLength == length))
                return;

            lsp_trace("position = %lld, length=%lld", (long long)position, (long long)length);

            LV2_Atom_Forge_Frame    frame;

            pExt->forge_frame_time(0); // Event header
            pExt->forge_object(&frame, pExt->uridPlayPositionUpdate, pExt->uridPlayPositionType);

            pExt->forge_key(pExt->uridPlayPositionPosition);
            pExt->forge_long(position);

            pExt->forge_key(pExt->uridPlayPositionLength);
            pExt->forge_long(length);

            pExt->forge_pop(&frame);

            // Commit new position
            nPlayPosition       = position;
            nPlayLength         = length;
        }

        void Wrapper::transmit_port_data_to_clients(bool sync_req, bool patch_req, bool state_req)
        {
            // Serialize time/position of plugin
            LV2_Atom_Forge_Frame    frame;

            // Serialize pending for transmission ports
            for (size_t i=0, n = vPluginPorts.size(); i<n; ++i)
            {
                // Get port
                lv2::Port *p = vPluginPorts[i];
                if (p == NULL)
                    continue;

                // Skip MESH, FBUFFER, PATH ports visible in global space
                switch (p->metadata()->role)
                {
                    case meta::R_AUDIO_IN:
                    case meta::R_AUDIO_OUT:
                    case meta::R_MIDI_IN:
                    case meta::R_MIDI_OUT:
                    case meta::R_OSC_IN:
                    case meta::R_OSC_OUT:
                    case meta::R_MESH:
                    case meta::R_STREAM:
                    case meta::R_FBUFFER:
                        continue;
                    case meta::R_PATH:
                        if (p->tx_pending()) // Tranmission request pending?
                            break;
                        if (state_req) // State request pending?
                            break;
                        if ((p->get_id() >= 0) && (patch_req)) // Global port and patch request pending?
                            break;
                        continue;
                    default:
                        if (p->tx_pending()) // Transmission request pending?
                            break;
                        if (state_req) // State request pending?
                            break;
                        continue;
                }

                // Check that we need to transmit the value
                if ((!state_req) && (!p->tx_pending()))
                    continue;

                // Create patch message containing valule of the port
                lsp_trace("Serialize port id=%s, value=%f", p->metadata()->id, p->value());

                pExt->forge_frame_time(0);
                pExt->forge_object(&frame, pExt->uridPatchMessage, pExt->uridPatchSet);
                pExt->forge_key(pExt->uridPatchProperty);
                pExt->forge_urid(p->get_urid());
                pExt->forge_key(pExt->uridPatchValue);
                p->serialize();
                pExt->forge_pop(&frame);
            }

            // Serialize meshes (it's own primitive MESH)
            for (size_t i=0, n=vMeshPorts.size(); i<n; ++i)
            {
                lv2::Port *p = vMeshPorts[i];
                if (p == NULL)
                    continue;
                if ((!sync_req) && (!p->tx_pending()))
                    continue;
                plug::mesh_t *mesh  = p->buffer<plug::mesh_t>();
                if ((mesh == NULL) || (!mesh->containsData()))
                    continue;

//                lsp_trace("transmit mesh id=%s", p->metadata()->id);
                pExt->forge_frame_time(0);  // Event header
                pExt->forge_object(&frame, p->get_urid(), pExt->uridMeshType);
                p->serialize();
                pExt->forge_pop(&frame);

                // Cleanup data of the mesh for refill
                mesh->markEmpty();
            }

            // Serialize streams (it's own primitive STREAM)
            for (size_t i=0, n=vStreamPorts.size(); i<n; ++i)
            {
                lv2::Port *p = vStreamPorts[i];
                if ((p == NULL) || (!p->tx_pending()))
                    continue;
                plug::stream_t *s = p->buffer<plug::stream_t>();
                if (s == NULL)
                    continue;

                pExt->forge_frame_time(0);  // Event header
                pExt->forge_object(&frame, p->get_urid(), pExt->uridStreamType);
                p->serialize();
                pExt->forge_pop(&frame);
            }

            // Serialize frame buffers (it's own primitive FRAMEBUFFER)
            for (size_t i=0, n=vFrameBufferPorts.size(); i<n; ++i)
            {
                lv2::Port *p = vFrameBufferPorts[i];
                if ((p == NULL) || (!p->tx_pending()))
                    continue;
                plug::frame_buffer_t *fb= p->buffer<plug::frame_buffer_t>();
                if (fb == NULL)
                    continue;

                pExt->forge_frame_time(0);  // Event header
                pExt->forge_object(&frame, p->get_urid(), pExt->uridFrameBufferType);
                p->serialize();
                pExt->forge_pop(&frame);
            }
        }

        void Wrapper::transmit_midi_events(lv2::Port *p)
        {
            plug::midi_t   *midi    = p->buffer<plug::midi_t>();
            if ((midi == NULL) || (midi->nEvents <= 0))  // There are no events ?
                return;

            midi->sort();   // Sort buffer chronologically

            // Serialize MIDI events
            LV2_Atom_Midi buf;
            buf.atom.type       = pExt->uridMidiEventType;

            for (size_t i=0; i<midi->nEvents; ++i)
            {
                const midi::event_t *me = &midi->vEvents[i];

                // Debug
                #ifdef LSP_TRACE
                    #define TRACE_KEY(x)    case midi::MIDI_MSG_ ## x: evt_type = #x; break;

                    char tmp_evt_type[32];
                    const char *evt_type = NULL;
                    switch (me->type)
                    {
                        TRACE_KEY(NOTE_OFF)
                        TRACE_KEY(NOTE_ON)
                        TRACE_KEY(NOTE_PRESSURE)
                        TRACE_KEY(NOTE_CONTROLLER)
                        TRACE_KEY(PROGRAM_CHANGE)
                        TRACE_KEY(CHANNEL_PRESSURE)
                        TRACE_KEY(PITCH_BEND)
                        TRACE_KEY(SYSTEM_EXCLUSIVE)
                        TRACE_KEY(MTC_QUARTER)
                        TRACE_KEY(SONG_POS)
                        TRACE_KEY(SONG_SELECT)
                        TRACE_KEY(TUNE_REQUEST)
                        TRACE_KEY(END_EXCLUSIVE)
                        TRACE_KEY(CLOCK)
                        TRACE_KEY(START)
                        TRACE_KEY(CONTINUE)
                        TRACE_KEY(STOP)
                        TRACE_KEY(ACTIVE_SENSING)
                        TRACE_KEY(RESET)
                        default:
                            snprintf(tmp_evt_type, sizeof(tmp_evt_type), "UNKNOWN(0x%02x)", int(me->type));
                            evt_type = tmp_evt_type;
                            break;
                    }

                    lsp_trace("MIDI Event: type=%s, timestamp=%ld", evt_type, (long)(me->timestamp));

                    #undef TRACE_KEY

                #endif /* LSP_TRACE */

                ssize_t size = midi::encode(buf.body, me);
                if (size <= 0)
                {
                    lsp_error("Tried to serialize invalid MIDI event, error=%d", int(-size));
                    continue;
                }
                buf.atom.size = size;

                lsp_trace("midi dump: %02x %02x %02x (%d: %d)",
                    int(buf.body[0]), int(buf.body[1]), int(buf.body[2]), int(buf.atom.size), int(buf.atom.size + sizeof(LV2_Atom)));

                // Serialize object
                pExt->forge_frame_time(0);
                pExt->forge_raw(&buf.atom, sizeof(LV2_Atom) + buf.atom.size);
                pExt->forge_pad(sizeof(LV2_Atom) + buf.atom.size);
            }
        }

        void Wrapper::transmit_kvt_events()
        {
            LV2_Atom atom;

            size_t size;
            while (true)
            {
                status_t res = pKVTDispatcher->fetch(pOscPacket, &size, OSC_PACKET_MAX);

                switch (res)
                {
                    case STATUS_OK:
                    {
                        lsp_trace("Transmitting OSC packet of %d bytes", int(size));
                        osc::dump_packet(pOscPacket, size);

                        atom.size       = size;
                        atom.type       = pExt->uridOscRawPacket;

                        pExt->forge_frame_time(0);
                        pExt->forge_raw(&atom, sizeof(LV2_Atom));
                        pExt->forge_raw(pOscPacket, size);
                        pExt->forge_pad(sizeof(LV2_Atom) + size);
                        break;
                    }

                    case STATUS_OVERFLOW:
                        lsp_warn("Received too big OSC packet, skipping");
                        pKVTDispatcher->skip();
                        break;

                    case STATUS_NO_DATA:
                        return;

                    default:
                        lsp_warn("Received error while deserializing KVT changes: %d", int(res));
                        return;
                }
            }
        }

        void Wrapper::transmit_atoms(size_t samples)
        {
            // Get sequence
            if (pAtomOut == NULL)
                return;

            // Update synchronization time
            nSyncTime      -= samples;
            bool sync_req       = nSyncTime <= 0;
            if (sync_req)
            {
                nSyncTime      += nSyncSamples;

                // Check that queue_draw() request for inline display is pending
                if ((bQueueDraw) && (pExt->iDisplay != NULL))
                {
                    pExt->iDisplay->queue_draw(pExt->iDisplay->handle);
                    bQueueDraw      = false;
                }
            }

            // Check that patch request is pending
            bool patch_req  = nPatchReqs > 0;
            if (patch_req)
                nPatchReqs      --;

            // Check that state request is pending
            bool state_req  = nStateReqs > 0;
            if (state_req)
                nStateReqs      --;

            // Initialize forge
            LV2_Atom_Sequence *sequence = reinterpret_cast<LV2_Atom_Sequence *>(pAtomOut);
            pExt->forge_set_buffer(sequence, sequence->atom.size);

            // Forge sequence header
            LV2_Atom_Forge_Frame    seq;
            pExt->forge_sequence_head(&seq, 0);

            // Transmit state change atom if state has been changed
            if (change_state_atomic(SM_CHANGED, SM_REPORTED))
            {
                LV2_Atom_Forge_Frame frame;
                pExt->forge_frame_time(0); // Event header
                pExt->forge_object(&frame, pExt->uridBlank, pExt->uridStateChanged);
                pExt->forge_pop(&frame);
                lsp_trace("#STATE MODE = %d", nStateMode);
            }

            // For each MIDI port, serialize it's data
            for (size_t i=0, n_midi=vMidiPorts.size(); i<n_midi; ++i)
            {
                lv2::Port *p    = vMidiPorts.uget(i);
                if (!meta::is_midi_out_port(p->metadata()))
                    continue;
                transmit_midi_events(p);
            }

            // For each OSC port, serialize it's data
            for (size_t i=0, n_osc=vOscPorts.size(); i<n_osc; ++i)
            {
                lv2::Port *p    = vOscPorts.uget(i);
                if (!meta::is_osc_out_port(p->metadata()))
                    continue;
                transmit_osc_events(p);
            }

            // Transmit different data to clients
            if (nClients > 0)
            {
                if (pKVTDispatcher != NULL)
                    transmit_kvt_events();

                transmit_time_position_to_clients();
                transmit_port_data_to_clients(sync_req, patch_req, state_req);
            }

            transmit_play_position_to_clients();

            // Complete sequence
            pExt->forge_pop(&seq);
        }

        void Wrapper::transmit_osc_events(lv2::Port *p)
        {
            core::osc_buffer_t *osc = p->buffer<core::osc_buffer_t>();
            if (osc == NULL)  // There are no events ?
                return;

            size_t size;
            LV2_Atom atom;

            while (true)
            {
                // Try to fetch record from buffer
                status_t res = osc->fetch(pOscPacket, &size, OSC_PACKET_MAX);

                switch (res)
                {
                    case STATUS_OK:
                    {
                        lsp_trace("Transmitting OSC packet of %d bytes", int(size));
                        osc::dump_packet(pOscPacket, size);

                        atom.size       = size;
                        atom.type       = pExt->uridOscRawPacket;

                        pExt->forge_frame_time(0);
                        pExt->forge_raw(&atom, sizeof(LV2_Atom));
                        pExt->forge_raw(pOscPacket, size);
                        pExt->forge_pad(sizeof(LV2_Atom) + size);
                        break;
                    }

                    case STATUS_NO_DATA: // No more data to transmit
                        return;

                    case STATUS_OVERFLOW:
                    {
                        lsp_warn("Too large OSC packet in the buffer, skipping");
                        osc->skip();
                        break;
                    }

                    default:
                    {
                        lsp_warn("OSC packet parsing error %d, skipping", int(res));
                        osc->skip();
                        break;
                    }
                }
            }
        }


        void Wrapper::save_kvt_parameters()
        {
            const core::kvt_param_t *p;

            status_t res    = STATUS_OK;

            // We should use our own forge to prevent from race condition
            LV2_Atom_Forge forge;
            LV2_Atom_Forge_Frame frame;
            lv2::lv2_sink   sink(0x100);

            // Initialize sink
            lv2_atom_forge_init(&forge, pExt->map);
            lv2_atom_forge_set_sink(&forge, lv2_sink::sink, lv2_sink::deref, &sink);
            lv2_atom_forge_tuple(&forge, &frame);

            // Read the whole KVT storage
            core::KVTIterator *it = sKVT.enum_all();
            while (it->next() == STATUS_OK)
            {
                res             = it->get(&p);
                if (res == STATUS_NOT_FOUND) // Not a parameter
                    continue;
                else if (res != STATUS_OK)
                {
                    lsp_warn("it->get() returned %d", int(res));
                    break;
                }
                else if (it->is_transient()) // Skip transient parameters
                    continue;

                int flags       = 0;
                if (it->is_private())
                    flags          |= LSP_LV2_PRIVATE;

                const char *name = it->name();
                if (name == NULL)
                {
                    lsp_trace("it->name() returned NULL");
                    break;
                }

                kvt_dump_parameter("Saving state of KVT parameter: %s = ", p, name);

                LV2_Atom_Forge_Frame prop;
                lv2_atom_forge_object(&forge, &prop, 0, pExt->uridKvtEntryType);

                // Key
                lv2_atom_forge_key(&forge, pExt->uridKvtEntryKey);
                lv2_atom_forge_string(&forge, name, strlen(name));

                // Value
                switch (p->type)
                {
                    case core::KVT_INT32:
                    case core::KVT_UINT32:
                    {
                        LV2_Atom_Int v;
                        v.atom.type     = (p->type == core::KVT_INT32) ? pExt->forge.Int : pExt->uridTypeUInt;
                        v.atom.size     = sizeof(v) - sizeof(LV2_Atom);
                        v.body          = p->i32;

                        lv2_atom_forge_key(&forge, pExt->uridKvtEntryValue);
                        lv2_atom_forge_primitive(&forge, &v.atom);
                        break;
                    }

                    case core::KVT_INT64:
                    case core::KVT_UINT64:
                    {
                        LV2_Atom_Long v;
                        v.atom.type     = (p->type == core::KVT_INT64) ? pExt->forge.Long : pExt->uridTypeULong;
                        v.atom.size     = sizeof(v) - sizeof(LV2_Atom);
                        v.body          = p->i64;
                        lv2_atom_forge_key(&forge, pExt->uridKvtEntryValue);
                        lv2_atom_forge_primitive(&forge, &v.atom);
                        break;
                    }
                    case core::KVT_FLOAT32:
                    {
                        LV2_Atom_Float v;
                        v.atom.type     = pExt->forge.Float;
                        v.atom.size     = sizeof(v) - sizeof(LV2_Atom);
                        v.body          = p->f32;
                        lv2_atom_forge_key(&forge, pExt->uridKvtEntryValue);
                        lv2_atom_forge_primitive(&forge, &v.atom);
                        break;
                    }
                    case core::KVT_FLOAT64:
                    {
                        LV2_Atom_Double v;
                        v.atom.type     = pExt->forge.Double;
                        v.atom.size     = sizeof(v) - sizeof(LV2_Atom);
                        v.body          = p->f64;
                        lv2_atom_forge_key(&forge, pExt->uridKvtEntryValue);
                        lv2_atom_forge_primitive(&forge, &v.atom);
                        break;
                    }
                    case core::KVT_STRING:
                    {
                        lv2_atom_forge_key(&forge, pExt->uridKvtEntryValue);
                        const char *str = (p->str != NULL) ? p->str : "";
                        lv2_atom_forge_typed_string(&forge, forge.String, str, ::strlen(str));
                        break;
                    }
                    case core::KVT_BLOB:
                    {
                        LV2_Atom_Forge_Frame obj;

                        lv2_atom_forge_key(&forge, pExt->uridKvtEntryValue);
                        lv2_atom_forge_object(&forge, &obj, 0, pExt->uridBlobType);
                        {
                            if (p->blob.ctype != NULL)
                            {
                                lv2_atom_forge_key(&forge, pExt->uridContentType);
                                lv2_atom_forge_typed_string(&forge, forge.String, p->blob.ctype, ::strlen(p->blob.ctype));
                            }

                            uint32_t size = ((p->blob.size > 0) && (p->blob.data != NULL)) ? p->blob.size : 0;
                            lv2_atom_forge_key(&forge, pExt->uridContent);
                            lv2_atom_forge_atom(&forge, size, forge.Chunk);
                            if (size > 0)
                                lv2_atom_forge_write(&forge, p->blob.data, size);
                        }
                        lv2_atom_forge_pop(&forge, &obj);
                        break;
                    }

                    default:
                        lsp_warn("Invalid KVT property type: %d", int(p->type));
                        res     = STATUS_BAD_TYPE;
                        break;
                }

                // Flags
                lv2_atom_forge_key(&forge, pExt->uridKvtEntryFlags);
                lv2_atom_forge_int(&forge, flags);

                // End of object
                lv2_atom_forge_pop(&forge, &prop);

                // Successful status?
                if (res != STATUS_OK)
                    break;
            } // while

            if ((res != STATUS_OK) ||(sink.res != STATUS_OK))
            {
                lsp_trace("Failed execution, result=%d, sink state=%d", int(res), int(sink.res));
                return;
            }

            lsp_trace("Generated memory chunk of %d bytes, capacity is %d bytes", int(sink.size), int(sink.cap));

            // TEST
            /*{
                kvt_param_t xp;
                xp.blob.ctype   = "text/plain";
                xp.blob.data    = "Test text";
                xp.blob.size    = strlen("Test text") + 1;
                p = &xp;

                LV2_Atom_Forge_Frame obj;
                LV2_URID key        =  pExt->map_kvt("/TEST_BLOB");

                lv2_atom_forge_key(&forge, key);
                lv2_atom_forge_object(&forge, &obj, 0, pExt->uridBlobType);
                {
                    if (p->blob.ctype != NULL)
                    {
                        lv2_atom_forge_key(&forge, pExt->uridContentType);
                        lv2_atom_forge_typed_string(&forge, forge.String, p->blob.ctype, ::strlen(p->blob.ctype));
                    }

                    uint32_t size = ((p->blob.size > 0) && (p->blob.data != NULL)) ? p->blob.size : 0;
                    lv2_atom_forge_key(&forge, pExt->uridContent);
                    lv2_atom_forge_atom(&forge, size, forge.Chunk);
                    if (size > 0)
                        lv2_atom_forge_write(&forge, p->blob.data, size);
                }
                lv2_atom_forge_pop(&forge, &obj);
            }*/

            lv2_atom_forge_pop(&forge, &frame);
            LV2_Atom *msg = reinterpret_cast<LV2_Atom *>(sink.buf);
            pExt->store_value(pExt->uridKvtObject, msg->type, &msg[1], msg->size);
        }

        void Wrapper::save_state(
                LV2_State_Store_Function   store,
                LV2_State_Handle           handle,
                uint32_t                   flags,
                const LV2_Feature *const * features)
        {
            pExt->init_state_context(store, NULL, handle, flags, features);

            // Mark state as synchronized
            nStateMode      = SM_SYNC;
            lsp_trace("#STATE MODE = %d", nStateMode);

            // Save state of all ports
            size_t ports_count = vAllPorts.size();

            for (size_t i=0; i<ports_count; ++i)
            {
                // Get port
                lv2::Port *lvp      = vAllPorts[i];
                if (lvp == NULL)
                    continue;

                // Save state of port
                lvp->save();
            }

            // Save state of all KVT parameters
            if (sKVTMutex.lock())
            {
                save_kvt_parameters();
                sKVT.gc();
                sKVTMutex.unlock();
            }

            pExt->reset_state_context();
            pPlugin->state_saved();
        }

        void Wrapper::restore_kvt_parameters()
        {
            uint32_t p_type = 0;
            size_t p_size = 0;
            const void *ptr = pExt->retrieve_value(pExt->uridKvtObject, &p_type, &p_size);
            if (ptr == NULL)
                return;

            lsp_trace("p_type = %d (%s), p_size = %d", int(p_type), pExt->unmap_urid(p_type), int(p_size));

            if ((p_type == pExt->forge.Object) || (p_type == pExt->uridBlank))
            {
                const LV2_Atom_Object_Body *obody = static_cast<const LV2_Atom_Object_Body *>(ptr);
                if (obody->otype == pExt->uridKvtType)
                    parse_kvt_v1(obody, p_size);
                else
                    lsp_warn("Unsupported KVT object type: %s", pExt->unmap_urid(obody->otype));
            }
            else if (p_type == pExt->forge.Tuple)
            {
                const LV2_Atom *body = static_cast<const LV2_Atom *>(ptr);
                parse_kvt_v2(body, p_size);
            }
            else
                lsp_warn("Unsupported KVT property type: %s", pExt->unmap_urid(p_type));
        }

        void Wrapper::parse_kvt_v2(const LV2_Atom *data, size_t size)
        {
            for (const LV2_Atom *item = data;
                !lv2_atom_tuple_is_end(data, size, item);
                item = lv2_atom_tuple_next(item))
            {
                if ((item->type != pExt->forge.Object) && (item->type != pExt->uridBlank))
                {
                    lsp_warn("Unsupported KVT item type: %d (%s)", int(item->type), pExt->unmap_urid(item->type));
                    continue;
                }
                const LV2_Atom_Object *obj = reinterpret_cast<const LV2_Atom_Object *>(item);
                if (obj->body.otype != pExt->uridKvtEntryType)
                {
                    lsp_warn("Unsupported KVT item instance type: %s", pExt->unmap_urid(obj->body.otype));
                    continue;
                }

                core::kvt_param_t p;
                char const *key = NULL;
                size_t flags    = core::KVT_TX;
                p.type          = core::KVT_ANY;
                size_t mask     = 0;

                for (
                    LV2_Atom_Property_Body *xbody = lv2_atom_object_begin(&obj->body) ;
                    !lv2_atom_object_is_end(&obj->body, obj->atom.size, xbody) ;
                    xbody = lv2_atom_object_next(xbody)
                )
                {

                    lsp_trace("xbody->key (%d) = %s", int(xbody->key), pExt->unmap_urid(xbody->key));
                    lsp_trace("xbody->value.type (%d) = %s", int(xbody->value.type), pExt->unmap_urid(xbody->value.type));

                    // Analyze type of value
                    if (xbody->key == pExt->uridKvtEntryKey)
                    {
                        // Key
                        if (parse_kvt_key(&key, &xbody->value))
                            mask |= KP_KEY;
                    }
                    else if (xbody->key == pExt->uridKvtEntryValue)
                    {
                        // Value
                        if (parse_kvt_value(&p, &xbody->value))
                            mask |= KP_VALUE;
                    }
                    else if (xbody->key == pExt->uridKvtEntryFlags)
                    {
                        // Flags
                        if (parse_kvt_flags(&flags, &xbody->value))
                            mask |= KP_FLAGS;
                    }
                    else
                        lsp_warn("Unknown KVT Entry property: %d (%s)", xbody->key, pExt->unmap_urid(xbody->key));
                } // for (item body)

                // Store property
                if ((key == NULL) || (!(mask & KP_KEY)))
                    lsp_warn("Failed to deserialize property missing key");
                else if ((p.type == core::KVT_ANY) || (!(mask & KP_VALUE)))
                    lsp_warn("Failed to deserialize property %s: missing or invalid value", key);
                else
                {
                    kvt_dump_parameter("Fetched parameter %s = ", &p, key);
                    status_t res = sKVT.put(key, &p, flags);
                    if (res != STATUS_OK)
                        lsp_warn("Could not store parameter to KVT, error: %d", int(res));
                }
            } // for (tuple body)
        }

        void Wrapper::parse_kvt_v1(const LV2_Atom_Object_Body *data, size_t size)
        {
            const size_t prefix_len = ::strlen(LSP_LEGACY_KVT_URI);
            const size_t prefix_len2 = ::strlen(pExt->uriKvt);

            for (
                LV2_Atom_Property_Body *item = lv2_atom_object_begin(data) ;
                !lv2_atom_object_is_end(data, size, item) ;
                item = lv2_atom_object_next(item)
            )
            {
//                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
//                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                const LV2_Atom_Object *pobject = reinterpret_cast<const LV2_Atom_Object *>(&item->value);
                if ((pobject->atom.type != pExt->uridObject) && (pobject->atom.type != pExt->uridBlank))
                {
                    lsp_warn("Unsupported value type (%d) = %s", int(item->value.type), pExt->unmap_urid(item->value.type));
                    continue;
                }
//                    lsp_trace("pobject->body.otype (%d) = %s", int(pobject->body.otype), pExt->unmap_urid(pobject->body.otype));
                if (pobject->body.otype != pExt->uridKvtPropertyType)
                {
                    lsp_warn("Unsupported object type (%d) = %s", int(pobject->body.otype), pExt->unmap_urid(pobject->body.otype));
                    continue;
                }

                // Remove prefix for the KVT parameter
                const char *uri = pExt->unmap_urid(item->key);
                if (uri == NULL)
                {
                    lsp_warn("Failed to unmap atom %d to URID value, skipping", int(item->key));
                    continue;
                }

                // Obtain the name of the property
                const char *name = uri;
                if (::strncmp(uri, LSP_LEGACY_KVT_URI, prefix_len) == 0)
                    name = (uri[prefix_len] == '/') ? &uri[prefix_len + 1] : NULL;
                else if (::strncmp(uri, pExt->uriKvt, prefix_len2) == 0)
                    name = (uri[prefix_len2] == '/') ? &uri[prefix_len2 + 1] : NULL;

                // Check that we have obtained the right name
                if (name == NULL)
                {
                    lsp_warn("Invalid property: urid=%d, uri=%s", item->key, uri);
                    continue;
                }

                core::kvt_param_t p;
                size_t flags    = core::KVT_TX;
                p.type          = core::KVT_ANY;
                size_t mask     = 0;

                for (
                    LV2_Atom_Property_Body *xbody = lv2_atom_object_begin(&pobject->body) ;
                    !lv2_atom_object_is_end(&pobject->body, item->value.size, xbody) ;
                    xbody = lv2_atom_object_next(xbody)
                )
                {
//                        lsp_trace("xbody->key (%d) = %s", int(xbody->key), pExt->unmap_urid(xbody->key));
//                        lsp_trace("xbody->value.type (%d) = %s", int(xbody->value.type), pExt->unmap_urid(xbody->value.type));

                    // Analyze type of value
                    if (xbody->key == pExt->uridKvtPropertyValue)
                    {
                        // Value
                        if (parse_kvt_value(&p, &xbody->value))
                            mask |= KP_VALUE;
                        else
                            lsp_warn("KVT property %s has unsupported type or is invalid: 0x%x (%s)",
                                name, xbody->value.type, pExt->unmap_urid(xbody->value.type));
                    }
                    else if (xbody->key == pExt->uridKvtPropertyFlags)
                    {
                        // Flags
                        if (parse_kvt_flags(&flags, &xbody->value))
                            mask |= KP_FLAGS;
                    }
                    else
                        lsp_warn("Unknown KVT Entry property: %d (%s)", xbody->key, pExt->unmap_urid(xbody->key));
                } // for (item body)

                // Store property
                if ((p.type != core::KVT_ANY) && (mask & KP_VALUE))
                {
                    kvt_dump_parameter("Fetched parameter %s = ", &p, name);
                    status_t res = sKVT.put(name, &p, flags);
                    if (res != STATUS_OK)
                        lsp_warn("Could not store parameter to KVT, error: %d", int(res));
                }
                else
                    lsp_warn("Failed to deserialize property %s: missing value", name);
            } // for (object body)
        }

        bool Wrapper::parse_kvt_key(char const **key, const LV2_Atom *value)
        {
            if (value->type != pExt->forge.String)
            {
                lsp_warn("Invalid type for key: %s", pExt->unmap_urid(value->type));
                return false;
            }

            *key = reinterpret_cast<const char *>(&value[1]);
            return true;
        }

        bool Wrapper::parse_kvt_flags(size_t *flags, const LV2_Atom *value)
        {
            if (value->type != pExt->forge.Int)
            {
                lsp_warn("Invalid type for flags");
                return false;
            }

            size_t result   = core::KVT_TX;
            size_t pflags   = (reinterpret_cast<const LV2_Atom_Int *>(value))->body;
            if (pflags & LSP_LV2_PRIVATE)
                result         |= core::KVT_PRIVATE;

            *flags          = result;
            return true;
        }

        bool Wrapper::parse_kvt_value(core::kvt_param_t *param, const LV2_Atom *value)
        {
            LV2_URID p_type     = value->type;
            core::kvt_param_t p;

            if (p_type == pExt->forge.Int)
            {
                p.type  = core::KVT_INT32;
                p.i32   = (reinterpret_cast<const LV2_Atom_Int *>(value))->body;
            }
            else if (p_type == pExt->uridTypeUInt)
            {
                p.type  = core::KVT_UINT32;
                p.u32   = (reinterpret_cast<const LV2_Atom_Int *>(value))->body;
            }
            else if (p_type == pExt->forge.Long)
            {
                p.type  = core::KVT_INT64;
                p.i64   = (reinterpret_cast<const LV2_Atom_Long *>(value))->body;
            }
            else if (p_type == pExt->uridTypeULong)
            {
                p.type  = core::KVT_UINT64;
                p.u64   = (reinterpret_cast<const LV2_Atom_Long *>(value))->body;
            }
            else if (p_type == pExt->forge.Float)
            {
                p.type  = core::KVT_FLOAT32;
                p.f32   = (reinterpret_cast<const LV2_Atom_Float *>(value))->body;
            }
            else if (p_type == pExt->forge.Double)
            {
                p.type  = core::KVT_FLOAT64;
                p.f64   = (reinterpret_cast<const LV2_Atom_Double *>(value))->body;
            }
            else if (p_type == pExt->forge.String)
            {
                p.type  = core::KVT_STRING;
                p.str   = reinterpret_cast<const char *>(&value[1]);
            }
            else if ((p_type == pExt->uridObject) || (p_type == pExt->uridBlank))
            {
                const LV2_Atom_Object *obj = reinterpret_cast<const LV2_Atom_Object *>(value);
                lsp_trace("obj->atom.type = %d (%s), obj->body.id = %d (%s), obj->body.otype = %d (%s)",
                        obj->atom.type, pExt->unmap_urid(obj->atom.type),
                        obj->body.id, pExt->unmap_urid(obj->body.id),
                        obj->body.otype, pExt->unmap_urid(obj->body.otype)
                        );

                if (obj->body.otype != pExt->uridBlobType)
                {
                    lsp_warn("Expected object of BLOB type but get: %d (%s)",
                        obj->body.otype, pExt->unmap_urid(obj->body.otype));
                    return false;
                }

                // Read the value
                p.blob.ctype    = NULL;
                p.blob.data     = NULL;
                p.blob.size     = ~size_t(0);

                for (
                    LV2_Atom_Property_Body *blob = lv2_atom_object_begin(&obj->body) ;
                    !lv2_atom_object_is_end(&obj->body, obj->atom.size, blob) ;
                    blob = lv2_atom_object_next(blob)
                )
                {
                    lsp_trace("blob->key (%d) = %s", int(blob->key), pExt->unmap_urid(blob->key));
                    lsp_trace("blob->value.type (%d) = %s", int(blob->value.type), pExt->unmap_urid(blob->value.type));

                    if ((blob->key == pExt->uridContentType) && (blob->value.type == pExt->forge.String))
                        p.blob.ctype    = reinterpret_cast<const char *>(&blob[1]);
                    else if ((blob->key == pExt->uridContent) && (blob->value.type == pExt->forge.Chunk))
                    {
                        p.blob.size     = blob->value.size;
                        if (p.blob.size > 0)
                            p.blob.data     = &blob[1];
                    }
                }

                // Change type
                if (p.blob.size != (~size_t(0)))
                    p.type          = core::KVT_BLOB;
            }
            else
                return false;

            *param = p;
            return true;
        }

        void Wrapper::restore_state(
            LV2_State_Retrieve_Function retrieve,
            LV2_State_Handle            handle,
            uint32_t                    flags,
            const LV2_Feature *const *  features
        )
        {
            pExt->init_state_context(NULL, retrieve, handle, flags, features);

            // Restore posts state
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                // Get port
                lv2::Port *lvp  = vAllPorts[i];
                if (lvp == NULL)
                    continue;

                // Restore state of port
                lvp->restore();
            }

            // Restore KVT state
            if (sKVTMutex.lock())
            {
                // Clear KVT
                sKVT.clear();
                restore_kvt_parameters();
                sKVT.gc();
                sKVTMutex.unlock();
            }

            pExt->reset_state_context();
            pPlugin->state_loaded();

            nStateMode = SM_LOADING;
            lsp_trace("#STATE MODE = %d", nStateMode);
        }

        bool Wrapper::change_state_atomic(state_mode_t from, state_mode_t to)
        {
            // Perform atomic state change
            while (true)
            {
                if (nStateMode != from)
                    return false;
                if (atomic_cas(&nStateMode, from, to))
                {
                    lsp_trace("#STATE state changed from %d to %d", int(from), int(to));
                    return true;
                }
            }
        }

        void Wrapper::connect_direct_ui()
        {
            // Increment number of clients
            nDirectClients++;
            if (pKVTDispatcher != NULL)
                pKVTDispatcher->connect_client();
        }

        void Wrapper::disconnect_direct_ui()
        {
            if (nDirectClients <= 0)
                return;
            --nDirectClients;
            if (pKVTDispatcher != NULL)
                pKVTDispatcher->disconnect_client();
        }

        void Wrapper::job_run(
            LV2_Worker_Respond_Handle   handle,
            LV2_Worker_Respond_Function respond,
            uint32_t                    size,
            const void*                 data
        )
        {
            lv2::Executor *executor = static_cast<lv2::Executor *>(pExecutor);
            executor->run_job(handle, respond, size, data);
        }

        ipc::IExecutor *Wrapper::executor()
        {
            lsp_trace("executor = %p", reinterpret_cast<void *>(pExecutor));
            if (pExecutor != NULL)
                return pExecutor;

            // Create executor service
            if (pExt->sched != NULL)
            {
                lsp_trace("Creating LV2 host-provided executor service");
                pExecutor       = new lv2::Executor(pExt->sched);
            }
            else
            {
                lsp_trace("Creating native executor service");
                ipc::NativeExecutor *exec = new ipc::NativeExecutor();
                if (exec == NULL)
                    return NULL;
                status_t res = exec->start();
                if (res != STATUS_OK)
                {
                    delete exec;
                    return NULL;
                }
                pExecutor   = exec;
            }
            return pExecutor;
        }

        LV2_Inline_Display_Image_Surface *Wrapper::render_inline_display(size_t width, size_t height)
        {
            // Allocate canvas for drawing
            plug::ICanvas *canvas       = create_canvas(width, height);
            if (canvas == NULL)
                return NULL;

            // Call plugin for rendering and return canvas data
            bool res = pPlugin->inline_display(canvas, width, height);
            canvas->sync();

            // Obtain canvas data
            plug::canvas_data_t *data   = canvas->data();
            if ((!res) || (data == NULL) || (data->pData == NULL))
                return NULL;

            // Fill-in surface and return
            sSurface.data           = reinterpret_cast<unsigned char *>(data->pData);
            sSurface.width          = data->nWidth;
            sSurface.height         = data->nHeight;
            sSurface.stride         = data->nStride;

            return &sSurface;
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

        const meta::package_t *Wrapper::package() const
        {
            return pPackage;
        }

        meta::plugin_format_t Wrapper::plugin_format() const
        {
            return meta::PLUGIN_LV2;
        }

        void Wrapper::state_changed()
        {
            change_state_atomic(SM_SYNC, SM_CHANGED);
        }

        void Wrapper::query_display_draw()
        {
            bQueueDraw = true;
        }

        void Wrapper::request_settings_update()
        {
            bUpdateSettings     = true;
        }

    } /* namespace lv2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_IMPL_WRAPPER_H_ */
