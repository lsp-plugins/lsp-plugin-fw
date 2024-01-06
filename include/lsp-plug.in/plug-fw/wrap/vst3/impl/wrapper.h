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
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/wrapper.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        Wrapper::Wrapper(PluginFactory *factory, plug::Module *plugin, resource::ILoader *loader, const meta::package_t *package):
            IWrapper(plugin, loader)
        {
            nRefCounter         = 1;
            pFactory            = safe_acquire(factory);
            pPackage            = package;
            pHostContext        = NULL;
            pPeerConnection     = NULL;
            pEventsIn           = NULL;
            pEventsOut          = NULL;

            nActLatency         = 0;
            nRepLatency         = 0;
            bUpdateSettings     = true;
        }

        Wrapper::~Wrapper()
        {
            // Destroy plugin
            if (pPlugin != NULL)
            {
                delete pPlugin;
                pPlugin         = NULL;
            }

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
            // TODO: implement this
        }

        Wrapper::audio_bus_t *Wrapper::alloc_audio_bus(const char *name, size_t ports)
        {
            LSPString tmp;
            if (!tmp.set_utf8(name))
                return NULL;
            Steinberg::CStringW u16name  = reinterpret_cast<Steinberg::CStringW>(tmp.get_utf16());
            if (u16name == NULL)
                return NULL;

            size_t szof_bus     = sizeof(audio_bus_t);
            size_t szof_name    = (Steinberg::strlen16(u16name) + 1) * sizeof(Steinberg::char16);
            size_t szof_ports   = sizeof(plug::IPort *) * ports;
            size_t szof         = align_size(szof_bus + szof_ports + szof_name, DEFAULT_ALIGN);

            audio_bus_t *bus    = static_cast<audio_bus_t *>(malloc(szof));
            if (bus == NULL)
                return NULL;

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

            size_t szof_bus     = sizeof(event_bus_t);
            size_t szof_name    = (Steinberg::strlen16(u16name) + 1) * sizeof(Steinberg::char16);
            size_t szof_ports   = sizeof(plug::IPort *) * ports;
            size_t szof         = align_size(szof_bus + szof_ports + szof_name, DEFAULT_ALIGN);

            event_bus_t *bus    = static_cast<event_bus_t *>(malloc(szof));
            if (bus == NULL)
                return NULL;

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
            if (a->speaker() > b->speaker())
                return 1;
            return (a->speaker() < b->speaker()) ? -1 : 0;
        }

        Wrapper::audio_bus_t *Wrapper::create_audio_bus(const meta::port_group_t *meta, lltl::parray<plug::IPort> *ins, lltl::parray<plug::IPort> *outs)
        {
            lltl::parray<vst3::AudioPort> channels;

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
                    return NULL;

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
                return NULL;
            lsp_finally {
                free_audio_bus(bus);
            };

            // Fill the audio bus data
            bus->nType          = meta->type;
            bus->nPorts         = channels.size();
            bus->nCurrArr       = 0;
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

            return release_ptr(bus);
        }

        Wrapper::audio_bus_t *Wrapper::create_audio_bus(plug::IPort *port)
        {
            const meta::port_t *meta = (port != NULL) ? port->metadata() : NULL;
            if (meta == NULL)
                return NULL;

            // Allocate the audio group object
            audio_bus_t *grp    = alloc_audio_bus(meta->id, 1);
            if (grp == NULL)
                return NULL;
            lsp_finally {
                free_audio_bus(grp);
            };

            // Fill the audio bus data
            vst3::AudioPort *xp = static_cast<vst3::AudioPort *>(port);

            grp->nType          = meta::GRP_MONO;
            grp->nPorts         = 1;
            grp->nCurrArr       = xp->speaker();
            grp->nMinArr        = (meta::is_optional_port(meta)) ? 0 : xp->speaker();
            grp->nFullArr       = grp->nCurrArr;
            grp->nBusType       = Steinberg::Vst::kMain;
            grp->vPorts[0]      = xp;

            return release_ptr(grp);
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

        bool Wrapper::create_busses()
        {
            // Obtain plugin metadata
            const meta::plugin_t *meta = (pPlugin != NULL) ? pPlugin->metadata() : NULL;
            if (meta == NULL)
                return false;

            // Generate modifiable lists of input and output ports
            lltl::parray<plug::IPort> ins, outs, midi_ins, midi_outs;
            for (size_t i=0, n=vAllPorts.size(); i < n; ++i)
            {
                plug::IPort *p = vAllPorts.uget(i);
                const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                if (meta == NULL)
                    continue;

                if (meta::is_audio_port(meta))
                {
                    if (meta::is_in_port(meta))
                        ins.add(p);
                    else
                        outs.add(p);
                }
                else if (meta::is_midi_port(meta))
                {
                    if (meta::is_in_port(meta))
                        midi_ins.add(p);
                    else
                        midi_outs.add(p);
                }
            }

            // Create audio busses based on the information about port groups
            audio_bus_t *in_main = NULL, *out_main = NULL, *grp = NULL;
            for (const meta::port_group_t *pg = meta->port_groups; pg != NULL && pg->id != NULL; ++pg)
            {
                // Create group and add to list
                if ((grp = create_audio_bus(pg, &ins, &outs)) == NULL)
                    return false;
                lsp_finally {
                    free_audio_bus(grp);
                };

                // Add the group to list or keep as a separate pointer because CLAP
                // requires main ports to be first in the overall port list
                if (pg->flags & meta::PGF_OUT)
                {
                    if (pg->flags & meta::PGF_MAIN)
                    {
                        if (in_main != NULL)
                        {
                            lsp_error("Duplicate main output group in metadata");
                            return false;
                        }
                        in_main         = grp;
                        if (vAudioOut.insert(0, grp))
                            return false;
                    }
                    else
                    {
                        if (!vAudioOut.add(grp))
                            return false;
                    }
                }
                else // meta::PGF_IN
                {
                    if (pg->flags & meta::PGF_MAIN)
                    {
                        if (out_main != NULL)
                        {
                            lsp_error("Duplicate main input group in metadata");
                            return false;
                        }
                        out_main    = grp;
                        if (!vAudioIn.insert(0, grp))
                            return false;
                    }
                    else
                    {
                        if (!vAudioIn.add(grp))
                            return false;
                    }
                }

                // Release the group pointer to prevent from destruction
                grp     = NULL;
            }

            // Create mono audio busses for non-assigned ports
            for (lltl::iterator<plug::IPort> it = ins.values(); it; ++it)
            {
                plug::IPort *p = it.get();
                if ((grp = create_audio_bus(p)) == NULL)
                    return false;
                lsp_finally {
                    free_audio_bus(grp);
                };

                if (!vAudioIn.add(grp))
                    return false;
                grp     = NULL;
            }

            for (lltl::iterator<plug::IPort> it = outs.values(); it; ++it)
            {
                plug::IPort *p = it.get();
                if ((grp = create_audio_bus(p)) == NULL)
                    return false;
                lsp_finally {
                    free_audio_bus(grp);
                };

                if (!vAudioOut.add(grp))
                    return false;
                grp     = NULL;
            }

            // Create MIDI busses
            if (midi_ins.size() > 0)
            {
                // Allocate the audio group object
                event_bus_t *ev     = alloc_event_bus("events_in", midi_ins.size());
                if (grp == NULL)
                    return NULL;
                lsp_finally {
                    free_event_bus(ev);
                };

                // Fill the audio bus data
                ev->nPorts          = midi_ins.size();

                for (size_t i=0, n=midi_ins.size(); i<n; ++i)
                {
                    plug::IPort *p      = midi_ins.uget(i);
                    if (p == NULL)
                        return false;
                    ev->vPorts[i]      = p;
                }
                pEventsIn           = release_ptr(ev);
            }

            if (midi_outs.size() > 0)
            {
                // Allocate the audio group object
                event_bus_t *ev     = alloc_event_bus("events_out", midi_outs.size());
                if (grp == NULL)
                    return NULL;
                lsp_finally {
                    free_event_bus(ev);
                };

                // Fill the audio bus data
                ev->nPorts          = midi_outs.size();

                for (size_t i=0, n=midi_outs.size(); i<n; ++i)
                {
                    plug::IPort *p      = midi_outs.uget(i);
                    if (p == NULL)
                        return false;
                    ev->vPorts[i]      = p;
                }
                pEventsOut          = release_ptr(ev);
            }

            return true;
        }

        Steinberg::tresult PLUGIN_API Wrapper::initialize(Steinberg::FUnknown *context)
        {
            if (pHostContext != NULL)
                return Steinberg::kResultFalse;
            pHostContext    = safe_acquire(context);

            if (!create_busses())
                return Steinberg::kInternalError;

            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::terminate()
        {
            // Destroy plugin
            if (pPlugin != NULL)
            {
                delete pPlugin;
                pPlugin         = NULL;
            }

            // Release host context
            safe_release(pHostContext);

            // Release the peer connection if host didn't disconnect us previously.
            if (pPeerConnection != NULL)
            {
                pPeerConnection->disconnect(this);
                safe_release(pPeerConnection);
            }

            // Release busses
            for (lltl::iterator<audio_bus_t> it = vAudioIn.values(); it; ++it)
                free_audio_bus(it.get());
            for (lltl::iterator<audio_bus_t> it = vAudioOut.values(); it; ++it)
                free_audio_bus(it.get());
            free_event_bus(pEventsIn);
            free_event_bus(pEventsOut);

            vAudioIn.flush();
            vAudioOut.flush();
            pEventsIn   = NULL;
            pEventsOut  = NULL;

            // Destroy ports
            for (lltl::iterator<plug::IPort> it = vAllPorts.values(); it; ++it)
            {
                plug::IPort *port = it.get();
                if (port != NULL)
                    delete port;
            }
            vAllPorts.flush();

            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getControllerClassId(Steinberg::TUID classId)
        {
            const meta::plugin_t *meta = pPlugin->metadata();
            if (meta->vst3ui_uid == NULL)
                return Steinberg::kResultFalse;

            Steinberg::TUID tuid;
            status_t res = parse_tuid(tuid, meta->vst3ui_uid);
            if (res != STATUS_OK)
                return Steinberg::kResultFalse;

            memcpy(classId, tuid, sizeof(tuid));
            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setIoMode(Steinberg::Vst::IoMode mode)
        {
            return Steinberg::kNotImplemented;
        }

        Steinberg::int32 PLUGIN_API Wrapper::getBusCount(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir)
        {
            if (type == Steinberg::Vst::kAudio)
            {
                if (dir == Steinberg::Vst::kInput)
                    return vAudioIn.size();
                else if (dir == Steinberg::Vst::kOutput)
                    return vAudioOut.size();
            }
            else if (type == Steinberg::Vst::kEvent)
            {
                if (dir == Steinberg::Vst::kInput)
                    return (pEventsIn != NULL) ? 1 : 0;
                else if (dir == Steinberg::Vst::kOutput)
                    return (pEventsOut != NULL) ? 1 : 0;
            }

            return 0;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getBusInfo(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::BusInfo & bus /*out*/)
        {
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

                    return Steinberg::kResultTrue;
                }
            }

            return Steinberg::kInvalidArgument;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getRoutingInfo(Steinberg::Vst::RoutingInfo & inInfo, Steinberg::Vst::RoutingInfo & outInfo /*out*/)
        {
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::activateBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::TBool state)
        {
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

        Steinberg::tresult PLUGIN_API Wrapper::setState(Steinberg::IBStream *state)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getState(Steinberg::IBStream *state)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::connect(Steinberg::Vst::IConnectionPoint *other)
        {
            // Check if peer connection is valid and was not previously estimated
            if (other == NULL)
                return Steinberg::kInvalidArgument;
            if (pPeerConnection)
                return Steinberg::kResultFalse;

            // Save the peer connection
            pPeerConnection = safe_acquire(other);

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Wrapper::disconnect(Steinberg::Vst::IConnectionPoint *other)
        {
            // Check that estimated peer connection matches the esimated one
            if (other == NULL)
                return Steinberg::kInvalidArgument;
            if (pPeerConnection != other)
                return Steinberg::kResultFalse;

            // Reset the peer connection
            safe_release(pPeerConnection);

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Wrapper::notify(Steinberg::Vst::IMessage *message)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setBusArrangements(Steinberg::Vst::SpeakerArrangement *inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts)
        {
            if (numIns < 0 || numOuts < 0)
                return Steinberg::kInvalidArgument;

            if ((size_t(numIns) > vAudioIn.size()) ||
                (size_t(numOuts) > vAudioOut.size()))
                return Steinberg::kResultFalse;

            // Check that we allow several ports to be disabled
            for (ssize_t i=0; i < numIns; ++i)
            {
                const audio_bus_t *bus = vAudioIn.get(i);
                if (bus == NULL)
                    return Steinberg::kInvalidArgument;

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

                const Steinberg::Vst::SpeakerArrangement arr = outputs[i];
                if ((arr & (~bus->nFullArr)) != 0)
                    return Steinberg::kInvalidArgument;
                if ((bus->nMinArr & arr) != bus->nMinArr)
                    return Steinberg::kResultFalse;
            }

            // Apply new configuration
            for (ssize_t i=0; i < numIns; ++i)
            {
                audio_bus_t *bus    = vAudioIn.get(i);
                bus->nCurrArr       = inputs[i];
                update_port_activity(bus);
            }

            for (ssize_t i=0; i < numOuts; ++i)
            {
                audio_bus_t *bus    = vAudioOut.get(i);
                bus->nCurrArr       = outputs[i];
                update_port_activity(bus);
            }

            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getBusArrangement(Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::SpeakerArrangement & arr)
        {
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
            // We support only 32-bit float samples
            return (symbolicSampleSize == Steinberg::Vst::kSample32) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::getLatencySamples()
        {
            uint32_t latency    = nActLatency;
            nRepLatency         = latency;
            return latency;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setupProcessing(Steinberg::Vst::ProcessSetup & setup)
        {
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

            // Adjust block size for input and output audio ports
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
            // This is almost useless
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::process(Steinberg::Vst::ProcessData & data)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::getTailSamples()
        {
            if (pPlugin == NULL)
                return Steinberg::kInternalError;

            ssize_t tail_size = pPlugin->tail_size();
            if (tail_size < 0)
                return Steinberg::Vst::kInfiniteTail;

            return (tail_size > 0) ? tail_size : Steinberg::Vst::kInfiniteTail;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::getProcessContextRequirements()
        {
            // TODO: implement this
            return 0;
        }

        ipc::IExecutor *Wrapper::executor()
        {
            // TODO: implement this
            return NULL;
        }

        core::KVTStorage *Wrapper::kvt_lock()
        {
            // TODO: implement this
            return NULL;
        }

        core::KVTStorage *Wrapper::kvt_trylock()
        {
            // TODO: implement this
            return NULL;
        }

        bool Wrapper::kvt_release()
        {
            // TODO: implement this
            return false;
        }

        void Wrapper::state_changed()
        {
            // TODO: implement this
        }

        void Wrapper::request_settings_update()
        {
            // TODO: implement this
        }

        const meta::package_t *Wrapper::package() const
        {
            return pPackage;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_WRAPPER_H_ */
