/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 дек. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/plug-fw/wrap/clap/wrapper.h>

namespace lsp
{
    namespace clap
    {
        Wrapper::Wrapper(
            plug::Module *module,
            const meta::package_t *package,
            resource::ILoader *loader,
            const clap_host_t *host):
            IWrapper(module, loader)
        {
            pHost               = host;
            pPackage            = package;
            pUIMetadata         = NULL;
            pUIFactory          = NULL;
            pUIWrapper          = NULL;
            nUIReq              = 0;
            nUIResp             = 0;
            pExt                = NULL;
            pExecutor           = NULL;

            nLatency            = 0;
            nTailSize           = 0;
            nDumpReq            = 0;
            nDumpResp           = 0;

            bLatencyChanged     = false;
            bUpdateSettings     = true;
            bStateManage        = false;
            pSamplePlayer       = NULL;
        }

        Wrapper::~Wrapper()
        {
            destroy();
        }

        void Wrapper::destroy()
        {
            // Shutdown and delete executor if exists
            if (pExecutor != NULL)
            {
                pExecutor->shutdown();
                delete pExecutor;
                pExecutor   = NULL;
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
                pPlugin->destroy();
                delete pPlugin;
                pPlugin         = NULL;
            }

            // Destroy audio groups
            for (size_t i=0, n=vAudioIn.size(); i<n; ++i)
                destroy_audio_group(vAudioIn.uget(i));
            for (size_t i=0, n=vAudioOut.size(); i<n; ++i)
                destroy_audio_group(vAudioOut.uget(i));
            vAudioIn.flush();
            vAudioOut.flush();

            // Destroy all ports
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                plug::IPort *p = vAllPorts.uget(i);
                delete p;
            }
            vAllPorts.flush();
            vSortedPorts.flush();
            vMidiIn.flush();
            vMidiOut.flush();

            // Cleanup generated metadata
            for (size_t i=0, n=vGenMetadata.size(); i<n; ++i)
            {
                meta::port_t *p = vGenMetadata.uget(i);
                lsp_trace("destroy generated port metadata %p", p);
                meta::drop_port_metadata(p);
            }
            vGenMetadata.flush();

            // Destroy the loader
            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader         = NULL;
            }

            // Destroy the extension
            if (pExt != NULL)
            {
                delete pExt;
                pExt            = NULL;
            }

            // Release some pointers
            pHost           = NULL;
            pPackage        = NULL;
        }

        void Wrapper::destroy_audio_group(audio_group_t *grp)
        {
            if (grp != NULL)
                free(grp);
        }

        void Wrapper::create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix)
        {
            clap::Port *cp      = NULL;

            switch (port->role)
            {
                case meta::R_MESH:
                    cp                      = new clap::MeshPort(port);
                    break;

                case meta::R_FBUFFER:
                    cp                      = new clap::FrameBufferPort(port);
                    break;

                case meta::R_STREAM:
                    cp                      = new clap::StreamPort(port);
                    break;

                case meta::R_MIDI_IN:
                {
                    clap::MidiInputPort *mi = new clap::MidiInputPort(port);
                    vMidiIn.add(mi);
                    cp  = mi;
                    break;
                }
                case meta::R_MIDI_OUT:
                {
                    clap::MidiOutputPort *mo = new clap::MidiOutputPort(port);
                    vMidiOut.add(mo);
                    cp  = mo;
                    break;
                }

                case meta::R_AUDIO_IN:
                case meta::R_AUDIO_OUT:
                    // Audio ports will be organized into groups after instantiation of all ports
                    cp = new clap::AudioPort(port);
                    break;

                case meta::R_OSC_IN:
                case meta::R_OSC_OUT:
                    cp = new clap::OscPort(port);
                    break;

                case meta::R_PATH:
                    cp                      = new clap::PathPort(port);
                    break;

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    clap::ParameterPort *pp = new clap::ParameterPort(port);
                    vParamPorts.add(pp);
                    cp  = pp;
                    break;
                }

                case meta::R_METER:
                    cp                      = new clap::MeterPort(port);
                    break;

                case meta::R_PORT_SET:
                {
                    LSPString postfix_str;
                    clap::PortGroup *pg     = new clap::PortGroup(port);
                    vAllPorts.add(pg);
                    vParamPorts.add(pg);
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

        ssize_t Wrapper::compare_ports_by_clap_id(const ParameterPort *a, const ParameterPort *b)
        {
            clap_id a_id = a->uid();
            clap_id b_id = b->uid();
            return (a_id < b_id) ? -1 : (a_id > b_id) ? 1 : 0;
        }

        ssize_t Wrapper::compare_ports_by_id(const clap::Port *a, const clap::Port *b)
        {
            const char *a_id = a->metadata()->id;
            const char *b_id = b->metadata()->id;
            return strcmp(a_id, b_id);
        }

        Wrapper::audio_group_t *Wrapper::alloc_audio_group(size_t ports)
        {
            // Allocate the audio group object
            size_t szof_group   = sizeof(audio_group_t);
            size_t szof_ports   = sizeof(plug::IPort *) * ports;
            size_t szof         = align_size(szof_group + szof_ports, DEFAULT_ALIGN);
            audio_group_t *grp  = static_cast<audio_group_t *>(malloc(szof));
            if (grp != NULL)
                grp->nPorts         = ports;
            return grp;
        }

        Wrapper::audio_group_t *Wrapper::create_audio_group(
            const meta::port_group_t *meta,
            lltl::parray<plug::IPort> *ins,
            lltl::parray<plug::IPort> *outs)
        {
            // Estimate the number of ports in the group
            size_t num_ports = 0;
            for (const meta::port_group_item_t *item = meta->items; (item != NULL) && (item->id != NULL); ++item)
                ++ num_ports;

            // Allocate the audio group object
            audio_group_t *grp  = alloc_audio_group(num_ports);
            if (grp == NULL)
                return NULL;

            // Initialize the audio group
            grp->nType          = meta->type;
            grp->nFlags         = CLAP_AUDIO_PORT_REQUIRES_COMMON_SAMPLE_SIZE;
            grp->nInPlace       = -1;
            grp->sName          = meta->id;
            grp->nPorts         = num_ports;

            lltl::parray<plug::IPort> * list = (meta->flags & meta::PGF_OUT) ? outs : ins;
            for (size_t i=0; i<num_ports; ++i)
            {
                const char *id  = meta->items[i].id;
                plug::IPort *p  = find_port(id, list);
                if (p == NULL)
                {
                    lsp_error("Missing %s port '%s' for the audio group '%s'",
                        (meta->flags & meta::PGF_OUT) ? "output" : "input", id, meta->id);
                    continue;
                }

                grp->vPorts[i]  = static_cast<clap::AudioPort *>(p);
                list->premove(p);
            }

            return grp;
        }

        Wrapper::audio_group_t *Wrapper::create_audio_group(plug::IPort *port)
        {
            // Obtain port metadata
            const meta::port_t *meta = port->metadata();
            if (meta == NULL)
                return NULL;

            // Allocate the audio group object
            audio_group_t *grp  = alloc_audio_group(1);
            if (grp == NULL)
                return NULL;

            // Initialize the audio group
            grp->nType          = meta::GRP_MONO;
            grp->nFlags         = CLAP_AUDIO_PORT_REQUIRES_COMMON_SAMPLE_SIZE;
            grp->nInPlace       = -1;
            grp->sName          = meta->id;
            grp->nPorts         = 1;
            grp->vPorts[0]      = static_cast<clap::AudioPort *>(port);

            return grp;
        }

        status_t Wrapper::generate_audio_port_groups(const meta::plugin_t *meta)
        {
            // Generate modifiable lists of input and output ports
            lltl::parray<plug::IPort> ins, outs;
            for (size_t i=0, n=vAllPorts.size(); i < n; ++i)
            {
                plug::IPort *p = vAllPorts.uget(i);
                const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                if ((meta != NULL) && (meta::is_audio_port(meta)))
                {
                    if (meta::is_in_port(meta))
                        ins.add(p);
                    else
                        outs.add(p);
                }
            }

            // Try to create ports using port groups
            audio_group_t *in_main = NULL, *out_main = NULL, *grp = NULL;
            const meta::port_group_t *port_groups = (meta != NULL) ? meta->port_groups : NULL;
            for (const meta::port_group_t *pg = port_groups; (pg != NULL) && (pg->id != NULL); ++pg)
            {
                // Create group and add to list
                if ((grp  = create_audio_group(pg, &ins, &outs)) == NULL)
                    return STATUS_NO_MEM;

                // Add the group to list or keep as a separate pointer because CLAP
                // requires main ports to be first in the overall port list
                if (pg->flags & meta::PGF_OUT)
                {
                    if (pg->flags & meta::PGF_MAIN)
                    {
                        if (in_main != NULL)
                        {
                            lsp_error("Duplicate main output group in metadata");
                            vAudioOut.add(grp);
                        }
                        else
                        {
                            in_main         = grp;
                            grp->nFlags    |= CLAP_AUDIO_PORT_IS_MAIN;
                            vAudioOut.insert(0, grp);
                        }
                    }
                    else
                        vAudioOut.add(grp);
                }
                else // meta::PGF_IN
                {
                    if (pg->flags & meta::PGF_MAIN)
                    {
                        if (out_main != NULL)
                        {
                            lsp_error("Duplicate main input group in metadata");
                            vAudioOut.add(grp);
                        }
                        else
                        {
                            out_main    = grp;
                            grp->nFlags    |= CLAP_AUDIO_PORT_IS_MAIN;
                            vAudioIn.insert(0, grp);
                        }
                    }
                    else
                        vAudioIn.add(grp);
                }
            }

            // We need to create main audio groups anyway
            if ((ins.size() > 0) && (!in_main))
            {
                if ((grp = alloc_audio_group(ins.size())) == NULL)
                    return STATUS_NO_MEM;

                // Initialize the audio group
                grp->nType          = meta::GRP_MONO;
                grp->nFlags         = CLAP_AUDIO_PORT_REQUIRES_COMMON_SAMPLE_SIZE | CLAP_AUDIO_PORT_IS_MAIN;
                grp->nInPlace       = -1;
                grp->sName          = "main_in";
                grp->nPorts         = ins.size();
                for (size_t i=0, n=ins.size(); i<n; ++i)
                    grp->vPorts[i]      = static_cast<clap::AudioPort *>(ins.uget(i));

                if (!vAudioIn.add(grp))
                {
                    destroy_audio_group(grp);
                    return STATUS_NO_MEM;
                }
                in_main             = grp;

                lsp_trace("Created default main input group id=%s for %d ports", grp->sName, grp->nPorts);
            }

            if ((outs.size() > 0) && (!out_main))
            {
                audio_group_t *grp  = alloc_audio_group(outs.size());
                if (grp == NULL)
                    return STATUS_NO_MEM;

                // Initialize the audio group
                grp->nType          = meta::GRP_MONO;
                grp->nFlags         = CLAP_AUDIO_PORT_REQUIRES_COMMON_SAMPLE_SIZE | CLAP_AUDIO_PORT_IS_MAIN;
                grp->nInPlace       = -1;
                grp->sName          = "main_out";
                grp->nPorts         = outs.size();
                for (size_t i=0, n=outs.size(); i<n; ++i)
                    grp->vPorts[i]      = static_cast<clap::AudioPort *>(outs.uget(i));

                if (!vAudioIn.add(grp))
                {
                    destroy_audio_group(grp);
                    return STATUS_NO_MEM;
                }
                out_main            = grp;

                lsp_trace("Created default main output group id=%s for %d ports", grp->sName, grp->nPorts);
            }

            // Do some optimizations for the host
            if ((in_main != NULL) && (out_main != NULL) && (in_main->nPorts == out_main->nPorts))
            {
                in_main->nInPlace   = vAudioOut.index_of(out_main);
                out_main->nInPlace  = vAudioIn.index_of(in_main);
            }

            // Create the rest input ports if they do not belong to any audio group
            for (size_t i=0, n=ins.size(); i<n; ++i)
            {
                plug::IPort *p      = ins.uget(i);
                audio_group_t *grp  = create_audio_group(p);
                if (grp == NULL)
                    return STATUS_NO_MEM;
                if (!vAudioIn.add(grp))
                {
                    destroy_audio_group(grp);
                    return STATUS_NO_MEM;
                }
            }

            // Create the rest output ports if they do not belong to any audio group
            for (size_t i=0, n=outs.size(); i<n; ++i)
            {
                plug::IPort *p      = outs.uget(i);
                audio_group_t *grp  = create_audio_group(p);
                if (grp == NULL)
                    return STATUS_NO_MEM;
                if (!vAudioOut.add(grp))
                {
                    destroy_audio_group(grp);
                    return STATUS_NO_MEM;
                }
            }

            return STATUS_OK;
        }

        status_t Wrapper::create_ports(lltl::parray<plug::IPort> *plugin_ports, const meta::plugin_t *meta)
        {
            // Create ports
            lsp_trace("Creating ports for %s - %s", meta->name, meta->description);
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(plugin_ports, port, NULL);
            vParamPorts.qsort(compare_ports_by_clap_id);

            // Create sorted copy of all ports
            if (!vSortedPorts.add(vAllPorts))
                return STATUS_NO_MEM;
            vSortedPorts.qsort(compare_ports_by_id);

        #ifdef LSP_TRACE
            // Validate that we don't have conflicts between uinque port identifiers
            for (size_t i=1, n=vParamPorts.size(); i<n; ++i)
            {
                clap::ParameterPort *curr = vParamPorts.uget(i);
                clap::ParameterPort *prev = vParamPorts.uget(i-1);
                if (curr->uid() == prev->uid())
                {
                    lsp_error("Conflicting clap_id hash=0x%08lx for ports '%s' and '%s', consider choosing another port identifier",
                        long(curr->uid()), prev->metadata()->id, curr->metadata()->id);
                    return STATUS_BAD_STATE;
                }
            }
        #endif /* LSP_TRACE */

            return STATUS_OK;
        }

        void Wrapper::lookup_ui_factory()
        {
            // Create UI wrapper
            const char *clap_uid = metadata()->uids.clap;

            // Lookup plugin identifier among all registered plugin factories
            for (ui::Factory *f = ui::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Check plugin identifier
                    if (!::strcmp(meta->uids.clap, clap_uid))
                    {
                        pUIMetadata     = meta;
                        pUIFactory      = f;

                        lsp_trace("UI factory: %p, UI metadata: %p", pUIFactory, pUIMetadata);
                        return;
                    }
                }
            }

            pUIMetadata     = NULL;
            pUIFactory      = NULL;
        }

        status_t Wrapper::init()
        {
            // Obtain the plugin metadata
            const meta::plugin_t *meta = pPlugin->metadata();
            if (meta == NULL)
                return STATUS_BAD_STATE;

            // Lookup for the UI factory
            lookup_ui_factory();

            // Create extensions
            pExt    = new HostExtensions(pHost);
            if (pExt == NULL)
                return STATUS_NO_MEM;

            // Create all possible ports for plugin and validate the state
            lltl::parray<plug::IPort> plugin_ports;
            LSP_STATUS_ASSERT(create_ports(&plugin_ports, meta));

            // Generate the input and output audio port groups
            LSP_STATUS_ASSERT(generate_audio_port_groups(meta));

            // Initialize plugin
            lsp_trace("pPlugin = %p", pPlugin);
            pPlugin->init(this, plugin_ports.array());

            // Create sample player if required
            if (meta->extensions & meta::E_FILE_PREVIEW)
            {
                pSamplePlayer       = new core::SamplePlayer(meta);
                if (pSamplePlayer == NULL)
                    return STATUS_NO_MEM;
                pSamplePlayer->init(this, plugin_ports.array(), plugin_ports.size());
            }

            return STATUS_OK;
        }

        status_t Wrapper::activate(double sample_rate, size_t min_frames_count, size_t max_frames_count)
        {
            // Clear the flag that the restart has been requested
            sPosition.sampleRate= sample_rate;
            bLatencyChanged     = false;
            bUpdateSettings     = true;

            if (sample_rate > MAX_SAMPLE_RATE)
            {
                lsp_warn(
                    "Unsupported sample rate: %f, maximum supported sample rate is %ld",
                    sample_rate,
                    long(MAX_SAMPLE_RATE));
                sample_rate  = MAX_SAMPLE_RATE;
            }

            lsp_trace("pPlugin = %p", pPlugin);
            pPlugin->set_sample_rate(sample_rate);
            if (pSamplePlayer != NULL)
                pSamplePlayer->set_sample_rate(sample_rate);

            // Activate audio ports
            for (size_t i=0, n=vAudioIn.size(); i<n; ++i)
            {
                audio_group_t *g = vAudioIn.uget(i);
                for (size_t j=0; j<g->nPorts; ++j)
                {
                    lsp_trace("activating input port id=%s", g->vPorts[j]->metadata()->id);
                    g->vPorts[j]->activate(min_frames_count, max_frames_count);
                }
            }
            for (size_t i=0, n=vAudioOut.size(); i<n; ++i)
            {
                audio_group_t *g = vAudioOut.uget(i);
                for (size_t j=0; j<g->nPorts; ++j)
                {
                    lsp_trace("activating output port id=%s", g->vPorts[j]->metadata()->id);
                    g->vPorts[j]->activate(min_frames_count, max_frames_count);
                }
            }

            // Call plugin for activation
            pPlugin->activate();

            return STATUS_OK;
        }

        void Wrapper::deactivate()
        {
            // Call plugin for deactivation
            pPlugin->deactivate();

            // Set the actual latency
            nLatency            = pPlugin->latency();
            bLatencyChanged     = false;
        }

        status_t Wrapper::start_processing()
        {
            return STATUS_OK;
        }

        void Wrapper::stop_processing()
        {
        }

        void Wrapper::reset()
        {
        }

        size_t Wrapper::prepare_block(size_t *ev_index, size_t offset, const clap_process_t *process)
        {
            // There are no more events in the block?
            size_t last_time    = process->frames_count;
            size_t num_ev       = process->in_events->size(process->in_events);

            // Estimate the block size
            for (size_t i = *ev_index; i < num_ev; ++i)
            {
                // Fetch the event
                const clap_event_header_t *hdr = process->in_events->get(process->in_events, i);
                if ((hdr->space_id != CLAP_CORE_EVENT_SPACE_ID) || (hdr->time < offset))
                    continue;

                // Allow several type of events only at the beginning of the processing block
                if (((hdr->type == CLAP_EVENT_PARAM_VALUE) ||
                     (hdr->type == CLAP_EVENT_PARAM_MOD) ||
                     (hdr->type == CLAP_EVENT_TRANSPORT))
                    && (hdr->time != offset))
                {
                    last_time   = hdr->time;
                    break;
                }
            }

            // Now we are ready to process each event
            for (size_t i = *ev_index; i < num_ev; ++i)
            {
                // Fetch the event until it's timestamp is not out of the block
                const clap_event_header_t *hdr = process->in_events->get(process->in_events, i);
                if ((hdr->space_id != CLAP_CORE_EVENT_SPACE_ID) || (hdr->time < offset))
                    continue;
                else if (hdr->time >= last_time)
                    break;

                // Process the event
                switch (hdr->type)
                {
                    case CLAP_EVENT_NOTE_ON:
                    {
                        // const clap_event_note_t *ev = reinterpret_cast<const clap_event_note_t *>(hdr);
                        // TODO: handle note on
                        break;
                    }
                    case CLAP_EVENT_NOTE_OFF:
                    {
                        // const clap_event_note_t *ev = reinterpret_cast<const clap_event_note_t *>(hdr);
                        // TODO: handle note off
                        break;
                    }
                    case CLAP_EVENT_NOTE_CHOKE:
                    {
                        // const clap_event_note_t *ev = reinterpret_cast<const clap_event_note_t *>(hdr);
                        // TODO: handle note choke
                        break;
                    }
                    case CLAP_EVENT_NOTE_EXPRESSION:
                    {
                        // const clap_event_note_expression_t *ev = reinterpret_cast<const clap_event_note_expression_t *>(hdr);
                        // TODO: handle note expression
                        break;
                    }
                    case CLAP_EVENT_PARAM_VALUE:
                    {
                        const clap_event_param_value_t *ev = reinterpret_cast<const clap_event_param_value_t *>(hdr);
                        clap::ParameterPort *pp = static_cast<clap::ParameterPort *>(ev->cookie);
                        if ((pp == NULL) || (pp->uid() != ev->param_id))
                            pp              = find_param(ev->param_id);

                        if ((pp != NULL) && (pp->clap_set_value(ev->value)))
                        {
                            lsp_trace("port changed (set): %s, offset=%d", pp->metadata()->id, int(offset));
//                            if (pExt->state != NULL)
//                                pExt->state->mark_dirty(pHost);
                            bUpdateSettings     = true;
                        }
                        break;
                    }
                    case CLAP_EVENT_PARAM_MOD:
                    {
                        const clap_event_param_mod_t *ev = reinterpret_cast<const clap_event_param_mod_t *>(hdr);
                        clap::ParameterPort *pp = static_cast<clap::ParameterPort *>(ev->cookie);
                        if ((pp == NULL) || (pp->uid() != ev->param_id))
                            pp              = find_param(ev->param_id);

                        if ((pp != NULL) && (pp->clap_mod_value(ev->amount)))
                        {
                            lsp_trace("port changed (mod): %s, offset=%d", pp->metadata()->id, int(offset));
//                            if (pExt->state != NULL)
//                                pExt->state->mark_dirty(pHost);
                            bUpdateSettings     = true;
                        }
                        break;
                    }
                    case CLAP_EVENT_TRANSPORT:
                    {
                        const clap_event_transport_t *ev = reinterpret_cast<const clap_event_transport_t *>(hdr);
                        // Update the transport state
                        if (ev->flags & CLAP_TRANSPORT_HAS_TEMPO)
                        {
                            sPosition.beatsPerMinute        = ev->tempo;
                            sPosition.beatsPerMinuteChange  = ev->tempo_inc;
                        }
                        if (ev->flags & CLAP_TRANSPORT_HAS_TIME_SIGNATURE)
                        {
                            sPosition.numerator             = ev->tsig_num;
                            sPosition.denominator           = ev->tsig_denom;
                        }
                        if (ev->flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE)
                        {
                            sPosition.frame                 = sPosition.sampleRate * (ev->song_pos_seconds / double(CLAP_SECTIME_FACTOR));
                            sPosition.ticksPerBeat          = DEFAULT_TICKS_PER_BEAT;
                            sPosition.tick                  = (sPosition.ticksPerBeat * double(ev->song_pos_beats - ev->bar_start) * sPosition.numerator * sPosition.beatsPerMinute) / (60.0 * CLAP_BEATTIME_FACTOR);
                        }
                        break;
                    }
                    case CLAP_EVENT_MIDI:
                    {
                        // Parse MIDI event and broadcast it to all input MIDI ports
                        const clap_event_midi_t *ev = reinterpret_cast<const clap_event_midi_t *>(hdr);
                        midi::event_t me;
                        ssize_t size = midi::decode(&me, ev->data);
                        if (size >= 0)
                        {
                            me.timestamp                    = hdr->time - offset;
                            for (size_t i=0, n=vMidiIn.size(); i<n; ++i)
                            {
                                MidiInputPort *in = vMidiIn.uget(i);
                                if (in == NULL)
                                    continue;
                                in->push(&me);
                            }
                        }
                        break;
                    }
                    case CLAP_EVENT_MIDI_SYSEX:
                    {
                        // const clap_event_midi_sysex_t *ev = reinterpret_cast<const clap_event_midi_sysex_t *>(hdr);
                        // We don't support MIDI System Exclusive messages
                        break;
                    }
                    case CLAP_EVENT_MIDI2:
                    {
                       // const clap_event_midi2_t *ev = reinterpret_cast<const clap_event_midi2_t *>(hdr);
                       // We don't support MIDI2 yet
                       break;
                    }
                    default:
                        break;
                } // switch
            } // for

            // Return the result
            return last_time - offset;
        }

        void Wrapper::generate_output_events(size_t offset, const clap_process_t *process)
        {
            // Are there any MIDI ports available?
            size_t n_outs = vMidiOut.size();
            if (n_outs <= 0)
                return;

            // Sort all MIDI events according to their timestamps
            for (size_t i=0; i<n_outs; ++i)
            {
                plug::midi_t *queue = static_cast<plug::midi_t *>(vMidiOut.uget(i)->buffer());
                if (queue != NULL)
                    queue->sort();
            }

            clap_event_midi_t msg;
            msg.header.size     = sizeof(clap_event_midi_t);
            msg.header.time     = 0;
            msg.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            msg.header.type     = CLAP_EVENT_MIDI;
            msg.header.flags    = 0;
            msg.port_index      = 0;

            // Merge the result
            while (true)
            {
                const midi::event_t *ev     = NULL;
                MidiOutputPort *port        = NULL;

                // Scan for the event with the minimum timestamp among all ports
                for (size_t i=0; i<n_outs; ++i)
                {
                    MidiOutputPort *p           = vMidiOut.uget(i);
                    if (p == NULL)
                        continue;
                    const midi::event_t *e      = p->peek();
                    if (e == NULL)
                        continue;
                    if ((ev == NULL) || (ev->timestamp < e->timestamp))
                    {
                        ev                  = e;
                        port                = p;
                    }
                }

                // No port/message found?
                if (port == NULL)
                    break;

                // Emit MIDI message
                msg.header.time     = offset + ev->timestamp;
                msg.data[0]         = 0;
                msg.data[1]         = 0;
                msg.data[2]         = 0;
                if (midi::encode(msg.data, ev) >= 0)
                    process->out_events->try_push(process->out_events, &msg.header);

                // Remove MIDI message from the output MIDI port
                port->peek();
            }
        }

        clap_process_status Wrapper::process(const clap_process_t *process)
        {
            if ((process->audio_inputs_count > vAudioIn.size()) ||
                (process->audio_outputs_count > vAudioOut.size()))
                return CLAP_PROCESS_ERROR;

            // Update UI activity state
            const uatomic_t ui_req = nUIReq;
            if (ui_req != nUIResp)
            {
                if (pPlugin->ui_active())
                    pPlugin->deactivate_ui();
                if ((pUIWrapper != NULL) && (pUIWrapper->ui_active()))
                    pPlugin->activate_ui();
                nUIResp     = ui_req;
            }

            // Bind audio inputs
//            lsp_trace("audio_inputs.count=%d", int(process->audio_inputs_count));
            for (size_t i=0, n=vAudioIn.size(); i<n; ++i)
            {
                audio_group_t *g = vAudioIn.uget(i);
                if (i < process->audio_inputs_count)
                {
                    const clap_audio_buffer_t *b = &process->audio_inputs[i];
//                    lsp_trace("audio_inputs[%d].channels = %d", int(i), int(b->channel_count));
                    for (size_t j=0; j<g->nPorts; ++j)
                    {
                        float *buf = (j < b->channel_count) ? b->data32[j] : NULL;
//                        lsp_trace("audio_inputs[%d].data32[%d] = %p", int(i), int(j), buf);
                        g->vPorts[j]->bind(buf, process->frames_count);
                    }
                }
                else
                {
//                    lsp_trace("audio_inputs[%d].channels = %d", int(i), 0);
                    for (size_t j=0; j<g->nPorts; ++j)
                    {
//                        lsp_trace("audio_inputs[%d].data32[%d] = %p", int(i), int(j), NULL);
                        g->vPorts[j]->bind(NULL, process->frames_count);
                    }
                }
            }

            // Bind audio outputs
//            lsp_trace("audio_outputs.count=%d", int(process->audio_outputs_count));
            for (size_t i=0, n=vAudioOut.size(); i<n; ++i)
            {
                audio_group_t *g = vAudioOut.uget(i);

                if (i < process->audio_outputs_count)
                {
                    const clap_audio_buffer_t *b = &process->audio_outputs[i];
//                    lsp_trace("audio_outputs[%d].channels = %d", int(i), int(b->channel_count));
                    for (size_t j=0; j<g->nPorts; ++j)
                    {
                        float *buf = (j < b->channel_count) ? b->data32[j] : NULL;
//                        lsp_trace("audio_outputs[%d].data32[%d] = %p", int(i), int(j), buf);
                        g->vPorts[j]->bind(buf, process->frames_count);
                    }
                }
                else
                {
//                    lsp_trace("audio_outputs[%d].channels = %d", int(i), 0);
                    for (size_t j=0; j<g->nPorts; ++j)
                    {
//                        lsp_trace("audio_outputs[%d].data32[%d] = %p", int(i), int(j), NULL);
                        g->vPorts[j]->bind(NULL, process->frames_count);
                    }
                }
            }

            // Sync the parameter ports with the UI
            for (size_t i=0, n=vParamPorts.size(); i<n; ++i)
            {
                clap::ParameterPort *port = vParamPorts.uget(i);
                if ((port != NULL) && (port->sync(process->out_events)))
                {
                    lsp_trace("port change from UI: id=%s value=%f", port->metadata()->id, port->value());
                    bUpdateSettings     = true;
                }
            }

            // Need to dump state?
            uatomic_t dump_req      = nDumpReq;
            if (dump_req != nDumpResp)
            {
                dump_plugin_state();
                nDumpResp           = dump_req;
            }

            // CLAP may deliver change of input parameters in the input events.
            // We need to split these events into the set of ranges and process
            // each range independently
            size_t ev_index = 0;
            for (size_t offset=0; offset < process->frames_count; )
            {
//                lsp_trace("offset=%d, frames_count=%d", int(offset), int(process->frames_count));

                // Cleanup stat of input and output MIDI ports
                for (size_t i=0,n=vMidiIn.size(); i<n; ++i)
                    vMidiIn.uget(i)->clear();
                for (size_t i=0,n=vMidiOut.size(); i<n; ++i)
                    vMidiOut.uget(i)->clear();

                // Prepare event block
                size_t block_size = prepare_block(&ev_index, offset, process);
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
                pPlugin->process(block_size);

                // Call the sampler for processing
                if (pSamplePlayer != NULL)
                    pSamplePlayer->process(block_size);

                // Do the post-processing stuff
                generate_output_events(offset, process);
                for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
                    vAllPorts.uget(i)->post_process(block_size);

                // Update the processing offset
                offset     += block_size;
            }

            // Unbind audio ports
            for (size_t i=0, n=vAudioIn.size(); i<n; ++i)
            {
                audio_group_t *g = vAudioIn.uget(i);
                for (size_t j=0; j<g->nPorts; ++j)
                    g->vPorts[j]->unbind();
            }
            for (size_t i=0, n=vAudioOut.size(); i<n; ++i)
            {
                audio_group_t *g = vAudioOut.uget(i);
                for (size_t j=0; j<g->nPorts; ++j)
                    g->vPorts[j]->unbind();
            }

            // Check that latency has changed
            // From CLAP documentation:
            // The latency is only allowed to change if the plugin is deactivated.
            // If the plugin is activated, call host->request_restart()
            // [main thread]
            ssize_t latency = pPlugin->latency();
            if ((latency != nLatency) && (!bLatencyChanged))
            {
                bLatencyChanged       = true;
                pHost->request_callback(pHost);
            }

            // Report the size of the plugin's tail.
            ssize_t tail_size = pPlugin->tail_size();
            if (nTailSize != tail_size)
            {
                nTailSize               = tail_size;
                pExt->tail->changed(pHost);
            }

            return CLAP_PROCESS_CONTINUE;
        }

        const void *Wrapper::get_extension(const char *id)
        {
            return NULL;
        }

        void Wrapper::on_main_thread()
        {
            if (pPlugin->active())
            {
                // Request plugin restart if latency has changed
                // From CLAP documentation:
                // The latency is only allowed to change if the plugin is deactivated.
                // If the plugin is activated, call host->request_restart()
                // [main thread]
                if (bLatencyChanged)
                    pHost->request_restart(pHost);
            }
        }

        size_t Wrapper::latency()
        {
            size_t latency      = pPlugin->latency();
            nLatency            = latency;
            bLatencyChanged     = false;
            return latency;
        }

        size_t Wrapper::has_note_ports() const
        {
            return (vMidiIn.size() + vMidiOut.size()) > 0;
        }

        size_t Wrapper::note_ports_count(bool is_input) const
        {
            return (is_input) ? vMidiIn.size() : vMidiOut.size();
        }

        status_t Wrapper::note_port_info(clap_note_port_info_t *info, size_t index, bool is_input) const
        {
            const char *id  = NULL;
            if (is_input)
            {
                const clap::MidiInputPort *mp = vMidiIn.get(index);
                id = (mp != NULL) ? mp->metadata()->id : NULL;
            }
            else
            {
                const clap::MidiOutputPort *mp = vMidiOut.get(index);
                id = (mp != NULL) ? mp->metadata()->id : NULL;
            }
            if (id == NULL)
                return STATUS_NOT_FOUND;

            info->id                    = clap_hash_string(id);
            info->supported_dialects    = CLAP_NOTE_DIALECT_MIDI;
            info->preferred_dialect     = CLAP_NOTE_DIALECT_MIDI;
            clap_strcpy(info->name, id, sizeof(info->name));

            return STATUS_OK;
        }

        size_t Wrapper::audio_ports_count(bool is_input) const
        {
            return (is_input) ? vAudioIn.size() : vAudioOut.size();
        }

        status_t Wrapper::audio_port_info(clap_audio_port_info_t *info, size_t index, bool is_input) const
        {
            const audio_group_t *grp = (is_input) ? vAudioIn.get(index) : vAudioOut.get(index);
            if (grp == NULL)
                return STATUS_NOT_FOUND;

            info->id            = clap_hash_string(grp->sName);
            clap_strcpy(info->name, grp->sName, sizeof(info->name));
            info->flags         = grp->nFlags;
            info->channel_count = grp->nPorts;
            info->in_place_pair = (grp->nInPlace >= 0) ? grp->nInPlace : CLAP_INVALID_ID;

            return STATUS_OK;
        }

        status_t Wrapper::save_state_work(const clap_ostream_t *os)
        {
            status_t res        = STATUS_OK;

            // Write the header (magic + version)
            if ((res = write_fully(os, uint32_t(LSP_CLAP_MAGIC))) != STATUS_OK)
            {
                lsp_warn("Error serializing header signature, code=%d", int(res));
                return res;
            }
            if ((res = write_fully(os, uint32_t(LSP_CLAP_VERSION))) != STATUS_OK)
            {
                lsp_warn("Error serializing header version, code=%d", int(res));
                return res;
            }

            // Serialize all regular ports
            for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
            {
                // Get VST port
                clap::Port *cp             = vAllPorts.uget(i);
                if (cp == NULL)
                    continue;

                // Get metadata
                const meta::port_t *p      = cp->metadata();
                if ((p == NULL) || (p->id == NULL) || (meta::is_out_port(p)) || (!cp->serializable()))
                    continue;

                // Check that port is serializable
//                lsp_trace("Serializing port id=%s", p->id);

                // Write port identifier
                if ((res = write_string(os, p->id)) != STATUS_OK)
                {
                    lsp_warn("Error serializing port identifier id=%s, code=%d", p->id, int(res));
                    return res;
                }

                // Serialize the port state
                if ((res = cp->serialize(os)) != STATUS_OK)
                {
                    lsp_warn("Error serializing port state id=%s, code=%d", p->id, int(res));
                    return res;
                }
            }

            // Serialize KVT storage
            if (sKVTMutex.lock())
            {
                lsp_finally {
                    sKVT.gc();
                    sKVTMutex.unlock();
                };

                const core::kvt_param_t *p;

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

                    const char *name = it->name();
                    if (name == NULL)
                    {
                        lsp_trace("it->name() returned NULL");
                        break;
                    }

                    uint8_t flags = 0;
                    if (it->is_private())
                        flags      |= clap::FLAG_PRIVATE;

                    kvt_dump_parameter("Saving state of KVT parameter: %s = ", p, name);

                    // Write KVT entry header
                    if ((res = write_string(os, name)) != STATUS_OK)
                    {
                        lsp_warn("Error serializing KVT record name id=%s, code=%d", name, int(res));
                        return res;
                    }
                    if ((res = write_fully(os, flags)) != STATUS_OK)
                    {
                        lsp_warn("Error serializing KVT record flags id=%s, code=%d", name, int(res));
                        return res;
                    }

                    // Serialize parameter according to it's type
                    switch (p->type)
                    {
                        case core::KVT_INT32:
                        {
                            res = write_fully(os, uint8_t(clap::TYPE_INT32));
                            if (res == STATUS_OK)
                                res = write_fully(os, p->i32);
                            break;
                        };
                        case core::KVT_UINT32:
                        {
                            res = write_fully(os, uint8_t(clap::TYPE_UINT32));
                            if (res == STATUS_OK)
                                res = write_fully(os, p->u32);
                            break;
                        }
                        case core::KVT_INT64:
                        {
                            res = write_fully(os, uint8_t(clap::TYPE_INT64));
                            if (res == STATUS_OK)
                                res = write_fully(os, p->i64);
                            break;
                        };
                        case core::KVT_UINT64:
                        {
                            res = write_fully(os, uint8_t(clap::TYPE_UINT64));
                            if (res == STATUS_OK)
                                res = write_fully(os, p->u64);
                            break;
                        }
                        case core::KVT_FLOAT32:
                        {
                            res = write_fully(os, uint8_t(clap::TYPE_FLOAT32));
                            if (res == STATUS_OK)
                                res = write_fully(os, p->f32);
                            break;
                        }
                        case core::KVT_FLOAT64:
                        {
                            res = write_fully(os, uint8_t(clap::TYPE_FLOAT64));
                            if (res == STATUS_OK)
                                res = write_fully(os, p->f64);
                            break;
                        }
                        case core::KVT_STRING:
                        {
                            res = write_fully(os, uint8_t(clap::TYPE_STRING));
                            if (res == STATUS_OK)
                                res = write_string(os, (p->str != NULL) ? p->str : "");
                            break;
                        }
                        case core::KVT_BLOB:
                        {
                            if ((p->blob.size > 0) && (p->blob.data == NULL))
                            {
                                res = STATUS_INVALID_VALUE;
                                break;
                            }

                            res = write_fully(os, uint8_t(clap::TYPE_BLOB));
                            if (res == STATUS_OK)
                                res = write_fully(os, uint32_t(p->blob.size));
                            if (res == STATUS_OK)
                                res = write_string(os, (p->blob.ctype != NULL) ? p->blob.ctype : "");
                            if ((res == STATUS_OK) && (p->blob.size > 0))
                                res = write_fully(os, p->blob.data, p->blob.size);
                            break;
                        }

                        default:
                            res     = STATUS_BAD_TYPE;
                            break;
                    }

                    // Successful status?
                    if (res != STATUS_OK)
                    {
                        lsp_warn("Failed to serialize value id=%s, code=%d", name, int(res));
                        break;
                    }
                }
            }

            return res;
        }

        status_t Wrapper::save_state(const clap_ostream_t *os)
        {
            // Set state management barrier
            bStateManage = true;
            lsp_finally { bStateManage = false; };

            // Trigger the plugin to prepare the internal state
            pPlugin->before_state_save();

            // Do the state save
            status_t res = save_state_work(os);

            // Notify the plugin
            if (res == STATUS_OK)
                pPlugin->state_saved();

            return res;
        }

        // CLAP tail extension
        uint32_t Wrapper::tail_size() const
        {
            ssize_t tail_size = pPlugin->tail_size();
            return (tail_size < 0) ? UINT32_MAX : tail_size;
        }

        clap::Port *Wrapper::find_by_id(const char *id)
        {
            ssize_t first=0, last = vSortedPorts.size() - 1;
            while (first <= last)
            {
                size_t center       = size_t(first + last) >> 1;
                clap::Port *p       = vSortedPorts.uget(center);
                int res             = strcmp(id, p->metadata()->id);
                if (res == 0)
                    return p;
                else if (res < 0)
                    last    = center - 1;
                else
                    first   = center + 1;
            }
            return NULL;
        }

        core::SamplePlayer *Wrapper::sample_player()
        {
            return pSamplePlayer;
        }

        UIWrapper *Wrapper::ui_wrapper()
        {
            return pUIWrapper;
        }

        UIWrapper *Wrapper::create_ui()
        {
            if (pUIWrapper != NULL)
                return pUIWrapper;

            const char *clap_uid = metadata()->uids.clap;
            if (!ui_provided())
            {
                fprintf(stderr, "Not found UI for plugin: %s, will continue in headless mode\n", clap_uid);
                return NULL;
            }

            // Instantiate the plugin UI and return
            ui::Module *ui = static_cast<ui::Factory *>(pUIFactory)->create(pUIMetadata);
            if (ui == NULL)
            {
                fprintf(stderr, "Failed to instantiate UI for plugin id=%s\n", clap_uid);
                return NULL;
            }
            lsp_finally {
                if (ui != NULL)
                {
                    ui->destroy();
                    delete ui;
                }
            };

            // Create wrapper
            UIWrapper *uw = new clap::UIWrapper(ui, this);
            if (uw == NULL)
            {
                fprintf(stderr, "Failed to instantiate UI wrapper plugin id=%s\n", clap_uid);
                return NULL;
            }
            ui  = NULL; // Will be destroyed by wrapper

            status_t res = uw->init(NULL);
            if (res != STATUS_OK)
            {
                uw->destroy();
                delete uw;
                return NULL;
            }

            // Return result
            pUIWrapper = uw;
            return pUIWrapper;
        }

        void Wrapper::destroy_ui()
        {
            if (pUIWrapper == NULL)
                return;

            pUIWrapper->destroy();
            delete pUIWrapper;
            pUIWrapper = NULL;
        }

        bool Wrapper::ui_provided()
        {
            return (pUIMetadata != NULL) && (pUIFactory != NULL);
        }

        void Wrapper::ui_visibility_changed()
        {
            atomic_add(&nUIReq, 1);
        }

        HostExtensions *Wrapper::extensions()
        {
            return pExt;
        }

        status_t Wrapper::read_value(const clap_istream_t *is, const char *name, core::kvt_param_t *p)
        {
            status_t res;
            uint8_t type = 0;

            p->type = core::KVT_ANY;

            // Read the type
            if ((res = read_fully(is, &type)) != STATUS_OK)
            {
                lsp_warn("Failed to read type for port id=%s", name);
                return res;
            }

            lsp_trace("Parameter type: '%c'", char(p->type));

            switch (type)
            {
                case clap::TYPE_INT32:
                    p->type         = core::KVT_INT32;
                    res             = read_fully(is, &p->i32);
                    break;
                case clap::TYPE_UINT32:
                    p->type         = core::KVT_UINT32;
                    res             = read_fully(is, &p->u32);
                    break;
                case clap::TYPE_INT64:
                    p->type         = core::KVT_INT64;
                    res             = read_fully(is, &p->i64);
                    break;
                case clap::TYPE_UINT64:
                    p->type         = core::KVT_UINT64;
                    res             = read_fully(is, &p->u64);
                    break;
                case clap::TYPE_FLOAT32:
                    p->type         = core::KVT_FLOAT32;
                    res             = read_fully(is, &p->f32);
                    break;
                case clap::TYPE_FLOAT64:
                    p->type         = core::KVT_FLOAT64;
                    res             = read_fully(is, &p->f64);
                    break;
                case clap::TYPE_STRING:
                {
                    char *str       = NULL;
                    size_t cap      = 0;

                    p->type         = core::KVT_STRING;
                    p->str          = NULL;
                    res             = read_string(is, &str, &cap);
                    if (res == STATUS_OK)
                        p->str      = str;
                    break;
                }
                case clap::TYPE_BLOB:
                {
                    uint32_t size   = 0;
                    char *ctype     = NULL;
                    uint8_t *data   = NULL;
                    size_t cap      = 0;

                    lsp_finally {
                        if (ctype != NULL)
                            free(ctype);
                        if (data != NULL)
                            free(data);
                    };

                    p->type         = core::KVT_BLOB;
                    p->blob.ctype   = NULL;
                    p->blob.data    = NULL;
                    if ((res = read_fully(is, &size)) != STATUS_OK)
                        return res;
                    lsp_trace("BLOB size: %d (0x%x)", int(size), int(size));
                    if ((res = read_string(is, &ctype, &cap)) != STATUS_OK)
                        return res;
                    lsp_trace("BLOB content type: %s", ctype);

                    if (size > 0)
                    {
                        data            = static_cast<uint8_t *>(malloc(size));
                        if (data == NULL)
                            return STATUS_NO_MEM;
                        if ((res = read_fully(is, data, size)) != STATUS_OK)
                            return res;
                    }

                    p->blob.ctype   = ctype;
                    p->blob.data    = data;
                    p->blob.size    = size;
                    ctype           = NULL;
                    data            = NULL;

                    break;
                }
                default:
                    lsp_warn("Unknown KVT parameter type: %d ('%c') for id=%s", type, type, name);
                    break;
            }

            return res;
        }

        void Wrapper::destroy_value(core::kvt_param_t *p)
        {
            switch (p->type)
            {
                case core::KVT_STRING:
                {
                    if (p->str != NULL)
                    {
                        free(const_cast<char *>(p->str));
                        p->str          = NULL;
                    }
                    break;
                }
                case core::KVT_BLOB:
                {
                    if (p->blob.ctype != NULL)
                    {
                        free(const_cast<char *>(p->blob.ctype));
                        p->blob.ctype   = NULL;
                    }
                    if (p->blob.data != NULL)
                    {
                        free(const_cast<void *>(p->blob.data));
                        p->blob.data    = NULL;
                    }
                    break;
                }
                default:
                    break;
            }

            p->type = core::KVT_ANY;
        }

        status_t Wrapper::load_state_work(const clap_istream_t *is)
        {
            status_t res;
            uint32_t magic = 0, version = 0;

            // Read magic value
            if ((res = read_fully(is, &magic)) != STATUS_OK)
            {
                lsp_warn("Failed to read state header, code=%d", int(res));
                return res;
            }
            lsp_trace("Magic signature: %08lx", int(magic));
            if (magic != clap::LSP_CLAP_MAGIC)
            {
                lsp_warn("Invalid state header signature");
                return STATUS_NO_DATA;
            }
            // Read version
            if ((res = read_fully(is, &version)) != STATUS_OK)
            {
                lsp_warn("Failed to read state version, code=%d", int(res));
                return res;
            }
            lsp_trace("Data version: %d", int(version));
            if (version != clap::LSP_CLAP_VERSION)
            {
                lsp_warn("Unsupported version %d", int(version));
                return STATUS_NO_DATA;
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
                    destroy_value(&p);
                };

                lsp_trace("Parameter name: %s", name);

                if (name[0] != '/')
                {
                    // Obtain the port by it's identifier
                    clap::Port *cp       = find_by_id(name);
                    if (cp != NULL)
                    {
                        if ((res = cp->deserialize(is)) != STATUS_OK)
                        {
                            lsp_warn("Failed to deserialize port id=%s", name);
                            return res;
                        }
                    }
                    else
                    {
                        if ((res = read_value(is, name, &p)) != STATUS_OK)
                        {
                            lsp_warn("Failed to read value for port id=%s", name);
                            return res;
                        }
                        lsp_warn("Missing port id=%s, skipping", name);
                    }
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
                    if ((res = read_value(is, name, &p)) != STATUS_OK)
                    {
                        lsp_warn("Failed to read value for KVT parameter id=%s, code=%d", name, int(res));
                        return res;
                    }

                    // This is KVT port
                    if (p.type != core::KVT_ANY)
                    {
                        size_t kflags = core::KVT_TX;
                        if (flags & clap::FLAG_PRIVATE)
                            kflags     |= core::KVT_PRIVATE;

                        kvt_dump_parameter("Fetched KVT parameter %s = ", &p, name);
                        sKVT.put(name, &p, kflags);
                    }
                }
            }

            // Analyze result
            res = (res == STATUS_EOF) ? STATUS_OK : STATUS_CORRUPTED;
            if (res == STATUS_OK)
                bUpdateSettings = true;

            return res;
        }

        status_t Wrapper::load_state(const clap_istream_t *is)
        {
            // Set state management barrier
            bStateManage = true;
            lsp_finally { bStateManage = false; };

            // Notify plugin that state is about to load
            pPlugin->before_state_load();

            // Do the state load
            status_t res = load_state_work(is);

            // Notify plugin about successful state load
            if (res == STATUS_OK)
                pPlugin->state_loaded();

            return res;
        }

        size_t Wrapper::params_count() const
        {
            return vParamPorts.size();
        }

        status_t Wrapper::param_info(clap_param_info_t *info, size_t index)
        {
            // Get the port and it's metadata
            plug::IPort *p = vParamPorts.get(index);
            if (p == NULL)
            {
                lsp_warn("Port with index=%d not found out of %d ports", int(index), int(vParamPorts.size()));
                return STATUS_NOT_FOUND;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
            {
                lsp_warn("Empty metadata for port index=%d", int(index));
                return STATUS_BAD_STATE;
            }

            // Fill-in parameter flags
            info->id        = clap_hash_string(meta->id);
            info->flags     = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_REQUIRES_PROCESS;
            if (meta::is_discrete_unit(meta->unit) )
                info->flags    |= CLAP_PARAM_IS_STEPPED;
            else if (meta->flags & meta::F_INT)
                info->flags    |= CLAP_PARAM_IS_STEPPED;
            if (meta->flags & meta::F_CYCLIC)
                info->flags    |= CLAP_PARAM_IS_PERIODIC;
// We do not link bypass with host bypass at this moment
//            if (meta->role == meta::R_BYPASS)
//                info->flags    |= CLAP_PARAM_IS_BYPASS;
            info->cookie    = p;
            clap_strcpy(info->name, meta->name, sizeof(info->name));
            info->module[0] = '\0';

            float min = 0.0f, max = 0.0f;
            float dfl = to_clap_value(meta, meta->start, &min, &max);

            info->min_value     = min;
            info->max_value     = max;
            info->default_value = dfl;

            lsp_trace("id=%s, min=%f (0), max=%f (1), dfl=%f (%f)", meta->id, min, max, meta->start, dfl);

            return STATUS_OK;
        }

        clap::ParameterPort *Wrapper::find_param(clap_id param_id)
        {
            ssize_t first=0, last = vParamPorts.size() - 1;
            while (first <= last)
            {
                size_t center       = size_t(first + last) >> 1;
                ParameterPort *p    = vParamPorts.uget(center);
                if (param_id == p->uid())
                    return p;
                else if (param_id < p->uid())
                    last    = center - 1;
                else
                    first   = center + 1;
            }
            return NULL;
        }

        status_t Wrapper::get_param_value(double *value, clap_id param_id)
        {
            // Get the parameter port
            clap::ParameterPort *p = find_param(param_id);
            if (p == NULL)
                return STATUS_NOT_FOUND;
            if (value != NULL)
            {
                float res   = p->value();
                lsp_trace("id=%s, res = %f", p->metadata()->id, res);
                res         = to_clap_value(p->metadata(), res, NULL, NULL);
                lsp_trace("to_clap_value id=%s, res = %f", p->metadata()->id, res);

                *value      = res;
            }

            return STATUS_OK;
        }

        status_t Wrapper::format_param_value(char *buffer, size_t buf_size, clap_id param_id, double value)
        {
            // Get the parameter port
            clap::ParameterPort *p = find_param(param_id);
            if (p == NULL)
                return STATUS_NOT_FOUND;
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return STATUS_BAD_STATE;

            lsp_trace("value = %f", value);
            value   = from_clap_value(meta, value);
            lsp_trace("from_clap_value = %f", value);
            meta::format_value(buffer, buf_size, meta, value, -1, true);
            return STATUS_OK;
        }

        status_t Wrapper::parse_param_value(double *value, clap_id param_id, const char *text)
        {
            // Get the parameter port
            plug::IPort *p = find_param(param_id);
            if (p == NULL)
            {
                lsp_warn("parameter %d not found", int(param_id));
                return STATUS_NOT_FOUND;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
            {
                lsp_warn("metadata for port %p is not present", p);
                return STATUS_BAD_STATE;
            }

            float parsed = 0.0f;
            status_t res = meta::parse_value(&parsed, text, meta, true);
            if (res != STATUS_OK)
            {
                lsp_warn("parse_value for port id=\"%s\" name=\"%s\", text=\"%s\" failed with code %d",
                    meta->id, meta->name, text, int(res));
                return res;
            }

            parsed      = meta::limit_value(meta, parsed);
            lsp_trace("port id=\"%s\" parsed = %f", meta->id, parsed);

            if (value != NULL)
            {
                parsed      = to_clap_value(meta, parsed, NULL, NULL);
                lsp_trace("to_clap_value = %f", parsed);

                *value      = parsed;
            }

            return STATUS_OK;
        }

        void Wrapper::flush_param_events(const clap_input_events_t *in, const clap_output_events_t *out)
        {
            // Now we are ready to process each event
            for (size_t i = 0, n = in->size(in); i < n; ++i)
            {
                // Fetch the event until it's timestamp is not out of the block
                const clap_event_header_t *hdr = in->get(in, i);
                if (hdr->space_id != CLAP_CORE_EVENT_SPACE_ID)
                    continue;

                // Process the event
                switch (hdr->type)
                {
                    case CLAP_EVENT_PARAM_VALUE:
                    {
                        const clap_event_param_value_t *ev = reinterpret_cast<const clap_event_param_value_t *>(hdr);
                        clap::ParameterPort *pp = static_cast<clap::ParameterPort *>(ev->cookie);
                        if ((pp == NULL) || (pp->uid() != ev->param_id))
                            pp              = find_param(ev->param_id);

                        if ((pp != NULL) && (pp->clap_set_value(ev->value)))
                        {
                            lsp_trace("port changed (set): %s, offset=%d", pp->metadata()->id, int(hdr->time));
//                            if (pExt->state != NULL)
//                                pExt->state->mark_dirty(pHost);
                            bUpdateSettings     = true;
                        }
                        break;
                    }
                    case CLAP_EVENT_PARAM_MOD:
                    {
                        const clap_event_param_mod_t *ev = reinterpret_cast<const clap_event_param_mod_t *>(hdr);
                        clap::ParameterPort *pp = static_cast<clap::ParameterPort *>(ev->cookie);
                        if ((pp == NULL) || (pp->uid() != ev->param_id))
                            pp              = find_param(ev->param_id);

                        if ((pp != NULL) && (pp->clap_mod_value(ev->amount)))
                        {
                            lsp_trace("port changed (mod): %s, offset=%d", pp->metadata()->id, int(hdr->time));
//                            if (pExt->state != NULL)
//                                pExt->state->mark_dirty(pHost);
                            bUpdateSettings     = true;
                        }
                        break;
                    }
                    default:
                        break;
                } // switch
            } // for
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

        const meta::package_t *Wrapper::package() const
        {
            return pPackage;
        }

        void Wrapper::state_changed()
        {
            if (bStateManage)
                return;

            if (pExt->state != NULL)
                pExt->state->mark_dirty(pHost);
        }

        void Wrapper::request_state_dump()
        {
            atomic_add(&nDumpReq, 1);
        }

        void Wrapper::request_settings_update()
        {
            bUpdateSettings     = true;
        }

        meta::plugin_format_t Wrapper::plugin_format() const
        {
            return meta::PLUGIN_CLAP;
        }

    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_WRAPPER_H_ */
