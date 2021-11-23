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
#include <lsp-plug.in/plug-fw/wrap/lv2/sink.h>

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
                void                            save_kvt_parameters();
                void                            restore_kvt_parameters();

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
            lv2_atom_forge_object(&forge, &frame, 0, pExt->uridKvtType);

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

                LV2_URID key        =  pExt->map_kvt(name);

                LV2_Atom_Forge_Frame prop;
                lv2_atom_forge_key(&forge, key);
                lv2_atom_forge_object(&forge, &prop, 0, pExt->uridKvtPropertyType);
                lv2_atom_forge_key(&forge, pExt->uridKvtPropertyFlags);
                lv2_atom_forge_int(&forge, flags);

                switch (p->type)
                {
                    case core::KVT_INT32:
                    case core::KVT_UINT32:
                    {
                        LV2_Atom_Int v;
                        v.atom.type     = (p->type == core::KVT_INT32) ? pExt->forge.Int : pExt->uridTypeUInt;
                        v.atom.size     = sizeof(v) - sizeof(LV2_Atom);
                        v.body          = p->i32;

                        lv2_atom_forge_key(&forge, pExt->uridKvtPropertyValue);
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
                        lv2_atom_forge_key(&forge, pExt->uridKvtPropertyValue);
                        lv2_atom_forge_primitive(&forge, &v.atom);
                        break;
                    }
                    case core::KVT_FLOAT32:
                    {
                        LV2_Atom_Float v;
                        v.atom.type     = pExt->forge.Float;
                        v.atom.size     = sizeof(v) - sizeof(LV2_Atom);
                        v.body          = p->f32;
                        lv2_atom_forge_key(&forge, pExt->uridKvtPropertyValue);
                        lv2_atom_forge_primitive(&forge, &v.atom);
                        break;
                    }
                    case core::KVT_FLOAT64:
                    {
                        LV2_Atom_Double v;
                        v.atom.type     = pExt->forge.Double;
                        v.atom.size     = sizeof(v) - sizeof(LV2_Atom);
                        v.body          = p->f64;
                        lv2_atom_forge_key(&forge, pExt->uridKvtPropertyValue);
                        lv2_atom_forge_primitive(&forge, &v.atom);
                        break;
                    }
                    case core::KVT_STRING:
                    {
                        lv2_atom_forge_key(&forge, pExt->uridKvtPropertyValue);
                        const char *str = (p->str != NULL) ? p->str : "";
                        lv2_atom_forge_typed_string(&forge, forge.String, str, ::strlen(str));
                        break;
                    }
                    case core::KVT_BLOB:
                    {
                        LV2_Atom_Forge_Frame obj;

                        lv2_atom_forge_key(&forge, pExt->uridKvtPropertyValue);
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
                        res     = STATUS_BAD_TYPE;
                        break;
                }

                lv2_atom_forge_pop(&forge, &prop);

                // Successful status?
                if (res != STATUS_OK)
                    break;
            } // while

            if ((res == STATUS_OK) && (sink.res == STATUS_OK))
            {
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
            else
                lsp_trace("Failed execution, result=%d, sink state=%d", int(res), int(sink.res));
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
            uint32_t p_type;
            size_t p_size;
            const void *ptr = pExt->retrieve_value(pExt->uridKvtObject, &p_type, &p_size);
            size_t prefix_len = ::strlen(pExt->uriKvt);
            if (ptr == NULL)
                return;

//            lsp_dumpb("Contents of the atom object:", ptr, p_size);
            lsp_trace("p_type = %d (%s), p_size = %d", int(p_type), pExt->unmap_urid(p_type), int(p_size));

            if ((p_type == pExt->forge.Object) || (p_type == pExt->uridBlank))
            {
                const LV2_Atom_Object_Body *obody = reinterpret_cast<const LV2_Atom_Object_Body *>(ptr);

                for (
                    LV2_Atom_Property_Body *body = lv2_atom_object_begin(obody) ;
                    !lv2_atom_object_is_end(obody, p_size, body) ;
                    body = lv2_atom_object_next(body)
                )
                {
//                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
//                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    const LV2_Atom_Object *pobject = reinterpret_cast<const LV2_Atom_Object *>(&body->value);
                    if ((pobject->atom.type != pExt->uridObject) && (pobject->atom.type != pExt->uridBlank))
                    {
                        lsp_warn("Unsupported value type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));
                        continue;
                    }
//                    lsp_trace("pobject->body.otype (%d) = %s", int(pobject->body.otype), pExt->unmap_urid(pobject->body.otype));
                    if (pobject->body.otype != pExt->uridKvtPropertyType)
                    {
                        lsp_warn("Unsupported object type (%d) = %s", int(pobject->body.otype), pExt->unmap_urid(pobject->body.otype));
                        continue;
                    }

                    const char *uri = pExt->unmap_urid(body->key);
                    if (::strncmp(uri, pExt->uriKvt, prefix_len) != uri)
                    {
                        lsp_warn("Invalid property: urid=%d, uri=%s", body->key, uri);
                        continue;
                    }
                    if (uri[prefix_len] != '/')
                    {
                        lsp_warn("Invalid property: urid=%d, uri=%s", body->key, uri);
                        continue;
                    }

                    const char *name = &uri[prefix_len + 1];
                    core::kvt_param_t p;
                    size_t flags    = core::KVT_TX;
                    p.type          = core::KVT_ANY;

                    for (
                        LV2_Atom_Property_Body *xbody = lv2_atom_object_begin(&pobject->body) ;
                        !lv2_atom_object_is_end(&pobject->body, body->value.size, xbody) ;
                        xbody = lv2_atom_object_next(xbody)
                    )
                    {
//                        lsp_trace("xbody->key (%d) = %s", int(xbody->key), pExt->unmap_urid(xbody->key));
//                        lsp_trace("xbody->value.type (%d) = %s", int(xbody->value.type), pExt->unmap_urid(xbody->value.type));

                        // Analyze type of value
                        if (xbody->key == pExt->uridKvtPropertyFlags)
                        {
                            if (xbody->value.type == pExt->forge.Int)
                            {
                                size_t pflags   = (reinterpret_cast<const LV2_Atom_Int *>(&xbody->value))->body;
                                lsp_trace("pflags = %d", int(pflags));
                                if (pflags & LSP_LV2_PRIVATE)
                                    flags          |= core::KVT_PRIVATE;
                            }
                        }
                        else if (xbody->key == pExt->uridKvtPropertyValue)
                        {
                            p_type      = xbody->value.type;

                            if (p_type == pExt->forge.Int)
                            {
                                p.type  = core::KVT_INT32;
                                p.i32   = (reinterpret_cast<const LV2_Atom_Int *>(&xbody->value))->body;
                            }
                            else if (p_type == pExt->uridTypeUInt)
                            {
                                p.type  = core::KVT_UINT32;
                                p.u32   = (reinterpret_cast<const LV2_Atom_Int *>(&xbody->value))->body;
                            }
                            else if (p_type == pExt->forge.Long)
                            {
                                p.type  = core::KVT_INT64;
                                p.i64   = (reinterpret_cast<const LV2_Atom_Long *>(&xbody->value))->body;
                            }
                            else if (p_type == pExt->uridTypeULong)
                            {
                                p.type  = core::KVT_UINT64;
                                p.u64   = (reinterpret_cast<const LV2_Atom_Long *>(&xbody->value))->body;
                            }
                            else if (p_type == pExt->forge.Float)
                            {
                                p.type  = core::KVT_FLOAT32;
                                p.f32   = (reinterpret_cast<const LV2_Atom_Float *>(&xbody->value))->body;
                            }
                            else if (p_type == pExt->forge.Double)
                            {
                                p.type  = core::KVT_FLOAT64;
                                p.f64   = (reinterpret_cast<const LV2_Atom_Double *>(&xbody->value))->body;
                            }
                            else if (p_type == pExt->forge.String)
                            {
                                p.type  = core::KVT_STRING;
                                p.str   = reinterpret_cast<const char *>(&body[1]);
                            }
                            else if ((p_type == pExt->uridObject) || (p_type == pExt->uridBlank))
                            {
                                const LV2_Atom_Object *obj = reinterpret_cast<const LV2_Atom_Object *>(&xbody->value);
                                lsp_trace("obj->atom.type = %d (%s), obj->body.id = %d (%s), obj->body.otype = %d (%s)",
                                        obj->atom.type, pExt->unmap_urid(obj->atom.type),
                                        obj->body.id, pExt->unmap_urid(obj->body.id),
                                        obj->body.otype, pExt->unmap_urid(obj->body.otype)
                                        );

                                if (obj->body.otype != pExt->uridBlobType)
                                    break;

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
                        }
                    }

                    // Store property
                    if (p.type != core::KVT_ANY)
                    {
                        kvt_dump_parameter("Fetched parameter %s = ", &p, name);
                        status_t res = sKVT.put(name, &p, flags);
                        if (res != STATUS_OK)
                            lsp_warn("Could not store parameter to KVT");
                    }
                    else
                        lsp_warn("KVT property %s has unsupported type or is invalid: 0x%x (%s)",
                                name, p_type, pExt->unmap_urid(p_type));
                }
            } // for
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
