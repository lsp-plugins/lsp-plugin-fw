/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
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
            pExt                = NULL;
            nLatency            = 0;

            bRestartRequested   = false;
            bUpdateSettings     = true;
        }

        Wrapper::~Wrapper()
        {
            destroy();
        }

        void Wrapper::destroy()
        {
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

            // Cleanup generated metadata
            for (size_t i=0, n=vGenMetadata.size(); i<n; ++i)
            {
                meta::port_t *port = vGenMetadata.uget(i);
                meta::drop_port_metadata(port);
            }

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
            plug::IPort *ip     = NULL;

            switch (port->role)
            {
                case meta::R_MESH:
                    // TODO
                    break;

                case meta::R_FBUFFER:
                    // TODO
                    break;

                case meta::R_STREAM:
                    // TODO
                    break;

                case meta::R_MIDI:
                    // TODO
                    break;

                case meta::R_AUDIO:
                    ip = new clap::AudioPort(port);
                    break;

                case meta::R_OSC:
                    // TODO
                    break;

                case meta::R_PATH:
                    // TODO
                    break;

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    clap::ParameterPort *pp = new clap::ParameterPort(port);
                    vParamPorts.add(pp);
                    ip  = pp;
                    break;
                }

                case meta::R_METER:
                    // TODO
                    break;

                case meta::R_PORT_SET:
                {
                    LSPString postfix_str;
                    clap::PortGroup *pg      = new clap::PortGroup(port);
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

            if (ip != NULL)
            {
                #ifdef LSP_DEBUG
                    const char *src_id = ip->metadata()->id;
                    for (size_t i=0, n=vAllPorts.size(); i<n; ++i)
                    {
                        plug::IPort *p = vAllPorts.uget(i);
                        if (!strcmp(src_id, p->metadata()->id))
                            lsp_error("ERROR: port %s already defined", src_id);
                    }
                #endif /* LSP_DEBUG */

                vAllPorts.add(ip);
                plugin_ports->add(ip);
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

        Wrapper::audio_group_t *Wrapper::alloc_audio_group(size_t ports)
        {
            // Allocate the audio group object
            size_t szof_group   = sizeof(audio_group_t);
            size_t szof_ports   = sizeof(plug::IPort) * ports;
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
                grp->vPorts[i]  = p;
                if (p != NULL)
                    list->premove(p);
                else
                    lsp_error("Missing %s port '%s' for the group '%s'",
                        (meta->flags & meta::PGF_OUT) ? "output" : "input", id, meta->id);
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
            grp->vPorts[0]      = port;

            return grp;
        }

        status_t Wrapper::generate_audio_port_groups(const meta::plugin_t *meta)
        {
            // Generate mofifiable lists of input and output ports
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
            audio_group_t *in_main = NULL, *out_main = NULL;
            for (const meta::port_group_t *pg = (meta != NULL) ? meta->port_groups : NULL;
                (pg != NULL) && (pg->id != NULL);
                ++pg)
            {
                // Create group and add to list
                audio_group_t *grp  = create_audio_group(pg, &ins, &outs);
                if (grp == NULL)
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

            // Do some optimizations for the host
            if ((in_main != NULL) && (out_main != NULL) && (in_main->nPorts == out_main->nPorts))
            {
                in_main->nInPlace   = vAudioOut.index_of(out_main);
                out_main->nInPlace  = vAudioIn.index_of(in_main);
            }

            // Create the rest input ports
            for (size_t i=0, n=ins.size(); i<n; ++i)
            {
                plug::IPort *p  = ins.uget(i);
                audio_group_t *grp  = create_audio_group(p);
                if (grp == NULL)
                    return STATUS_NO_MEM;
                vAudioIn.add(grp);
            }

            // Create the rest output ports
            for (size_t i=0, n=ins.size(); i<n; ++i)
            {
                plug::IPort *p  = ins.uget(i);
                audio_group_t *grp  = create_audio_group(p);
                if (grp == NULL)
                    return STATUS_NO_MEM;
                vAudioOut.add(grp);
            }

            return STATUS_OK;
        }

        status_t Wrapper::create_ports(const meta::plugin_t *meta)
        {
            // Create ports
            lsp_trace("Creating ports for %s - %s", meta->name, meta->description);
            lltl::parray<plug::IPort> plugin_ports;
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(&plugin_ports, port, NULL);
            vParamPorts.qsort(compare_ports_by_clap_id);

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

        status_t Wrapper::init()
        {
            // Obtain the plugin metadata
            const meta::plugin_t *meta = pPlugin->metadata();
            if (meta == NULL)
                return STATUS_BAD_STATE;

            // Create extensions
            pExt    = new HostExtensions(pHost);
            if (pExt == NULL)
                return STATUS_NO_MEM;

            // Create all possible ports for plugin and validate the state
            LSP_STATUS_ASSERT(create_ports(meta));

            // Generate the input and output audio port groups
            LSP_STATUS_ASSERT(generate_audio_port_groups(meta));


            // TODO
            return STATUS_OK;
        }

        status_t Wrapper::activate(double sample_rate, size_t min_frames_count, size_t max_frames_count)
        {
            // Clear the flag that the restart has been requested
            bRestartRequested   = false;
            bUpdateSettings     = true;

            if (sample_rate > MAX_SAMPLE_RATE)
            {
                lsp_warn(
                    "Unsupported sample rate: %f, maximum supported sample rate is %ld",
                    sample_rate,
                    long(MAX_SAMPLE_RATE));
                sample_rate  = MAX_SAMPLE_RATE;
            }
            pPlugin->set_sample_rate(sample_rate);

            return STATUS_OK;
        }

        void Wrapper::deactivate()
        {
            // Set the actual latency
            nLatency            = pPlugin->latency();
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

        clap_process_status Wrapper::process(const clap_process_t *process)
        {
            // Report the latency
            // From CLAP documentation:
            // The latency is only allowed to change if the plugin is deactivated.
            // If the plugin is activated, call host->request_restart()
            ssize_t latency = pPlugin->latency();
            if ((latency != nLatency) && (!bRestartRequested))
            {
                pHost->request_restart(pHost);
                bRestartRequested       = true;
            }

            return CLAP_PROCESS_CONTINUE;
        }

        const void *Wrapper::get_extension(const char *id)
        {
            return NULL;
        }

        void Wrapper::on_main_thread()
        {
        }

        size_t Wrapper::latency() const
        {
            return nLatency;
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

            info->id            = index;
            clap_strcpy(info->name, grp->sName, sizeof(info->name));
            info->flags         = grp->nFlags;
            info->channel_count = grp->nPorts;
            info->in_place_pair = (grp->nInPlace >= 0) ? grp->nInPlace : CLAP_INVALID_ID;

            return STATUS_OK;
        }

        status_t Wrapper::save_state(const clap_ostream_t *stream)
        {
            return STATUS_OK;
        }

        status_t Wrapper::load_state(const clap_istream_t *stream)
        {
            return STATUS_OK;
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
                return STATUS_NOT_FOUND;
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return STATUS_BAD_STATE;

            // Fill-in parameter flags
            info->id        = clap_hash_string(meta->id);
            info->flags     = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_REQUIRES_PROCESS;
            if (meta::is_discrete_unit(meta->unit) )
                info->flags    |= CLAP_PARAM_IS_STEPPED;
            else if (meta->flags & meta::F_INT)
                info->flags    |= CLAP_PARAM_IS_STEPPED;
            if (meta->flags & meta::F_CYCLIC)
                info->flags    |= CLAP_PARAM_IS_PERIODIC;
            if (meta->role == meta::R_BYPASS)
                info->flags    |= CLAP_PARAM_IS_BYPASS;
            info->cookie    = p;
            clap_strcpy(info->name, meta->name, sizeof(info->name));
            info->module[0] = '\0';

            float min = 0.0f, max = 0.0f;
            meta::get_port_parameters(meta, &min, &max, NULL);

            info->min_value     = min;
            info->max_value     = max;
            info->default_value = meta->start;

            return STATUS_NOT_IMPLEMENTED;
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
                *value      = p->value();

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

            meta::format_value(buffer, buf_size, meta, value, -1, false);
            return STATUS_OK;
        }

        status_t Wrapper::parse_param_value(double *value, clap_id param_id, const char *text)
        {
            // Get the parameter port
            plug::IPort *p = find_param(param_id);
            if (p == NULL)
                return STATUS_NOT_FOUND;
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return STATUS_BAD_STATE;

            float parsed = 0.0f;
            status_t res = meta::parse_value(&parsed, text, meta, true);
            if (res != STATUS_OK)
                return res;

            if (value != NULL)
                *value  = meta::limit_value(meta, parsed);

            return STATUS_OK;
        }

        void Wrapper::flush_param_events(const clap_input_events_t *in, const clap_output_events_t *out)
        {
            // TODO: process events
        }

    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_WRAPPER_H_ */
