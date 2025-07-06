/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 6 февр. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_CONTROLLER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_CONTROLLER_H_

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
#include <lsp-plug.in/plug-fw/core/KVTDispatcher.h>
#include <lsp-plug.in/plug-fw/core/ShmStateBuilder.h>
#include <lsp-plug.in/plug-fw/core/ShmState.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/controller.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/debug.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>

#include <steinberg/vst3.h>

#ifdef WITH_UI_FEATURE
    #include <lsp-plug.in/plug-fw/wrap/vst3/ui_wrapper.h>
#endif /* WITH_UI_FEATURE */

namespace lsp
{
    namespace vst3
    {
        Controller::Controller(
            PluginFactory *factory,
            resource::ILoader *loader,
            const meta::package_t *package,
            const meta::plugin_t *meta):
            sShmState(shm_state_deleter)
        {
            lsp_trace("this=%p", this);

            atomic_store(&nRefCounter, 1);
            pFactory            = safe_acquire(factory);
            pLoader             = loader;
            pPackage            = package;
            pUIMetadata         = meta;
            pHostContext        = NULL;
            pHostApplication    = NULL;
            pPeerConnection     = NULL;
            pComponentHandler   = NULL;
            pComponentHandler2  = NULL;
            pComponentHandler3  = NULL;
            pOscPacket          = NULL;
            nLatency            = 0;
            fScalingFactor      = -1.0f;
            bMidiMapping        = false;
            bMsgWorkaround      = false;

            core::init_preset_state(&sPresetState);
        }

        Controller::~Controller()
        {
            lsp_trace("this=%p", this);

            pFactory->unregister_data_sync(this);

            destroy();

            // Release factory
            safe_release(pFactory);
        }

        ssize_t Controller::compare_param_ports(const vst3::CtlParamPort *a, const vst3::CtlParamPort *b)
        {
            const Steinberg::Vst::ParamID a_id = a->parameter_id();
            const Steinberg::Vst::ParamID b_id = b->parameter_id();

            return (a_id > b_id) ? 1 :
                   (a_id < b_id) ? -1 : 0;
        }

        vst3::CtlPort *Controller::create_port(const meta::port_t *port, const char *postfix)
        {
            // Find the matching port for the backend
            vst3::CtlPort *vup = NULL;

            switch (port->role)
            {
                case meta::R_AUDIO_IN:
                case meta::R_AUDIO_OUT:
                    // Stub port
                    lsp_trace("creating stub audio port %s", port->id);
                    vup = new vst3::CtlPort(port);
                    break;

                case meta::R_AUDIO_SEND:
                case meta::R_AUDIO_RETURN:
                    // Stub port
                    lsp_trace("creating stub audio buffer port %s", port->id);
                    vup = new vst3::CtlPort(port);
                    break;

                case meta::R_MIDI_IN:
                    lsp_trace("creating stub input MIDI port %s", port->id);
                    vup = new vst3::CtlPort(port);
                    bMidiMapping        = true;
                    break;

                case meta::R_MIDI_OUT:
                    lsp_trace("creating stub output MIDI port %s", port->id);
                    vup = new vst3::CtlPort(port);
                    break;

                case meta::R_MESH:
                    lsp_trace("creating mesh port %s", port->id);
                    vup = new vst3::CtlMeshPort(port);
                    break;

                case meta::R_STREAM:
                    lsp_trace("creating stream port %s", port->id);
                    vup = new vst3::CtlStreamPort(port);
                    break;

                case meta::R_FBUFFER:
                    lsp_trace("creating fbuffer port %s", port->id);
                    vup = new vst3::CtlFrameBufferPort(port);
                    break;

                case meta::R_OSC_IN:
                case meta::R_OSC_OUT:
                    break;

                case meta::R_PATH:
                    lsp_trace("creating path port %s", port->id);
                    vup = new vst3::CtlPathPort(port, this);
                    break;

                case meta::R_STRING:
                case meta::R_SEND_NAME:
                case meta::R_RETURN_NAME:
                    lsp_trace("creating string port %s", port->id);
                    vup = new vst3::CtlStringPort(port, this);
                    break;

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    const Steinberg::Vst::ParamID id = vst3::gen_parameter_id(port->id);
                    lsp_trace("creating parameter port %s id=0x%08x", port->id, int(id));

                    vst3::CtlParamPort *p   = new vst3::CtlParamPort(port, this, id, postfix != NULL);
                    if (postfix == NULL)
                        vParams.add(p);
                    vup = p;
                    break;
                }

                case meta::R_METER:
                {
                    lsp_trace("creating meter port %s", port->id);
                    vst3::CtlMeterPort *p   = new vst3::CtlMeterPort(port);
                    vMeters.add(p);
                    vup = p;
                    break;
                }

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];
                    const Steinberg::Vst::ParamID id = vst3::gen_parameter_id(port->id);

                    lsp_trace("creating port group %s id=0x%08x", port->id, int(id));
                    vst3::CtlPortGroup *upg = new vst3::CtlPortGroup(port, this, id, postfix != NULL);

                    // Add immediately port group to list
                    vPorts.add(upg);
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

        vst3::CtlParamPort *Controller::find_param(Steinberg::Vst::ParamID param_id)
        {
            ssize_t first=0, last = vParams.size() - 1;
            while (first <= last)
            {
                size_t center           = size_t(first + last) >> 1;
                vst3::CtlParamPort *p   = vParams.uget(center);
                if (param_id == p->parameter_id())
                    return p;
                else if (param_id < p->parameter_id())
                    last    = center - 1;
                else
                    first   = center + 1;
            }
            return NULL;
        }

        ssize_t Controller::compare_ports_by_id(const vst3::CtlPort *a, const vst3::CtlPort *b)
        {
            const meta::port_t *pa = a->metadata(), *pb = b->metadata();
            if (pa == NULL)
                return (pb == NULL) ? 0 : -1;
            else if (pb == NULL)
                return 1;

            return strcmp(pa->id, pb->id);
        }

        void Controller::shm_state_deleter(core::ShmState *state)
        {
            delete state;
        }

        status_t Controller::init()
        {
            lsp_trace("this=%p", this);

            // Perform all port bindings
            for (const meta::port_t *port = pUIMetadata->ports ; port->id != NULL; ++port)
                create_port(port, NULL);

            // Create MIDI CC mapping ports if we have MIDI mapping
            if (bMidiMapping)
            {
                char port_id[32], port_desc[32];
                meta::port_t meta =
                {
                    port_id,
                    port_desc,
                    NULL,
                    meta::U_NONE,
                    meta::R_CONTROL,
                    meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
                    0.0f, 1.0f, 0.0f, 0.00001f,
                    NULL, NULL, NULL
                };

                // Generate ports for all possible control codes
                Steinberg::Vst::ParamID id = MIDI_MAPPING_PARAM_BASE;

                for (size_t i=0; i<midi::MIDI_CHANNELS; ++i)
                {
                    for (size_t j=0; j<Steinberg::Vst::kCountCtrlNumber; ++j, ++id)
                    {
                        snprintf(port_id, sizeof(port_id), "midicc_%d_%d", int(j), int(i));
                        snprintf(port_desc, sizeof(port_id), "MIDI CC=%d | C=%d", int(j), int(i));

                        meta::port_t *cm    = clone_single_port_metadata(&meta);
                        if (cm == NULL)
                            return STATUS_NO_MEM;
                        vGenMetadata.add(cm);

                        lsp_trace("creating MIDI CC port %s id=0x%08x", cm->id, id);
                        vst3::CtlParamPort *p   = new vst3::CtlParamPort(cm, this, id, false);

                        vParams.add(p);
                        vPorts.add(p);
                    }
                }
            }

            vPlainParams.add(vParams);
            vParams.qsort(compare_param_ports);
            vPorts.qsort(compare_ports_by_id);

            return STATUS_OK;
        }

        void Controller::destroy()
        {
            lsp_trace("this=%p", this);

            // Unregister data sync
            pFactory->unregister_data_sync(this);

            // Remove port bindings
            vPlainParams.flush();
            vParams.flush();
            vMeters.flush();

            // Destroy ports
            for (size_t i=0, n=vPorts.size(); i<n; ++i)
            {
                vst3::CtlPort *p    = vPorts.uget(i);
                lsp_trace("destroy controller port id=%s", p->id());
                delete p;
            }

            // Cleanup generated metadata
            for (size_t i=0; i<vGenMetadata.size(); ++i)
            {
                meta::port_t *p = vGenMetadata.uget(i);
                lsp_trace("destroy generated port metadata %p", p);
                drop_port_metadata(p);
            }
            vGenMetadata.flush();
        }

        Steinberg::tresult PLUGIN_API Controller::queryInterface(const Steinberg::TUID _iid, void **obj)
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
            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IMidiMapping::iid))
                return cast_interface<Steinberg::Vst::IMidiMapping>(this, obj);

            return no_interface(obj);
        }

        Steinberg::uint32 PLUGIN_API Controller::addRef()
        {
            return atomic_add(&nRefCounter, 1) + 1;
        }

        Steinberg::uint32 PLUGIN_API Controller::release()
        {
            atomic_t ref_count = atomic_add(&nRefCounter, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        void PLUGIN_API Controller::update(FUnknown *changedUnknown, Steinberg::int32 message)
        {
            lsp_trace("this=%p, changedUnknown=%p, message=%d", this, changedUnknown, int(message));
        }

        Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown *context)
        {
            lsp_trace("this=%p, context=%p", this, context);

            // Acquire host context
            if (pHostContext != NULL)
                return Steinberg::kResultFalse;

            Steinberg::Linux::IRunLoop *run_loop = safe_query_iface<Steinberg::Linux::IRunLoop>(context);
            lsp_trace("RUN LOOP object=%p", run_loop);
            safe_release(run_loop);

            pHostContext        = safe_acquire(context);
            pHostApplication    = safe_query_iface<Steinberg::Vst::IHostApplication>(context);
            bMsgWorkaround      = use_message_workaround(pHostApplication);

            // Allocate OSC packet data
            lsp_trace("Creating OSC data buffer");
            pOscPacket      = reinterpret_cast<uint8_t *>(::malloc(OSC_PACKET_MAX));
            if (pOscPacket == NULL)
                return Steinberg::kOutOfMemory;

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Controller::terminate()
        {
            lsp_trace("this=%p", this);

            // Unregister data sync
            pFactory->unregister_data_sync(this);

            if (pOscPacket != NULL)
            {
                free(pOscPacket);
                pOscPacket      = NULL;
            }

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

        Steinberg::tresult PLUGIN_API Controller::connect(Steinberg::Vst::IConnectionPoint *other)
        {
            lsp_trace("this=%p, other=%p", this, other);

            // Check if peer connection is valid and was not previously estimated
            if (other == NULL)
                return Steinberg::kInvalidArgument;
            if (pPeerConnection)
                return Steinberg::kResultFalse;

            // Save the peer connection
            pPeerConnection = safe_acquire(other);

            // Register data sync
            if (pPeerConnection != NULL)
                pFactory->register_data_sync(this);

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Controller::disconnect(Steinberg::Vst::IConnectionPoint *other)
        {
            lsp_trace("this=%p, other=%p", this, other);

            // Unregister data sync
            pFactory->unregister_data_sync(this);

            // Check that estimated peer connection matches the esimated one
            if (other == NULL)
                return Steinberg::kInvalidArgument;
            if (pPeerConnection != other)
                return Steinberg::kResultFalse;

            // Reset the peer connection
            safe_release(pPeerConnection);

            return Steinberg::kResultOk;
        }

        vst3::CtlPort *Controller::port_by_id(const char *id)
        {
            // Try to find the corresponding port
            ssize_t first = 0, last     = vPorts.size() - 1;
            while (first <= last)
            {
                size_t center           = (first + last) >> 1;
                vst3::CtlPort *p        = vPorts.uget(center);
                if (p == NULL)
                    break;
                const meta::port_t *ctl = p->metadata();
                if (ctl == NULL)
                    break;

                int cmp     = strcmp(id, ctl->id);
                if (cmp < 0)
                    last    = center - 1;
                else if (cmp > 0)
                    first   = center + 1;
                else
                    return p;
            }

            return NULL;
        }

        Steinberg::tresult PLUGIN_API Controller::notify(Steinberg::Vst::IMessage *message)
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
                // lsp_trace("Received message id=%s", message_id);

                Steinberg::int64 latency = 0;

                // Write the actual value
                if (atts->getInt("value", latency) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                if (nLatency != latency)
                {
                    lsp_trace("Plugin latency changed from %d to %d", int(nLatency), int(latency));

                    nLatency    = latency;
                    pComponentHandler->restartComponent(Steinberg::Vst::RestartFlags::kLatencyChanged);
                }
            }
            else if (!strcmp(message_id, ID_MSG_STATE_DIRTY))
            {
                // lsp_trace("Received message id=%s", message_id);

                // Mark state as dirty
                if (pComponentHandler2 != NULL)
                {
                    lsp_trace("Notified component handler about dirty state");
                    pComponentHandler2->setDirty(true);
                }
                else
                {
                    lsp_trace("pComponentHandler2 is NULL");
                }
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

            #ifdef WITH_UI_FEATURE
                // Notify UI about position update
                lltl::parray<UIWrapper> receivers;
                if (sWrappersLock.lock())
                {
                    lsp_finally { sWrappersLock.unlock(); };
                    receivers.add(vWrappers);
                }

                for (lltl::iterator<UIWrapper> it = receivers.values(); it; ++it)
                {
                    UIWrapper *w = it.get();
                    if (w != NULL)
                        w->commit_position(&pos);
                }
            #endif /* WITH_UI_FEATURE */
            }
            else if (!strcmp(message_id, ID_MSG_PLAY_SAMPLE_POSITION))
            {
                Steinberg::int64 position = 0, length = 0;

                if (atts->getInt("position", position) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getInt("length", length) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // lsp_trace("Received play position = %lld, length=%lld", (long long)position, (long long)length);

            #ifdef WITH_UI_FEATURE
                // Notify UI about position update
                lltl::parray<UIWrapper> receivers;
                if (sWrappersLock.lock())
                {
                    lsp_finally { sWrappersLock.unlock(); };
                    receivers.add(vWrappers);
                }

                for (lltl::iterator<UIWrapper> it = receivers.values(); it; ++it)
                {
                    UIWrapper *w = it.get();
                    if (w != NULL)
                        w->set_play_position(position, length);
                }
            #endif /* WITH_UI_FEATURE */
            }
            else if (!strcmp(message_id, ID_MSG_METERS))
            {
                double value = 0.0;
//                lsp_trace("Received message id=%s", message_id);

                // Get actual value for each meter
                for (lltl::iterator<vst3::CtlMeterPort> it = vMeters.values(); it; ++it)
                {
                    vst3::CtlMeterPort *p = it.get();
                    if (p == NULL)
                        continue;

                    // Fetch meter value
                    if (atts->getFloat(p->id(), value) != Steinberg::kResultOk)
                        continue;

                    // Sync value with meter port and notify listeners if value has changed
//                    lsp_trace("Received meter id=%s, value=%f", p->id(), value);
                    if (p->commit_value(value))
                        p->mark_changed();
                }
            }
            else if (!strcmp(message_id, ID_MSG_MESH))
            {
//                lsp_trace("Received message id=%s", message_id);

                // Read endianess
                if (atts->getInt("endian", byte_order) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // Read identifier of the mesh port
                const char *param_id = sRxNotifyBuf.get_string(atts, "id", byte_order);
                if (param_id == NULL)
                {
                    lsp_trace("No mesh port identifier");
                    return Steinberg::kResultFalse;
                }

                // Get port and validate it's type
                vst3::CtlPort *port     = port_by_id(param_id);
                if (port == NULL)
                {
                    lsp_trace("Not found mesh port id=%s", param_id);
                    return Steinberg::kResultFalse;
                }
                const meta::port_t *meta = port->metadata();
                if ((meta == NULL) || (!meta::is_mesh_port(meta)))
                {
                    lsp_trace("Not a mesh port id=%s", param_id);
                    return Steinberg::kResultFalse;
                }

                // Read number of buffers
                Steinberg::int64 buffers = 0;
                if (atts->getInt("buffers", buffers) != Steinberg::kResultOk)
                {
                    lsp_trace("Not found attribute 'buffers'");
                    return Steinberg::kResultFalse;
                }
                if ((buffers < 0) || (buffers > meta->step))
                {
                    lsp_trace("Invalid value buffers=%d", int(buffers));
                    return Steinberg::kResultFalse;
                }

                // Read number of elements per buffer
                Steinberg::int64 items = 0;
                if (atts->getInt("items", items) != Steinberg::kResultOk)
                {
                    lsp_trace("Not found attribute 'items'");
                    return Steinberg::kResultFalse;
                }
                if ((items < 0) || (items > meta->start))
                {
                    lsp_trace("Invalid value items=%d", int(items));
                    return Steinberg::kResultFalse;
                }

                // Encode data for each buffer
                plug::mesh_t *mesh = port->buffer<plug::mesh_t>();
                const void *data = NULL;
                Steinberg::uint32 sizeInBytes = 0;
                for (size_t i=0, n=size_t(buffers); i<n; ++i)
                {
                    snprintf(key, sizeof(key), "data[%d]", int(i));
                    if (atts->getBinary(key, data, sizeInBytes) != Steinberg::kResultOk)
                    {
                        lsp_trace("Failed to get binary for key=%s", key);
                        return Steinberg::kResultFalse;
                    }
                    if (sizeInBytes != items * sizeof(float))
                    {
                        lsp_trace("size of binary key=%s does not match: size=%d, expected=%d",
                            key, int(sizeInBytes), int(items * sizeof(float)));
                        return Steinberg::kResultFalse;
                    }

                    if (byte_order != VST3_BYTEORDER)
                    {
                        byte_swap_copy(mesh->pvData[i], static_cast<const float *>(data), items);
                        dsp::saturate(mesh->pvData[i], items);
                    }
                    else
                        dsp::copy_saturated(mesh->pvData[i], static_cast<const float *>(data), items);
                }

                // Update state of the mesh and notify
//                lsp_trace("Committed mesh data buffers=%d, items=%d", int(buffers), int(items));
                mesh->data(buffers, items);
                port->mark_changed();
            }
            else if (!strcmp(message_id, ID_MSG_FRAMEBUFFER))
            {
                // lsp_trace("Received message id=%s", message_id);

                // Read endianess
                if (atts->getInt("endian", byte_order) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // Read identifier of the mesh port
                const char *param_id = sRxNotifyBuf.get_string(atts, "id", byte_order);
                if (param_id == NULL)
                    return Steinberg::kResultFalse;

                // Get port and validate it's type
                vst3::CtlPort *port     = port_by_id(param_id);
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
                port->mark_changed();
            }
            else if (!strcmp(message_id, ID_MSG_STREAM))
            {
//                lsp_trace("Received message id=%s", message_id);

                // Read endianess
                if (atts->getInt("endian", byte_order) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // Read identifier of the mesh port
                const char *param_id = sRxNotifyBuf.get_string(atts, "id", byte_order);
                if (param_id == NULL)
                    return Steinberg::kResultFalse;

                // Get port and validate it's type
                vst3::CtlPort *port    = port_by_id(param_id);
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
                        {
//                            lsp_trace("mismatched size in bytes %d vs %d", int(sizeInBytes), int(frame_size * sizeof(float)));
                            return Steinberg::kResultFalse;
                        }

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
//                    lsp_trace("deserialized frame id=%d of %d elements", int(frame_id), int(f_size));
                }
                port->mark_changed();
            }
            else if (!strcmp(message_id, ID_MSG_STRING))
            {
                // Get endianess
                if (atts->getInt("endian", byte_order) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // Get port identifier
                const char *param_id = sRxNotifyBuf.get_string(atts, "id", byte_order);
                if (param_id == NULL)
                    return Steinberg::kResultFalse;

                // Find string port
                // Get port and validate it's type
                vst3::CtlPort *port    = port_by_id(param_id);
                if (port == NULL)
                    return Steinberg::kResultFalse;
                const meta::port_t *meta = port->metadata();
                if ((meta == NULL) || (!meta::is_string_holding_port(meta)))
                    return Steinberg::kResultFalse;

                // Get the string data
                const char *in_str = sRxNotifyBuf.get_string(atts, "value", byte_order);
                if (in_str == NULL)
                    return Steinberg::kResultFalse;

                // Submit string data
                lsp_trace("string %s = %s", meta->id, in_str);
                port->write(in_str, strlen(in_str));
                port->mark_changed();
            }
            else if (!strcmp(message_id, ID_MSG_SHM_STATE))
            {
                // Get endianess
                if (atts->getInt("endian", byte_order) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // Read number of frames
                Steinberg::int64 size = 0;
                if (atts->getInt("size", size) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (size < 0)
                    return Steinberg::kResultFalse;

                LSPString id, name;
                core::ShmStateBuilder bld;

                for (size_t i=0, n=size; i<n; ++i)
                {
                    // Read record index
                    Steinberg::int64 index = 0;
                    snprintf(key, sizeof(key), "index[%d]", int(i));
                    if (atts->getInt(key, index) != Steinberg::kResultOk)
                        return Steinberg::kResultFalse;

                    // Read record magic
                    Steinberg::int64 magic = 0;
                    snprintf(key, sizeof(key), "magic[%d]", int(i));
                    if (atts->getInt(key, magic) != Steinberg::kResultOk)
                        return Steinberg::kResultFalse;

                    // Read record identifier
                    snprintf(key, sizeof(key), "id[%d]", int(i));
                    const char *id_str = sRxNotifyBuf.get_string(atts, key, byte_order);
                    if (id_str == NULL)
                        return Steinberg::kResultFalse;
                    if (!id.set_utf8(id_str))
                        return Steinberg::kResultFalse;

                    // Read record name
                    snprintf(key, sizeof(key), "name[%d]", int(i));
                    const char *name_str = sRxNotifyBuf.get_string(atts, key, byte_order);
                    if (name_str == NULL)
                        return Steinberg::kResultFalse;
                    if (!name.set_utf8(name_str))
                        return Steinberg::kResultFalse;

                    // Add record to builder
                    if (bld.append(name, id, index, magic) != STATUS_OK)
                        return Steinberg::kResultFalse;
                }

                // Push new state
                core::ShmState *state = bld.build();
                if (state != NULL)
                {
                    sShmState.push(state);
                    lsp_trace("Pushed new shared memory state");
                }
            }
            else if (!strcmp(message_id, ID_MSG_KVT))
            {
                lsp_trace("Received message id=%s", message_id);

                const void *data = NULL;
                Steinberg::uint32 sizeInBytes = 0;

                if (atts->getBinary("data", data, sizeInBytes) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                if (sKVTMutex.lock())
                {
                    receive_raw_osc_packet(data, sizeInBytes);
                    sKVTMutex.unlock();
                }
            }
        #ifdef WITH_UI_FEATURE
            else if (!strcmp(message_id, ID_MSG_PRESET_STATE))
            {
                // Get endianess
                if (atts->getInt("endian", byte_order) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                Steinberg::int64 flags = 0, tab = 0;

                if (atts->getInt("flags", flags) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;
                if (atts->getInt("tab", tab) != Steinberg::kResultOk)
                    return Steinberg::kResultFalse;

                // Read identifier of the mesh port
                const char *name = sRxNotifyBuf.get_string(atts, "name", byte_order);
                if (name == NULL)
                    return Steinberg::kResultFalse;

                lsp_trace("Received preset state flags=0x%x, tab=%d, name='%s'",
                    int(flags), int(tab), name);

                // Commit state
                sPresetState.flags  = uint32_t(flags);
                sPresetState.tab    = uint32_t(tab);
                const size_t length = lsp_min(strnlen(name, core::PRESET_NAME_BYTES), core::PRESET_NAME_BYTES - 1);
                memcpy(sPresetState.name, name, length);
                sPresetState.name[length]   = '\0';

                receive_preset_state(NULL);
            }
        #endif /* WITH_UI_FEATURE */

            return Steinberg::kResultOk;
        }

        void Controller::parse_raw_osc_event(osc::parse_frame_t *frame)
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

        void Controller::receive_raw_osc_packet(const void *data, size_t size)
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

        status_t Controller::load_state(Steinberg::IBStream *is)
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

            bool param_types = false;
            if (version == 2)
                param_types     = true;
            else if (version != 1)
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

                if (name[0] == '/')
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

                    // This is KVT port, skip private data for DSP code
                    if ((p.type != core::KVT_ANY) && (!(flags & vst3::FLAG_PRIVATE)))
                    {
                        size_t kflags = core::KVT_TX;
                        kvt_dump_parameter("Fetched KVT parameter %s = ", &p, name);
                        sKVT.put(name, &p, kflags);
                    }
                }
                else if (name[0] == '!')
                {
                    if (strcmp(name, "!preset_state") == 0)
                    {
                        core::preset_state_t state;
                        if ((res = vst3::deserialize_preset_state(&state, is)) != STATUS_OK)
                        {
                            lsp_warn("Failed to deserialize preset state, error code=%d", int(res));
                            return res;
                        }

                        core::copy_preset_state(&sPresetState, &state);
                    }
                    else
                    {
                        lsp_warn("Unknown special variable: %s, skipping", name);
                        if ((res = read_string(is, &name, &name_cap)) != STATUS_OK)
                        {
                            lsp_warn("Failed to skip special variable", int(res));
                            return res;
                        }
                    }
                }
                else
                {
                    // Read parameter type
                    uint8_t param_type      = '?';
                    if (param_types)
                    {
                        if ((res = read_fully(is, &param_type)) != STATUS_OK)
                        {
                            lsp_warn("Failed to deserialize parameter type, error code=%d", int(res));
                            return res;
                        }
                    }

                    // Try to find virtual port
                    vst3::CtlPort *p        = port_by_id(name);
                    if (p != NULL)
                    {
                        const meta::port_t *meta = p->metadata();
                        if (meta::is_path_port(meta))
                        {
                            if ((param_type != '?') && (param_type != 's'))
                            {
                                lsp_warn("Failed to deserialize port id=%s: invalid parameter type '%c'", meta->id, char(param_type));
                                return res;
                            }

                            if ((res = read_string(is, &name, &name_cap)) != STATUS_OK)
                            {
                                lsp_warn("Failed to deserialize port id=%s", meta->id);
                                return res;
                            }

                            vst3::CtlPathPort *pp       = static_cast<vst3::CtlPathPort *>(p);
                            lsp_trace("  %s = %s", meta->id, name);
                            pp->commit_value(name);
                            pp->mark_changed();
                        }
                        else if (meta::is_string_holding_port(meta))
                        {
                            if ((param_type != '?') && (param_type != 's'))
                            {
                                lsp_warn("Failed to deserialize port id=%s: invalid parameter type '%c'", meta->id, char(param_type));
                                return res;
                            }

                            if ((res = read_string(is, &name, &name_cap)) != STATUS_OK)
                            {
                                lsp_warn("Failed to deserialize port id=%s", meta->id);
                                return res;
                            }

                            vst3::CtlStringPort *sp     = static_cast<vst3::CtlStringPort *>(p);
                            lsp_trace("  %s = %s", meta->id, name);
                            sp->commit_value(name);
                            sp->mark_changed();
                        }
                        else if ((meta::is_control_port(meta)) || (meta::is_bypass_port(meta)) || (meta::is_port_set_port(meta)))
                        {
                            if ((param_type != '?') && (param_type != 'f'))
                            {
                                lsp_warn("Failed to deserialize port id=%s: invalid parameter type '%c'", meta->id, char(param_type));
                                return res;
                            }

                            float v = 0.0f;
                            if ((res = read_fully(is, &v)) != STATUS_OK)
                            {
                                lsp_warn("Failed to deserialize port id=%s", name);
                                return res;
                            }

                            vst3::CtlParamPort *pp      = static_cast<vst3::CtlParamPort *>(p);
                            lsp_trace("  %s = %f", meta->id, v);
                            pp->commit_value(v);
                            pp->mark_changed();
                        }
                        else
                            lsp_warn("Port id=%s is present in serial data but has invalid type", name);
                    }
                    else
                    {
                        lsp_warn("Missing port id=%s, skipping", name);
                        switch (param_type)
                        {
                            case 'f':
                            {
                                float v;
                                if ((res = read_fully(is, &v)) != STATUS_OK)
                                {
                                    lsp_warn("Failed to skip floating-point parameter");
                                    return res;
                                }
                                break;
                            }
                            case 's':
                            {
                                if ((res = read_string(is, &name, &name_cap)) != STATUS_OK)
                                {
                                    lsp_warn("Failed to skip string");
                                    return res;
                                }
                                break;
                            }
                            case '?':
                                break;
                            default:
                                lsp_warn("Unknown parameter type: '%c'", param_type);
                                return STATUS_CORRUPTED;
                        }
                    }
                }
            }

            // Analyze result
            return (res == STATUS_EOF) ? STATUS_OK : STATUS_CORRUPTED;
        }

        Steinberg::tresult PLUGIN_API Controller::setState(Steinberg::IBStream *state)
        {
            lsp_trace("this=%p, state=%p", this, state);
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Controller::getState(Steinberg::IBStream *state)
        {
            lsp_trace("this=%p, state=%p", this, state);
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Controller::setComponentState(Steinberg::IBStream *state)
        {
            lsp_trace("this=%p, state=%p", this, state);

            IF_TRACE(
                DbgInStream is(state);
                state = &is;
                lsp_dumpb("State dump:", is.data(), is.size());
            );

            status_t res = load_state(state);
            if (res != STATUS_OK)
                return Steinberg::kInternalError;

        #ifdef WITH_UI_FEATURE
            receive_preset_state(NULL);
        #endif /* WITH_UI_FEATURE */

            return Steinberg::kResultOk;
        }

        Steinberg::int32 PLUGIN_API Controller::getParameterCount()
        {
            lsp_trace("this=%p, result=%d", this, int(vPlainParams.size()));
            return vPlainParams.size();
        }

        Steinberg::tresult PLUGIN_API Controller::getParameterInfo(Steinberg::int32 paramIndex, Steinberg::Vst::ParameterInfo & info /*out*/)
        {
            lsp_trace("this=%p, paramIndex=%d", this, int(paramIndex));

            vst3::CtlParamPort *p   = vPlainParams.get(paramIndex);
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

            IF_TRACE( log_parameter_info(&info); );

            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API Controller::getParamStringByValue(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized /*in*/, Steinberg::Vst::String128 string /*out*/)
        {
            lsp_trace("this=%p, id=0x%08x, valueNormalized=%f", this, int(id), valueNormalized);

            // Get port
            vst3::CtlParamPort *p   = find_param(id);
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

        Steinberg::tresult PLUGIN_API Controller::getParamValueByString(Steinberg::Vst::ParamID id, Steinberg::Vst::TChar *string /*in*/, Steinberg::Vst::ParamValue & valueNormalized /*out*/)
        {
            lsp_trace("this=%p, id=0x%08x, string=%s", this, int(id), string);

            // Get port
            vst3::CtlParamPort *p   = find_param(id);
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

        Steinberg::Vst::ParamValue PLUGIN_API Controller::normalizedParamToPlain(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized)
        {
//            lsp_trace("this=%p, id=0x%08x, valueNormalzed=%f", this, int(id), valueNormalized);

            // Get port
            vst3::CtlParamPort *p   = find_param(id);
            if (p == NULL)
            {
//                lsp_warn("parameter id=0x%08x not found", int(id));
                return 0.0;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return 0.0;

            // Convert value
            const float plainValue = from_vst_value(meta, valueNormalized);
            lsp_trace("id=%s, normalizedValue=%f -> plainValue=%f", p->metadata()->id, valueNormalized, plainValue);

            return plainValue;
        }

        Steinberg::Vst::ParamValue PLUGIN_API Controller::plainParamToNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue plainValue)
        {
//            lsp_trace("this=%p, id=0x%08x, plainValue=%f", this, int(id), plainValue);

            // Get port
            vst3::CtlParamPort *p   = find_param(id);
            if (p == NULL)
            {
//                lsp_warn("parameter id=0x%08x not found", int(id));
                return 0.0;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return 0.0;

            // Convert value
            const float valueNormalized = to_vst_value(meta, plainValue);
            lsp_trace("id=%s, plainValue=%f -> normalizedValue=%f", p->metadata()->id, plainValue, valueNormalized);

            return valueNormalized;
        }

        Steinberg::Vst::ParamValue PLUGIN_API Controller::getParamNormalized(Steinberg::Vst::ParamID id)
        {
//            lsp_trace("this=%p, id=0x%08x", this, int(id));

            // Get port
            vst3::CtlParamPort *p   = find_param(id);
            if (p == NULL)
            {
//                lsp_warn("parameter id=0x%08x not found", int(id));
                return 0.0;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return 0.0;

            // Convert value
            const float value   = p->value();
            const float result  = to_vst_value(meta, value);
            lsp_trace("id=%s, plainValue=%f -> normalizedValue=%f", p->metadata()->id, value, result);

            // Return result
            return result;
        }

        Steinberg::tresult PLUGIN_API Controller::setParamNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value)
        {
//            lsp_trace("this=%p, id=0x%08x, value=%f", this, int(id), value);

            // Get port
            vst3::CtlParamPort *p   = find_param(id);
            if (p == NULL)
            {
//                lsp_warn("parameter id=0x%08x not found", int(id));
                return Steinberg::kInvalidArgument;
            }
            const meta::port_t *meta = p->metadata();
            if (meta == NULL)
                return Steinberg::kInternalError;

            // Convert value
            const float plainValue = from_vst_value(meta, value);
            lsp_trace("id=%s, normalizedValue=%f -> plainValue=%f", p->metadata()->id, value, plainValue);

            // Commit value
            p->commit_value(plainValue);
            p->mark_changed();

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Controller::setComponentHandler(Steinberg::Vst::IComponentHandler *handler)
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

    #ifdef WITH_UI_FEATURE
        ui::Module *Controller::create_ui()
        {
            if ((pUIMetadata == NULL) || (pUIMetadata->uids.vst3ui == NULL))
            {
                lsp_trace("pUIMetadata is not valid");
                return NULL;
            }

            // Watch UI factories next
            for (ui::Factory *f = ui::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *plug_meta = f->enumerate(i);
                    if (plug_meta == NULL)
                        break;
                    if (plug_meta->uids.vst3ui == NULL)
                        continue;
                    if (memcmp(plug_meta->uids.vst3ui, pUIMetadata->uids.vst3ui, sizeof(Steinberg::TUID)) != 0)
                        continue;

                    // Allocate UI module
                    ui::Module *ui = f->create(plug_meta);
                    if (ui == NULL)
                        return NULL;
                    lsp_trace("Created UI module ptr=%p", ui);

                    // Return the UI
                    return ui;
                }
            }

            lsp_trace("Not found matching factory");

            return NULL;
        }

        status_t Controller::detach_ui_wrapper(UIWrapper *wrapper)
        {
            if (sWrappersLock.lock())
            {
                lsp_finally { sWrappersLock.unlock(); };
                if (!vWrappers.qpremove(wrapper))
                    return STATUS_NOT_FOUND;
            }

            // Notify backend about UI deactivation
            if (pPeerConnection != NULL)
            {
                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication, bMsgWorkaround);
                if (msg == NULL)
                    return STATUS_OK;
                lsp_finally { safe_release(msg); };

                // Initialize and send the message
                msg->setMessageID(vst3::ID_MSG_DEACTIVATE_UI);
                pPeerConnection->notify(msg);
            }

            return STATUS_OK;
        }

        void Controller::send_preset_state(UIWrapper *wrapper, const core::preset_state_t *state)
        {
            // Copy preset state
            core::copy_preset_state(&sPresetState, state);

            // Notify receivers
            receive_preset_state(wrapper);

            // Send message to DSP backend
            if (pPeerConnection != NULL)
            {
                LSPString name;
                if (!name.set_utf8(state->name))
                    return;

                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication, bMsgWorkaround);
                if (msg == NULL)
                    return;
                lsp_finally { safe_release(msg); };

                // Initialize and send the message
                msg->setMessageID(vst3::ID_MSG_PRESET_STATE);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                if (list->setInt("endian", VST3_BYTEORDER) != Steinberg::kResultOk)
                    return;
                if (list->setInt("flags", state->flags) != Steinberg::kResultOk)
                    return;
                if (list->setInt("tab", state->tab) != Steinberg::kResultOk)
                    return;
                if (list->setString("name", to_tchar(name.get_utf16())) != Steinberg::kResultOk)
                    return;

                // Notify peer
                pPeerConnection->notify(msg);
            }
        }

        Steinberg::IPlugView * PLUGIN_API Controller::createView(Steinberg::FIDString name)
        {
            lsp_trace("this=%p, name=%s", this, name);
            if (strcmp(name, Steinberg::Vst::ViewType::kEditor) != 0)
                return NULL;

            // Create UI
            ui::Module *ui = create_ui();
            if (ui == NULL)
            {
                lsp_trace("Failed to create UI");
                return NULL;
            }
            lsp_finally {
                if (ui != NULL)
                {
                    ui->destroy();
                    delete ui;
                }
            };

            // Create Wrapper
            UIWrapper *w = new UIWrapper(this, ui, pLoader);
            if (w == NULL)
            {
                lsp_trace("Failed to create wrapper");
                return NULL;
            }
            ui      = NULL; // Will be destroyed by wrapper

            // Initialize wrapper
            status_t res = w->init(NULL);
            if (res != STATUS_OK)
            {
                lsp_trace("Failed to initialize wrapper");
                w->destroy();
                delete w;
                return NULL;
            }

            // Add wrapper to list of wrappers
            if (sWrappersLock.lock())
            {
                lsp_finally { sWrappersLock.unlock(); };
                vWrappers.add(w);
            }

            // Notify backend about UI activation
            if (pPeerConnection != NULL)
            {
                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication, bMsgWorkaround);
                if (msg != NULL)
                {
                    lsp_finally { safe_release(msg); };
                    // Initialize and send the message
                    msg->setMessageID(vst3::ID_MSG_ACTIVATE_UI);
                    pPeerConnection->notify(msg);
                }
            }

            // Send peset state
            w->receive_preset_state(&sPresetState);

            // Return pointer to wrapper
            return w;
        }

        Steinberg::tresult PLUGIN_API Controller::openHelp(Steinberg::TBool onlyCheck)
        {
            lsp_trace("this=%p, onlyCheck=%d", this, int(onlyCheck));

            if (onlyCheck)
                return Steinberg::kResultOk;

            if (!sWrappersLock.lock())
                return Steinberg::kResultOk;
            lsp_finally { sWrappersLock.unlock(); };

            vst3::UIWrapper *w = vWrappers.last();
            return (w != NULL) ? w->show_help() : Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API Controller::openAboutBox(Steinberg::TBool onlyCheck)
        {
            lsp_trace("this=%p, onlyCheck=%d", this, int(onlyCheck));

            if (onlyCheck)
                return Steinberg::kResultOk;

            if (!sWrappersLock.lock())
                return Steinberg::kResultOk;
            lsp_finally { sWrappersLock.unlock(); };

            vst3::UIWrapper *w = vWrappers.last();
            return (w != NULL) ? w->show_about_box() : Steinberg::kResultOk;
        }
    #else
        Steinberg::IPlugView * PLUGIN_API Controller::createView(Steinberg::FIDString name)
        {
            return NULL;
        }

        Steinberg::tresult PLUGIN_API Controller::openHelp(Steinberg::TBool onlyCheck)
        {
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Controller::openAboutBox(Steinberg::TBool onlyCheck)
        {
            return Steinberg::kNotImplemented;
        }
    #endif /* WITH_UI_FEATURE */

        Steinberg::tresult PLUGIN_API Controller::setKnobMode(Steinberg::Vst::KnobMode mode)
        {
            lsp_trace("this=%p, mode=%d", this, int(mode));

            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API Controller::getMidiControllerAssignment(
            Steinberg::int32 busIndex,
            Steinberg::int16 channel,
            Steinberg::Vst::CtrlNumber midiControllerNumber,
            Steinberg::Vst::ParamID & id)
        {
            // Check that we have MIDI mapping support
            if (!bMidiMapping)
                return Steinberg::kNotImplemented;
            if (busIndex != 0)
                return Steinberg::kInvalidArgument;
            if ((channel < 0) || (channel >= Steinberg::int16(midi::MIDI_CHANNELS)))
                return Steinberg::kInvalidArgument;
            if ((midiControllerNumber < 0) || (midiControllerNumber >= Steinberg::Vst::kCountCtrlNumber))
                return Steinberg::kInvalidArgument;

            // Compute the actual mapping parameter identifier
            id      = vst3::MIDI_MAPPING_PARAM_BASE + channel * Steinberg::Vst::kCountCtrlNumber + midiControllerNumber;

            lsp_trace("this=%p, busIndex=%d, channel=%d, midiControllerNumber=%d -> id=0x%08x",
                this, int(busIndex), int(channel), int(midiControllerNumber), int(id));

            return Steinberg::kResultOk;
        }

        core::KVTStorage *Controller::kvt_storage()
        {
            return &sKVT;
        }

        ipc::Mutex &Controller::kvt_mutex()
        {
            return sKVTMutex;
        }

        void Controller::dump_state_request()
        {
            // Create message
            if (pPeerConnection == NULL)
                return;

            // Allocate new message
            Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication, bMsgWorkaround);
            if (msg == NULL)
                return;
            lsp_finally { safe_release(msg); };

            // Initialize the message
            msg->setMessageID(vst3::ID_MSG_DUMP_STATE);

            // Send the message
            pPeerConnection->notify(msg);
        }

        void Controller::notify_preset_changed()
        {
        #ifdef WITH_UI_FEATURE
            // Notify UI about preset change
            if (sWrappersLock.lock())
            {
                lsp_finally { sWrappersLock.unlock(); };
                for (lltl::iterator<UIWrapper> it = vWrappers.values(); it; ++it)
                {
                    UIWrapper *w = it.get();
                    if (w != NULL)
                        w->mark_active_preset_dirty();
                }
            }
        #endif /* WITH_UI_FEATURE */
        }

        const core::ShmState *Controller::shm_state()
        {
            return sShmState.get();
        }

        const meta::package_t *Controller::package() const
        {
            return pPackage;
        }

        status_t Controller::play_file(const char *file, wsize_t position, bool release)
        {
            // Create message
            if (pPeerConnection == NULL)
                return STATUS_OK;

            // Allocate new message
            Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication, bMsgWorkaround);
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
            if (file == NULL)
                file        = "";
            if (!sTxNotifyBuf.set_string(atts, "file", file))
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
            if ((res = atts->setFloat("release", (release) ? 1.0f : 0.0f)) != Steinberg::kResultOk)
            {
                lsp_warn("Failed to set property 'release' to %s", (release) ? "true" : "false");
                return STATUS_OK;
            }

            // Finally, we're ready to send message
            return (pPeerConnection->notify(msg) == Steinberg::kResultOk) ? STATUS_OK : STATUS_UNKNOWN_ERR;
        }

        void Controller::port_write(vst3::CtlPort *port, size_t flags)
        {
            const meta::port_t *meta = port->metadata();
            if (meta::is_path_port(meta))
            {
                const char *path = port->buffer<char>();
                lsp_trace("port write: id=%s, value='%s', flags=0x%x", port->id(), path, int(flags));

                // Check that we are available to send messages
                if (pPeerConnection == NULL)
                    return;

                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication, bMsgWorkaround);
                if (msg == NULL)
                    return;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_PATH);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                // Write port identifier
                if (!sTxNotifyBuf.set_string(list, "id", meta->id))
                    return;
                // Write endianess
                if (list->setInt("endian", VST3_BYTEORDER) != Steinberg::kResultOk)
                    return;
                // Write the actual flags value
                if (list->setInt("flags", flags) != Steinberg::kResultOk)
                    return;
                // Write port identifier
                if (!sTxNotifyBuf.set_string(list, "value", path))
                    return;

                // Finally, we're ready to send message
                pPeerConnection->notify(msg);
                notify_preset_changed();
            }
            else if (meta::is_string_holding_port(meta))
            {
                const char *str = port->buffer<char>();
                lsp_trace("port write: id=%s, value='%s'", port->id(), str);

                // Check that we are available to send messages
                if (pPeerConnection == NULL)
                    return;

                // Allocate new message
                Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication, bMsgWorkaround);
                if (msg == NULL)
                    return;
                lsp_finally { safe_release(msg); };

                // Initialize the message
                msg->setMessageID(vst3::ID_MSG_STRING);
                Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                // Write port identifier
                if (!sTxNotifyBuf.set_string(list, "id", meta->id))
                    return;
                // Write endianess
                if (list->setInt("endian", VST3_BYTEORDER) != Steinberg::kResultOk)
                    return;
                // Write port identifier
                if (!sTxNotifyBuf.set_string(list, "value", str))
                    return;

                // Finally, we're ready to send message
                pPeerConnection->notify(msg);
                notify_preset_changed();
            }
            else
            {
                vst3::CtlParamPort *ip = static_cast<vst3::CtlParamPort *>(port);

                lsp_trace("port write: id=%s, value=%f, flags=0x%x", ip->id(), ip->value(), int(flags));

                if (ip->is_virtual())
                {
                    // Check that we are available to send messages
                    if (pPeerConnection == NULL)
                        return;

                    // Allocate new message
                    Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication, bMsgWorkaround);
                    if (msg == NULL)
                        return;
                    lsp_finally { safe_release(msg); };

                    // Initialize the message
                    msg->setMessageID(vst3::ID_MSG_VIRTUAL_PARAMETER);
                    Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                    // Write port identifier
                    if (!sTxNotifyBuf.set_string(list, "id", meta->id))
                        return;
                    // Write flags
                    if (list->setInt("flags", flags) != Steinberg::kResultOk)
                        return;
                    // Write the actual value
                    if (list->setFloat("value", ip->value()) != Steinberg::kResultOk)
                        return;

                    // Finally, we're ready to send message
                    pPeerConnection->notify(msg);
                    notify_preset_changed();
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
                    notify_preset_changed();
                }
            }
        }

    #ifdef VST_USE_RUNLOOP_IFACE
        Steinberg::Linux::IRunLoop *Controller::acquire_run_loop()
        {
            Steinberg::Linux::IRunLoop *run_loop = safe_query_iface<Steinberg::Linux::IRunLoop>(pHostContext);
            if (run_loop != NULL)
                return run_loop;

            return pFactory->acquire_run_loop();
        }
    #endif /* VST_USE_RUNLOOP_IFACE */

        void Controller::send_kvt_state()
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
                res = core::KVTDispatcher::build_message(kvt_name, p, pOscPacket, &size, OSC_PACKET_MAX);
                if (res == STATUS_OK)
                {
                    lsp_trace("Sending UI->DSP KVT message of %d bytes", int(size));
                    osc::dump_packet(pOscPacket, size);

                    // Allocate new message
                    Steinberg::Vst::IMessage *msg = alloc_message(pHostApplication, bMsgWorkaround);
                    if (msg == NULL)
                        return;
                    lsp_finally { safe_release(msg); };

                    // Initialize the message
                    msg->setMessageID(vst3::ID_MSG_KVT);
                    Steinberg::Vst::IAttributeList *list = msg->getAttributes();

                    if (list->setBinary("data", pOscPacket, size) != Steinberg::kResultOk)
                        continue;
                    pPeerConnection->notify(msg);
                }

                // Commit transfer
                iter->commit(core::KVT_RX);
            }
        }

        void Controller::sync_data()
        {
            if ((pPeerConnection == NULL) || (pHostApplication == NULL))
                return;

            // Transmit KVT state
            if (sKVTMutex.lock())
            {
                send_kvt_state();
                sKVT.gc();
                sKVTMutex.unlock();
            }
        }

    #ifdef WITH_UI_FEATURE
        void Controller::receive_preset_state(ui::IWrapper *except)
        {
            // Notify UI about position update
            lltl::parray<UIWrapper> receivers;
            if (sWrappersLock.lock())
            {
                lsp_finally { sWrappersLock.unlock(); };
                receivers.add(vWrappers);
            }

            for (lltl::iterator<UIWrapper> it = receivers.values(); it; ++it)
            {
                UIWrapper *w = it.get();
                if ((w != NULL) && (w != except))
                    w->receive_preset_state(&sPresetState);
            }
        }
    #endif /* WITH_UI_FEATURE */

    } /* namespace vst3 */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_CONTROLLER_H_ */
