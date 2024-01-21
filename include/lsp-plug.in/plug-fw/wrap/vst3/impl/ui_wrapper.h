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
            nRefCounter         = 1;
            pFactory            = safe_acquire(factory);
            pPackage            = package;
            pHostContext        = NULL;
            pHostApplication    = NULL;
            pPeerConnection     = NULL;
            pComponentHandler   = NULL;
            pComponentHandler2  = NULL;
            pComponentHandler3  = NULL;
        }

        UIWrapper::~UIWrapper()
        {
            destroy();

            // Release factory
            safe_release(pFactory);
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
                    break;

                case meta::R_CONTROL:
                case meta::R_BYPASS:
                    lsp_trace("creating control port %s", port->id);
                    vup     = new vst3::UIInParamPort(port, this, postfix != NULL);
                    break;

                case meta::R_METER:
                    lsp_trace("creating meter port %s", port->id);
                    vup     = new vst3::UIOutParamPort(port);
                    break;

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES];
                    lsp_trace("creating port group %s", port->id);
                    vst3::UIPortGroup *upg = new vst3::UIPortGroup(port, this, postfix != NULL);

                    // Add immediately port group to list
                    vPorts.add(upg);

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

        status_t UIWrapper::init(void *root_widget)
        {
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

            // Initialize wrapper
            if ((res = ui::IWrapper::init(root_widget)) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        void UIWrapper::destroy()
        {
            // Destroy plugin UI
            if (pUI != NULL)
            {
                delete pUI;
                pUI         = NULL;
            }

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

            if (Steinberg::iidEqual(_iid, Steinberg::Vst::IComponent::iid))
                return cast_interface<Steinberg::Vst::IComponent>(this, obj);
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
            // TODO: implement this
        }

        Steinberg::tresult PLUGIN_API UIWrapper::initialize(Steinberg::FUnknown *context)
        {
            status_t res;

            // Acquire host context
            if (pHostContext != NULL)
                return Steinberg::kResultFalse;

            pHostContext        = safe_acquire(context);
            pHostApplication    = safe_query_iface<Steinberg::Vst::IHostApplication>(context);

            // Initialize
            res = init(NULL);

            return (res == STATUS_OK) ? Steinberg::kResultOk : Steinberg::kInternalError;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::terminate()
        {
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

        Steinberg::tresult PLUGIN_API UIWrapper::getControllerClassId(Steinberg::TUID classId)
        {
            return Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setIoMode(Steinberg::Vst::IoMode mode)
        {
            return Steinberg::kNotImplemented;
        }

        Steinberg::int32 PLUGIN_API UIWrapper::getBusCount(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir)
        {
            return 0;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getBusInfo(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::BusInfo & bus /*out*/)
        {
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getRoutingInfo(Steinberg::Vst::RoutingInfo & inInfo, Steinberg::Vst::RoutingInfo & outInfo /*out*/)
        {
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::activateBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::TBool state)
        {
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setActive(Steinberg::TBool state)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setState(Steinberg::IBStream *state)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getState(Steinberg::IBStream *state)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::connect(Steinberg::Vst::IConnectionPoint *other)
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

        Steinberg::tresult PLUGIN_API UIWrapper::disconnect(Steinberg::Vst::IConnectionPoint *other)
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

        Steinberg::tresult PLUGIN_API UIWrapper::notify(Steinberg::Vst::IMessage *message)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setComponentState(Steinberg::IBStream *state)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::int32 PLUGIN_API UIWrapper::getParameterCount()
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getParameterInfo(Steinberg::int32 paramIndex, Steinberg::Vst::ParameterInfo & info /*out*/)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getParamStringByValue(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized /*in*/, Steinberg::Vst::String128 string /*out*/)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::getParamValueByString(Steinberg::Vst::ParamID id, Steinberg::Vst::TChar *string /*in*/, Steinberg::Vst::ParamValue & valueNormalized /*out*/)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::Vst::ParamValue PLUGIN_API UIWrapper::normalizedParamToPlain(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized)
        {
            // TODO: implement this
            return 0.0;
        }

        Steinberg::Vst::ParamValue PLUGIN_API UIWrapper::plainParamToNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue plainValue)
        {
            // TODO: implement this
            return 0.0;
        }

        Steinberg::Vst::ParamValue PLUGIN_API UIWrapper::getParamNormalized(Steinberg::Vst::ParamID id)
        {
            // TODO: implement this
            return 0.0;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setParamNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setComponentHandler(Steinberg::Vst::IComponentHandler *handler)
        {
            if (pComponentHandler == handler)
                return Steinberg::kResultTrue;

            safe_release(pComponentHandler);
            safe_release(pComponentHandler2);
            safe_release(pComponentHandler3);

            pComponentHandler = handler;
            if (pComponentHandler != NULL)
            {
                pComponentHandler2 = safe_query_iface<Steinberg::Vst::IComponentHandler2>(pComponentHandler);
                pComponentHandler3 = safe_query_iface<Steinberg::Vst::IComponentHandler3>(pComponentHandler);
            }
            return Steinberg::kResultTrue;
        }

        Steinberg::IPlugView * PLUGIN_API UIWrapper::createView(Steinberg::FIDString name)
        {
            // TODO: implement this
            return NULL;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setKnobMode(Steinberg::Vst::KnobMode mode)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::openHelp(Steinberg::TBool onlyCheck)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::openAboutBox(Steinberg::TBool onlyCheck)
        {
            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        core::KVTStorage *UIWrapper::kvt_lock()
        {
            // TODO: implement this
            return NULL;
        }

        core::KVTStorage *UIWrapper::kvt_trylock()
        {
            // TODO: implement this
            return NULL;
        }

        bool UIWrapper::kvt_release()
        {
            // TODO: implement this
            return false;
        }

        void UIWrapper::dump_state_request()
        {
        }

        const meta::package_t *UIWrapper::package() const
        {
            // TODO: implement this
            return pPackage;
        }

        status_t UIWrapper::play_file(const char *file, wsize_t position, bool release)
        {
            // TODO: implement this
            return STATUS_NOT_IMPLEMENTED;
        }

        float UIWrapper::ui_scaling_factor(float scaling)
        {
            // TODO: implement this
            return 0.0f;
        }

        void UIWrapper::main_iteration()
        {
            // TODO: implement this
        }

        bool UIWrapper::accept_window_size(size_t width, size_t height)
        {
            // TODO: implement this
            return true;
        }

        void UIWrapper::port_write(ui::IPort *port, size_t flags)
        {
            const meta::port_t *meta = port->metadata();
            if (meta::is_control_port(meta))
            {
                vst3::UIInParamPort *ip = static_cast<vst3::UIInParamPort *>(port);
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

                    const Steinberg::Vst::ParamID param_id = ip->parameter_id();
                    pComponentHandler->beginEdit(param_id);
                    pComponentHandler->performEdit(param_id, ip->vst_value());
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

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_UI_WRAPPER_H_ */
