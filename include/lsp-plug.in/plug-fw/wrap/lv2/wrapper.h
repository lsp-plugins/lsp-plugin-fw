/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 20 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/core/KVTDispatcher.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/executor.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/ports.h>

namespace lsp
{
    namespace lv2
    {
        /**
         * LV2 format plugin wrapper
         */
        class Wrapper: public plug::IWrapper
        {
            public:
                enum state_mode_t
                {
                    SM_SYNC,        // State is in sync with host
                    SM_CHANGED,     // State has been changed
                    SM_REPORTED,    // State change has been reported to the host
                    SM_LOADING      // State has been loaded but still not committed
                };

            private:
                class LV2KVTListener: public core::KVTListener
                {
                    private:
                        lv2::Wrapper *pWrapper;

                    public:
                        explicit LV2KVTListener(lv2::Wrapper *wrapper) { pWrapper = wrapper; }

                    public:
                        virtual void created(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending);
                        virtual void changed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *oval, const core::kvt_param_t *nval, size_t pending);
                        virtual void removed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending);
                };

            protected:
                lltl::parray<lv2::Port>         vExtPorts;
                lltl::parray<lv2::Port>         vAllPorts;      // List of all created ports, for garbage collection
                lltl::parray<lv2::Port>         vPluginPorts;   // All plugin ports sorted in urid order
                lltl::parray<lv2::Port>         vMeshPorts;
                lltl::parray<lv2::Port>         vFrameBufferPorts;
                lltl::parray<lv2::Port>         vStreamPorts;
                lltl::parray<lv2::Port>         vMidiInPorts;
                lltl::parray<lv2::Port>         vMidiOutPorts;
                lltl::parray<lv2::Port>         vOscInPorts;
                lltl::parray<lv2::Port>         vOscOutPorts;
                lltl::parray<lv2::AudioPort>    vAudioPorts;
                lltl::parray<meta::port_t>      vGenMetadata;   // Generated metadata

                lv2::Extensions        *pExt;
                ipc::IExecutor         *pExecutor;      // Executor service
                void                   *pAtomIn;        // Atom input port
                void                   *pAtomOut;       // Atom output port
                float                  *pLatency;       // Latency output port
                size_t                  nPatchReqs;     // Number of patch requests
                size_t                  nStateReqs;     // Number of state requests
                ssize_t                 nSyncTime;      // Synchronization time
                ssize_t                 nSyncSamples;   // Synchronization counter
                ssize_t                 nClients;       // Number of clients
                ssize_t                 nDirectClients; // Number of direct clients
                bool                    bQueueDraw;     // Queue draw request
                bool                    bUpdateSettings;// Settings update
                float                   fSampleRate;
                uint8_t                *pOscPacket;     // OSC packet data
                volatile atomic_t       nStateMode;     // State change flag
                volatile atomic_t       nDumpReq;
                atomic_t                nDumpResp;

                plug::position_t        sPosition;
                core::KVTStorage        sKVT;
                LV2KVTListener          sKVTListener;
                ipc::Mutex              sKVTMutex;
                core::KVTDispatcher    *pKVTDispatcher;

                plug::ICanvas          *pCanvas;        // Canvas for drawing inline display
                LV2_Inline_Display_Image_Surface sSurface; // Canvas surface

            protected:
                lv2::Port                      *create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *meta, const char *postfix, bool virt);
                void                            clear_midi_ports();
                void                            receive_atoms(size_t samples);
                void                            transmit_atoms(size_t samples);

            public:
                explicit Wrapper(plug::Module *plugin, resource::ILoader *loader, lv2::Extensions *ext);
                virtual ~Wrapper();

                status_t                        init(float srate);
                void                            destroy();

            public:
                inline void                     activate()          { pPlugin->activate(); }
                inline void                     deactivate()        { pPlugin->deactivate(); }

                inline void                     connect(size_t id, void *data);
                inline void                     run(size_t samples);
                bool                            change_state_atomic(state_mode_t from, state_mode_t to);

                // State part
                inline void save_state(
                    LV2_State_Store_Function   store,
                    LV2_State_Handle           handle,
                    uint32_t                   flags,
                    const LV2_Feature *const * features
                );

                inline void restore_state(
                    LV2_State_Retrieve_Function retrieve,
                    LV2_State_Handle            handle,
                    uint32_t                    flags,
                    const LV2_Feature *const *  features
                );

                // Job part
                virtual ipc::IExecutor         *executor();

                inline void job_run(
                    LV2_Worker_Respond_Handle   handle,
                    LV2_Worker_Respond_Function respond,
                    uint32_t                    size,
                    const void*                 data
                );

                inline void job_response(size_t size, const void *body) {}
                inline void job_end() {}

                // Inline display part
                LV2_Inline_Display_Image_Surface *render_inline_display(size_t width, size_t height);

                inline void                     query_display_draw()    { bQueueDraw = true; }

                virtual const plug::position_t *position()              { return &sPosition; }

                lv2::Port                      *port(const char *id);

                void                            connect_direct_ui();

                void                            disconnect_direct_ui();

                inline float                    get_sample_rate() const { return fSampleRate; }

                virtual core::KVTStorage       *kvt_lock();

                virtual core::KVTStorage       *kvt_trylock();

                virtual bool                    kvt_release();

                virtual void                    state_changed()         { change_state_atomic(SM_SYNC, SM_CHANGED); }

                inline core::KVTDispatcher     *kvt_dispatcher()        { return pKVTDispatcher; }
        };
    } /* namespace lv2 */
} /* namespace lsp */


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
            pCanvas         = NULL;
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
            pKVTDispatcher  = NULL;

            plug::position_t::init(&sPosition);
        }

        Wrapper::~Wrapper()
        {
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
            pCanvas         = NULL;
            sSurface.data   = NULL;
            sSurface.width  = 0;
            sSurface.height = 0;
            sSurface.stride = 0;
        }

        static ssize_t compare_ports_by_urid(const lv2::Port *a, const lv2::Port *b)
        {
            return ssize_t(a->get_urid()) - ssize_t(b->get_urid());
        }

        status_t Wrapper::init(float srate)
        {
            // Update sample rate
            fSampleRate = srate;

            // Get plugin metadata
            const meta::plugin_t *m  = pPlugin->metadata();

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

            // Update refresh rate
            nSyncSamples        = srate / pExt->ui_refresh_rate();
            nClients            = 0;

            return STATUS_OK;
        }

        void Wrapper::destroy()
        {
            // Stop KVT dispatcher
            if (pKVTDispatcher != NULL)
            {
                lsp_trace("Stopping KVT dispatcher thread...");
                pKVTDispatcher->cancel();
                pKVTDispatcher->join();
                delete pKVTDispatcher;

                sKVT.unbind(&sKVTListener);
            }

            // Drop surface
            sSurface.data           = NULL;
            sSurface.width          = 0;
            sSurface.height         = 0;
            sSurface.stride         = 0;

            // Drop canvas
            lsp_trace("canvas = %p", pCanvas);
            if (pCanvas != NULL)
            {
                pCanvas->destroy();
                delete pCanvas;
                pCanvas     = NULL;
            }

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

            vAllPorts.flush();
            vExtPorts.flush();
            vMeshPorts.flush();
            vStreamPorts.flush();
            vMidiInPorts.flush();
            vMidiOutPorts.flush();
            vOscInPorts.flush();
            vOscOutPorts.flush();
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

                case meta::R_MIDI:
                    if (pExt->atom_supported())
                        result = new lv2::MidiPort(p, pExt);
                    else
                        result = new lv2::Port(p, pExt, false);
                    if (meta::is_out_port(p))
                        vMidiOutPorts.add(result);
                    else
                        vMidiInPorts.add(result);
                    plugin_ports->add(result);
                    break;
                case meta::R_OSC:
                    if (pExt->atom_supported())
                        result = new lv2::OscPort(p, pExt);
                    else
                        result = new lv2::Port(p, pExt, false);
                    if (meta::is_out_port(p))
                        vOscOutPorts.add(result);
                    else
                        vOscInPorts.add(result);
                    plugin_ports->add(result);
                    break;


                case meta::R_AUDIO:
                    result = new lv2::AudioPort(p, pExt);

                    result->set_id(vPluginPorts.size());
                    vPluginPorts.add(result);
                    vAudioPorts.add(static_cast<lv2::AudioPort *>(result));
                    vExtPorts.add(result);
                    plugin_ports->add(result);
                    lsp_trace("Added external port id=%s, external_id=%d", result->metadata()->id, int(vExtPorts.size() - 1));
                    break;
                case meta::R_CONTROL:
                case meta::R_METER:
                    if (meta::is_out_port(p))
                        result      = new lv2::OutputPort(p, pExt);
                    else
                        result      = new lv2::InputPort(p, pExt, virt);

                    result->set_id(vPluginPorts.size());
                    vPluginPorts.add(result);
                    vExtPorts.add(result);
                    plugin_ports->add(result);
                    lsp_trace("Added external port id=%s, external_id=%d", result->metadata()->id, int(vExtPorts.size() - 1));
                    break;

                case meta::R_BYPASS:
                    if (meta::is_out_port(p))
                        result      = new lv2::Port(p, pExt, false);
                    else
                        result      = new lv2::BypassPort(p, pExt);

                    result->set_id(vPluginPorts.size());
                    vPluginPorts.add(result);
                    vExtPorts.add(result);
                    plugin_ports->add(result);
                    lsp_trace("Added bypass port id=%s, external_id=%d", result->metadata()->id, int(vExtPorts.size() - 1));
                    break;

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];
                    lv2::PortGroup   *pg    = new lv2::PortGroup(p, pExt, virt);
                    vPluginPorts.add(pg);

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
                            lv2::Port *p    = create_port(plugin_ports, cm, postfix_buf, true);
                            if ((p != NULL) && (p->metadata()->role != meta::R_PORT_SET))
                            {
                                vPluginPorts.add(p);
                                plugin_ports->add(p);
                            }
                        }
                    }

                    result  = pg;
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
            size_t n_all_ports      = vAllPorts.size();
            lv2::Port **v_all_ports = vAllPorts.array();
            size_t smode            = nStateMode;
            for (size_t i=0; i<n_all_ports; ++i)
            {
                // Get port
                lv2::Port *port = v_all_ports[i];
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
                lsp_trace("updating settings");
                pPlugin->update_settings();
                bUpdateSettings     = false;
            }

            // Need to dump state?
            atomic_t dump_req   = nDumpReq;
            if (dump_req != nDumpResp)
            {
                dump_plugin_state();
                nDumpResp           = dump_req;
            }

            // Call the main processing unit (split data buffers into chunks not greater than MaxBlockLength)
            size_t n_in_ports = vAudioPorts.size();
            for (size_t off=0; off < samples; )
            {
                size_t to_process = samples - off;
                if (to_process > pExt->nMaxBlockLength)
                    to_process = pExt->nMaxBlockLength;

                for (size_t i=0; i<n_in_ports; ++i)
                    vAudioPorts.uget(i)->sanitize(off, to_process);
                pPlugin->process(to_process);

                off += to_process;
            }

            // Transmit atoms (if possible)
            transmit_atoms(samples);
            clear_midi_ports();

            // Post-process regular ports for changes
            for (size_t i=0; i<n_all_ports; ++i)
            {
                lv2::Port *port = v_all_ports[i];
                if (port != NULL)
                    port->post_process(samples);
            }

            // Transmit latency (if possible)
            if (pLatency != NULL)
            {
    //            lsp_trace("Reporting latency: %d", int(pPlugin->get_latency()));
                *pLatency   = pPlugin->latency();
            }
            else
            {
                lsp_trace("Could not report latency, NULL latency binding");
            }
        }

        void Wrapper::clear_midi_ports()
        {
            // Clear all MIDI_IN ports
            for (size_t i=0, n=vMidiInPorts.size(); i<n; ++i)
            {
                lv2::Port *p        = vMidiInPorts.uget(i);
                if ((p == NULL) || (p->metadata()->role != meta::R_MIDI))
                    continue;
                plug::midi_t *midi  = p->buffer<plug::midi_t>();
                if (midi != NULL)
                    midi->clear();
            }

            // Clear all MIDI_OUT ports
            for (size_t i=0, n=vMidiOutPorts.size(); i<n; ++i)
            {
                lv2::Port *p        = vMidiOutPorts.uget(i);
                if ((p == NULL) || (p->metadata()->role != meta::R_MIDI))
                    continue;
                plug::midi_t *midi  = p->buffer<plug::midi_t>();
                if (midi != NULL)
                    midi->clear();
            }
        }

        void Wrapper::receive_atoms(size_t samples)
        {
            // TODO
        }

        void Wrapper::transmit_atoms(size_t samples)
        {
            // TODO
        }

        void Wrapper::save_state(
                LV2_State_Store_Function   store,
                LV2_State_Handle           handle,
                uint32_t                   flags,
                const LV2_Feature *const * features)
        {
            // TODO
        }

        void Wrapper::restore_state(
            LV2_State_Retrieve_Function retrieve,
            LV2_State_Handle            handle,
            uint32_t                    flags,
            const LV2_Feature *const *  features
        )
        {
            // TODO
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
            plug::canvas_data_t *data   = pCanvas->data();
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
    } /* namespace lv2 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_WRAPPER_H_ */
