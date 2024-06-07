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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/io/IInStream.h>
#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/factory.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/wrapper.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace gst
    {
        Factory::Factory()
        {
            pLoader         = NULL;
            pPackage        = NULL;
            nReferences     = 1;
            nRefExecutor    = 0;
            pExecutor       = NULL;
        }

        Factory::~Factory()
        {
            destroy();
        }

        void Factory::destroy()
        {
            // Shutdown executor
            if (pExecutor != NULL)
            {
                pExecutor->shutdown();
                delete pExecutor;
            }

            // Forget the package
            if (pPackage != NULL)
            {
                meta::free_manifest(pPackage);
                pPackage    = NULL;
            }

            // Release resource loader
            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader     = NULL;
            }
        }

        status_t Factory::init()
        {
            // Create resource loader
            pLoader             = core::create_resource_loader();
            if (pLoader == NULL)
            {
                lsp_error("No resource loader available");
                return STATUS_BAD_STATE;
            }

            // Obtain the manifest
            lsp_trace("Obtaining manifest...");

            status_t res;
            meta::package_t *manifest = NULL;
            io::IInStream *is = pLoader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
            if (is != NULL)
            {
                lsp_finally {
                    is->close();
                    delete is;
                };

                if ((res = meta::load_manifest(&manifest, is)) != STATUS_OK)
                {
                    lsp_warn("Error loading manifest file, error=%d", int(res));
                    manifest = NULL;
                }
            }
            if (manifest == NULL)
            {
                lsp_trace("No manifest file found");
                return STATUS_BAD_STATE;
            }
            lsp_finally {
                meta::free_manifest(manifest);
            };

            // Remember the package
            pPackage    = release_ptr(manifest);

            return STATUS_OK;
        }

        atomic_t Factory::acquire()
        {
            return atomic_add(&nReferences, 1) + 1;
        }

        atomic_t Factory::release()
        {
            const atomic_t ref_count = atomic_add(&nReferences, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        ipc::IExecutor *Factory::acquire_executor()
        {
            lsp_trace("this=%p", this);

            if (!sMutex.lock())
                return NULL;
            lsp_finally { sMutex.unlock(); };

            // Try to perform quick access
            if (pExecutor != NULL)
            {
                ++nRefExecutor;
                return pExecutor;
            }

            // Create executor
            ipc::NativeExecutor *executor = new ipc::NativeExecutor();
            if (executor == NULL)
                return NULL;
            lsp_trace("Allocated executor=%p", executor);

            // Launch executor
            status_t res = executor->start();
            if (res != STATUS_OK)
            {
                lsp_trace("Failed to start executor=%p, code=%d", executor, int(res));
                delete executor;
                return NULL;
            }

            // Update status
            ++nRefExecutor;
            return pExecutor = executor;
        }

        void Factory::release_executor()
        {
            lsp_trace("this=%p", this);

            if (!sMutex.lock())
                return;
            lsp_finally { sMutex.unlock(); };

            if ((--nRefExecutor) > 0)
                return;
            if (pExecutor == NULL)
                return;

            lsp_trace("Destroying executor pExecutor=%p", pExecutor);
            pExecutor->shutdown();
            delete pExecutor;
            pExecutor   = NULL;
        }

        const meta::package_t *Factory::package() const
        {
            return pPackage;
        }

        const meta::plugin_t *Factory::find_plugin(const char *id)
        {
            // Lookup plugin factories
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *plug_meta = f->enumerate(i);
                    if (plug_meta == NULL)
                        break;
                    if (plug_meta->gst_uid == NULL)
                        continue;
                    if (strcmp(plug_meta->gst_uid, id) == 0)
                        return plug_meta;
                }
            }

            return NULL;
        }

        plug::Module *Factory::create_plugin(const char *id)
        {
            // Lookup plugin factories
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *plug_meta = f->enumerate(i);
                    if (plug_meta == NULL)
                        break;
                    if (plug_meta->gst_uid == NULL)
                        continue;
                    if (strcmp(plug_meta->gst_uid, id) == 0)
                        return f->create(plug_meta);
                }
            }

            return NULL;
        }

        void Factory::destroy_enumeration(enumeration_t *en)
        {
            for (size_t i=0, n=en->generated.size(); i<n; ++i)
                meta::drop_port_metadata(en->generated.uget(i));

            en->inputs.flush();
            en->outputs.flush();
            en->params.flush();
            en->generated.flush();
        }

        bool Factory::enumerate_port(enumeration_t *en, const meta::port_t *port, const char *postfix)
        {
            switch (port->role)
            {
                case meta::R_AUDIO_IN:
                {
                    en->inputs.add(const_cast<meta::port_t *>(port));
                    lsp_trace("audio_in id=%s", port->id);
                    break;
                }
                case meta::R_AUDIO_OUT:
                {
                    en->outputs.add(const_cast<meta::port_t *>(port));
                    lsp_trace("audio_out id=%s", port->id);
                    break;
                }
                case meta::R_CONTROL:
                case meta::R_BYPASS:
                case meta::R_METER:
                case meta::R_PATH:
                {
                    en->params.add(const_cast<meta::port_t *>(port));
                    lsp_trace("parameter id=%s, index=%d", port->id, int(en->params.size() - 1));
                    break;
                }

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];

                    // Add Port Set immediately
                    en->params.add(const_cast<meta::port_t *>(port));
                    lsp_trace("port_set id=%s, index=%d", port->id, int(en->params.size() - 1));

                    // Generate nested ports
                    const size_t rows   = meta::list_size(port->items);
                    for (size_t row=0; row < rows; ++row)
                    {
                        // Generate postfix
                        snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                        // Clone port metadata
                        meta::port_t *cm        = meta::clone_port_metadata(port->members, postfix_buf);
                        if (cm == NULL)
                            return false;

                        en->generated.add(cm);
                        size_t col          = 0;
                        for (; cm->id != NULL; ++cm, ++col)
                        {
                            if (meta::is_growing_port(cm))
                                cm->start    = cm->min + ((cm->max - cm->min) * row) / float(rows);
                            else if (meta::is_lowering_port(cm))
                                cm->start    = cm->max - ((cm->max - cm->min) * row) / float(rows);

                            // Recursively generate new ports associated with the port set
                            if (!enumerate_port(en, cm, postfix_buf))
                                return false;
                        }
                    }

                    break;
                }

                // Not supported by GStreamer, skip
                case meta::R_MESH:
                case meta::R_STREAM:
                case meta::R_FBUFFER:
                case meta::R_MIDI_IN:
                case meta::R_MIDI_OUT:
                case meta::R_OSC_IN:
                case meta::R_OSC_OUT:
                default:
                {
                    lsp_trace("stub id=%s", port->id);
                    break;
                }
            }

            return true;
        }

        const meta::port_group_t *Factory::find_main_group(const meta::plugin_t *plug, bool in)
        {
            size_t direction = (in) ? meta::PGF_IN : meta::PGF_OUT;

            for (const meta::port_group_t *pg = plug->port_groups; (pg != NULL) && (pg->id != NULL); ++pg)
            {
                if (!(pg->flags & meta::PGF_MAIN))
                    continue;

                if ((pg->flags & meta::PGF_OUT) == direction)
                    return pg;
            }

            return NULL;
        }

        bool Factory::is_canonical_gst_name(const char *name)
        {
            return strchr(name, '_') == NULL;
        }

        const char *Factory::make_canonical_gst_name(char *buf, const char *name)
        {
            size_t i = 0;
            char ch;
            do
            {
                ch = *(name++);
                if (i >= MAX_PARAM_ID_BYTES)
                    return name;
                buf[i++] = (ch == '_') ? '-' : ch;
            } while (ch != '\0');

            return buf;
        }

        void Factory::init_class(GstAudioFilterClass *klass, const char *plugin_id)
        {
            // Find the plugin
            const meta::plugin_t *meta = find_plugin(plugin_id);
            if (meta == NULL)
                return;

            GstElementClass *element_class = reinterpret_cast<GstElementClass *>(klass);
            GObjectClass *gobject_class = reinterpret_cast<GObjectClass *>(klass);

            gst_element_class_set_details_simple(
                element_class,
                meta->description,
                "Filter/Effect/Audio",
                meta->bundle->description,
                meta->developer->name);

            // Enumerate plugin ports
            enumeration_t en;
            lsp_finally { destroy_enumeration(&en); };

            for (const meta::port_t *port = meta->ports; (port != NULL) && (port->id != NULL); ++port)
            {
                if (!enumerate_port(&en, port, NULL))
                    return;
            }

            // Create sink pad
            {
                const meta::port_group_t *main_in = find_main_group(meta, true);
                const gint min_count = (main_in != NULL) ? meta::port_count(main_in) : en.inputs.size();
                const gint max_count = en.inputs.size();

                lsp_trace("Creating sink pad channels=[%d, %d]", int(min_count), int(max_count));

                GstCaps *caps = gst_caps_new_empty();
                lsp_finally {
                    gst_caps_unref(caps);
                };

                GstStructure *gsts = gst_structure_new(
                    "audio/x-raw",
                    "format", G_TYPE_STRING, GST_AUDIO_NE(F32),
                    "rate", GST_TYPE_INT_RANGE, 1, G_MAXINT,
                    "layout", G_TYPE_STRING, "interleaved", NULL);

                if (min_count >= max_count)
                    gst_structure_set(gsts, "channels", G_TYPE_INT, min_count, NULL);
                else
                    gst_structure_set(gsts, "channels", GST_TYPE_INT_RANGE, min_count, max_count, NULL);
                gst_caps_append_structure(caps, gst_structure_copy(gsts));

                gst_structure_set(gsts, "layout", G_TYPE_STRING, "non_interleaved", NULL);
                gst_caps_append_structure(caps, gsts);

                GstPadTemplate *pad_template = gst_pad_template_new(
                    GST_BASE_TRANSFORM_SINK_NAME, GST_PAD_SINK, GST_PAD_ALWAYS,
                    caps);

                gst_element_class_add_pad_template(element_class, pad_template);
            }

            // Create source pad
            {
                const meta::port_group_t *main_out = find_main_group(meta, false);
                const gint min_count = (main_out != NULL) ? meta::port_count(main_out) : en.outputs.size();
                const gint max_count = en.outputs.size();

                lsp_trace("Creating source pad channels=[%d, %d]", int(min_count), int(max_count));

                GstCaps *caps = gst_caps_new_empty();
                lsp_finally {
                    gst_caps_unref(caps);
                };

                GstStructure *gsts = gst_structure_new(
                    "audio/x-raw",
                    "format", G_TYPE_STRING, GST_AUDIO_NE(F32),
                    "rate", GST_TYPE_INT_RANGE, 1, G_MAXINT,
                    "layout", G_TYPE_STRING, "interleaved", NULL);

                if (min_count >= max_count)
                    gst_structure_set(gsts, "channels", G_TYPE_INT, min_count, NULL);
                else
                    gst_structure_set(gsts, "channels", GST_TYPE_INT_RANGE, min_count, max_count, NULL);
                gst_caps_append_structure(caps, gst_structure_copy(gsts));

                gst_structure_set(gsts, "layout", G_TYPE_STRING, "non_interleaved", NULL);
                gst_caps_append_structure(caps, gsts);

                GstPadTemplate *pad_template = gst_pad_template_new(
                    GST_BASE_TRANSFORM_SRC_NAME, GST_PAD_SRC, GST_PAD_ALWAYS,
                    caps);

                gst_element_class_add_pad_template(element_class, pad_template);
            }

            // Install properties
            LSPString tmp;
            char port_name[MAX_PARAM_ID_BYTES];

            for (size_t i=0, n=en.params.size(); i < n; ++i)
            {
                const meta::port_t *port = en.params.uget(i);
                if (port == NULL)
                    continue;

                const int param_id = i + 1;

                int param_flags = G_PARAM_READABLE | G_PARAM_STATIC_NICK;
                if (meta::is_in_port(port))
                    param_flags        |= G_PARAM_WRITABLE;

                const char *name    = port->id;
                if (is_canonical_gst_name(name))
                    param_flags        |= G_PARAM_STATIC_NAME;
                else
                    name                = make_canonical_gst_name(port_name, port->id);

                const char *blurb   = port->name;
                const char *unit    = meta::get_unit_name(port->unit);
                if ((unit != NULL) && (strlen(unit) > 0))
                {
                    tmp.fmt_utf8("%s [%s]", port->name, unit);
                    blurb               = tmp.get_native();
                }
                else
                    param_flags        |= G_PARAM_STATIC_BLURB;

                GParamSpec * spec = NULL;

                if (meta::is_bool_unit(port->unit))
                {
                    lsp_trace("[%d] g_param_spec_boolean(%s, %s, %s, %s)",
                        param_id, name, port->name, blurb, (port->start >= 0.5f) ? "true" : "false");
                    spec = g_param_spec_boolean(
                        name, port->name, blurb,
                        (port->start >= 0.5f),
                        GParamFlags(param_flags));
                }
                else if (meta::is_discrete_unit(port->unit))
                {
                    if (meta::is_enum_unit(port->unit))
                    {
                        const int min = int(port->min);
                        const int max = min + meta::list_size(port->items) - 1;

                        lsp_trace("[%d] g_param_spec_float(%s, %s, %s, %f, %f, %f)",
                            param_id, name, port->name, blurb, min, max, port->start);
                        spec = g_param_spec_int(
                            name, port->name, blurb,
                            min, max, port->start,
                            GParamFlags(param_flags));
                    }
                    else
                    {
                        int min = port->min;
                        int max = port->max;
                        if (min > max)
                            lsp::swap(min, max);

                        lsp_trace("[%d] g_param_spec_int(%s, %s, %s, %d, %d, %d)",
                            param_id, name, port->name, blurb, min, max, int(port->start));
                        spec = g_param_spec_int(
                            name, port->name, blurb,
                            min, max, int(port->start),
                            GParamFlags(param_flags));
                    }
                }
                else if (meta::is_path_port(port))
                {
                    lsp_trace("[%d] g_param_spec_string(%s, %s, %s, \"\")",
                        param_id, name, port->name, blurb);
                    spec = g_param_spec_string(
                        name, port->name, blurb,
                        "",
                        GParamFlags(param_flags));
                }
                else
                {
                    float min = port->min;
                    float max = port->max;
                    if (min > max)
                        lsp::swap(min, max);

                    lsp_trace("[%d] g_param_spec_float(%s, %s, %s, %f, %f, %f)",
                        param_id, name, port->name, blurb, min, max, port->start);
                    spec = g_param_spec_float(
                        name, port->name, blurb,
                        min, max, port->start,
                        GParamFlags(param_flags));
                }

                g_object_class_install_property(gobject_class, param_id, spec);
            }
        }

        gst::IWrapper *Factory::instantiate(const char *plugin_id, GstAudioFilter *filter)
        {
            // Find and instantiate the plugin
            plug::Module *plugin = create_plugin(plugin_id);
            if (plugin == NULL)
                return NULL;
            lsp_finally {
                if (plugin != NULL)
                {
                    plugin->destroy();
                    delete plugin;
                }
            };

            // Create the wrapper
            gst::Wrapper *wrapper = new gst::Wrapper(this, filter, plugin, pLoader);
            if (wrapper == NULL)
                return NULL;
            plugin  = NULL;     // Will be destroyed by the wrapper

            // Initialize wrapper
            lsp::status_t res   = wrapper->init();
            if (res != lsp::STATUS_OK)
            {
                delete wrapper;
                return NULL;
            }

            return wrapper;
        }

    } /* namespace gst */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_IMPL_FACTORY_H_ */
