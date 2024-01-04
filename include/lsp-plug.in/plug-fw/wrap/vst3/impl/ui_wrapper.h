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
            nRefCounter     = 1;
            pFactory        = safe_acquire(factory);
            pPackage        = package;
            pHostContext    = NULL;
            pPeerConnection = NULL;
        }

        UIWrapper::~UIWrapper()
        {
            // Destroy plugin
            if (pUI != NULL)
            {
                delete pUI;
                pUI         = NULL;
            }

            // Release factory
            safe_release(pFactory);
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
            if (pHostContext != NULL)
                return Steinberg::kResultFalse;
            pHostContext    = safe_acquire(context);

            // TODO: implement this
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::terminate()
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

        Steinberg::tresult PLUGIN_API UIWrapper::getControllerClassId(Steinberg::TUID classId)
        {
            return Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API UIWrapper::setIoMode(Steinberg::Vst::IoMode mode)
        {
            // TODO: implement this
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
            // TODO: implement this
            return Steinberg::kNotImplemented;
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

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_UI_WRAPPER_H_ */
