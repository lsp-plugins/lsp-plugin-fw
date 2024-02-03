/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 янв. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_UI_WRAPPER_H_

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
#include <lsp-plug.in/plug-fw/wrap/vst3/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/debug.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {

        UIWrapper::UIWrapper(PluginFactory *factory, ui::Module *plugin, resource::ILoader *loader, const meta::package_t *package):
            ui::IWrapper(plugin, loader)
        {
            lsp_trace("this=%p", this);
            nRefCounter         = 1;
            pFactory            = safe_acquire(factory);
            pLoader             = loader;
            pPackage            = package;
            pHostContext        = NULL;
            pPluginView         = NULL;
            pHostApplication    = NULL;
            pPeerConnection     = NULL;
            pComponentHandler   = NULL;
            pComponentHandler2  = NULL;
            pComponentHandler3  = NULL;
            nLatency            = 0;
            fScalingFactor      = -1.0f;
            bUIInitialized      = false;
        }

        UIWrapper::~UIWrapper()
        {
            lsp_trace("this=%p", this);
            destroy();

            // Release factory
            safe_release(pFactory);
        }

        ssize_t UIWrapper::compare_param_ports(const vst3::UIParameterPort *a, const vst3::UIParameterPort *b)
        {
            const Steinberg::Vst::ParamID a_id = a->parameter_id();
            const Steinberg::Vst::ParamID b_id = b->parameter_id();

            return (a_id > b_id) ? 1 :
                   (a_id < b_id) ? -1 : 0;
        }

        vst3::UIPort *UIWrapper::create_port(const meta::port_t *port, const char *postfix)
        {
            // Find the matching port for the backend
            vst3::UIPort *vup = NULL;

            switch (port->role)
            {
                case meta::R_AUDIO: // Stub port
                    lsp_trace("creating stub audio port %s", port->id);
                    vup = new vst3::UIPort(port);
                    break;

                case meta::R_MESH:
                    lsp_trace("creating mesh port %s", port->id);
                    vup = new vst3::UIMeshPort(port);
                    break;

                case meta::R_STREAM:
                    lsp_trace("creating stream port %s", port->id);
                    vup = new vst3::UIStreamPort(port);
                    break;

                case meta::R_FBUFFER:
                    lsp_trace("creating fbuffer port %s", port->id);
                    vup = new vst3::UIFrameBufferPort(port);
                    break;

                case meta::R_OSC:
                    break;

                case meta::R_PATH:
                    lsp_trace("creating path port %s", port->id);
                    vup = new vst3::UIPathPort(port, this);
                    vParamMapping.create(port->id, vup);
                    break;

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    lsp_trace("creating parameter port %s", port->id);
                    vst3::UIParameterPort *p    = new vst3::UIParameterPort(port, this, postfix != NULL);
                    vParamMapping.create(port->id, p);
                    if (postfix == NULL)
                        vParams.add(p);
                    vup = p;
                    break;
                }

                case meta::R_METER:
                {
                    lsp_trace("creating meter port %s", port->id);
                    vst3::UIMeterPort *p        = new vst3::UIMeterPort(port);
                    vMeters.add(p);
                    vup = p;
                    break;
                }

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];
                    lsp_trace("creating port group %s", port->id);
                    vst3::UIPortGroup *upg = new vst3::UIPortGroup(port, this, postfix != NULL);

                    // Add immediately port group to list
                    vPorts.add(upg);
                    vParamMapping.create(port->id, upg);
                    if (postfix == NULL)
                        vParams.add(upg);

                    // Add nested ports
                    for (size_t row=0; row<upg->rows(); ++row)
                    {
                        // Generate postfix
                        snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                        // Clone port metadata
                        meta::port_t *cm    = clone_port_metadata(port->members, postfix_buf);
                        if (cm == NULL)
                            continue;

                        vGenMetadata.add(cm);

                        // Create nested ports
                        for (; cm->id != NULL; ++cm)
                        {
                            if (meta::is_growing_port(cm))
                                cm->start       = cm->min + ((cm->max - cm->min) * row) / float(upg->rows());
                            else if (meta::is_lowering_port(cm))
                                cm->start       = cm->max - ((cm->max - cm->min) * row) / float(upg->rows());

                            // Create port
                            create_port(cm, postfix_buf);
                        }
                    }

                    break;
                }

                default:
                    break;
            }

            // Add port to the list of UI ports
            if (vup != NULL)
                vPorts.add(vup);

            return vup;
        }

        vst3::UIParameterPort *UIWrapper::find_param(Steinberg::Vst::ParamID param_id)
        {
            ssize_t first=0, last = vParams.size() - 1;
            while (first <= last)
            {
                size_t center           = size_t(first + last) >> 1;
                vst3::UIParameterPort *p= vParams.uget(center);
                if (param_id == p->parameter_id())
                    return p;
                else if (param_id < p->parameter_id())
                    last    = center - 1;
                else
                    first   = center + 1;
            }
            return NULL;
        }

        status_t UIWrapper::init(void *root_widget)
        {
            lsp_trace("this=%p", this);

            status_t res;

            // Get plugin metadata
            const meta::plugin_t *meta  = pUI->metadata();
            if (meta == NULL)
            {
                lsp_warn("No plugin metadata found");
                return STATUS_BAD_STATE;
            }

            // Perform all port bindings
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(port, NULL);
            vParams.qsort(compare_param_ports);

            // Initialize wrapper
            if ((res = ui::IWrapper::init(root_widget)) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        void UIWrapper::destroy()
        {
            lsp_trace("this=%p", this);

            // Destroy plugin UI
            if (pUI != NULL)
            {
                delete pUI;
                pUI         = NULL;
            }

            // Remove port bindings
            vParams.flush();
            vParamMapping.flush();

            ui::IWrapper::destroy();

            // Cleanup generated metadata
            for (size_t i=0; i<vGenMetadata.size(); ++i)
            {
                meta::port_t *p = vGenMetadata.uget(i);
                lsp_trace("destroy generated port metadata %p", p);
                drop_port_metadata(p);
            }
            vGenMetadata.flush();
        }

        Steinberg::tresult PLUGIN_API UIWrapper::queryInterface(const Steinberg::TUID _iid, void **obj)
        {
            // Cast to the requested interface
            if (Steinberg::iidEqual(_iid, Steinberg::FUnknown::iid))
                return cast_interface<Steinberg::FUnknown>(static_cast<Steinberg::IDependent *>(this), obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IDependent::iid))
                return cast_interface<Steinberg::IDependent>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPluginBase::iid))
                return cast_interface<Steinberg::IPluginBase>(static_cast<Steinberg::Vst::IEditController *>(this), obj);

            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IConnectionPoint::iid))
                return cast_interface<Steinberg::Vst::IConnectionPoint>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IEditController::iid))
                return cast_interface<Steinberg::Vst::IEditController>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IEditController2::iid))
                return cast_interface<Steinberg::Vst::IEditController2>(this, obj);

            return no_interface(obj);
        }

        Steinberg::uint32 PLUGIN_API UIWrapper::addRef()
        {
            return atomic_add(&nRefCounter, 1) + 1;
        }

        Steinberg::uint32 PLUGIN_API UIWrapper::release()
        {
            atomic_t ref_count = atomic_add(&nRefCounter, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        void PLUGIN_API UIWrapper::update(FUnknown *changedUnknown, Steinberg::int32 message)
        {
            lsp_trace("this=%p, changedUnknown=%p, message=%d", this, changedUnknown, int(message));
        }

        Steinberg::tresult PLUGIN_API UIWrapper::initialize(Steinberg::FUnknown *context)
        {
            lsp_trace("this=%p, context=%p", this, context);

            status_t res;

            // Acquire host context
            if (pHostContext != NULL)
                return Steinberg::kResultFalse;

            Steinberg::Linux::IRunLoop *run_loop = safe_query_iface<Steinberg::Linux::IRunLoop>(context);
            lsp_trace("RUN LOOP object=%p", run_loop);
            safe_release(run_loop);

            pHostContext        = safe_acquire(context);
            pHostApplication    = safe_query_iface<Steinberg::Vst::IHostApplication>(context);

            // Initialize
            res = init(NULL);

            return (res == STATUS_OK) ? Steinberg::kResultOk : Steinberg::kInternalError;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::terminate()
        {
            lsp_trace("this=%p", this);

            // Release host context
            safe_release(pHostContext);
            safe_release(pHostApplication);
            safe_release(pComponentHandler);
            safe_release(pComponentHandler2);
            safe_release(pComponentHandler3);

            // Release the peer connection if host didn't disconnect us previously.
            if (pPeerConnection != NULL)
            {
                pPeerConnection->disconnect(this);
                safe_release(pPeerConnection);
            }

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::connect(Steinberg::Vst::IConnectionPoint *other)
        {
            lsp_trace("this=%p, other=%p", this, other);

            // Check if peer connection is valid and was not previously estimated
            if (other == NULL)
                return Steinberg::kInvalidArgument;
            if (pPeerConnection)
                return Steinberg::kResultFalse;

            // Save the peer connection
            pPeerConnection = safe_acquire(other);

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::disconnect(Steinberg::Vst::IConnectionPoint *other)
        {
            lsp_trace("this=%p, other=%p", this, other);

            // Check that estimated peer connection matches the esimated one
            if (other == NULL)
                return Steinberg::kInvalidArgument;
            if (pPeerConnection != other)
                return Steinberg::kResultFalse;

            // Reset the peer connection
            safe_release(pPeerConnection);

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::notify(Steinberg::Vst::IMessage *message)
        {
            // Set-up DSP context
            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

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
            Steinberg::char8 key[32];
            Steinberg::int64 byte_order = VST3_BYTEORDER;

            if (!strcmp(message_id, ID_MSG_LATENCY))
            {
                lsp_trace("Received message id=%s", message_id);

                Steinberg::int64 latency = 0;

                // Write the actual value
                if (atts->getInt("value", latency) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                if (nLatency != latency)
                {
                    nLatency    = latency;
                    pComponentHandler->restartComponent(Steinberg::Vst::RestartFlags::kLatencyChanged);
                }
            }
            else if (!strcmp(message_id, ID_MSG_STATE_DIRTY))
            {
                lsp_trace("Received message id=%s", message_id);

                // Mark state as dirty
                if (pComponentHandler2 != NULL)
                    pComponentHandler2->setDirty(true);
            }
            else if (!strcmp(message_id, ID_MSG_MUSIC_POSITION))
            {
                plug::position_t pos;
                plug::position_t::init(&pos);

                double sr;
                Steinberg::int64 frame;

                if (atts->getFloat("sample_rate", sr) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getFloat("speed", pos.speed) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getInt("frame", frame) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getFloat("numerator", pos.numerator) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getFloat("denominator", pos.denominator) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getFloat("bpm", pos.beatsPerMinute) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getFloat("bpm_change", pos.beatsPerMinuteChange) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getFloat("tick", pos.tick) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getFloat("ticks_per_beat", pos.ticksPerBeat) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                pos.sampleRate      = sr;
                pos.frame           = frame;

                // Notify UI about position update
                position_updated(&pos);
            }
            else if (!strcmp(message_id, ID_MSG_METERS))
            {
                double value = 0.0;

                // Get actual value for each meter
                for (lltl::iterator<vst3::UIMeterPort> it = vMeters.values(); it; ++it)
                {
                    vst3::UIMeterPort *p = it.get();
                    if (p == NULL)
                        continue;

                    // Fetch meter value
                    if (atts->getFloat(p->id(), value) != Steinberg::kResultOk)
                        continue;

                    // Sync value with meter port and notify listeners if value has changed
                    if (p->commit_value(value))
                        p->notify_all(ui::PORT_NONE);
                }
            }
            else if (!strcmp(message_id, ID_MSG_MESH))
            {
                // Read endianess
                if (atts->getInt("endian", byte_order) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // Read identifier of the mesh port
                const char *param_id = sNotifyBuf.get_string(atts, "id", byte_order);
                if (param_id == NULL)
                    return Steinberg::kResultFalse;

                // Get port and validate it's type
                ui::IPort *port = port_by_id(param_id);
                if (port == NULL)
                    return Steinberg::kResultFalse;
                const meta::port_t *meta = port->metadata();
                if ((meta == NULL) || (!meta::is_mesh_port(meta)))
                    return Steinberg::kResultFalse;

                // Read number of buffers
                Steinberg::int64 buffers = 0;
                if (atts->getInt("buffers", buffers) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if ((buffers < 0) || (buffers > meta->step))
                    return Steinberg::kResultFalse;

                // Read number of elements per buffer
                Steinberg::int64 items = 0;
                if (atts->setInt("items", items) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if ((items < 0) || (items > meta->start))
                    return Steinberg::kResultFalse;

                // Encode data for each buffer
                plug::mesh_t *mesh = port->buffer<plug::mesh_t>();
                const void *data = NULL;
                Steinberg::uint32 sizeInBytes = 0;
                for (size_t i=0, n=size_t(buffers); i<n; ++i)
                {
                    snprintf(key, sizeof(key), "data[%d]", int(i));
                    if (atts->getBinary(key, data, sizeInBytes) != Steinberg::kResultOk)
                        return Steinberg::kResultFalse;
                    if (sizeInBytes != items * sizeof(float))
                        return Steinberg::kResultFalse;

                    if (byte_order != VST3_BYTEORDER)
                    {
                        byte_swap_copy(mesh->pvData[i], static_cast<const float *>(data), items);
                        dsp::saturate(mesh->pvData[i], items);
                    }
                    else
                        dsp::copy_saturated(mesh->pvData[i], static_cast<const float *>(data), items);
                }

                // Update state of the mesh and notify
                mesh->data(buffers, items);
                port->notify_all(ui::PORT_NONE);
            }
            else if (!strcmp(message_id, ID_MSG_FRAMEBUFFER))
            {
                // Read endianess
                if (atts->getInt("endian", byte_order) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // Read identifier of the mesh port
                const char *param_id = sNotifyBuf.get_string(atts, "id", byte_order);
                if (param_id == NULL)
                    return Steinberg::kResultFalse;

                // Get port and validate it's type
                ui::IPort *port = port_by_id(param_id);
                if (port == NULL)
                    return Steinberg::kResultFalse;
                const meta::port_t *meta = port->metadata();
                if ((meta == NULL) || (!meta::is_framebuffer_port(meta)))
                    return Steinberg::kResultFalse;

                // Read number of rows
                Steinberg::int64 rows = 0;
                if (atts->getInt("rows", rows) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if ((rows < 0) || (rows > meta->start))
                    return Steinberg::kResultFalse;

                // Read number of columns
                Steinberg::int64 cols = 0;
                if (atts->getInt("cols", cols) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if ((cols < 0) || (cols > meta->step))
                    return Steinberg::kResultFalse;

                // Read first row identifier
                Steinberg::int64 first_row_id = 0;
                if (atts->getInt("first_row_id", first_row_id) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (first_row_id < 0)
                    return Steinberg::kResultFalse;

                // Read first row identifier
                Steinberg::int64 last_row_id = 0;
                if (atts->getInt("last_row_id", last_row_id) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (last_row_id < 0)
                    return Steinberg::kResultFalse;

                // Now parse each vector
                plug::frame_buffer_t *fbuffer = port->buffer<plug::frame_buffer_t>();
                const void *data = NULL;
                Steinberg::uint32 sizeInBytes = 0;

                for (uint32_t first_row=first_row_id, last_row=last_row_id, i=0; first_row != last_row; ++first_row, ++i)
                {
                    snprintf(key, sizeof(key), "row[%d]", int(i));
                    if (atts->getBinary(key, data, sizeInBytes) != Steinberg::kResultOk)
                        return Steinberg::kResultFalse;
                    if (sizeInBytes != cols * sizeof(float))
                        return Steinberg::kResultFalse;

                    float *dst = fbuffer->get_row(first_row);
                    if (byte_order != VST3_BYTEORDER)
                        byte_swap_copy(dst, static_cast<const float *>(data), cols);
                    else
                        dsp::copy(dst, static_cast<const float *>(data), cols);
                }

                // Update state of the mesh and notify
                fbuffer->seek(first_row_id);
                port->notify_all(ui::PORT_NONE);
            }
            else if (!strcmp(message_id, ID_MSG_STREAM))
            {
                // Read endianess
                if (atts->getInt("endian", byte_order) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // Read identifier of the mesh port
                const char *param_id = sNotifyBuf.get_string(atts, "id", byte_order);
                if (param_id == NULL)
                    return Steinberg::kResultFalse;

                // Get port and validate it's type
                ui::IPort *port = port_by_id(param_id);
                if (port == NULL)
                    return Steinberg::kResultFalse;
                const meta::port_t *meta = port->metadata();
                if ((meta == NULL) || (!meta::is_stream_port(meta)))
                    return Steinberg::kResultFalse;

                // Read number of buffers
                Steinberg::int64 buffers = 0;
                if (atts->getInt("buffers", buffers) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (buffers < 0)
                    return Steinberg::kResultFalse;

                // Read number of frames
                Steinberg::int64 frames = 0;
                if (atts->getInt("frames", frames) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (frames < 0)
                    return Steinberg::kResultFalse;

                // Read stream content
                const void *data = NULL;
                Steinberg::uint32 sizeInBytes = 0;
                plug::stream_t *stream = port->buffer<plug::stream_t>();

                for (size_t i=0, n=frames; i<n; ++i)
                {
                    // Read frame number
                    Steinberg::int64 frame_id = 0;
                    snprintf(key, sizeof(key), "frame_id[%d]", int(i));
                    if (atts->getInt(key, frame_id) != Steinberg::kResultOk)
                        return Steinberg::kResultFalse;

                    // Read frame size
                    Steinberg::int64 frame_size = 0;
                    snprintf(key, sizeof(key), "frame_size[%d]", int(i));
                    if (atts->getInt(key, frame_size) != Steinberg::kResultOk)
                        return Steinberg::kResultFalse;

                    // Cleanup stream if it is too far from actual state
                    uint32_t prev_id    = frame_id - 1;
                    if (stream->frame_id() != prev_id)
                        stream->clear(prev_id);

                    // Read frame components
                    size_t f_size   = stream->add_frame(frame_size);
                    for (size_t j=0, m=buffers; j < m; ++j)
                    {
                        snprintf(key, sizeof(key), "data[%d][%d]", int(i), int(j));
                        if (atts->getBinary(key, data, sizeInBytes) != Steinberg::kResultOk)
                            return Steinberg::kResultFalse;
                        if (sizeInBytes != frame_size * sizeof(float))
                            return Steinberg::kResultFalse;

                        // Copy frame buffer
                        const float *src = reinterpret_cast<const float *>(data);
                        if (byte_order != VST3_BYTEORDER)
                        {
                            // The frame can be split into parts because of the frame buffer
                            size_t count = 0;
                            float *dst = stream->frame_data(j, 0, &count);
                            byte_swap_copy(dst, src, count);
                            if (count < f_size)
                            {
                                dst     = stream->frame_data(j, count, NULL);
                                byte_swap_copy(dst, &src[count], f_size - count);
                            }
                        }
                        else
                            stream->write_frame(j, src, 0, f_size);
                    }

                    // Commit the frame for the stream
                    stream->commit_frame();
                }
            }
            else if (!strcmp(message_id, ID_MSG_KVT))
            {
                lsp_trace("Received message id=%s", message_id);

                const void *data = NULL;
                Steinberg::uint32 sizeInBytes = 0;

                if (atts->getBinary(key, data, sizeInBytes) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                receive_raw_osc_packet(data, sizeInBytes);
            }

            return Steinberg::kResultOk;
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

                // Try to parse KVT message
                core::KVTDispatcher::parse_message(&sKVT, msg_start, msg_size, core::KVT_TX);
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

        status_t UIWrapper::load_state(Steinberg::IBStream *is)
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
                    vst3::UIPort *p         = vParamMapping.get(name);
                    if (p != NULL)
                    {
                        const meta::port_t *meta = p->metadata();
                        if ((meta::is_control_port(meta)) || (meta::is_bypass_port(meta)))
                        {
                            vst3::UIParameterPort *pp   = static_cast<vst3::UIParameterPort *>(p);
                            float v = 0.0f;
                            if ((res = read_fully(is, &v)) != STATUS_OK)
                            {
                                lsp_warn("Failed to deserialize port id=%s", name);
                                return res;
                            }
                            lsp_trace("  %s = %f", meta->id, v);
                            pp->commit_value(v);
                            pp->notify_all(ui::PORT_NONE);
                        }
                        else if (meta::is_path_port(meta))
                        {
                            vst3::UIPathPort *pp        = static_cast<vst3::UIPathPort *>(p);

                            if ((res = read_string(is, &name, &name_cap)) != STATUS_OK)
                            {
                                lsp_warn("Failed to deserialize port id=%s", meta->id);
                                return res;
                            }
                            lsp_trace("  %s = %s", meta->id, name);
                            pp->commit_value(name);
                            pp->notify_all(ui::PORT_NONE);
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
                        if (!(flags & vst3::FLAG_PRIVATE))
                        {
                            kvt_dump_parameter("Fetched KVT parameter %s = ", &p, name);
                            sKVT.put(name, &p, kflags);
                        }
                    }
                }
            }

            // Analyze result
            return (res == STATUS_EOF) ? STATUS_OK : STATUS_CORRUPTED;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setState(Steinberg::IBStream *state)
        {
            lsp_trace("this=%p, state=%p", this, state);
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getState(Steinberg::IBStream *state)
        {
            lsp_trace("this=%p, state=%p", this, state);
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setComponentState(Steinberg::IBStream *state)
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

        Steinberg::int32 PLUGIN_API UIWrapper::getParameterCount()
        {
            lsp_trace("this=%p, result=%d", this, int(vParams.size()));
            return vParams.size();
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getParameterInfo(Steinberg::int32 paramIndex, Steinberg::Vst::ParameterInfo & info /*out*/)
        {
            lsp_trace("this=%p, paramIndex=%d", this, int(paramIndex));

            vst3::UIParameterPort *p = vParams.get(paramIndex);
            if (p == NULL)
                return Steinberg::kInvalidArgument;

            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return Steinberg::kInternalError;

            const char *units   = vst3::get_unit_name(meta->unit);
            const float dfl     = p->default_value();

            info.id             = p->parameter_id();

            lsp::utf8_to_utf16(
                to_utf16(info.title),
                meta->name,
                sizeof(Steinberg::Vst::String128)/sizeof(Steinberg::Vst::TChar));
            lsp::utf8_to_utf16(
                to_utf16(info.shortTitle),
                meta->id,
                sizeof(Steinberg::Vst::String128)/sizeof(Steinberg::Vst::TChar));
            lsp::utf8_to_utf16(
                to_utf16(info.units),
                units,
                sizeof(Steinberg::Vst::String128)/sizeof(Steinberg::Vst::TChar));

            lsp_trace("parameter id=%s, default value=%f", meta->id, dfl);

            info.stepCount      = 0;
            info.flags          = 0;
            info.unitId         = Steinberg::Vst::kRootUnitId;
            info.defaultNormalizedValue = to_vst_value(meta, dfl);

            if (meta::is_meter_port(meta))
                info.flags         |= Steinberg::Vst::ParameterInfo::kIsReadOnly;
            else
                info.flags         |= Steinberg::Vst::ParameterInfo::kCanAutomate;
            if (meta->flags & meta::F_CYCLIC)
                info.flags         |= Steinberg::Vst::ParameterInfo::kIsWrapAround;
            if (meta::is_bypass_port(meta))
                info.flags         |= Steinberg::Vst::ParameterInfo::kIsBypass;

            if (meta::is_bool_unit(meta->unit))
                info.stepCount      = 1;
            else if (meta::is_enum_unit(meta->unit))
            {
                info.stepCount      = meta::list_size(meta->items) - 1;
                info.flags         |= Steinberg::Vst::ParameterInfo::kIsList;
            }
            else if (meta->flags & meta::F_INT)
                info.stepCount      = (lsp_max(meta->min, meta->max) - lsp_min(meta->min, meta->max)) / meta->step;

            log_paremeter_info(&info);

            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getParamStringByValue(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized /*in*/, Steinberg::Vst::String128 string /*out*/)
        {
            lsp_trace("this=%p, id=0x%08x, valueNormalized=%f", this, int(id), valueNormalized);

            // Get port
            vst3::UIParameterPort *p = find_param(id);
            if (p == NULL)
            {
                lsp_trace("parameter id=0x%08x not found", int(id));
                return Steinberg::kInvalidArgument;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return Steinberg::kInternalError;

            // Format value
            char buffer[128];
            const float value   = vst3::from_vst_value(meta, valueNormalized);
            meta::format_value(buffer, sizeof(buffer), meta, value, -1, false);
            const size_t res    = lsp::utf8_to_utf16(to_utf16(string), buffer,
                sizeof(Steinberg::Vst::String128)/sizeof(Steinberg::Vst::TChar));

            lsp_trace("valueNormalized=%f -> plainValue=%f -> formatted=%s", valueNormalized, value, buffer);

            return (res > 0) ? Steinberg::kResultOk : Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getParamValueByString(Steinberg::Vst::ParamID id, Steinberg::Vst::TChar *string /*in*/, Steinberg::Vst::ParamValue & valueNormalized /*out*/)
        {
            lsp_trace("this=%p, id=0x%08x, string=%s", this, int(id), string);

            // Get port
            vst3::UIParameterPort *p = find_param(id);
            if (p == NULL)
            {
                lsp_trace("parameter id=0x%08x not found", int(id));
                return Steinberg::kInvalidArgument;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return Steinberg::kInternalError;

            // Parse value
            float parsed = 0.0f;
            char buffer[128];
            if (lsp::utf16_to_utf8(buffer, to_utf16(string), sizeof(buffer)) == 0)
            {
                lsp_warn("falied UTF16->UTF8 conversion port id=\"%s\" name=\"%s\", buffer=\"%s\"",
                    meta->id, meta->name, buffer);
                return Steinberg::kResultFalse;
            }

            status_t res = meta::parse_value(&parsed, buffer, meta, false);
            if (res != STATUS_OK)
            {
                lsp_warn("parse_value for port id=\"%s\" name=\"%s\", buffer=\"%s\" failed with code %d",
                    meta->id, meta->name, buffer, int(res));
                return Steinberg::kResultFalse;
            }

            parsed              = meta::limit_value(meta, parsed);
            valueNormalized     = to_vst_value(meta, parsed);

            lsp_trace("port id=\"%s\" buffer=\"%s\" -> parsed=%f -> normalizedValue=%f", meta->id, buffer, parsed, valueNormalized);

            return Steinberg::kResultOk;
        }

        Steinberg::Vst::ParamValue PLUGIN_API UIWrapper::normalizedParamToPlain(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized)
        {
            lsp_trace("this=%p, id=0x%08x, valueNormalzed=%f", this, int(id), valueNormalized);

            // Get port
            vst3::UIParameterPort *p = find_param(id);
            if (p == NULL)
            {
                lsp_warn("parameter id=0x%08x not found", int(id));
                return 0.0;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return 0.0;

            // Convert value
            const float plainValue = from_vst_value(meta, valueNormalized);
            lsp_trace("normalizedValue=%f -> plainValue=%f", plainValue, valueNormalized);

            return plainValue;
        }

        Steinberg::Vst::ParamValue PLUGIN_API UIWrapper::plainParamToNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue plainValue)
        {
            lsp_trace("this=%p, id=0x%08x, plainValue=%f", this, int(id), plainValue);

            // Get port
            vst3::UIParameterPort *p = find_param(id);
            if (p == NULL)
            {
                lsp_warn("parameter id=0x%08x not found", int(id));
                return 0.0;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return 0.0;

            // Convert value
            const float valueNormalized = to_vst_value(meta, plainValue);
            lsp_trace("plainValue=%f -> normalizedValue=%f", plainValue, valueNormalized);

            return valueNormalized;
        }

        Steinberg::Vst::ParamValue PLUGIN_API UIWrapper::getParamNormalized(Steinberg::Vst::ParamID id)
        {
            lsp_trace("this=%p, id=0x%08x", this, int(id));

            // Get port
            vst3::UIParameterPort *p = find_param(id);
            if (p == NULL)
            {
                lsp_warn("parameter id=0x%08x not found", int(id));
                return 0.0;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return 0.0;

            // Convert value
            const float value   = p->value();
            const float result  = to_vst_value(meta, value);
            lsp_trace("plainValue=%f -> normalizedValue=%f", value, result);

            // Return result
            return result;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setParamNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value)
        {
            lsp_trace("this=%p, id=0x%08x, value=%f", this, int(id), value);

            // Get port
            vst3::UIParameterPort *p = find_param(id);
            if (p == NULL)
            {
                lsp_warn("parameter id=0x%08x not found", int(id));
                return Steinberg::kInvalidArgument;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return Steinberg::kInternalError;

            // Convert value
            const float plainValue = from_vst_value(meta, value);
            lsp_trace("normalizedValue=%f -> plainValue=%f", value, plainValue);

            // Commit value
            p->commit_value(plainValue);
            p->notify_all(ui::PORT_NONE);

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setComponentHandler(Steinberg::Vst::IComponentHandler *handler)
        {
            lsp_trace("this=%p, handler=%p", this, handler);

            if (pComponentHandler == handler)
                return Steinberg::kResultTrue;

            safe_release(pComponentHandler);
            safe_release(pComponentHandler2);
            safe_release(pComponentHandler3);

            pComponentHandler = safe_acquire(handler);
            if (pComponentHandler != NULL)
            {
                pComponentHandler2 = safe_query_iface<Steinberg::Vst::IComponentHandler2>(pComponentHandler);
                pComponentHandler3 = safe_query_iface<Steinberg::Vst::IComponentHandler3>(pComponentHandler);
            }
            return Steinberg::kResultTrue;
        }

        status_t UIWrapper::initialize_ui()
        {
            // Initialize parent
            status_t res = STATUS_OK;
            if (bUIInitialized)
                return res;
            if ((res = IWrapper::init(NULL)) != STATUS_OK)
                return res;

            // Initialize display settings
            tk::display_settings_t settings;
            resource::Environment env;

            settings.resources      = pLoader;
            settings.environment    = &env;

            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_DICT_PATH, LSP_BUILTIN_PREFIX "i18n"));
            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_LANG, "us"));
            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_CONFIG, "lsp-plugins"));

            // Create the display
            pDisplay = new tk::Display(&settings);
            if (pDisplay == NULL)
                return STATUS_NO_MEM;
            if ((res = pDisplay->init(0, NULL)) != STATUS_OK)
                return res;

            // Bind the display idle handler
            pDisplay->slots()->bind(tk::SLOT_IDLE, slot_display_idle, this);
            pDisplay->set_idle_interval(1000 / UI_FRAMES_PER_SECOND);

            // Load visual schema
            if ((res = init_visual_schema()) != STATUS_OK)
                return res;

            // Initialize the UI
            if ((res = pUI->init(this, pDisplay)) != STATUS_OK)
                return res;

            lsp_trace("Initializing UI contents");
            const meta::plugin_t *meta = pUI->metadata();
            if (pUI->metadata() == NULL)
                return STATUS_BAD_STATE;

            if (meta->ui_resource != NULL)
            {
                if ((res = build_ui(meta->ui_resource, NULL)) != STATUS_OK)
                {
                    lsp_error("Error building UI for resource %s: code=%d", meta->ui_resource, int(res));
                    return res;
                }
            }

            // Bind different slots
            lsp_trace("Binding slots");
            tk::Window *wnd  = window();
            if (wnd != NULL)
            {
                wnd->slots()->bind(tk::SLOT_RESIZE, slot_ui_resize, this);
                wnd->slots()->bind(tk::SLOT_SHOW, slot_ui_show, this);
                wnd->slots()->bind(tk::SLOT_REALIZED, slot_ui_realized, this);
                wnd->slots()->bind(tk::SLOT_CLOSE, slot_ui_close, this);
            }

            // Call the post-initialization routine
            lsp_trace("Doing post-init");
            res = pUI->post_init();
            if (res != STATUS_OK)
                return res;

            return res;
        }

        Steinberg::IPlugView * PLUGIN_API UIWrapper::createView(Steinberg::FIDString name)
        {
            lsp_trace("this=%p, name=%s", this, name);
            if (strcmp(name, Steinberg::Vst::ViewType::kEditor) != 0)
                return NULL;

            if (pPluginView != NULL)
                return safe_acquire(pPluginView);

            if (initialize_ui() != STATUS_OK)
                return NULL;

            // Create plugin view
            pPluginView     = new PluginView(this);
            if (pPluginView == NULL)
                return NULL;
            lsp_finally { safe_release(pPluginView); };

            // Notify backend about UI activation
            if (pPeerConnection != NULL)
            {
                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg != NULL)
                {
                    lsp_finally { safe_release(msg); };
                    // Initialize and send the message
                    msg->setMessageID(vst3::ID_MSG_ACTIVATE_UI);
                    pPeerConnection->notify(msg);
                }
            }

            return safe_acquire(pPluginView);
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setKnobMode(Steinberg::Vst::KnobMode mode)
        {
            lsp_trace("this=%p, mode=%d", this, int(mode));

            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::openHelp(Steinberg::TBool onlyCheck)
        {
            lsp_trace("this=%p, onlyCheck=%d", this, int(onlyCheck));

            if (onlyCheck)
                return Steinberg::kResultOk;

            if (pPluginView == NULL)
                return Steinberg::kResultFalse;

            return pPluginView->show_help();
        }

        Steinberg::tresult PLUGIN_API UIWrapper::openAboutBox(Steinberg::TBool onlyCheck)
        {
            lsp_trace("this=%p, onlyCheck=%d", this, int(onlyCheck));

            if (onlyCheck)
                return Steinberg::kResultOk;

            if (pPluginView == NULL)
                return Steinberg::kResultFalse;

            return pPluginView->show_about_box();
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

        void UIWrapper::dump_state_request()
        {
            // Create message
            if (pPeerConnection == NULL)
                return;

            // Allocate new message
            Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
            if (msg == NULL)
                return;
            lsp_finally { safe_release(msg); };

            // Initialize the message
            msg->setMessageID(vst3::ID_MSG_DUMP_STATE);

            // Send the message
            pPeerConnection->notify(msg);
        }

        const meta::package_t *UIWrapper::package() const
        {
            return pPackage;
        }

        status_t UIWrapper::play_file(const char *file, wsize_t position, bool release)
        {
            // Create message
            if (pPeerConnection == NULL)
                return STATUS_OK;

            // Allocate new message
            Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
            if (msg == NULL)
                return STATUS_OK;
            lsp_finally { safe_release(msg); };

            // Initialize the message
            msg->setMessageID(vst3::ID_MSG_PLAY_SAMPLE);
            Steinberg::Vst::IAttributeList *atts = msg->getAttributes();

            // Set endianess
            Steinberg::tresult res;
            if ((res = atts->setInt("endian", VST3_BYTEORDER)) != Steinberg::kResultOk)
            {
                lsp_warn("Failed to set property 'endian'");
                return STATUS_OK;
            }

            // Set file name
            if (!sNotifyBuf.set_string(atts, "file", file))
            {
                lsp_warn("Failed to set property 'file' to %s", file);
                return STATUS_OK;
            }

            // Set play position
            if ((res = atts->setInt("position", position)) != Steinberg::kResultOk)
            {
                lsp_warn("Failed to set property 'position' to %lld", static_cast<long long>(position));
                return STATUS_OK;
            }

            // Get release flag
            if ((res = atts->setFloat("release", release)) != Steinberg::kResultOk)
            {
                lsp_warn("Failed to set property 'release' to %s", (release) ? "true" : "false");
                return STATUS_OK;
            }

            // Finally, we're ready to send message
            return (pPeerConnection->notify(msg) == Steinberg::kResultOk) ? STATUS_OK : STATUS_UNKNOWN_ERR;
        }

        float UIWrapper::ui_scaling_factor(float scaling)
        {
            return (fScalingFactor > 0.0f) ? fScalingFactor : scaling;
        }

        bool UIWrapper::accept_window_size(tk::Window *wnd, size_t width, size_t height)
        {
            // Iterate over all views
            if (pPluginView != NULL)
            {
                status_t res = pPluginView->accept_window_size(wnd, width, height);
                if (res == STATUS_OK)
                    return true;
            }

            return ui::IWrapper::accept_window_size(wnd, width, height);
        }

        status_t UIWrapper::detach_ui(PluginView *view)
        {
            if (pPluginView != view)
                return STATUS_NOT_FOUND;
            lsp_finally { pPluginView = NULL; };

            // Notify backend about UI deactivation
            if (pPeerConnection != NULL)
            {
                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg == NULL)
                    return STATUS_OK;
                lsp_finally { safe_release(msg); };

                // Initialize and send the message
                msg->setMessageID(vst3::ID_MSG_DEACTIVATE_UI);
                pPeerConnection->notify(msg);
            }

            return STATUS_OK;
        }

        void UIWrapper::sync_ui()
        {
            if (pDisplay != NULL)
                pDisplay->main_iteration();
        }

        void UIWrapper::port_write(ui::IPort *port, size_t flags)
        {
            const meta::port_t *meta = port->metadata();
            if (meta::is_control_port(meta))
            {
                vst3::UIParameterPort *ip = static_cast<vst3::UIParameterPort *>(port);
                if (ip->is_virtual())
                {
                    // Check that we are available to send messages
                    if (pPeerConnection == NULL)
                        return;

                    // Allocate new message
                    Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                    if (msg == NULL)
                        return;
                    lsp_finally { safe_release(msg); };

                    // Initialize the message
                    msg->setMessageID(vst3::ID_MSG_VIRTUAL_PARAMETER);
                    Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                    // Write port identifier
                    if (!sNotifyBuf.set_string(list, "id", meta->id))
                        return;
                    // Write flags
                    if (list->setInt("flags", flags) != Steinberg::kResultOk)
                        return;
                    // Write the actual value
                    if (list->setFloat("value", ip->value()) != Steinberg::kResultOk)
                        return;

                    // Finally, we're ready to send message
                    pPeerConnection->notify(msg);
                }
                else
                {
                    if (pComponentHandler == NULL)
                        return;

                    const float valueNormalized = to_vst_value(meta, ip->value());
                    const Steinberg::Vst::ParamID param_id = ip->parameter_id();
                    pComponentHandler->beginEdit(param_id);
                    pComponentHandler->performEdit(param_id, valueNormalized);
                    pComponentHandler->endEdit(param_id);
                }
            }
            else if (meta::is_path_port(meta))
            {
                // Check that we are available to send messages
                if (pPeerConnection == NULL)
                    return;

                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication);
                if (msg == NULL)
                    return;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_PATH);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                // Write port identifier
                if (!sNotifyBuf.set_string(list, "id", meta->id))
                    return;
                // Write endianess
                if (list->setInt("endian", VST3_BYTEORDER) != Steinberg::kResultOk)
                    return;
                // Write the actual value
                if (list->setFloat("flags", flags) != Steinberg::kResultOk)
                    return;
                // Write port identifier
                if (!sNotifyBuf.set_string(list, "value", meta->id))
                    return;

                // Finally, we're ready to send message
                pPeerConnection->notify(msg);
            }
        }

        void UIWrapper::set_scaling_factor(float factor)
        {
            fScalingFactor      = factor;
        }

#ifdef VST_USE_RUNLOOP_IFACE
        Steinberg::Linux::IRunLoop *UIWrapper::acquire_run_loop()
        {
            Steinberg::Linux::IRunLoop *run_loop = safe_query_iface<Steinberg::Linux::IRunLoop>(pHostContext);
            if (run_loop != NULL)
                return run_loop;

            return pFactory->acquire_run_loop();
        }
#endif /* VST_USE_RUNLOOP_IFACE */

        status_t UIWrapper::slot_ui_resize(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);

            UIWrapper *self     = static_cast<UIWrapper *>(ptr);
            tk::Window *wnd     = self->window();
            if ((wnd == NULL) || (!wnd->visibility()->get()))
                return STATUS_OK;

            ws::rectangle_t rr;
            if (wnd->get_screen_rectangle(&rr) != STATUS_OK)
                return STATUS_OK;

            if (self->pPluginView != NULL)
                self->pPluginView->query_resize(&rr);

            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_show(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_realized(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            return STATUS_OK;
        }

        status_t UIWrapper::slot_ui_close(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            return STATUS_OK;
        }

        status_t UIWrapper::slot_display_idle(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            UIWrapper *self = static_cast<UIWrapper *>(ptr);
            if (self != NULL)
                self->main_iteration();

            return STATUS_OK;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_UI_WRAPPER_H_ */
