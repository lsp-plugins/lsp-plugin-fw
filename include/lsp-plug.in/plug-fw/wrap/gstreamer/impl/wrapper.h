/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 25 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/helpers.h>

#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace gst
    {
        Wrapper::Wrapper(gst::Factory *factory, plug::Module *plugin, resource::ILoader *loader):
            IWrapper(plugin, loader)
        {
            pFactory            = safe_acquire(pFactory);
            pExecutor           = NULL;
            bUpdateSettings     = true;
            pPackage            = NULL;
            nOldLatency         = -1;
            nNewLatency         = 0;
        }

        Wrapper::~Wrapper()
        {
            destroy();
        }

        void Wrapper::destroy()
        {
            // Release executor
            if (pExecutor != NULL)
            {
                pExecutor->shutdown();
                pFactory->release_executor();

                delete pExecutor;
                pExecutor = NULL;
            }

            // Now we are able to destroy the plugin
            if (pPlugin != NULL)
            {
                pPlugin->destroy();
                delete pPlugin;
                pPlugin = NULL;
            }

            // Cleanup ports
            for (size_t i=0; i < vAllPorts.size(); ++i)
            {
                lsp_trace("destroy port id=%s", vAllPorts[i]->metadata()->id);
                delete vAllPorts[i];
            }
            vAllPorts.flush();
            vAudioIn.flush();
            vAudioOut.flush();
            vParameters.flush();
            vMeters.flush();
            vPortMapping.flush();

            // Cleanup generated metadata
            for (size_t i=0; i<vGenMetadata.size(); ++i)
            {
                lsp_trace("destroy generated port metadata %p", vGenMetadata[i]);
                drop_port_metadata(vGenMetadata[i]);
            }
            vGenMetadata.flush();

            // Release factory
            safe_release(pFactory);
        }

        gst::AudioPort *Wrapper::find_port(lltl::parray<gst::AudioPort> & list, const char *id)
        {
            for (size_t i=0, n=list.size(); i<n; ++i)
            {
                gst::AudioPort *p = list.uget(i);
                if (p == NULL)
                    continue;
                const meta::port_t *meta = p->metadata();
                if ((meta == NULL) || (meta->id == NULL))
                    continue;

                if (strcmp(meta->id, id) == 0)
                    return p;
            }

            return NULL;
        }

        ssize_t Wrapper::compare_port_items(const meta::port_group_item_t *a, const meta::port_group_item_t *b)
        {
            return a->role - b->role;
        }

        void Wrapper::make_port_group_mapping(
            lltl::parray<gst::AudioPort> & sink,
            lltl::parray<gst::AudioPort> & list,
            const meta::port_group_t *grp)
        {
            // Fill list of ports and sort by the role
            lltl::parray<meta::port_group_item_t> ports;
            for (const meta::port_group_item_t *item = grp->items; (item != NULL) && (item->id != NULL); ++item)
                ports.add(const_cast<meta::port_group_item_t *>(item));
            ports.qsort(compare_port_items);

            // Add new unique ports to the output list
            for (size_t i=0, n=ports.size(); i<n; ++i)
            {
                const meta::port_group_item_t *item = ports.uget(i);
                if (item == NULL)
                    continue;

                gst::AudioPort *p = find_port(list, item->id);
                if (p == NULL)
                    continue;
                if (!sink.contains(p))
                    sink.add(p);
            }
        }

        void Wrapper::make_port_mapping(
            lltl::parray<gst::AudioPort> & sink,
            lltl::parray<gst::AudioPort> & list)
        {
            for (size_t i=0, n=list.size(); i<n; ++i)
            {
                gst::AudioPort *p = list.uget(i);
                if (p == NULL)
                    continue;

                if (!sink.contains(p))
                    sink.add(p);
            }
        }

        void Wrapper::make_audio_mapping(
            lltl::parray<gst::AudioPort> & sink,
            lltl::parray<gst::AudioPort> & list,
            const meta::plugin_t *meta,
            bool out)
        {
            const int grp_flags = (out) ? meta::PGF_OUT : meta::PGF_IN;

            // Find main group and emit ports for it
            const meta::port_group_t *main = NULL;
            for (const meta::port_group_t *grp = meta->port_groups; (grp != NULL) && (grp->id != NULL); ++grp)
            {
                if ((grp->flags & meta::PGF_OUT) != grp_flags)
                    continue;
                if (grp->flags & meta::PGF_MAIN)
                {
                    main = grp;
                    make_port_group_mapping(sink, list, grp);
                    break;
                }
            }

            // Now iterate over all groups and make mapping
            for (const meta::port_group_t *grp = meta->port_groups; (grp != NULL) && (grp->id != NULL); ++grp)
            {
                if ((grp->flags & meta::PGF_OUT) != grp_flags)
                    continue;
                if (grp != main)
                {
                    make_port_group_mapping(sink, list, grp);
                    break;
                }
            }

            // Make mapping for non-assigned ports
            make_port_mapping(sink, list);
        }

        status_t Wrapper::init()
        {
            const meta::plugin_t *meta = pPlugin->metadata();

            // Create all possible ports for plugin
            lltl::parray<plug::IPort> plugin_ports;
            lsp_trace("Creating ports for %s - %s", meta->name, meta->description);
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(&plugin_ports, port, NULL);

            // Create audio port mapping
            make_audio_mapping(vSinkIn, vAudioIn, meta, false);
            make_audio_mapping(vSinkOut, vAudioOut, meta, true);

            // Create executor service
            lsp_trace("Creating executor service");
            ipc::IExecutor *executor    = pFactory->acquire_executor();
            if (executor != NULL)
            {
                // Create wrapper around native executor
                pExecutor           = new gst::Executor(executor);
                if (pExecutor == NULL)
                {
                    pFactory->release_executor();
                    return STATUS_NO_MEM;
                }
            }

            return STATUS_OK;
        }

        plug::IPort *Wrapper::create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix)
        {
            plug::IPort *result = NULL;

            switch (port->role)
            {
                case meta::R_AUDIO_IN:
                {
                    gst::AudioPort *p = new gst::AudioPort(port);
                    result = p;
                    vAudioIn.add(p);
                    plugin_ports->add(p);

                    lsp_trace("audio_in id=%s", result->metadata()->id);
                    break;
                }
                case meta::R_AUDIO_OUT:
                {
                    gst::AudioPort *p = new gst::AudioPort(port);
                    result = p;
                    vAudioIn.add(p);
                    plugin_ports->add(p);

                    lsp_trace("audio_out id=%s", result->metadata()->id);
                    break;
                }
                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    gst::ParameterPort *p = new gst::ParameterPort(port);
                    result = p;
                    vParameters.add(p);
                    vPortMapping.add(p);
                    plugin_ports->add(p);

                    lsp_trace("control id=%s, index=%d", result->metadata()->id, int(vPortMapping.size() - 1));
                    break;
                }
                case meta::R_METER:
                {
                    gst::MeterPort *p = new gst::MeterPort(port);
                    result = p;
                    vMeters.add(p);
                    vPortMapping.add(p);
                    plugin_ports->add(p);

                    lsp_trace("meter id=%s, index=%d", result->metadata()->id, int(vPortMapping.size() - 1));
                    break;
                }

                case meta::R_PATH:
                {
                    gst::PathPort *p = new gst::PathPort(port);
                    result = p;
                    vPortMapping.add(p);
                    plugin_ports->add(p);

                    lsp_trace("path id=%s, index=%d", result->metadata()->id, int(vPortMapping.size() - 1));
                    break;
                }

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];
                    gst::PortGroup   *pg    = new gst::PortGroup(port);

                    // Add Port Set immediately
                    vAllPorts.add(pg);
                    vParameters.add(pg);
                    vPortMapping.add(pg);
                    plugin_ports->add(pg);

                    // Generate nested ports
                    for (size_t row=0; row<pg->rows(); ++row)
                    {
                        // Generate postfix
                        snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                        // Clone port metadata
                        meta::port_t *cm        = meta::clone_port_metadata(port->members, postfix_buf);
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
                            create_port(plugin_ports, cm, postfix_buf);
                        }
                    }

                    break;
                }

                // Not supported by GStreamer, make it as stub ports
                case meta::R_MESH:
                case meta::R_STREAM:
                case meta::R_FBUFFER:
                case meta::R_MIDI_IN:
                case meta::R_MIDI_OUT:
                case meta::R_OSC_IN:
                case meta::R_OSC_OUT:
                default:
                {
                    result  = new plug::IPort(port);
                    plugin_ports->add(result);
                    lsp_trace("stub id=%s", result->metadata()->id);
                    break;
                }
            }

            // Add the created port to complete list of ports
            if (result != NULL)
                vAllPorts.add(result);

            return result;
        }

        ipc::IExecutor *Wrapper::executor()
        {
            return pExecutor;
        }

        const meta::package_t *Wrapper::package() const
        {
            return (pFactory != NULL) ? pFactory->package() : NULL;
        }

        void Wrapper::request_settings_update()
        {
            bUpdateSettings     = true;
        }

        meta::plugin_format_t Wrapper::plugin_format() const
        {
            return meta::PLUGIN_GSTREAMER;
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

        void Wrapper::set_property(guint prop_id, const GValue *value, GParamSpec *pspec)
        {
            if (prop_id-- < 1)
                return;
            if (prop_id >= vPortMapping.size())
                return;

            plug::IPort *pp = vPortMapping.uget(prop_id);
            if (pp == NULL)
                return;

            const meta::port_t *meta = pp->metadata();
            if (meta == NULL)
                return;

            switch (meta->role)
            {
                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    gst::ParameterPort *p   = static_cast<gst::ParameterPort *>(pp);
                    float xv = 0.0f;

                    if (meta::is_bool_unit(meta->role))
                        xv = g_value_get_boolean(value) ? 1.0f : 0.0f;
                    else if (meta::is_discrete_unit(meta->role))
                        xv = g_value_get_int(value);
                    else
                        xv = g_value_get_float(value);

                    if (p->submit_value(xv))
                        bUpdateSettings = true;
                    break;
                }

                case meta::R_METER:
                {
                    lsp_warn("Attempt to set read-only port id=%s (index=%d)", meta->id, int(prop_id));
                    break;
                }

                case meta::R_PATH:
                {
                    gst::PathPort *p        = static_cast<gst::PathPort *>(pp);
                    const gchar *xv         = g_value_get_string(value);
                    LSPString path;
                    if (!path.set_native(reinterpret_cast<const char *>(xv)))
                    {
                        lsp_warn("Failed to parse native string for port id=%s (index=%d)", meta->id, int(prop_id));
                        return;
                    }

                    const char *xpath       = path.get_utf8();
                    if (xpath == NULL)
                        return;

                    p->submit(xpath, 0);
                    bUpdateSettings = true;

                    break;
                }
                default:
                    lsp_warn("Could not set port id=%s (index=%d): unsupported operation", meta->id, int(prop_id));
                    break;
            }
        }

        void Wrapper::get_property(guint prop_id, GValue * value, GParamSpec * pspec)
        {
            if (prop_id-- < 1)
                return;
            if (prop_id >= vPortMapping.size())
                return;

            plug::IPort *pp = vPortMapping.uget(prop_id);
            if (pp == NULL)
                return;

            const meta::port_t *meta = pp->metadata();
            if (meta == NULL)
                return;

            switch (meta->role)
            {
                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    gst::ParameterPort *p   = static_cast<gst::ParameterPort *>(pp);
                    float xv                = p->value();

                    if (meta::is_bool_unit(meta->role))
                        g_value_set_boolean(value, xv >= 0.5f);
                    else if (meta::is_discrete_unit(meta->role))
                        g_value_set_int(value, gint(xv));
                    else
                        g_value_set_float(value, xv);

                    break;
                }

                case meta::R_METER:
                {
                    gst::MeterPort *p       = static_cast<gst::MeterPort *>(pp);
                    float xv                = p->value();

                    g_value_set_float(value, xv);
                    break;
                }

                case meta::R_PATH:
                {
                    gst::PathPort *p        = static_cast<gst::PathPort *>(pp);

                    LSPString path;
                    path.set_utf8(p->get());

                    const gchar *xv         = reinterpret_cast<const gchar *>(path.get_native());
                    g_value_set_string(value, xv);

                    break;
                }
                default:
                    lsp_warn("Could not get port id=%s (index=%d): unsupported operation", meta->id, int(prop_id));
                    break;
            }
        }

        void Wrapper::setup(const GstAudioInfo * info)
        {
            // TODO
        }

        void Wrapper::change_state(GstStateChange transition)
        {
            // TODO
        }

        void Wrapper::process(guint8 *out, const guint8 *in, size_t out_size, size_t in_size)
        {
            // TODO
        }

    } /* namespace gst */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_WRAPPER_H_ */


