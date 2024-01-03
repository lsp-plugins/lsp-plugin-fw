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

#ifndef MODULES_LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_WRAPPER_H_
#define MODULES_LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_WRAPPER_H_

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
        Wrapper::Wrapper(PluginFactory *factory, plug::Module *plugin, resource::ILoader *loader):
            IWrapper(plugin, loader)
        {
            nRefCounter     = 1;
            pFactory        = safe_acquire(factory);
            pHostContext    = NULL;
            pPeerConnection = NULL;
            pEventIn        = NULL;
            pEventOut       = NULL;
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

        ssize_t Wrapper::compare_audio_channels(const audio_channel_t *a, const audio_channel_t *b)
        {
            if (a->nSpeaker > b->nSpeaker)
                return 1;
            return (a->nSpeaker < b->nSpeaker) ? -1 : 0;
        }

        Wrapper::audio_bus_t *Wrapper::create_audio_bus(const meta::port_group_t *meta, lltl::parray<plug::IPort> *ins, lltl::parray<plug::IPort> *outs)
        {
            lltl::darray<audio_channel_t> channels;

            // Form the list of channels sorted according to the speaker ordering bits
            lltl::parray<plug::IPort> *list = (meta->flags & meta::PGF_OUT) ? outs : ins;
            for (const meta::port_group_item_t *item = meta->items; (item != NULL) && (item->id != NULL); ++item)
            {
                // Find port
                plug::IPort *p  = find_port(item->id, list);
                if (p == NULL)
                {
                    lsp_error("Missing %s port '%s' for the audio group '%s'",
                        (meta->flags & meta::PGF_OUT) ? "output" : "input", item->id, meta->id);
                    return NULL;
                }

                // Create new channel record
                audio_channel_t *c = channels.add();
                if (c == NULL)
                    return NULL;

                c->pPort            = static_cast<vst3::AudioPort *>(p);
                switch (item->role)
                {
                    case meta::PGR_CENTER:
                        c->nSpeaker     = (meta->type == meta::GRP_MONO) ? Steinberg::Vst::kSpeakerM : Steinberg::Vst::kSpeakerC;
                        break;
                    case meta::PGR_CENTER_LEFT:     c->nSpeaker     = Steinberg::Vst::kSpeakerLc;   break;
                    case meta::PGR_CENTER_RIGHT:    c->nSpeaker     = Steinberg::Vst::kSpeakerRc;   break;
                    case meta::PGR_LEFT:            c->nSpeaker     = Steinberg::Vst::kSpeakerL;    break;
                    case meta::PGR_LO_FREQ:         c->nSpeaker     = Steinberg::Vst::kSpeakerLfe;  break;
                    case meta::PGR_REAR_CENTER:     c->nSpeaker     = Steinberg::Vst::kSpeakerTrc;  break;
                    case meta::PGR_REAR_LEFT:       c->nSpeaker     = Steinberg::Vst::kSpeakerTrl;  break;
                    case meta::PGR_REAR_RIGHT:      c->nSpeaker     = Steinberg::Vst::kSpeakerTrr;  break;
                    case meta::PGR_RIGHT:           c->nSpeaker     = Steinberg::Vst::kSpeakerR;    break;
                    case meta::PGR_SIDE_LEFT:       c->nSpeaker     = Steinberg::Vst::kSpeakerSl;   break;
                    case meta::PGR_SIDE_RIGHT:      c->nSpeaker     = Steinberg::Vst::kSpeakerSr;   break;
                    case meta::PGR_MS_SIDE:         c->nSpeaker     = Steinberg::Vst::kSpeakerC;    break;
                    case meta::PGR_MS_MIDDLE:       c->nSpeaker     = Steinberg::Vst::kSpeakerS;    break;
                    default:
                        lsp_error("Unsupported role %d for channel '%s' in group '%s'",
                            int(item->role), item->id, meta->id);
                        return NULL;
                }

                // Exclude port from list
                list->premove(p);
            }
            channels.qsort(compare_audio_channels);

            // Allocate the audio group object
            audio_bus_t *grp    = alloc_audio_bus(meta->id, channels.size());
            if (grp == NULL)
                return NULL;
            lsp_finally {
                if (grp != NULL)
                    free(grp);
            };

            // Fill the audio bus data
            grp->nType          = meta->type;
            grp->nPorts         = channels.size();
            grp->nBusType       = (meta->flags & meta::PGF_SIDECHAIN) ? Steinberg::Vst::kAux : Steinberg::Vst::kMain;

            for (size_t i=0; i<grp->nPorts; ++i)
            {
                audio_channel_t *c   = channels.uget(i);
                if (c == NULL)
                    return NULL;
                grp->vPorts[i]       = c->pPort;
            }

            return release_ptr(grp);
        }

        bool Wrapper::create_audio_busses()
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
                    if (grp != NULL)
                        free(grp);
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

            // TODO: handle creation of busses for other ports

            return true;
        }

        Steinberg::tresult PLUGIN_API Wrapper::initialize(Steinberg::FUnknown *context)
        {
            if (pHostContext != NULL)
                return Steinberg::kResultFalse;
            pHostContext    = safe_acquire(context);

            if (!create_audio_busses())
                return Steinberg::kInternalError;

            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::terminate()
        {
            safe_release(pHostContext);

            // Release the peer connection if host didn't disconnect us previously.
            if (pPeerConnection != NULL)
            {
                pPeerConnection->disconnect(this);
                safe_release(pPeerConnection);
            }

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
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::int32 PLUGIN_API Wrapper::getBusCount(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getBusInfo(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::BusInfo & bus /*out*/)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getRoutingInfo(Steinberg::Vst::RoutingInfo & inInfo, Steinberg::Vst::RoutingInfo & outInfo /*out*/)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::activateBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::TBool state)
        {
            // TODO: implement this
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

        Steinberg::tresult PLUGIN_API Wrapper::setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::getBusArrangement(Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::SpeakerArrangement & arr)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::canProcessSampleSize(Steinberg::int32 symbolicSampleSize)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::getLatencySamples()
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setupProcessing(Steinberg::Vst::ProcessSetup & setup)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::setProcessing(Steinberg::TBool state)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Wrapper::process(Steinberg::Vst::ProcessData & data)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::getTailSamples()
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::uint32 PLUGIN_API Wrapper::getProcessContextRequirements()
        {
            // TODO: implement this
            return 0;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* MODULES_LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_WRAPPER_H_ */
