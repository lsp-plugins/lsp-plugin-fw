/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_IMPL_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_IMPL_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/static.h>

namespace lsp
{
    namespace lv2
    {
        //---------------------------------------------------------------------
        ssize_t UIWrapper::compare_ports_by_urid(const lv2::UIPort *a, const lv2::UIPort *b)
        {
            return ssize_t(a->get_urid()) - ssize_t(b->get_urid());
        }

        ssize_t UIWrapper::compare_abstract_ports_by_urid(const ui::IPort *a, const ui::IPort *b)
        {
            return compare_ports_by_urid(
                static_cast<const lv2::UIPort *>(a),
                static_cast<const lv2::UIPort *>(b)
            );
        }

        lv2::UIPort *UIWrapper::find_by_urid(lltl::parray<lv2::UIPort> &v, LV2_URID urid)
        {
            // Try to find the corresponding port
            ssize_t first = 0, last = v.size() - 1;
            while (first <= last)
            {
                size_t center   = (first + last) >> 1;
                lv2::UIPort *p  = v[center];
                if (urid == p->get_urid())
                    return p;
                else if (urid < p->get_urid())
                    last    = center - 1;
                else
                    first   = center + 1;
            }
            return NULL;
        }

        lv2::UIPort *UIWrapper::find_by_urid(lltl::parray<ui::IPort> &v, LV2_URID urid)
        {
            // Try to find the corresponding port
            ssize_t first = 0, last = v.size() - 1;
            while (first <= last)
            {
                size_t center   = (first + last) >> 1;
                lv2::UIPort *p  = static_cast<lv2::UIPort *>(v[center]);
                if (urid == p->get_urid())
                    return p;
                else if (urid < p->get_urid())
                    last    = center - 1;
                else
                    first   = center + 1;
            }
            return NULL;
        }

        //---------------------------------------------------------------------
        UIWrapper::UIWrapper(ui::Module *ui, resource::ILoader *loader, lv2::Extensions *ext): ui::IWrapper(ui, loader)
        {
            pExt        = ext;
            nLatencyID  = 0;
            pLatency    = NULL;
            bConnected  = false;
            pOscBuffer  = NULL;
            pPackage    = NULL;
        }

        UIWrapper::~UIWrapper()
        {
            pUI         = NULL;
            pExt        = NULL;
            nLatencyID  = 0;
            pLatency    = NULL;
            bConnected  = false;
            pPackage    = NULL;
        }

        void UIWrapper::destroy()
        {
            // Disconnect UI
            ui_deactivated();

            // Drop plugin UI
            if (pUI != NULL)
            {
                pUI->destroy();
                delete pUI;

                pUI         = NULL;
            }

            // Destroy data
            IWrapper::destroy();

            // Cleanup ports
            pLatency        = NULL;

            // Cleanup generated metadata
            for (size_t i=0; i<vGenMetadata.size(); ++i)
            {
                lsp_trace("destroy generated port metadata %p", vGenMetadata[i]);
                drop_port_metadata(vGenMetadata[i]);
            }

            vExtPorts.flush();
            vMeshPorts.flush();
            vStreamPorts.flush();
            vFrameBufferPorts.flush();

            // Drop OSC buffer
            if (pOscBuffer != NULL)
            {
                ::free(pOscBuffer);
                pOscBuffer = NULL;
            }

            // Drop extensions
            if (pExt != NULL)
            {
                delete pExt;
                pExt        = NULL;
            }

            // Destroy manifest
            if (pPackage != NULL)
            {
                meta::free_manifest(pPackage);
                pPackage        = NULL;
            }

            // Drop loader
            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader     = NULL;
            }
        }

        status_t UIWrapper::init(void *root_widget)
        {
            // Get plugin metadata
            const meta::plugin_t *meta  = pUI->metadata();
            if (meta == NULL)
                lsp_warn("NO PLUGIN METADATA FOUND");

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

            // Create OSC packet buffer
            pOscBuffer      = reinterpret_cast<uint8_t *>(::malloc(OSC_PACKET_MAX + sizeof(LV2_Atom)));

            // Perform all port bindings
            if (meta != NULL)
            {
                for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                    create_port(port, NULL);
            }

            // Create atom transport
            if (pExt->atom_supported())
                pExt->ui_create_atom_transport(vExtPorts.size(), lv2_all_port_sizes(meta->ports, true, false));

            // Add stub for latency reporting
            {
                const meta::port_t *port = &latency_port;
                if ((port->id != NULL) && (port->name != NULL))
                {
                    pLatency = new lv2::UIFloatPort(port, pExt, NULL);
                    vPorts.add(pLatency);
                    nLatencyID  = vExtPorts.size();
                    if (pExt->atom_supported())
                        nLatencyID  += 2;           // Skip AtomIn, AtomOut
                }
            }

            // Sort plugin ports
            vPorts.qsort(compare_abstract_ports_by_urid);
            vMeshPorts.qsort(compare_ports_by_urid);
            vStreamPorts.qsort(compare_ports_by_urid);
            vFrameBufferPorts.qsort(compare_ports_by_urid);

            // Initialize wrapper
            if ((res = IWrapper::init(root_widget)) != STATUS_OK)
                return res;

            // Initialize display settings
            tk::display_settings_t settings;
            resource::Environment env;

            settings.resources      = pLoader;
            settings.environment    = &env;

            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_DICT_PATH, LSP_BUILTIN_PREFIX "i18n"));
            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_LANG, "en_US"));
            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_CONFIG, "lsp-plugins"));

            // Create the display
            lsp_trace("Initializing display");
            pDisplay = new tk::Display(&settings);
            if (pDisplay == NULL)
                return STATUS_NO_MEM;
            if ((res = pDisplay->init(0, NULL)) != STATUS_OK)
                return res;

            // Load visual schema
            if ((res = init_visual_schema()) != STATUS_OK)
                return res;

            // Initialize the UI
            lsp_trace("Initializing UI");
            if ((res = pUI->init(this, pDisplay)) != STATUS_OK)
                return res;

            // Build the UI
            if (meta->ui_resource != NULL)
            {
                void *parent_wnd = pExt->parent_window();
                lsp_info("Building UI from %s, parent window=%p", meta->ui_resource, parent_wnd);
                if ((res = build_ui(meta->ui_resource, parent_wnd)) != STATUS_OK)
                {
                    lsp_error("Error building UI for resource %s: code=%d", meta->ui_resource, int(res));
                    return res;
                }
            }

            // Call the post-initialization routine
            if (res == STATUS_OK)
                res = pUI->post_init();

            // Initialize size of root window
            ws::size_limit_t sr;
            tk::Window *root    = window();
            if (root == NULL)
            {
                lsp_error("No root window present!\n");
                return STATUS_BAD_STATE;
            }

            root->slot(tk::SLOT_SHOW)->bind(slot_ui_show, this);
            root->slot(tk::SLOT_HIDE)->bind(slot_ui_hide, this);
            root->slot(tk::SLOT_RESIZE)->bind(slot_ui_resize, this);

            // Sync state of UI ports with the UI
            for (size_t i=0, n=vPorts.size(); i<n; ++i)
            {
                ui::IPort *p = vPorts.uget(i);
                if (p != NULL)
                    p->notify_all();
            }

            // Resize UI and show
            root->get_padded_size_limits(&sr);
            pExt->resize_ui(sr.nMinWidth, sr.nMinHeight);
            root->show();

            return STATUS_OK;
        }

        lv2::UIPort *UIWrapper::create_port(const meta::port_t *p, const char *postfix)
        {
            lv2::UIPort *result = NULL;
            lv2::Wrapper *w = pExt->wrapper();

            switch (p->role)
            {
                case meta::R_MIDI: // Skip all MIDI ports
                    break;
                case meta::R_AUDIO: // Stub port
                    result = new lv2::UIPort(p, pExt);
                    if (postfix == NULL)
                    {
                        result->set_id(vExtPorts.size());
                        vExtPorts.add(result);
                        lsp_trace("Added external audio port id=%s, external_id=%d", p->id, int(result->get_id()));
                    }
                    break;
                case meta::R_CONTROL:
                    result = new lv2::UIFloatPort(p, pExt, (w != NULL) ? w->port(p->id) : NULL);
                    if (postfix == NULL)
                    {
                        result->set_id(vExtPorts.size());
                        vExtPorts.add(result);
                        lsp_trace("Added external control port id=%s, external_id=%d", p->id, int(result->get_id()));
                    }
                    break;
                case meta::R_BYPASS:
                    result = new lv2::UIBypassPort(p, pExt, (w != NULL) ? w->port(p->id) : NULL);
                    if (postfix == NULL)
                    {
                        result->set_id(vExtPorts.size());
                        vExtPorts.add(result);
                        lsp_trace("Added external bypass id=%s, external_id=%d", p->id, int(result->get_id()));
                    }
                    break;
                case meta::R_METER:
                    result = new lv2::UIPeakPort(p, pExt, (w != NULL) ? w->port(p->id) : NULL);
                    if (postfix == NULL)
                    {
                        result->set_id(vExtPorts.size());
                        vExtPorts.add(result);
                        lsp_trace("Added external metering port id=%s, external_id=%d", p->id, int(result->get_id()));
                    }
                    break;
                case meta::R_PATH:
                    if (pExt->atom_supported())
                        result = new lv2::UIPathPort(p, pExt, (w != NULL) ? w->port(p->id) : NULL);
                    else
                        result = new lv2::UIPort(p, pExt); // Stub port
                    lsp_trace("Added path port id=%", p->id);
                    break;
                case meta::R_MESH:
                    if (pExt->atom_supported())
                    {
                        result = new lv2::UIMeshPort(p, pExt, (w != NULL) ? w->port(p->id) : NULL);
                        vMeshPorts.add(result);
                    }
                    else // Stub port
                        result = new lv2::UIPort(p, pExt);
                    lsp_trace("Added mesh port id=%", p->id);
                    break;
                case meta::R_STREAM:
                    if (pExt->atom_supported())
                    {
                        result = new lv2::UIStreamPort(p, pExt, (w != NULL) ? w->port(p->id) : NULL);
                        vStreamPorts.add(result);
                    }
                    else // Stub port
                        result = new lv2::UIPort(p, pExt);
                    lsp_trace("Added stream port id=%", p->id);
                    break;
                case meta::R_FBUFFER:
                    if (pExt->atom_supported())
                    {
                        result = new lv2::UIFrameBufferPort(p, pExt, (w != NULL) ? w->port(p->id) : NULL);
                        vFrameBufferPorts.add(result);
                    }
                    else // Stub port
                        result = new lv2::UIPort(p, pExt);
                    lsp_trace("Added framebuffer port id=%", p->id);
                    break;
                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];
                    lv2::UIPortGroup *pg    = new lv2::UIPortGroup(p, pExt, (w != NULL) ? w->port(p->id) : NULL);
                    vPorts.add(pg);

                    // Add nested ports
                    for (size_t row=0; row<pg->rows(); ++row)
                    {
                        // Generate postfix
                        snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                        // Clone port metadata
                        meta::port_t *cm    = clone_port_metadata(p->members, postfix_buf);
                        if (cm == NULL)
                            continue;

                        vGenMetadata.add(cm);

                        // Create nested ports
                        for (; cm->id != NULL; ++cm)
                        {
                            if (meta::is_growing_port(cm))
                                cm->start       = cm->min + ((cm->max - cm->min) * row) / float(pg->rows());
                            else if (meta::is_lowering_port(cm))
                                cm->start       = cm->max - ((cm->max - cm->min) * row) / float(pg->rows());

                            // Create port
                            create_port(cm, postfix_buf);
                        }
                    }

                    lsp_trace("Added port set port id=%", p->id);
                    break;
                }

                default:
                    break;
            }

            if (result != NULL)
                vPorts.add(result);

            return result;
        }

        int UIWrapper::resize_ui(ssize_t width, ssize_t height)
        {
            tk::Window *root = (pUI != NULL) ? window() : NULL;
            if (root == NULL)
                return 0;

            // Resize UI and show
            lsp_trace("width=%d, height=%d", int(width), int(height));
            ws::size_limit_t sr;
            root->get_padded_size_limits(&sr);

            // Apply size constraints
            if ((sr.nMaxWidth >= 0) && (width > sr.nMaxWidth))
                width = sr.nMaxWidth;
            if ((sr.nMaxHeight >= 0) && (height > sr.nMaxHeight))
                height = sr.nMaxHeight;

            if ((sr.nMinWidth >= 0) && (width < sr.nMinWidth))
                width = sr.nMinWidth;
            if ((sr.nMinHeight >= 0) && (height < sr.nMinHeight))
                height = sr.nMinHeight;

            // Perform resize
            //TODO: root->realize_widget(r)resize(width, height);
            return 0;
        }

        void UIWrapper::ui_activated()
        {
            if (bConnected)
                return;

            lsp_trace("UI has been activated");
            if (pExt != NULL)
            {
                lv2::Wrapper *w = pExt->wrapper();
                if (w != NULL)
                {
                    lsp_trace("Connecting directly to plugin");
                    w->connect_direct_ui();
                }
                else
                    pExt->ui_connect_to_plugin();
                bConnected = true;
            }
        }

        void UIWrapper::ui_deactivated()
        {
            if (!bConnected)
                return;

            lsp_trace("UI has been deactivated");
            if (pExt != NULL)
            {
                lv2::Wrapper *w = pExt->wrapper();
                if (w != NULL)
                {
                    lsp_trace("Disconnecting directly from plugin");
                    w->disconnect_direct_ui();
                }
                else
                    pExt->ui_disconnect_from_plugin();
                bConnected = false;
            }
        }

        float UIWrapper::ui_scaling_factor(float scaling)
        {
            return (pExt != NULL) ? pExt->fUIScaleFactor * 100.0f : scaling;
        }

        void UIWrapper::parse_raw_osc_event(osc::parse_frame_t *frame)
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
                parse_raw_osc_event(&child); // Perform recursive call
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

                lsp_trace("Received OSC message, address=%s, size=%d", msg_addr, int(msg_size));
                osc::dump_packet(msg_start, msg_size);

                // Try to parse KVT message first
                res = core::KVTDispatcher::parse_message(&sKVT, msg_start, msg_size, core::KVT_TX);
                if (res != STATUS_SKIP)
                    return;

                // Not a KVT message, submit to OSC ports (if present)
                for (size_t i=0, n=vOscInPorts.size(); i<n; ++i)
                {
                    lv2::UIPort *p = vOscInPorts.uget(i);
                    if (p == NULL)
                        continue;

                    // Submit message to the buffer
                    plug::osc_buffer_t *buf = p->buffer<plug::osc_buffer_t>();
                    if (buf != NULL)
                        buf->submit(msg_start, msg_size);
                }
            }
        }

        void UIWrapper::receive_raw_osc_packet(const void *data, size_t size)
        {
            osc::parser_t parser;
            osc::parser_frame_t root;
            status_t res = osc::parse_begin(&root, &parser, data, size);
            if (res == STATUS_OK)
            {
                parse_raw_osc_event(&root);
                osc::parse_end(&root);
                osc::parse_destroy(&parser);
            }
        }

        void UIWrapper::receive_atom(const LV2_Atom_Object * obj)
        {
            if (obj->body.otype == pExt->uridPatchSet)
            {
                lsp_trace("received PATCH_SET message");

                // Parse atom body
                LV2_Atom_Property_Body *body    = lv2_atom_object_begin(&obj->body);
                const LV2_Atom_URID    *key     = NULL;
                const LV2_Atom         *value   = NULL;

                while (!lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                {
                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    if ((body->key == pExt->uridPatchProperty) && (body->value.type == pExt->uridAtomUrid))
                    {
                        key     = reinterpret_cast<const LV2_Atom_URID *>(&body->value);
                        lsp_trace("body->value.body (%d) = %s", int(key->body), pExt->unmap_urid(key->body));
                    }
                    else if (body->key == pExt->uridPatchValue)
                        value   = &body->value;

                    if ((key != NULL) && (value != NULL))
                    {
                        lv2::UIPort *p      = find_by_urid(vPorts, key->body);
                        if ((p != NULL) && (value->type == p->get_type_urid()))
                        {
                            p->deserialize(value);
                            p->notify_all();
                        }

                        key     = NULL;
                        value   = NULL;
                    }

                    body = lv2_atom_object_next(body);
                }
            }
            else if (obj->body.otype == pExt->uridTimePosition)
            {
    //            lsp_trace("Received timePosition");
                plug::position_t pos    = sPosition;

                pos.ticksPerBeat        = DEFAULT_TICKS_PER_BEAT;

    //            lsp_trace("triggered timePosition event");
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
                        pos.denominator     = (reinterpret_cast<LV2_Atom_Int *>(&body->value))->body;
                    else if ((body->key == pExt->uridTimeBeatsPerBar) && (body->value.type == pExt->forge.Float))
                        pos.numerator       = (reinterpret_cast<LV2_Atom_Float *>(&body->value))->body;
                    else if ((body->key == pExt->uridTimeBarBeat) && (body->value.type == pExt->forge.Float))
                        pos.tick            = (reinterpret_cast<LV2_Atom_Float *>(&body->value))->body * pos.ticksPerBeat;
                    else if ((body->key == pExt->uridTimeFrameRate) && (body->value.type == pExt->forge.Float))
                        pos.sampleRate      = (reinterpret_cast<LV2_Atom_Float *>(&body->value))->body;
                }

                // Call plugin callback and update position
                IWrapper::position_updated(&pos);
            }
            else if (obj->body.otype == pExt->uridMeshType)
            {
                // Try to find the corresponding mesh port
                lv2::UIPort *p      = find_by_urid(vMeshPorts, obj->body.id);
                if (p != NULL)
                {
                    p->deserialize(obj);
                    p->notify_all();
                }
            }
            else if (obj->body.otype == pExt->uridStreamType)
            {
                // Try to find the corresponding mesh port
                lv2::UIPort *p      = find_by_urid(vStreamPorts, obj->body.id);
                if (p != NULL)
                {
                    p->deserialize(obj);
                    p->notify_all();
                }
            }
            else if (obj->body.otype == pExt->uridFrameBufferType)
            {
                // Try to find the corresponding mesh port
                lv2::UIPort *p      = find_by_urid(vFrameBufferPorts, obj->body.id);
                if (p != NULL)
                {
                    p->deserialize(obj);
                    p->notify_all();
                }
            }
            else
            {
                lsp_trace("obj->body.otype = %d (%s)", int(obj->body.otype), pExt->unmap_urid(obj->body.otype));
                lsp_trace("obj->body.id = %d (%s)", int(obj->body.id), pExt->unmap_urid(obj->body.id));
            }
        }

        void UIWrapper::notify(size_t id, size_t size, size_t format, const void *buf)
        {
            if (id < vExtPorts.size())
            {
                lv2::UIPort *p = vExtPorts[id];
                if (p != NULL)
                {
                    p->notify(buf, format, size);
                    p->notify_all();
                }
            }
            else if ((pExt->nAtomIn >= 0) && (id == size_t(pExt->nAtomIn)))
            {
                if (format != pExt->uridEventTransfer)
                    return;

                // Check that event is an object
                const LV2_Atom* atom = reinterpret_cast<const LV2_Atom*>(buf);
                lsp_trace("atom.type = %d (%s)", int(atom->type), pExt->unmap_urid(atom->type));

                if ((atom->type == pExt->uridObject) || (atom->type == pExt->uridBlank))
                    receive_atom(reinterpret_cast<const LV2_Atom_Object *>(atom));
                else if (atom->type == pExt->uridOscRawPacket)
                    receive_raw_osc_packet(&atom[1], atom->size);
            }
            else if (id == nLatencyID)
            {
                if (pLatency != NULL)
                    pLatency->notify(buf, format, size);
            }
        }

        void UIWrapper::send_kvt_state()
        {
            core::KVTIterator *iter = sKVT.enum_rx_pending();
            if (iter == NULL)
                return;

            const core::kvt_param_t *p;
            const char *kvt_name;
            size_t size;
            status_t res;

            while (iter->next() == STATUS_OK)
            {
                // Fetch next change
                res = iter->get(&p);
                kvt_name = iter->name();
                if ((res != STATUS_OK) || (kvt_name == NULL))
                    break;

                // Try to serialize changes
                res = core::KVTDispatcher::build_message(kvt_name, p, &pOscBuffer[sizeof(LV2_Atom)], &size, OSC_PACKET_MAX);
                if (res == STATUS_OK)
                {
                    core::KVTDispatcher *d = (pExt->wrapper() != NULL) ? pExt->wrapper()->kvt_dispatcher() : NULL;

                    // Forge raw OSC message as an atom message
                    if (d != NULL)
                    {
                        lsp_trace("Submitting OSC message");
                        osc::dump_packet(&pOscBuffer[sizeof(LV2_Atom)], size);
                        d->submit(&pOscBuffer[sizeof(LV2_Atom)], size); // Submit directly to the KVT dispatcher
                    }
                    else
                    {
                        lsp_trace("Sending OSC message");
                        osc::dump_packet(&pOscBuffer[sizeof(LV2_Atom)], size);

                        // Transmit message via atom interface
                        LV2_Atom *atom  = reinterpret_cast<LV2_Atom *>(pOscBuffer);
                        atom->size      = size;
                        atom->type      = pExt->uridOscRawPacket;
                        size            = (size + sizeof(LV2_Atom) + sizeof(uint64_t) - 1) & ~(sizeof(uint64_t) - 1); // padding

                        // Submit message to the atom output port
                        pExt->write_data(pExt->nAtomOut, size, pExt->uridEventTransfer, pOscBuffer);
                    }
                }

                // Commit transfer
                iter->commit(core::KVT_RX);
            }
        }

        void UIWrapper::receive_kvt_state()
        {
            lv2::Wrapper *w = pExt->wrapper();
            if (w == NULL)
                return;

            // Obtain the dispatcher
            core::KVTDispatcher *d = (pExt->wrapper() != NULL) ? pExt->wrapper()->kvt_dispatcher() : NULL;
            if (d == NULL)
                return;
            if (d->tx_size() <= 0) // Is there data for transfer?
                return;

            core::KVTStorage *skvt = w->kvt_trylock();
            if (skvt == NULL)
                return;

            size_t size;
            if (sKVTMutex.lock())
            {
                status_t res;

                do
                {
                    // Try to fetch record from buffer
                    res = d->fetch(pOscBuffer, &size, OSC_PACKET_MAX);

                    switch (res)
                    {
                        case STATUS_OK:
                        {
                            lsp_trace("Fetched OSC packet of %d bytes", int(size));
                            osc::dump_packet(pOscBuffer, size);
                            core::KVTDispatcher::parse_message(&sKVT, pOscBuffer, size, core::KVT_TX);
                            break;
                        }

                        case STATUS_NO_DATA: // No more data to transmit
                            break;

                        case STATUS_OVERFLOW:
                        {
                            lsp_warn("Too large OSC packet in the buffer, skipping");
                            d->skip();
                            break;
                        }

                        default:
                        {
                            lsp_warn("OSC packet parsing error %d, skipping", int(res));
                            d->skip();
                            break;
                        }
                    }
                } while (res != STATUS_NO_DATA);

                sKVTMutex.unlock();
            }
            w->kvt_release();
        }

        void UIWrapper::sync_kvt_state()
        {
            // Synchronize DSP -> UI transfer
            size_t sync;
            const char *kvt_name;
            const core::kvt_param_t *kvt_value;

            do
            {
                sync = 0;

                core::KVTIterator *it = sKVT.enum_tx_pending();
                while (it->next() == STATUS_OK)
                {
                    kvt_name = it->name();
                    if (kvt_name == NULL)
                        break;
                    status_t res = it->get(&kvt_value);
                    if (res != STATUS_OK)
                        break;
                    if ((res = it->commit(core::KVT_TX)) != STATUS_OK)
                        break;

                    kvt_dump_parameter("TX kvt param (DSP->UI): %s = ", kvt_value, kvt_name);
                    kvt_notify_write(&sKVT, kvt_name, kvt_value);
                    ++sync;
                }
            } while (sync > 0);

            // Synchronize UI -> DSP transfer
            #ifdef LSP_DEBUG
            {
                core::KVTIterator *it = sKVT.enum_rx_pending();
                while (it->next() == STATUS_OK)
                {
                    kvt_name = it->name();
                    if (kvt_name == NULL)
                        break;
                    status_t res = it->get(&kvt_value);
                    if (res != STATUS_OK)
                        break;
                    if ((res = it->commit(core::KVT_RX)) != STATUS_OK)
                        break;

                    kvt_dump_parameter("RX kvt param (UI->DSP): %s = ", kvt_value, kvt_name);
                }
            }
            #else
                sKVT.commit_all(core::KVT_RX);  // Just clear all RX queue for non-debug version
            #endif
        }

        void UIWrapper::main_iteration()
        {
            // Synchronize port states avoiding LV2 Atom transport
            Wrapper *w = pExt->wrapper();
            if (w != NULL)
            {
                for (size_t i=0, n=vPorts.size(); i<n; ++i)
                {
                    lv2::UIPort *p = static_cast<lv2::UIPort *>(vPorts.uget(i));
                    if (p == NULL)
                        continue;
                    if (p->sync())
                        p->notify_all();
                }

                // Check that sample rate has changed
                position_updated(w->position());
            }

            // Transmit KVT state
            if (sKVTMutex.try_lock())
            {
                receive_kvt_state();
                send_kvt_state();
                sync_kvt_state();
                sKVT.gc();
                sKVTMutex.unlock();
            }

            // Call the parent wrapper code
            IWrapper::main_iteration();
        }

        void UIWrapper::dump_state_request()
        {
            pExt->request_state_dump();
        }

        status_t UIWrapper::slot_ui_hide(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *_this = static_cast<UIWrapper *>(ptr);
            _this->ui_deactivated();
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_show(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *_this = static_cast<UIWrapper *>(ptr);
            _this->ui_activated();
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_resize(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *_this = static_cast<UIWrapper *>(ptr);

            tk::Window *wnd = _this->window();
            if (wnd == NULL)
                return STATUS_OK;

            ws::rectangle_t r;
            ws::size_limit_t sr;
            bool resize = false;

            // Get actual geometry and size constraints
            wnd->get_screen_rectangle(&r);
            wnd->get_padded_size_limits(&sr);
            lsp_trace("r  = {%d %d %d %d}", int(r.nLeft), int(r.nTop), int(r.nWidth), int(r.nHeight));
            lsp_trace("sr = {%d %d %d %d}", int(sr.nMinWidth), int(sr.nMinHeight), int(sr.nMaxWidth), int(sr.nMaxHeight));

            if ((sr.nMaxWidth > 0) && (r.nWidth > sr.nMaxWidth))
            {
                r.nWidth        = sr.nMaxWidth;
                resize          = true;
            }
            if ((sr.nMaxHeight > 0) && (r.nWidth > sr.nMaxHeight))
            {
                r.nHeight       = sr.nMaxHeight;
                resize          = true;
            }
            if ((sr.nMinWidth > 0) && (r.nWidth < sr.nMinWidth))
            {
                r.nWidth        = sr.nMinWidth;
                resize          = true;
            }
            if ((sr.nMinHeight > 0) && (r.nHeight < sr.nMinHeight))
            {
                r.nHeight       = sr.nMinHeight;
                resize          = true;
            }

            lsp_trace("final r = {%d %d %d %d}, resize=%s",
                    int(r.nLeft), int(r.nTop), int(r.nWidth), int(r.nHeight),
                    (resize) ? "true" : "false"
                );

            if (resize)
                _this->pExt->resize_ui(r.nWidth, r.nHeight);
            return STATUS_OK;
        }

        core::KVTStorage *UIWrapper::kvt_lock()
        {
            return (sKVTMutex.lock()) ? &sKVT : NULL;
        }

        core::KVTStorage *UIWrapper::kvt_trylock()
        {
            return (sKVTMutex.try_lock()) ? &sKVT : NULL;
        }

        bool UIWrapper::kvt_release()
        {
            return sKVTMutex.unlock();
        }

        const meta::package_t *UIWrapper::package() const
        {
            return pPackage;
        }
    } /* namespace lv2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_IMPL_UI_WRAPPER_H_ */
