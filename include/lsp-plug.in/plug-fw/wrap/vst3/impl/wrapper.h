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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/io/charset.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/data.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/debug.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        //---------------------------------------------------------------------
        void Wrapper::VST3KVTListener::created(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending)
        {
            pWrapper->state_changed();
        }

        void Wrapper::VST3KVTListener::changed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *oval, const core::kvt_param_t *nval, size_t pending)
        {
            pWrapper->state_changed();
        }

        void Wrapper::VST3KVTListener::removed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending)
        {
            pWrapper->state_changed();
        }

        //---------------------------------------------------------------------
        Wrapper::Wrapper(PluginFactory *factory, plug::Module *plugin, resource::ILoader *loader, const meta::package_t *package):
            IWrapper(plugin, loader),
            sKVTListener(this)
        {
            nRefCounter         = 1;
            pFactory            = safe_acquire(factory);
            pPackage            = package;
            pHostContext        = NULL;
            pHostApplication    = NULL;
            pPeerConnection     = NULL;
            pExecutor           = NULL;
            pEventsIn           = NULL;
            pEventsOut          = NULL;
            pSamplePlayer       = NULL;

            pKVTDispatcher      = NULL;
            pOscPacket          = NULL;

            sUIPosition         = sPosition;

            atomic_init(nPositionLock);
            nUICounterReq       = 0;
            nUICounterResp      = 0;
            nDirtyReq           = 0;
            nDirtyResp          = 0;
            nDumpReq            = 0;
            nDumpResp           = 0;
            nMaxSamplesPerBlock = 0;
            bUpdateSettings     = true;

            nLatency            = 0;
        }

        Wrapper::~Wrapper()
        {
            // Destroy plugin
            if (pPlugin != NULL)
            {
                delete pPlugin;
                pPlugin         = NULL;
            }

            // Remove self from synchronization list
            pFactory->unregister_data_sync(this);

            // Release factory
            safe_release(pFactory);
        }

        Steinberg::tresult PLUGIN_API Wrapper::queryInterface(const Steinberg::TUID _iid, void **obj)
        {
            // Cast to the requested interface
            if (Steinberg::iidEqual(_iid, Steinberg::FUnknown::iid))
                return cast_interface<Steinberg::FUnknown>(static_cast<Steinberg::IDependent *>(this), obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IDependent::iid))
                return cast_interface<Steinberg::IDependent>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPluginBase::iid))
                return cast_interface<Steinberg::IPluginBase>(this, obj);

            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IComponent::iid))
                return cast_interface<Steinberg::Vst::IComponent>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IConnectionPoint::iid))
                return cast_interface<Steinberg::Vst::IConnectionPoint>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IAudioProcessor::iid))
                return cast_interface<Steinberg::Vst::IAudioProcessor>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IProcessContextRequirements::iid))
                return cast_interface<Steinberg::Vst::IProcessContextRequirements>(this, obj);

            return no_interface(obj);
        }

        Steinberg::uint32 PLUGIN_API Wrapper::addRef()
        {
            return atomic_add(&nRefCounter, 1) + 1;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::release()
        {
            atomic_t ref_count = atomic_add(&nRefCounter, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        void PLUGIN_API Wrapper::update(FUnknown* changedUnknown, Steinberg::int32 message)
        {
        }

        Wrapper::audio_bus_t *Wrapper::alloc_audio_bus(const char *name, size_t ports)
        {
            LSPString tmp;
            if (!tmp.set_utf8(name))
                return NULL;
            Steinberg::CStringW u16name  = reinterpret_cast<Steinberg::CStringW>(tmp.get_utf16());
            if (u16name == NULL)
                return NULL;

            // Allocate bus structure
            size_t szof_bus     = sizeof(audio_bus_t);
            size_t szof_ports   = sizeof(plug::IPort *) * ports;
            size_t szof_name    = (Steinberg::strlen16(u16name) + 1) * sizeof(Steinberg::char16);
            size_t szof         = align_size(szof_bus + szof_ports + szof_name, DEFAULT_ALIGN);

            uint8_t *ptr        = static_cast<uint8_t *>(malloc(szof));
            if (ptr == NULL)
                return NULL;

            audio_bus_t *bus    = advance_ptr_bytes<audio_bus_t>(ptr, szof_bus + szof_ports);
            bus->sName          = advance_ptr_bytes<Steinberg::char16>(ptr, szof_name);

            // Fill bus structure
            memcpy(bus->sName, u16name, szof_name);
            bus->nPorts         = ports;
            bus->bActive        = false;

            return bus;
        }

        void Wrapper::free_audio_bus(audio_bus_t *bus)
        {
            if (bus != NULL)
                free(bus);
        }

        Wrapper::event_bus_t *Wrapper::alloc_event_bus(const char *name, size_t ports)
        {
            LSPString tmp;
            if (!tmp.set_utf8(name))
                return NULL;
            Steinberg::CStringW u16name  = reinterpret_cast<Steinberg::CStringW>(tmp.get_utf16());
            if (u16name == NULL)
                return NULL;

            // Allocate bus structure
            size_t szof_bus     = sizeof(event_bus_t);
            size_t szof_name    = (Steinberg::strlen16(u16name) + 1) * sizeof(Steinberg::char16);
            size_t szof_ports   = sizeof(plug::IPort *) * ports;
            size_t szof         = align_size(szof_bus + szof_ports + szof_name, DEFAULT_ALIGN);

            uint8_t *ptr        = static_cast<uint8_t *>(malloc(szof));
            if (ptr == NULL)
                return NULL;

            event_bus_t *bus    = advance_ptr_bytes<event_bus_t>(ptr, szof_bus + szof_ports);
            bus->sName          = advance_ptr_bytes<Steinberg::char16>(ptr, szof_name);

            // Fill bus structure
            memcpy(bus->sName, u16name, szof_name);
            bus->nPorts         = ports;
            bus->bActive        = false;

            return bus;
        }

        void Wrapper::free_event_bus(event_bus_t *bus)
        {
            if (bus != NULL)
                free(bus);
        }

        plug::IPort *Wrapper::find_port(const char *id, lltl::parray<plug::IPort> *list)
        {
            for (size_t i=0, n=list->size(); i<n; ++i)
            {
                plug::IPort *p = list->uget(i);
                const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                if ((meta != NULL) && (!strcmp(meta->id, id)))
                    return p;
            }
            return NULL;
        }

        ssize_t Wrapper::compare_audio_ports_by_speaker(const vst3::AudioPort *a, const vst3::AudioPort *b)
        {
            const Steinberg::Vst::Speaker sp_a = a->speaker();
            const Steinberg::Vst::Speaker sp_b = b->speaker();

            return (sp_a > sp_b) ? 1 :
                   (sp_a < sp_b) ? -1 : 0;
        }

        ssize_t Wrapper::compare_in_param_ports(const vst3::InParamPort *a, const vst3::InParamPort *b)
        {
            const Steinberg::Vst::ParamID a_id = a->parameter_id();
            const Steinberg::Vst::ParamID b_id = b->parameter_id();

            return (a_id > b_id) ? 1 :
                   (a_id < b_id) ? -1 : 0;
        }

        ssize_t Wrapper::compare_out_param_ports(const vst3::OutParamPort *a, const vst3::OutParamPort *b)
        {
            const Steinberg::Vst::ParamID a_id = a->parameter_id();
            const Steinberg::Vst::ParamID b_id = b->parameter_id();

            return (a_id > b_id) ? 1 :
                   (a_id < b_id) ? -1 : 0;
        }

        Wrapper::audio_bus_t *Wrapper::create_audio_bus(const meta::port_group_t *meta, lltl::parray<plug::IPort> *ins, lltl::parray<plug::IPort> *outs)
        {
            lltl::parray<vst3::AudioPort> channels;

            lsp_trace("Creating audio bus %s", meta->id);

            // Form the list of channels sorted according to the speaker ordering bits
            lltl::parray<plug::IPort> *list = (meta->flags & meta::PGF_OUT) ? outs : ins;
            for (const meta::port_group_item_t *item = meta->items; (item != NULL) && (item->id != NULL); ++item)
            {
                // Find port and add to list
                plug::IPort *p  = find_port(item->id, list);
                if (p == NULL)
                {
                    lsp_error("Missing %s port '%s' for the audio group '%s'",
                        (meta->flags & meta::PGF_OUT) ? "output" : "input", item->id, meta->id);
                    return NULL;
                }

                vst3::AudioPort *xp = static_cast<vst3::AudioPort *>(p);
                if (!channels.add(xp))
                {
                    lsp_error("Failed channels.add");
                    return NULL;
                }

                // Set-up speaker role
                Steinberg::Vst::Speaker speaker = Steinberg::Vst::kSpeakerM;
                switch (item->role)
                {
                    case meta::PGR_CENTER:
                        speaker = (meta->type == meta::GRP_MONO) ? Steinberg::Vst::kSpeakerM : Steinberg::Vst::kSpeakerC;
                        break;
                    case meta::PGR_CENTER_LEFT:     speaker     = Steinberg::Vst::kSpeakerLc;   break;
                    case meta::PGR_CENTER_RIGHT:    speaker     = Steinberg::Vst::kSpeakerRc;   break;
                    case meta::PGR_LEFT:            speaker     = Steinberg::Vst::kSpeakerL;    break;
                    case meta::PGR_LO_FREQ:         speaker     = Steinberg::Vst::kSpeakerLfe;  break;
                    case meta::PGR_REAR_CENTER:     speaker     = Steinberg::Vst::kSpeakerTrc;  break;
                    case meta::PGR_REAR_LEFT:       speaker     = Steinberg::Vst::kSpeakerTrl;  break;
                    case meta::PGR_REAR_RIGHT:      speaker     = Steinberg::Vst::kSpeakerTrr;  break;
                    case meta::PGR_RIGHT:           speaker     = Steinberg::Vst::kSpeakerR;    break;
                    case meta::PGR_SIDE_LEFT:       speaker     = Steinberg::Vst::kSpeakerSl;   break;
                    case meta::PGR_SIDE_RIGHT:      speaker     = Steinberg::Vst::kSpeakerSr;   break;
                    case meta::PGR_MS_SIDE:         speaker     = Steinberg::Vst::kSpeakerC;    break;
                    case meta::PGR_MS_MIDDLE:       speaker     = Steinberg::Vst::kSpeakerS;    break;
                    default:
                        lsp_error("Unsupported role %d for channel '%s' in group '%s'",
                            int(item->role), item->id, meta->id);
                        return NULL;
                }
                xp->set_speaker(speaker);

                // Exclude port from list
                list->premove(p);
            }
            channels.qsort(compare_audio_ports_by_speaker);

            // Allocate the audio group object
            audio_bus_t *bus    = alloc_audio_bus(meta->id, channels.size());
            if (bus == NULL)
            {
                lsp_error("failed alloc_audio_bus");
                return NULL;
            }
            lsp_finally {
                free_audio_bus(bus);
            };

            // Fill the audio bus data
            bus->nType          = meta->type;
            bus->nPorts         = channels.size();
            bus->nFullArr       = 0;
            bus->nMinArr        = 0;
            bus->nBusType       = (meta->flags & meta::PGF_SIDECHAIN) ? Steinberg::Vst::kAux : Steinberg::Vst::kMain;

            for (size_t i=0; i<bus->nPorts; ++i)
            {
                vst3::AudioPort *xp = channels.uget(i);
                bus->nFullArr      |= xp->speaker();
                bus->nMinArr       |= (meta::is_optional_port(xp->metadata())) ? 0 : xp->speaker();
                bus->vPorts[i]      = xp;
            }
            bus->nCurrArr       = bus->nFullArr;

            lsp_trace("Created audio bus id=%s, ptr=%p", meta->id, bus);

            return release_ptr(bus);
        }

        Wrapper::audio_bus_t *Wrapper::create_audio_bus(plug::IPort *port)
        {
            const meta::port_t *meta = (port != NULL) ? port->metadata() : NULL;
            if (meta == NULL)
                return NULL;

            lsp_trace("Creating audio bus %s", meta->id);

            // Allocate the audio group object
            audio_bus_t *bus    = alloc_audio_bus(meta->id, 1);
            if (bus == NULL)
            {
                lsp_error("failed alloc_audio_bus");
                return NULL;
            }
            lsp_finally {
                free_audio_bus(bus);
            };

            // Fill the audio bus data
            vst3::AudioPort *xp = static_cast<vst3::AudioPort *>(port);

            bus->nType          = meta::GRP_MONO;
            bus->nPorts         = 1;
            bus->nCurrArr       = xp->speaker();
            bus->nMinArr        = (meta::is_optional_port(meta)) ? 0 : xp->speaker();
            bus->nFullArr       = bus->nCurrArr;
            bus->nBusType       = Steinberg::Vst::kMain;
            bus->vPorts[0]      = xp;

            lsp_trace("Created audio bus id=%s, ptr=%p", meta->id, bus);

            return release_ptr(bus);
        }

        void Wrapper::update_port_activity(audio_bus_t *bus)
        {
            const Steinberg::Vst::SpeakerArrangement arr    = (bus->bActive) ? bus->nCurrArr : 0;
            for (size_t i=0; i<bus->nPorts; ++i)
            {
                vst3::AudioPort *p  = bus->vPorts[i];
                p->set_active(p->speaker() & arr);
            }
        }

        bool Wrapper::create_busses(const meta::plugin_t *meta)
        {
            lsp_trace("Generating list of audio and midi ports");

            // Generate modifiable lists of input and output ports
            lltl::parray<plug::IPort> ins, outs, midi_ins, midi_outs;
            for (size_t i=0, n=vAllPorts.size(); i < n; ++i)
            {
                plug::IPort *p = vAllPorts.uget(i);
                const meta::port_t *port = (p != NULL) ? p->metadata() : NULL;
                if (port == NULL)
                    continue;

                if (meta::is_audio_port(port))
                {
                    if (meta::is_in_port(port))
                        ins.add(p);
                    else
                        outs.add(p);
                }
                else if (meta::is_midi_port(port))
                {
                    if (meta::is_in_port(port))
                        midi_ins.add(p);
                    else
                        midi_outs.add(p);
                }
            }

            // Create audio busses based on the information about port groups
            lsp_trace("Creating audio busses");
            audio_bus_t *in_main = NULL, *out_main = NULL, *bus = NULL;
            for (const meta::port_group_t *pg = meta->port_groups; (pg != NULL) && (pg->id != NULL); ++pg)
            {
                lsp_trace("Processing port group id=%s", pg->id);

                // Create group and add to list
                if ((bus = create_audio_bus(pg, &ins, &outs)) == NULL)
                {
                    lsp_error("failed to create audio bus %s", pg->id);
                    return false;
                }
                lsp_finally { free_audio_bus(bus); };

                // Add the group to list or keep as a separate pointer because CLAP
                // requires main ports to be first in the overall port list
                if (pg->flags & meta::PGF_OUT)
                {
                    if (pg->flags & meta::PGF_MAIN)
                    {
                        if (in_main != NULL)
                        {
                            lsp_error("Duplicate main output bus in metadata");
                            return false;
                        }
                        in_main         = bus;
                        if (!vAudioOut.insert(0, bus))
                        {
                            lsp_error("failed to register audio bus %s", pg->id);
                            return false;
                        }
                    }
                    else
                    {
                        if (!vAudioOut.add(bus))
                        {
                            lsp_error("failed to register audio bus %s", pg->id);
                            return false;
                        }
                    }
                }
                else // meta::PGF_IN
                {
                    if (pg->flags & meta::PGF_MAIN)
                    {
                        if (out_main != NULL)
                        {
                            lsp_error("Duplicate main input bus in metadata");
                            return false;
                        }
                        out_main    = bus;
                        if (!vAudioIn.insert(0, bus))
                        {
                            lsp_error("failed to register audio bus %s", pg->id);
                            return false;
                        }
                    }
                    else
                    {
                        if (!vAudioIn.add(bus))
                        {
                            lsp_error("failed to register audio bus %s", pg->id);
                            return false;
                        }
                    }
                }

                // Release the group pointer to prevent from destruction
                bus     = NULL;
            }

            // Create mono audio busses for non-assigned ports
            lsp_trace("Creating input audio busses for non-assigned ports");
            for (lltl::iterator<plug::IPort> it = ins.values(); it; ++it)
            {
                plug::IPort *p = it.get();
                if ((bus = create_audio_bus(p)) == NULL)
                {
                    lsp_error("failed to create audio bus %s", p->metadata()->id);
                    return false;
                }
                lsp_finally { free_audio_bus(bus); };

                if (!vAudioIn.add(bus))
                {
                    lsp_error("failed to register audio bus %s", p->metadata()->id);
                    return false;
                }
                bus     = NULL;
            }

            lsp_trace("Creating output audio busses for non-assigned ports");
            for (lltl::iterator<plug::IPort> it = outs.values(); it; ++it)
            {
                plug::IPort *p = it.get();
                if ((bus = create_audio_bus(p)) == NULL)
                {
                    lsp_error("failed to create audio bus %s", p->metadata()->id);
                    return false;
                }
                lsp_finally { free_audio_bus(bus); };

                if (!vAudioOut.add(bus))
                {
                    lsp_error("failed to register audio bus %s", p->metadata()->id);
                    return false;
                }
                bus     = NULL;
            }

            // Create MIDI busses
            if (midi_ins.size() > 0)
            {
                lsp_trace("Creating input event bus");

                // Allocate the audio group object
                event_bus_t *ev     = alloc_event_bus("events_in", midi_ins.size());
                if (ev == NULL)
                {
                    lsp_error("failed to create input event bus");
                    return NULL;
                }
                lsp_finally { free_event_bus(ev); };

                // Fill the audio bus data
                ev->nPorts          = midi_ins.size();

                for (size_t i=0, n=midi_ins.size(); i<n; ++i)
                {
                    plug::IPort *p      = midi_ins.uget(i);
                    if (p == NULL)
                    {
                        lsp_warn("Failed to obtain midi input port %d", int(i));
                        return false;
                    }
                    ev->vPorts[i]      = p;
                }
                pEventsIn           = release_ptr(ev);

                lsp_trace("Created input event bus id=events_in ptr=%p", ev);
            }

            if (midi_outs.size() > 0)
            {
                lsp_trace("Creating output event bus");

                // Allocate the audio group object
                event_bus_t *ev     = alloc_event_bus("events_out", midi_outs.size());
                if (ev == NULL)
                {
                    lsp_error("failed to create output event bus");
                    return NULL;
                }
                lsp_finally { free_event_bus(ev); };

                // Fill the audio bus data
                ev->nPorts          = midi_outs.size();

                for (size_t i=0, n=midi_outs.size(); i<n; ++i)
                {
                    plug::IPort *p      = midi_outs.uget(i);
                    if (p == NULL)
                    {
                        lsp_warn("Failed to obtain midi output port %d", int(i));
                        return false;
                    }
                    ev->vPorts[i]      = p;
                }
                pEventsOut          = release_ptr(ev);

                lsp_trace("Created output event bus id=events_out ptr=%p", ev);
            }

            return true;
        }

        void Wrapper::create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix)
        {
            vst3::Port *cp      = NULL;

            switch (port->role)
            {
                case meta::R_MESH:
                {
                    lsp_trace("Creating mesh port id=%s", port->id);
                    vst3::MeshPort *p       = new vst3::MeshPort(port);
                    vMeshes.add(p);
                    cp                      = p;
                    break;
                }

                case meta::R_FBUFFER:
                {
                    lsp_trace("Creating framebuffer port id=%s", port->id);
                    vst3::FrameBufferPort *p= new vst3::FrameBufferPort(port);
                    vFBuffers.add(p);
                    cp                      = p;
                    break;
                }

                case meta::R_STREAM:
                {
                    lsp_trace("Creating stream port id=%s", port->id);
                    vst3::StreamPort *p     = new vst3::StreamPort(port);
                    vStreams.add(p);
                    cp                      = p;
                    break;
                }

                case meta::R_MIDI:
                    // MIDI ports will be organized into groups after instantiation of all ports
                    lsp_trace("Creating midi port id=%s", port->id);
                    cp                      = new vst3::MidiPort(port);
                    break;

                case meta::R_AUDIO:
                    // Audio ports will be organized into groups after instantiation of all ports
                    lsp_trace("Creating audio port id=%s", port->id);
                    cp                      = new vst3::AudioPort(port);
                    break;

                case meta::R_OSC:
                    lsp_trace("Creating OSC port id=%s", port->id);
                    cp                      = new vst3::OscPort(port);
                    break;

                case meta::R_PATH:
                {
                    vst3::PathPort *p       = new vst3::PathPort(port);
                    vPathPorts.add(p);
                    vParamMapping.create(port->id, p);
                    cp                      = p;
                    break;
                }

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    lsp_trace("Creating in_param port id=%s", port->id);
                    vst3::InParamPort *p    = new vst3::InParamPort(port, postfix != NULL);
                    if (postfix == NULL)
                        vParamIn.add(p);
                    vParamMapping.create(port->id, p);
                    cp  = p;
                    break;
                }

                case meta::R_METER:
                {
                    lsp_trace("Creating out_param port id=%s", port->id);
                    vst3::OutParamPort *p   = new vst3::OutParamPort(port);
                    if (postfix == NULL)
                        vParamOut.add(p);
                    else
                        vVParamOut.add(p);
                    cp                      = p;
                    break;
                }

                case meta::R_PORT_SET:
                {
                    lsp_trace("Creating port_set port id=%s", port->id);

                    LSPString postfix_str;
                    vst3::PortGroup *pg     = new vst3::PortGroup(port, postfix != NULL);
                    vAllPorts.add(pg);
                    if (postfix == NULL)
                        vParamIn.add(pg);
                    vParamMapping.create(port->id, pg);
                    plugin_ports->add(pg);

                    for (size_t row=0; row<pg->rows(); ++row)
                    {
                        // Generate postfix
                        postfix_str.fmt_ascii("%s_%d", (postfix != NULL) ? postfix : "", int(row));
                        const char *port_post   = postfix_str.get_ascii();

                        // Clone port metadata
                        meta::port_t *cm        = meta::clone_port_metadata(port->members, port_post);
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

            if (cp != NULL)
            {
                #ifdef LSP_DEBUG
                    const char *src_id = cp->metadata()->id;
                    for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
                    {
                        plug::IPort *p = vAllPorts.uget(i);
                        if (!strcmp(src_id, p->metadata()->id))
                            lsp_error("ERROR: port %s already defined", src_id);
                    }
                #endif /* LSP_DEBUG */

                vAllPorts.add(cp);
                plugin_ports->add(cp);
            }
        }

        status_t Wrapper::create_ports(lltl::parray<plug::IPort> *plugin_ports, const meta::plugin_t *meta)
        {
            lsp_trace("Creating ports for %s - %s", meta->name, meta->description);
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(plugin_ports, port, NULL);

            lsp_trace("Sorting input and output ports");
            vParamIn.qsort(compare_in_param_ports);
            vParamOut.qsort(compare_out_param_ports);

            return STATUS_OK;
        }

        Steinberg::tresult PLUGIN_API Wrapper::initialize(Steinberg::FUnknown *context)
        {
            lsp_trace("this=%p, context=%p", this, context);

            // Check that host context is already set
            if (pHostContext != NULL)
                return Steinberg::kResultFalse;

            // Set up host context
            pHostContext        = safe_acquire(context);
            pHostApplication    = safe_query_iface<Steinberg::Vst::IHostApplication>(context);

            // Add self to the synchronization list
            status_t res = pFactory->register_data_sync(this);
            if (res != STATUS_OK)
                return Steinberg::kInternalError;

            lsp_trace("Creating executor service");
            ipc::IExecutor *executor    = pFactory->acquire_executor();
            if (executor != NULL)
            {
                // Create wrapper around native executor
                pExecutor           = new vst3::Executor(executor);
                if (pExecutor == NULL)
                {
                    pFactory->release_executor();
                    return Steinberg::kInternalError;
                }
            }

            // Obtain plugin metadata
            const meta::plugin_t *meta = (pPlugin != NULL) ? pPlugin->metadata() : NULL;
            if (meta == NULL)
                return Steinberg::kInternalError;

            // Create all possible ports for plugin and validate the state
            lltl::parray<plug::IPort> plugin_ports;
            if (create_ports(&plugin_ports, meta) != STATUS_OK)
            {
                lsp_error("Failed to create ports");
                return Steinberg::kInternalError;
            }

            // Generate audio busses
            if (!create_busses(meta))
            {
                lsp_error("Failed to create busses");
                return Steinberg::kInternalError;
            }

            // Allocate OSC packet data
            lsp_trace("Creating OSC data buffer");
            pOscPacket      = reinterpret_cast<uint8_t *>(::malloc(OSC_PACKET_MAX));
            if (pOscPacket == NULL)
                return Steinberg::kOutOfMemory;

            if (meta->extensions & meta::E_KVT_SYNC)
            {
                lsp_trace("Binding KVT listener");
                sKVT.bind(&sKVTListener);
                lsp_trace("Creating KVT dispatcher...");
                pKVTDispatcher         = new core::KVTDispatcher(&sKVT, &sKVTMutex);
            }

            // Initialize plugin
            lsp_trace("Initializing plugin");
            pPlugin->init(this, plugin_ports.array());

            // Create sample player if required
            if (meta->extensions & meta::E_FILE_PREVIEW)
            {
                pSamplePlayer       = new core::SamplePlayer(meta);
                if (pSamplePlayer == NULL)
                    return STATUS_NO_MEM;
                pSamplePlayer->init(this, plugin_ports.array(), plugin_ports.size());
            }

            lsp_trace("Successful initialization this=%p", this);

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Wrapper::terminate()
        {
            lsp_trace("this=%p", this);

            // Remove self from synchronization list
            pFactory->unregister_data_sync(this);
            if (pExecutor != NULL)
            {
                lsp_finally { pFactory->release_executor(); };
                pExecutor->shutdown();
                delete pExecutor;
                pExecutor       = NULL;
            }

            // Destroy sample player
            if (pSamplePlayer != NULL)
            {
                pSamplePlayer->destroy();
                delete pSamplePlayer;
                pSamplePlayer = NULL;
            }

            // Destroy plugin
            if (pPlugin != NULL)
            {
                delete pPlugin;
                pPlugin         = NULL;
            }

            // Delete temporary buffer for OSC serialization
            if (pOscPacket != NULL)
            {
                ::free(pOscPacket);
                pOscPacket = NULL;
            }

            // Release host context
            safe_release(pHostContext);
            safe_release(pHostApplication);

            // Release the peer connection if host didn't disconnect us previously.
            if (pPeerConnection != NULL)
                pPeerConnection->disconnect(this);
            safe_release(pPeerConnection);

            // Release busses
            for (lltl::iterator<audio_bus_t> it = vAudioIn.values(); it; ++it)
                free_audio_bus(it.get());
            for (lltl::iterator<audio_bus_t> it = vAudioOut.values(); it; ++it)
                free_audio_bus(it.get());
            free_event_bus(pEventsIn);
            free_event_bus(pEventsOut);

            // Destroy ports
            for (lltl::iterator<plug::IPort> it = vAllPorts.values(); it; ++it)
            {
                plug::IPort *port = it.get();
                if (port != NULL)
                    delete port;
            }
            vAllPorts.flush();
            vAudioIn.flush();
            vAudioOut.flush();
            vParamIn.flush();
            vParamOut.flush();
            vMeshes.flush();
            vFBuffers.flush();
            vStreams.flush();
            vPathPorts.flush();
            vParamMapping.flush();
            pEventsIn   = NULL;
            pEventsOut  = NULL;

            // Cleanup generated metadata
            for (size_t i=0, n=vGenMetadata.size(); i<n; ++i)
            {
                meta::port_t *p = vGenMetadata.uget(i);
                lsp_trace("destroy generated port metadata %p", p);
                meta::drop_port_metadata(p);
            }
            vGenMetadata.flush();

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getControllerClassId(Steinberg::TUID classId)
        {
            lsp_trace("this=%p", this);

            const meta::plugin_t *meta = pPlugin->metadata();
            if (meta->vst3ui_uid == NULL)
                return Steinberg::kResultFalse;

            Steinberg::TUID tuid;
            status_t res = parse_tuid(tuid, meta->vst3ui_uid);
            if (res != STATUS_OK)
                return Steinberg::kResultFalse;

            memcpy(classId, tuid, sizeof(tuid));
            IF_TRACE(
                char dump[36];
                lsp_trace("controller class id=%s", fmt_tuid(dump, tuid, sizeof(dump)));
            );

            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setIoMode(Steinberg::Vst::IoMode mode)
        {
            lsp_trace("this=%p, mode = %d", this, int(mode));

            return Steinberg::kNotImplemented;
        }

        Steinberg::int32 PLUGIN_API Wrapper::getBusCount(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir)
        {
            Steinberg::int32 count = 0;

            if (type == Steinberg::Vst::kAudio)
            {
                if (dir == Steinberg::Vst::kInput)
                    count = vAudioIn.size();
                else if (dir == Steinberg::Vst::kOutput)
                    count = vAudioOut.size();
            }
            else if (type == Steinberg::Vst::kEvent)
            {
                if (dir == Steinberg::Vst::kInput)
                    count = (pEventsIn != NULL) ? 1 : 0;
                else if (dir == Steinberg::Vst::kOutput)
                    count = (pEventsOut != NULL) ? 1 : 0;
            }

            lsp_trace("this=%p, type=%s, dir=%s -> count=%d",
                this,
                media_type_to_str(type),
                bus_direction_to_str(dir),
                int(count));

            return count;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getBusInfo(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::BusInfo & bus /*out*/)
        {
            lsp_trace("this=%p, type=%s, dir=%s, index=%d",
                this,
                media_type_to_str(type),
                bus_direction_to_str(dir),
                int(index));

            if (type == Steinberg::Vst::kAudio)
            {
                if (dir == Steinberg::Vst::kInput)
                {
                    if ((index < 0) || (size_t(index) >= vAudioIn.size()))
                        return Steinberg::kInvalidArgument;

                    audio_bus_t *b = vAudioIn.uget(index);
                    if (b == NULL)
                        return Steinberg::kInternalError;

                    bus.mediaType       = type;
                    bus.direction       = dir;
                    bus.channelCount    = b->nPorts;
                    bus.busType         = b->nBusType;
                    bus.flags           = Steinberg::Vst::BusInfo::kDefaultActive;
                    Steinberg::strncpy16(bus.name, b->sName, sizeof(bus.name)/sizeof(Steinberg::char16));

                    log_bus_info(&bus);

                    return Steinberg::kResultTrue;
                }
                else if (dir == Steinberg::Vst::kOutput)
                {
                    if ((index < 0) || (size_t(index) >= vAudioOut.size()))
                        return Steinberg::kInvalidArgument;

                    audio_bus_t *b = vAudioOut.uget(index);
                    if (b == NULL)
                        return Steinberg::kInternalError;

                    bus.mediaType       = type;
                    bus.direction       = dir;
                    bus.channelCount    = b->nPorts;
                    bus.busType         = b->nBusType;
                    bus.flags           = Steinberg::Vst::BusInfo::kDefaultActive;
                    Steinberg::strncpy16(bus.name, b->sName, sizeof(bus.name)/sizeof(Steinberg::char16));

                    log_bus_info(&bus);

                    return Steinberg::kResultTrue;
                }
            }
            else if (type == Steinberg::Vst::kEvent)
            {
                if (dir == Steinberg::Vst::kInput)
                {
                    if ((index != 0) || (pEventsIn == NULL))
                        return Steinberg::kInvalidArgument;

                    bus.mediaType       = type;
                    bus.direction       = dir;
                    bus.channelCount    = pEventsIn->nPorts;
                    bus.busType         = Steinberg::Vst::kMain;
                    bus.flags           = Steinberg::Vst::BusInfo::kDefaultActive;
                    Steinberg::strncpy16(bus.name, pEventsIn->sName, sizeof(bus.name)/sizeof(Steinberg::char16));

                    log_bus_info(&bus);

                    return Steinberg::kResultTrue;
                }
                else if (dir == Steinberg::Vst::kOutput)
                {
                    if ((index != 0) || (pEventsOut == NULL))
                        return Steinberg::kInvalidArgument;

                    bus.mediaType       = type;
                    bus.direction       = dir;
                    bus.channelCount    = pEventsOut->nPorts;
                    bus.busType         = Steinberg::Vst::kMain;
                    bus.flags           = Steinberg::Vst::BusInfo::kDefaultActive;
                    Steinberg::strncpy16(bus.name, pEventsOut->sName, sizeof(bus.name)/sizeof(Steinberg::char16));

                    log_bus_info(&bus);

                    return Steinberg::kResultTrue;
                }
            }

            return Steinberg::kInvalidArgument;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getRoutingInfo(Steinberg::Vst::RoutingInfo & inInfo, Steinberg::Vst::RoutingInfo & outInfo /*out*/)
        {
            lsp_trace("this=%p", this);
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::activateBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::TBool state)
        {
            lsp_trace("this=%p, type=%s, dir=%s, index=%d, state=%s",
                this,
                media_type_to_str(type),
                bus_direction_to_str(dir),
                int(index),
                (state) ? "true" : "false");

            if (index < 0)
                return Steinberg::kInvalidArgument;

            if (type == Steinberg::Vst::kAudio)
            {
                audio_bus_t *bus =
                    (dir == Steinberg::Vst::kInput) ? vAudioIn.get(index) :
                    (dir == Steinberg::Vst::kOutput) ? vAudioOut.get(index) :
                    NULL;
                if (bus == NULL)
                    return Steinberg::kInvalidArgument;

                bus->bActive    = state;
                update_port_activity(bus);

                return Steinberg::kResultTrue;
            }
            else if (type == Steinberg::Vst::kEvent)
            {
                if (index != 0)
                    return Steinberg::kInvalidArgument;;
                event_bus_t *bus =
                    (dir == Steinberg::Vst::kInput) ? pEventsIn :
                    (dir == Steinberg::Vst::kOutput) ? pEventsOut :
                    NULL;
                if (bus == NULL)
                    return Steinberg::kInvalidArgument;

                bus->bActive    = state;
                return Steinberg::kResultTrue;
            }

            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setActive(Steinberg::TBool state)
        {
            lsp_trace("this=%p, state=%d", this, int(state));

            if (pPlugin == NULL)
                return Steinberg::kNotInitialized;

            if (state != pPlugin->active())
            {
                if (state)
                    pPlugin->activate();
                else
                    pPlugin->deactivate();
            }

            return Steinberg::kResultOk;
        }

        status_t Wrapper::save_kvt_parameters_v1(Steinberg::IBStream *os, core::KVTStorage *kvt)
        {
            status_t res;
            const core::kvt_param_t *p = NULL;

            // Read the whole KVT storage
            core::KVTIterator *it = kvt->enum_all();
            while (it->next() == STATUS_OK)
            {
                // Get parameter
                res             = it->get(&p);
                if (res == STATUS_NOT_FOUND) // Not a parameter
                    continue;
                else if (res != STATUS_OK)
                {
                    lsp_warn("it->get() returned %d", int(res));
                    return res;
                }
                else if (it->is_transient()) // Skip transient parameters
                    continue;

                // Privacy flag
                uint8_t flags   = 0;
                if (it->is_private())
                    flags          |= vst3::FLAG_PRIVATE;

                const char *name = it->name();
                if (name == NULL)
                {
                    lsp_trace("it->name() returned NULL");
                    return STATUS_CORRUPTED;
                }

                kvt_dump_parameter("Saving state of KVT parameter: %s = ", p, name);

                // Serialize parameter name and flags
                size_t name_len = strlen(name);
                if ((res = write_fully(os, name, name_len)) != STATUS_OK)
                {
                    lsp_warn("Failed to save KVT parameter name for id = %s", name);
                    return res;
                }

                // Successful status?
                if ((res = write_kvt_value(os, p, flags)) != STATUS_OK)
                {
                    lsp_warn("KVT parameter serialization failed id=%s", name);
                    return res;
                }
            } // while

            return STATUS_OK;
        }

        status_t Wrapper::save_state(Steinberg::IBStream *os)
        {
            status_t res;
            const uint16_t version      = 1;

            // Set-up DSP context
            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            // Write header
            if ((res = write_fully(os, STATE_SIGNATURE, 4)) != STATUS_OK)
                return res;
            if ((res = write_fully(os, &version, sizeof(version))) != STATUS_OK)
                return res;

            // Write parameters
            for (lltl::iterator<vst3::Port> it = vParamMapping.values(); it; ++it)
            {
                vst3::Port *p = it.get();
                const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                if ((meta == NULL) || (meta->id == NULL))
                    continue;

                if ((meta::is_control_port(meta)) || (meta::is_bypass_port(meta)))
                {
                    vst3::InParamPort *pp = static_cast<vst3::InParamPort *>(p);
                    lsp_trace("Saving state of %sparameter: %s = %f",
                        pp->is_virtual() ? " virtual" : "",
                        meta->id,
                        p->value());

                    if ((res = write_value(os, meta->id, p->value())) != STATUS_OK)
                        return res;
                }
                else if (meta::is_path_port(meta))
                {
                    path_t *path = p->buffer<path_t>();
                    if (path == NULL)
                        return STATUS_CORRUPTED;

                    lsp_trace("Saving state of path parameter: %s = %s", meta->id, path->path());

                    const char *path_value = path->path();
                    if ((res = write_value(os, meta->id, path_value)) != STATUS_OK)
                        return res;
                }
                else
                    return STATUS_CORRUPTED;
            }

            // Save state of all KVT parameters
            if (sKVTMutex.lock())
            {
                lsp_finally { sKVTMutex.unlock(); };
                res = save_kvt_parameters_v1(os, &sKVT);
                sKVT.gc();
            }

            if (res == STATUS_OK)
                pPlugin->state_saved();

            return res;
        }

        status_t Wrapper::load_state(Steinberg::IBStream *is)
        {
            status_t res;
            char signature[4];
            uint16_t version = 0;

            // Set-up DSP context
            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            // Read and validate signature
            if ((res = read_fully(is, &signature[0], 4)) != STATUS_OK)
            {
                lsp_warn("Can not read state signature");
                return STATUS_CORRUPTED;
            }
            if (memcmp(signature, STATE_SIGNATURE, 4) != 0)
            {
                lsp_warn("Invalid state signature");
                return STATUS_CORRUPTED;
            }

            // Read and validate version
            if ((res = read_fully(is, &version)) != STATUS_OK)
            {
                lsp_warn("Failed to read serial version");
                return STATUS_CORRUPTED;
            }
            if (version != 1)
            {
                lsp_warn("Unsupported serial version %d", int(version));
                return STATUS_CORRUPTED;
            }

            // Lock the KVT
            if (!sKVTMutex.lock())
            {
                lsp_warn("Failed to lock KVT");
                return STATUS_UNKNOWN_ERR;
            }
            lsp_finally {
                sKVT.gc();
                sKVTMutex.unlock();
            };
            sKVT.clear();

            // Read the state
            lsp_debug("Reading state...");
            char *name = NULL;
            size_t name_cap = 0;
            lsp_finally {
                if (name != NULL)
                    free(name);
            };

            // Read and parse the record
            while ((res = read_string(is, &name, &name_cap)) == STATUS_OK)
            {
                core::kvt_param_t p;
                p.type  = core::KVT_ANY;
                lsp_finally {
                    destroy_kvt_value(&p);
                };

                lsp_trace("Parameter name: %s", name);

                if (name[0] != '/')
                {
                    // Try to find virtual port
                    vst3::Port *p           = vParamMapping.get(name);
                    if (p != NULL)
                    {
                        const meta::port_t *meta = p->metadata();
                        if ((meta::is_control_port(meta)) || (meta::is_bypass_port(meta)))
                        {
                            vst3::InParamPort *pp = static_cast<vst3::InParamPort *>(p);
                            float v = 0.0f;
                            if ((res = read_fully(is, &v)) != STATUS_OK)
                            {
                                lsp_warn("Failed to deserialize port id=%s", name);
                                return res;
                            }
                            lsp_trace("  %s = %f", meta->id, v);
                            pp->submit(v);
                        }
                        else if (meta::is_path_port(meta))
                        {
                            path_t *xp  = p->buffer<path_t>();

                            if ((res = read_string(is, &name, &name_cap)) != STATUS_OK)
                            {
                                lsp_warn("Failed to deserialize port id=%s", meta->id);
                                return res;
                            }
                            lsp_trace("  %s = %s", meta->id, name);
                            xp->submit(name, strlen(name), plug::PF_STATE_RESTORE);
                        }
                    }
                    else
                        lsp_warn("Missing port id=%s, skipping", name);
                }
                else
                {
                    // Read the KVT parameter flags
                    uint8_t flags = 0;
                    if ((res = read_fully(is, &flags)) != STATUS_OK)
                    {
                        lsp_warn("Failed to resolve flags for parameter id=%s", name);
                        return res;
                    }

                    lsp_trace("Parameter flags: 0x%x", int(flags));
                    if ((res = read_kvt_value(is, name, &p)) != STATUS_OK)
                    {
                        lsp_warn("Failed to read value for KVT parameter id=%s, code=%d", name, int(res));
                        return res;
                    }

                    // This is KVT port
                    if (p.type != core::KVT_ANY)
                    {
                        size_t kflags = core::KVT_TX;
                        if (flags & vst3::FLAG_PRIVATE)
                            kflags     |= core::KVT_PRIVATE;

                        kvt_dump_parameter("Fetched KVT parameter %s = ", &p, name);
                        sKVT.put(name, &p, kflags);
                    }
                }
            }

            // Analyze result
            res = (res == STATUS_EOF) ? STATUS_OK : STATUS_CORRUPTED;
            if (res == STATUS_OK)
            {
                bUpdateSettings = true;
                pPlugin->state_loaded();
            }

            return STATUS_OK;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setState(Steinberg::IBStream *state)
        {
            lsp_trace("this=%p, state=%p", this, state);
            IF_TRACE(
                DbgInStream is(state);
                state = &is;
                lsp_dumpb("State dump:", is.data(), is.size());
            );

            status_t res = load_state(state);
            return (res == STATUS_OK) ? Steinberg::kResultOk : Steinberg::kInternalError;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getState(Steinberg::IBStream *state)
        {
            lsp_trace("this=%p, state=%p", this, state);

            IF_TRACE(
                DbgOutStream os(state);
                state = &os;
            );

            status_t res = save_state(state);

            IF_TRACE(
                lsp_dumpb("State dump:", os.data(), os.size());
            );

            return (res == STATUS_OK) ? Steinberg::kResultOk : Steinberg::kInternalError;
        }

        Steinberg::tresult PLUGIN_API Wrapper::connect(Steinberg::Vst::IConnectionPoint *other)
        {
            lsp_trace("this=%p, other=%p", this, other);

            // Check if peer connection is valid and was not previously estimated
            if (other == NULL)
                return Steinberg::kInvalidArgument;
            if (pPeerConnection)
                return Steinberg::kResultFalse;

            // Save the peer connection
            pPeerConnection = safe_acquire(other);
            if (pKVTDispatcher != NULL)
                pKVTDispatcher->connect_client();

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Wrapper::disconnect(Steinberg::Vst::IConnectionPoint *other)
        {
            lsp_trace("this=%p, other=%p", this, other);

            // Check that estimated peer connection matches the esimated one
            if (other == NULL)
                return Steinberg::kInvalidArgument;
            if (pPeerConnection != other)
                return Steinberg::kResultFalse;

            // Reset the peer connection
            safe_release(pPeerConnection);
            if (pKVTDispatcher != NULL)
                pKVTDispatcher->disconnect_client();

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setBusArrangements(Steinberg::Vst::SpeakerArrangement *inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts)
        {
            IF_TRACE(
                lsp_trace("this=%p, inputs=%p, numIns=%d, outputs=%p, numOuts=%d", this, inputs, int(numIns), outputs, int(numOuts));
            )

            if (numIns < 0 || numOuts < 0)
                return Steinberg::kInvalidArgument;

            if ((size_t(numIns) > vAudioIn.size()) ||
                (size_t(numOuts) > vAudioOut.size()))
                return Steinberg::kResultFalse;

            // Step 1. Check that we allow several ports to be disabled
            for (ssize_t i=0; i < numIns; ++i)
            {
                const audio_bus_t *bus = vAudioIn.get(i);
                if (bus == NULL)
                    return Steinberg::kInvalidArgument;

                IF_TRACE(
                    LSPString tmp;
                    lsp_trace("  in_bus[%d] min   = %s", int(i), speaker_arrangement_to_str(&tmp, bus->nMinArr));
                    lsp_trace("  in_bus[%d] max   = %s", int(i), speaker_arrangement_to_str(&tmp, bus->nFullArr));
                    lsp_trace("  in_bus[%d] curr  = %s", int(i), speaker_arrangement_to_str(&tmp, bus->nCurrArr));
                    lsp_trace("  in_proposed[%d]  = %s", int(i), speaker_arrangement_to_str(&tmp, inputs[i]));
                );

                const Steinberg::Vst::SpeakerArrangement arr = inputs[i];
                if ((arr & (~bus->nFullArr)) != 0)
                    return Steinberg::kInvalidArgument;
                if ((bus->nMinArr & arr) != bus->nMinArr)
                    return Steinberg::kResultFalse;
            }

            for (ssize_t i=0; i < numOuts; ++i)
            {
                const audio_bus_t *bus = vAudioOut.get(i);
                if (bus == NULL)
                    return Steinberg::kInvalidArgument;

                IF_TRACE(
                    LSPString tmp;
                    lsp_trace("  out_bus[%d] min  = %s", int(i), speaker_arrangement_to_str(&tmp, bus->nMinArr));
                    lsp_trace("  out_bus[%d] max  = %s", int(i), speaker_arrangement_to_str(&tmp, bus->nFullArr));
                    lsp_trace("  out_bus[%d] curr = %s", int(i), speaker_arrangement_to_str(&tmp, bus->nCurrArr));
                    lsp_trace("  out_proposed[%d] = %s", int(i), speaker_arrangement_to_str(&tmp, inputs[i]));
                );

                const Steinberg::Vst::SpeakerArrangement arr = outputs[i];
                if ((arr & (~bus->nFullArr)) != 0)
                    return Steinberg::kInvalidArgument;
                if ((bus->nMinArr & arr) != bus->nMinArr)
                    return Steinberg::kResultFalse;
            }

            // Step 2. Apply new configuration
            lsp_trace("Bus configuration matched");
            for (ssize_t i=0; i < numIns; ++i)
            {
                audio_bus_t *bus    = vAudioIn.get(i);
                bus->nCurrArr       = inputs[i];

                IF_TRACE(
                    LSPString tmp;
                    lsp_trace("  in_bus[%d] new   = %s", int(i), speaker_arrangement_to_str(&tmp, bus->nCurrArr));
                );

                update_port_activity(bus);
            }

            for (ssize_t i=0; i < numOuts; ++i)
            {
                audio_bus_t *bus    = vAudioOut.get(i);
                bus->nCurrArr       = outputs[i];

                IF_TRACE(
                    LSPString tmp;
                    lsp_trace("  out_bus[%d] new  = %s", int(i), speaker_arrangement_to_str(&tmp, bus->nCurrArr));
                );

                update_port_activity(bus);
            }

            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getBusArrangement(Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::SpeakerArrangement & arr)
        {
            lsp_trace("this=%p, dir=%d, index=%d", this, int(dir), int(index));

            if (index < 0)
                return Steinberg::kInvalidArgument;
            audio_bus_t *bus    = (dir == Steinberg::Vst::kInput) ? vAudioIn.get(index) :
                                  (dir == Steinberg::Vst::kOutput) ? vAudioOut.get(index) :
                                  NULL;
            if (bus == NULL)
                return Steinberg::kInvalidArgument;

            arr                 = bus->nCurrArr;
            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API Wrapper::canProcessSampleSize(Steinberg::int32 symbolicSampleSize)
        {
            lsp_trace("this=%p, symbolicSampleSize=%d", this, int(symbolicSampleSize));

            // We support only 32-bit float samples
            return (symbolicSampleSize == Steinberg::Vst::kSample32) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::getLatencySamples()
        {
            lsp_trace("this=%p, latency=%d", this, int(pPlugin->latency()));

            return pPlugin->latency();
        }

        Steinberg::tresult PLUGIN_API Wrapper::setupProcessing(Steinberg::Vst::ProcessSetup & setup)
        {
            lsp_trace("this=%p", this);

            // Check that processing mode is valid
            switch (setup.processMode)
            {
                case Steinberg::Vst::kRealtime:
                case Steinberg::Vst::kPrefetch:
                case Steinberg::Vst::kOffline:
                    break;
                default:
                    return Steinberg::kInvalidArgument;
            }

            // We do not accept any sample format except 32-bit float
            if (setup.symbolicSampleSize != Steinberg::Vst::kSample32)
                return Steinberg::kInvalidArgument;

            // Save new sample rate
            size_t sample_rate = setup.sampleRate;
            if (sample_rate > MAX_SAMPLE_RATE)
            {
                lsp_warn(
                    "Unsupported sample rate: %f, maximum supported sample rate is %ld",
                    sample_rate,
                    long(MAX_SAMPLE_RATE));
                sample_rate  = MAX_SAMPLE_RATE;
            }
            pPlugin->set_sample_rate(sample_rate);
            if (pSamplePlayer != NULL)
                pSamplePlayer->set_sample_rate(sample_rate);

            // Adjust block size for input and output audio ports
            nMaxSamplesPerBlock     = setup.maxSamplesPerBlock;
            for (lltl::iterator<audio_bus_t> it = vAudioIn.values(); it; ++it)
            {
                audio_bus_t *bus = it.get();
                if (bus == NULL)
                    continue;
                for (size_t i=0; i<bus->nPorts; ++i)
                    bus->vPorts[i]->setup(setup.maxSamplesPerBlock);
            }

            for (lltl::iterator<audio_bus_t> it = vAudioOut.values(); it; ++it)
            {
                audio_bus_t *bus = it.get();
                if (bus == NULL)
                    continue;
                for (size_t i=0; i<bus->nPorts; ++i)
                    bus->vPorts[i]->setup(setup.maxSamplesPerBlock);
            }

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setProcessing(Steinberg::TBool state)
        {
            lsp_trace("this=%p, state=%d", this, int(state));
            // This is almost useless
            return Steinberg::kNotImplemented;
        }

        void Wrapper::sync_position(Steinberg::Vst::ProcessContext *pctx)
        {
            plug::position_t *pos       = &sPosition;

            pos->sampleRate             = pPlugin->sample_rate();
            pos->speed                  = 1.0f;
            pos->frame                  = 0;
            if ((pctx != NULL) && (pctx->state & Steinberg::Vst::ProcessContext::kTimeSigValid))
            {
                pos->numerator              = pctx->timeSigNumerator;
                pos->denominator            = pctx->timeSigDenominator;
            }
            else
            {
                pos->numerator              = 4.0;
                pos->denominator            = 4.0;
            }
            if (pctx->state & Steinberg::Vst::ProcessContext::kTempoValid)
                pos->beatsPerMinute         = pctx->tempo;
            else
                pos->beatsPerMinute         = BPM_DEFAULT;
            pos->beatsPerMinuteChange   = 0.0f;
            pos->ticksPerBeat           = DEFAULT_TICKS_PER_BEAT;

            if ((pctx->state & Steinberg::Vst::ProcessContext::kProjectTimeMusicValid) &&
                (pctx->state & Steinberg::Vst::ProcessContext::kBarPositionValid))
            {
                double uppqPos              = (pctx->projectTimeMusic - pctx->barPositionMusic) * pctx->timeSigDenominator * 0.25;
                pos->tick                   = pos->ticksPerBeat * (uppqPos - int64_t(uppqPos));
            }
            else
                pos->tick                   = 0.0;

            // Sync position with UI
            if (atomic_trylock(nPositionLock))
            {
                sUIPosition                 = sPosition;
                atomic_unlock(nPositionLock);
            }
        }

        vst3::InParamPort *Wrapper::input_parameter(Steinberg::Vst::ParamID id)
        {
            // Perform binary search agains list of ports sorted in ascending order of VST parameter identifier
            ssize_t first = 0, last = vParamIn.size() - 1;
            while (first <= last)
            {
                const ssize_t middle = (first + last) >> 1;
                vst3::InParamPort *port = vParamIn.uget(middle);
                const Steinberg::Vst::ParamID port_id = port->parameter_id();

                if (id < port_id)
                    last    = middle - 1;
                else if (id > port_id)
                    first   = middle + 1;
                else
                    return port;
            }
            return NULL;
        }

        size_t Wrapper::prepare_block(int32_t frame, Steinberg::Vst::ProcessData *data)
        {
            // Obtain number of parameters
            Steinberg::Vst::IParameterChanges *changes = data->inputParameterChanges;
            size_t num_params = (changes != NULL) ? changes->getParameterCount() : 0;
            if (num_params <= 0)
                return data->numSamples - frame;

            int32_t first_change = data->numSamples;
            Steinberg::int32 sampleOffset;
            Steinberg::Vst::ParamValue value;

            // Pass 1: find the most recent change in change queues
            for (size_t i=0; i<num_params; ++i)
            {
                Steinberg::Vst::IParamValueQueue *queue = changes->getParameterData(i);
                vst3::InParamPort *port = input_parameter(queue->getParameterId());
                if (port == NULL)
                    continue;

                // Lookup for the first change
                for (ssize_t index = port->change_index(), changes = queue->getPointCount(); index < changes; )
                {
                    if (queue->getPoint(index, sampleOffset, value) != Steinberg::kResultOk)
                        break;

                    if (sampleOffset < frame)
                        port->set_change_index(++index);
                    else
                    {
                        first_change = lsp_min(first_change, sampleOffset);
                        break;
                    }
                }
            }

            // Pass 2: adjust port values accoding to the pending changes
            for (size_t i=0; i<num_params; ++i)
            {
                Steinberg::Vst::IParamValueQueue *queue = changes->getParameterData(i);
                vst3::InParamPort *port = input_parameter(queue->getParameterId());
                if (port == NULL)
                    continue;

                // Obtain the change point
                const ssize_t index = port->change_index();
                if (index >= queue->getPointCount())
                    continue;
                if (queue->getPoint(index, sampleOffset, value) != Steinberg::kResultOk)
                    continue;

                // The value has changed?
                if (sampleOffset <= first_change)
                {
                    port->set_change_index(index + 1);  // We already can move the change index forward
                    if (port->commit_value(value))
                        bUpdateSettings     = true;
                }
            }

            return first_change - frame;
        }

        void Wrapper::bind_bus_buffers(lltl::parray<audio_bus_t> *busses, Steinberg::Vst::AudioBusBuffers *buffers, size_t num_buffers, size_t num_samples)
        {
            for (size_t i=0, n=busses->size(); i<n; ++i)
            {
                audio_bus_t *bus = busses->uget(i);
                if (i < num_buffers)
                {
                    Steinberg::Vst::Sample32 **sbuffers = buffers[i].channelBuffers32;
                    for (size_t j=0; j<bus->nPorts; ++j)
                    {
                        vst3::AudioPort *p = bus->vPorts[j];
                        if (bus->nCurrArr & p->speaker())
                            p->bind(*(sbuffers++), num_samples);
                        else
                            p->bind(NULL, num_samples);
                    }
                }
                else
                {
                    for (size_t j=0; j<bus->nPorts; ++j)
                    {
                        vst3::AudioPort *p = bus->vPorts[j];
                        p->bind(NULL, num_samples);
                    }
                }
            }
        }

        void Wrapper::advance_bus_buffers(lltl::parray<audio_bus_t> *busses, size_t samples)
        {
            for (size_t i=0, n=busses->size(); i<n; ++i)
            {
                audio_bus_t *bus = busses->uget(i);
                for (size_t j=0; j<bus->nPorts; ++j)
                    bus->vPorts[j]->advance(samples);
            }
        }

        void Wrapper::transmit_output_parameters(Steinberg::Vst::IParameterChanges *changes)
        {
            Steinberg::int32 index = 0;

            for (size_t i=0, n=vParamOut.size(); i<n; ++i)
            {
                vst3::OutParamPort *p = vParamOut.uget(i);
                if ((p == NULL) || (!p->changed()))
                    continue;

                Steinberg::Vst::IParamValueQueue *queue = changes->addParameterData(p->parameter_id(), index);
                if ((queue != NULL) && (queue->addPoint(0, p->value(), index) == Steinberg::kResultOk))
                    p->commit();
            }
        }

        void Wrapper::toggle_ui_state()
        {
            uatomic_t counter = nUICounterReq;
            if (counter == nUICounterResp)
                return;

            if (counter <= 0)
            {
                if (pPlugin->ui_active())
                    pPlugin->deactivate_ui();
                return;
            }

            // Notify UI
            if (!pPlugin->ui_active())
                pPlugin->activate_ui();

            // Force virtual meters to sync
            for (lltl::iterator<vst3::OutParamPort> it = vVParamOut.values(); it; ++it)
                it->make_changed();

            // Force meshes to sync with UI
            for (lltl::iterator<plug::IPort> it=vMeshes.values(); it; ++it)
            {
                plug::mesh_t *m = it->buffer<plug::mesh_t>();
                if (m == NULL)
                    continue;
                m->cleanup();
            }

            // Force frame buffers to sync with UI
            for (lltl::iterator<plug::IPort> it=vFBuffers.values(); it; ++it)
            {
                // Get the frame buffer data
                vst3::FrameBufferPort *fb_port = static_cast<vst3::FrameBufferPort *>(it.get());
                if (fb_port == NULL)
                    continue;
                plug::frame_buffer_t *fb = it->buffer<plug::frame_buffer_t>();
                if (fb == NULL)
                    continue;

                fb_port->set_row_id(fb->next_rowid() - fb->rows());
            }

            // Force streams to sync with UI
            for (lltl::iterator<plug::IPort> it=vStreams.values(); it; ++it)
            {
                // Get the frame buffer data
                vst3::StreamPort *s_port = static_cast<vst3::StreamPort *>(it.get());
                if (s_port == NULL)
                    continue;
                plug::stream_t *s = it->buffer<plug::stream_t>();
                if (s == NULL)
                    continue;

                s_port->set_frame_id(s->frame_id() - s->frames());
            }
        }

        Steinberg::tresult PLUGIN_API Wrapper::process(Steinberg::Vst::ProcessData & data)
        {
            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

//            lsp_trace("this=%p processMode=%d, symbolicSampleSize=%d, numSamples=%d, numInputs=%d, numOutputs=%d",
//                this,
//                int(data.processMode),
//                int(data.symbolicSampleSize),
//                int(data.numSamples),
//                int(data.numInputs),
//                int(data.numOutputs));

            // We do not support any samples except 32-bit floating-point values
            if (data.symbolicSampleSize != Steinberg::Vst::kSample32)
                return Steinberg::kInternalError;

            // Update UI activity state
            toggle_ui_state();

            // Bind audio buffers
            bind_bus_buffers(&vAudioIn, data.inputs, data.numInputs, data.numSamples);
            bind_bus_buffers(&vAudioOut, data.outputs, data.numOutputs, data.numSamples);

            // Reset change indices for parameters
            for (size_t i=0, n=vParamIn.size(); i<n; ++i)
            {
                vst3::InParamPort *p = vParamIn.uget(i);
                if (p != NULL)
                    p->set_change_index(0);
                if (p->check_pending())
                    bUpdateSettings     = true;
            }

            // Reset output port state
            for (size_t i=0, n=vParamOut.size(); i<n; ++i)
            {
                vst3::OutParamPort *p = vParamOut.uget(i);
                if (p != NULL)
                    p->set_empty();
            }

            sync_position(data.processContext);

            for (int32_t frame=0; frame < data.numSamples; )
            {
                // Cleanup stat of input and output MIDI ports
                if (pEventsIn != NULL)
                {
//                    TODO
//                    for (size_t i=0; i<pEventsIn->nPorts; ++i)
//                        pEventsIn->vPorts[i]->clear();
                }
                if (pEventsOut != NULL)
                {
//                    TODO
//                    for (size_t i=0; i<pEventsOut->nPorts; ++i)
//                        pEventsOut->vPorts[i]->clear();
                }

                // Prepare event block
                size_t block_size = prepare_block(frame, &data);
//                lsp_trace("block size=%d", int(block_size));
                for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
                    vAllPorts.uget(i)->pre_process(block_size);

                // Update the settings for the plugin
                if (bUpdateSettings)
                {
                    lsp_trace("Updating settings");
                    pPlugin->update_settings();
                    bUpdateSettings     = false;
                }

                // Call the plugin for processing
                if (block_size > 0)
                {
                    sPosition.frame     = frame;
                    pPlugin->set_position(&sPosition);
                    pPlugin->process(block_size);

                    // Call the sampler for processing
                    if (pSamplePlayer != NULL)
                        pSamplePlayer->process(block_size);

                    // Do the post-processing stuff
                    // TODO: generate_output_events(frame, process);

                    // Advance audio ports and update processing offset
                    advance_bus_buffers(&vAudioIn, block_size);
                    advance_bus_buffers(&vAudioOut, block_size);
                    frame      += block_size;
                }
            }

            // Transmit all outgoing information
            transmit_output_parameters(data.outputParameterChanges);

            // Dump state if requested
            const uatomic_t dump_req    = nDumpReq;
            if (dump_req != nDumpResp)
            {
                dump_plugin_state();
                nDumpResp               = dump_req;
            }

            return Steinberg::kResultOk;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::getTailSamples()
        {
            lsp_trace("this=%p", this);

            if (pPlugin == NULL)
                return Steinberg::kInternalError;

            ssize_t tail_size = pPlugin->tail_size();
            if (tail_size < 0)
                return Steinberg::Vst::kInfiniteTail;

            return (tail_size > 0) ? tail_size : Steinberg::Vst::kInfiniteTail;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::getProcessContextRequirements()
        {
            lsp_trace("this=%p", this);

            return
                Steinberg::Vst::IProcessContextRequirements::kNeedProjectTimeMusic |
                Steinberg::Vst::IProcessContextRequirements::kNeedBarPositionMusic |
                Steinberg::Vst::IProcessContextRequirements::kNeedTempo |
                Steinberg::Vst::IProcessContextRequirements::kNeedTimeSignature;
        }

        ipc::IExecutor *Wrapper::executor()
        {
            return pExecutor;
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

        void Wrapper::state_changed()
        {
            atomic_add(&nDirtyReq, 1);
        }

        void Wrapper::request_settings_update()
        {
            bUpdateSettings     = true;
        }

        const meta::package_t *Wrapper::package() const
        {
            return pPackage;
        }

        Steinberg::tresult PLUGIN_API Wrapper::notify(Steinberg::Vst::IMessage *message)
        {
            lsp_trace("this=%p, message=%p", this, message);

            // Obtain the message data
            if (message == NULL)
                return Steinberg::kInvalidArgument;
            const char *message_id = reinterpret_cast<const char *>(message->getMessageID());
            if (message_id == NULL)
                return Steinberg::kInvalidArgument;
            Steinberg::Vst::IAttributeList *atts = message->getAttributes();
            if (atts == NULL)
                return Steinberg::kInvalidArgument;

            // Analyze the message
            Steinberg::tresult res;
            Steinberg::int64 byte_order = VST3_BYTEORDER;

            lsp_trace("Received message id=%s", message_id);

            if (!strcmp(message_id, ID_MSG_PATH))
            {
                // Get endianess
                if ((res = atts->getInt("endian", byte_order)) != Steinberg::kResultOk)
                {
                    lsp_warn("Failed to read property 'endian'");
                    return Steinberg::kResultFalse;
                }

                // Get port identifier
                const char *id = sNotifyBuf.get_string(atts, "id", byte_order);
                if (id == NULL)
                    return Steinberg::kResultFalse;

                // Find path port
                vst3::Port *p = vParamMapping.get(id);
                if ((p == NULL) || (!meta::is_path_port(p->metadata())))
                {
                    lsp_warn("Invalid path port specified: %s", id);
                    return Steinberg::kResultFalse;
                }

                // Get the path flags
                Steinberg::int64 flags = 0;
                if ((res = atts->getInt("flags", flags)) != Steinberg::kResultOk)
                {
                    lsp_warn("Failed to read property 'flags'");
                    return Steinberg::kResultFalse;
                }

                // Get the path data
                const char *in_path = sNotifyBuf.get_string(atts, "value", byte_order);
                if (in_path == NULL)
                    return Steinberg::kResultFalse;

                // Submit path data
                vst3::path_t *path = static_cast<vst3::path_t *>(p->buffer<plug::path_t>());
                if (path != NULL)
                {
                    lsp_trace("path %s = %s", p->metadata()->id, in_path);
                    path->submit_async(in_path, flags);
                }
            }
            else if (!strcmp(message_id, vst3::ID_MSG_VIRTUAL_PARAMETER))
            {
                // Get port identifier
                const char *id = sNotifyBuf.get_string(atts, "id", byte_order);
                if (id == NULL)
                    return Steinberg::kResultFalse;

                // Obtain the destination port
                vst3::Port *p = vParamMapping.get(id);
                if ((p == NULL) || (!meta::is_control_port(p->metadata())) || (!meta::is_bypass_port(p->metadata())))
                {
                    lsp_warn("Invalid virtual parameter port specified: %s", id);
                    return Steinberg::kResultFalse;
                }
                vst3::InParamPort *pp = static_cast<vst3::InParamPort *>(p);
                if (!pp->is_virtual())
                {
                    lsp_warn("Not a virtual parameter: %s", id);
                    return Steinberg::kResultFalse;
                }

                // Read value
                double value = 0.0f;
                if ((res = atts->getFloat("value", value)) != Steinberg::kResultOk)
                {
                    lsp_warn("Failed to read property 'value'");
                    return Steinberg::kResultFalse;
                }

                // Submit new port value
                lsp_trace("virtual parameter %s = %f", pp->metadata()->id, value);
                pp->submit(value);
            }
            else if (!strcmp(message_id, vst3::ID_MSG_ACTIVATE_UI))
            {
                atomic_add(&nUICounterReq, 1);
            }
            else if (!strcmp(message_id, vst3::ID_MSG_DEACTIVATE_UI))
            {
                atomic_add(&nUICounterReq, -1);
            }
            else if (!strcmp(message_id, vst3::ID_MSG_DUMP_STATE))
            {
                atomic_add(&nDumpReq, 1);
            }
            else if (!strcmp(message_id, vst3::ID_MSG_PLAY_SAMPLE))
            {
                // Get endianess
                if ((res = atts->getInt("endian", byte_order)) != Steinberg::kResultOk)
                {
                    lsp_warn("Failed to read property 'endian'");
                    return Steinberg::kResultFalse;
                }

                // Get file name
                const char *file = sNotifyBuf.get_string(atts, "file", byte_order);
                if (file == NULL)
                    return Steinberg::kResultFalse;

                // Get play position
                int64_t position = 0;
                if ((res = atts->getInt("position", position)) != Steinberg::kResultOk)
                {
                    lsp_warn("Failed to read property 'position'");
                    return Steinberg::kResultFalse;
                }

                // Get release flag
                double release = 0.0f;
                if ((res = atts->getFloat("release", release)) != Steinberg::kResultOk)
                {
                    lsp_warn("Failed to read property 'release'");
                    return Steinberg::kResultFalse;
                }

                // Request sample player for playback
                if (pSamplePlayer != NULL)
                {
                    lsp_trace("play_sample file=%s, position=%ld, release=%s",
                        file, long(position), (release > 0.5f) ? "true" : "false");
                    pSamplePlayer->play_sample(file, position, release > 0.5f);
                }
            }

            return Steinberg::kResultOk;
        }

        void Wrapper::sync_data()
        {
            // We have nothing to do if we can not allocate messages nor notify peer
            if ((pHostApplication == NULL) || (pPeerConnection == NULL))
                return;

            // Set-up DSP context
            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            Steinberg::char8 key[32];

            // Report latency
            size_t latency  = pPlugin->latency();
            if (latency != nLatency)
            {
                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg == NULL)
                    return;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_LATENCY);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                // Write the actual value
                if (list->setInt("value", latency) != Steinberg::kResultOk)
                    return;

                // Send the message and commit state
                if (pPeerConnection->notify(msg) == Steinberg::kResultOk)
                    nLatency = latency;
            }

            // Report state change
            uatomic_t dirty_req     = nDirtyReq;
            if (dirty_req != nDirtyResp)
            {
                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg == NULL)
                    return;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_STATE_DIRTY);

                // Send the message and commit state
                if (pPeerConnection->notify(msg) == Steinberg::kResultOk)
                    nDirtyResp = dirty_req;
            }

            // Send position information
            if (atomic_trylock(nPositionLock))
            {
                plug::position_t pos        = sUIPosition;
                atomic_unlock(nPositionLock);

                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg == NULL)
                    return;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_MUSIC_POSITION);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                if (list->setFloat("sample_rate", pos.sampleRate) != Steinberg::kResultOk)
                    return;
                if (list->setFloat("speed", pos.speed) != Steinberg::kResultOk)
                    return;
                if (list->setInt("frame", pos.frame) != Steinberg::kResultOk)
                    return;
                if (list->setFloat("numerator", pos.numerator) != Steinberg::kResultOk)
                    return;
                if (list->setFloat("denominator", pos.denominator) != Steinberg::kResultOk)
                    return;
                if (list->setFloat("bpm", pos.beatsPerMinute) != Steinberg::kResultOk)
                    return;
                if (list->setFloat("bpm_change", pos.beatsPerMinuteChange) != Steinberg::kResultOk)
                    return;
                if (list->setFloat("tick", pos.tick) != Steinberg::kResultOk)
                    return;
                if (list->setFloat("ticks_per_beat", pos.ticksPerBeat) != Steinberg::kResultOk)
                    return;

                // Send the message and commit state
                pPeerConnection->notify(msg);
            }

            // Transmit KVT state
            if (pKVTDispatcher != NULL)
            {
                size_t size = 0;
                bool encoded = true;

                do
                {
                    pKVTDispatcher->iterate();
                    status_t res = pKVTDispatcher->fetch(pOscPacket, &size, OSC_PACKET_MAX);

                    switch (res)
                    {
                        case STATUS_OK:
                        {
                            lsp_trace("Transmitting KVT-related OSC packet of %d bytes", int(size));
                            osc::dump_packet(pOscPacket, size);

                            // Allocate new message
                            Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                            if (msg == NULL)
                                continue;
                            lsp_finally { safe_release(msg); };

                            // Initialize the message
                            msg->setMessageID(vst3::ID_MSG_KVT);
                            Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                            encoded = list->setBinary("data", pOscPacket, size) == Steinberg::kResultOk;
                            break;
                        }

                        case STATUS_OVERFLOW:
                            lsp_warn("Received too big OSC packet, skipping");
                            pKVTDispatcher->skip();
                            break;

                        case STATUS_NO_DATA:
                            encoded = false;
                            break;

                        default:
                            lsp_warn("Received error while deserializing KVT changes: %d", int(res));
                            encoded = false;
                            break;
                    }
                } while (encoded);
            }

            // Do not synchronize other data until there is no UI visible
            if (nUICounterResp <= 0)
                return;

            // Synchronize virtual meters
            for (lltl::iterator<vst3::OutParamPort> it = vVParamOut.values(); it; ++it)
            {
                vst3::OutParamPort *pp = it.get();
                if (!pp->changed())
                    continue;
                const meta::port_t *meta = pp->metadata();
                if (meta == NULL)
                    continue;

                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg == NULL)
                    return;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_VIRTUAL_METER);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                // Write endianess
                if (list->setInt("endian", VST3_BYTEORDER) != Steinberg::kResultOk)
                    continue;
                // Write port identifier
                if (!sNotifyBuf.set_string(list, "id", meta->id))
                    return;
                // Write the actual value
                if (list->setFloat("value", pp->value()) != Steinberg::kResultOk)
                    return;

                // Finally, we're ready to send message
                if (pPeerConnection->notify(msg) == Steinberg::kResultOk)
                    pp->commit();
            }

            // Synchronize meshes
            for (lltl::iterator<plug::IPort> it = vMeshes.values(); it; ++it)
            {
                // Check that we have data in mesh
                vst3::MeshPort *m_port = static_cast<vst3::MeshPort *>(it.get());
                if (m_port == NULL)
                    continue;
                plug::mesh_t *mesh = it->buffer<plug::mesh_t>();
                if ((mesh == NULL) || (!mesh->containsData()))
                    continue;

                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg == NULL)
                    continue;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_MESH);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                // Write endianess
                if (list->setInt("endian", VST3_BYTEORDER) != Steinberg::kResultOk)
                    continue;
                // Write identifier of the mesh port
                if (!sSyncBuf.set_string(list, "id", m_port->metadata()->id))
                    continue;
                // Write number of buffers
                if (list->setInt("buffers", mesh->nBuffers) != Steinberg::kResultOk)
                    continue;
                // Write number of elements per buffer
                if (list->setInt("items", mesh->nItems) != Steinberg::kResultOk)
                    continue;

                // Encode data for each buffer
                bool encoded = true;
                for (size_t i=0; i<mesh->nBuffers; ++i)
                {
                    snprintf(key, sizeof(key), "data[%d]", int(i));
                    if (list->setBinary(key, mesh->pvData[i], mesh->nItems * sizeof(float)) != Steinberg::kResultOk)
                    {
                        encoded     = false;
                        break;
                    }
                }
                if (!encoded)
                    continue;

                // Finally, we're ready to send message
                if (pPeerConnection->notify(msg) == Steinberg::kResultOk)
                    mesh->cleanup();
            }

            // Synchronize frame buffers
            for (lltl::iterator<plug::IPort> it=vFBuffers.values(); it; ++it)
            {
                // Get the frame buffer data
                vst3::FrameBufferPort *fb_port = static_cast<vst3::FrameBufferPort *>(it.get());
                if (fb_port == NULL)
                    continue;
                plug::frame_buffer_t *fb = it->buffer<plug::frame_buffer_t>();
                if (fb == NULL)
                    continue;

                // Serialize not more than 4 rows
                size_t delta = fb->next_rowid() - fb_port->row_id();
                uint32_t first_row = (delta > fb->rows()) ? fb->next_rowid() - fb->rows() : fb_port->row_id();
                if (delta > FRAMEBUFFER_BULK_MAX)
                    delta = FRAMEBUFFER_BULK_MAX;
                uint32_t last_row = first_row + delta;

                lsp_trace("id = %s, first=%d, last=%d", fb_port->metadata()->id, int(first_row), int(last_row));

                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg == NULL)
                    continue;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_FRAMEBUFFER);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                // Write endianess
                if (list->setInt("endian", VST3_BYTEORDER) != Steinberg::kResultOk)
                    continue;
                // Write identifier of the frame buffer port
                if (!sSyncBuf.set_string(list, "id", fb_port->metadata()->id))
                    continue;
                // Write number of rows
                if (list->setInt("rows", fb->rows()) != Steinberg::kResultOk)
                    continue;
                // Write number of columns
                if (list->setInt("cols", fb->cols()) != Steinberg::kResultOk)
                    continue;
                // Write number of first row
                if (list->setInt("first_row_id", first_row) != Steinberg::kResultOk)
                    continue;
                // Write number of last row
                if (list->setInt("last_row_id", last_row) != Steinberg::kResultOk)
                    continue;

                // Encode data for each row
                bool encoded = true;
                for (size_t i=0; first_row != last_row; ++i, ++first_row)
                {
                    snprintf(key, sizeof(key), "row[%d]", int(i));
                    if (list->setBinary(key, fb->get_row(first_row), fb->cols() * sizeof(float)) != Steinberg::kResultOk)
                    {
                        encoded     = false;
                        break;
                    }
                }
                if (!encoded)
                    continue;

                // Finally, we're ready to send message
                if (pPeerConnection->notify(msg) == Steinberg::kResultOk)
                    fb_port->set_row_id(first_row);
            }

            // Synchronize streams
            for (lltl::iterator<plug::IPort> it=vStreams.values(); it; ++it)
            {
                // Get the frame buffer data
                vst3::StreamPort *s_port = static_cast<vst3::StreamPort *>(it.get());
                if (s_port == NULL)
                    continue;
                plug::stream_t *s        = it->buffer<plug::stream_t>();
                if (s == NULL)
                    continue;

                // Serialize not more than number of predefined frames
                uint32_t frame_id  = s_port->frame_id();
                uint32_t src_id    = s->frame_id();
                uint32_t delta     = src_id - s->frame_id();
                if (delta == 0)
                    continue;

                size_t num_frames  = s->frames();
                uint32_t last_id   = src_id + 1;
                if (delta > num_frames)
                {
                    delta              = num_frames;
                    frame_id           = last_id - num_frames;
                }
                if (delta > STREAM_BULK_MAX)
                    last_id            = frame_id + STREAM_BULK_MAX;
                size_t nbuffers     = s->channels();

                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg == NULL)
                    continue;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_STREAM);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                // Write endianess
                if (list->setInt("endian", VST3_BYTEORDER) != Steinberg::kResultOk)
                    continue;
                // Write identifier of the frame buffer port
                if (!sSyncBuf.set_string(list, "id", s_port->metadata()->id))
                    continue;
                // Write number of buffers
                if (list->setInt("buffers", nbuffers) != Steinberg::kResultOk)
                    continue;

                // Forge vectors
                size_t frames = 0;
                bool encoded = true;
                for ( ; frame_id != last_id; ++frame_id)
                {
                    ssize_t frame_size = s->get_frame_size(frame_id);
                    if (frame_size < 0)
                        continue;

                    // Forge frame number
                    snprintf(key, sizeof(key), "frame_id[%d]", int(frames));
                    if (list->setInt(key, frame_id) != Steinberg::kResultOk)
                    {
                        encoded = false;
                        break;
                    }
                    snprintf(key, sizeof(key), "frame_size[%d]", int(frames));
                    if (list->setInt(key, frame_size) != Steinberg::kResultOk)
                    {
                        encoded = false;
                        break;
                    }

                    // Forge vectors
                    for (size_t i=0; i < nbuffers; ++i)
                    {
                        float *data = s_port->read_frame(frame_id, i, 0, frame_size);

                        snprintf(key, sizeof(key), "data[%d][%d]", int(frames), int(i));
                        if (list->setBinary(key, data, frame_size * sizeof(float)) != Steinberg::kResultOk)
                        {
                            encoded     = false;
                            break;
                        }
                    }
                    if (!encoded)
                        break;

                    // Increment number of frames
                    ++frames;
                }

                if (!encoded)
                    continue;
                if (list->setInt("frames", frames) != Steinberg::kResultOk)
                    continue;

                // Update current RowID
                s_port->set_frame_id(frame_id);
            }
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_WRAPPER_H_ */
