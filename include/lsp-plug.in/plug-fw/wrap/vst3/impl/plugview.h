/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 30 янв. 2024 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_PLUGVIEW_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_PLUGVIEW_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/plugview.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ui_wrapper.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        PluginView::PluginView(UIWrapper *wrapper)
        {
            pWrapper    = safe_acquire(wrapper);
            wWindow     = NULL;
            pWindow     = NULL;
        }

        PluginView::~PluginView()
        {
            safe_release(pWrapper);
        }

        status_t PluginView::init()
        {
            return STATUS_OK;
        }

        Steinberg::tresult PLUGIN_API PluginView::queryInterface(const Steinberg::TUID _iid, void **obj)
        {
            // Cast to the requested interface
            if (Steinberg::iidEqual(_iid, Steinberg::FUnknown::iid))
                return cast_interface<Steinberg::FUnknown>(static_cast<Steinberg::IDependent *>(this), obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IDependent::iid))
                return cast_interface<Steinberg::IDependent>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPlugView::iid))
                return cast_interface<Steinberg::IPlugView>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPlugViewContentScaleSupport::iid))
                return cast_interface<Steinberg::IPlugViewContentScaleSupport>(this, obj);

            return no_interface(obj);
        }

        Steinberg::uint32 PLUGIN_API PluginView::addRef()
        {
            return atomic_add(&nRefCounter, 1) + 1;
        }

        Steinberg::uint32 PLUGIN_API PluginView::release()
        {
            atomic_t ref_count = atomic_add(&nRefCounter, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        void PLUGIN_API PluginView::update(Steinberg::FUnknown *changedUnknown, Steinberg::int32 message)
        {
        }

        Steinberg::tresult PLUGIN_API PluginView::isPlatformTypeSupported(Steinberg::FIDString type)
        {
            bool supported = false;
#if defined(PLATFORM_WINDOWS)
            supported = (strcmp(type, Steinberg::kPlatformTypeHWND) == 0);
#elif defined(PLATFORM_MACOSX)
#elif defined(PLATFORM_POSIX)
            supported = (strcmp(type, Steinberg::kPlatformTypeX11EmbedWindowID) == 0);
#endif
            return (supported) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API PluginView::attached(void *parent, Steinberg::FIDString type)
        {
            // TODO: implement this
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::removed()
        {
            // TODO: implement this
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::onWheel(float distance)
        {
            // No, please!
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::onKeyDown(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers)
        {
            // No, please!
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::onKeyUp(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers)
        {
            // No, please!
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::getSize(Steinberg::ViewRect *size)
        {
            // TODO: implement this
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::onSize(Steinberg::ViewRect *newSize)
        {
            // TODO: implement this
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::onFocus(Steinberg::TBool state)
        {
            // TODO: implement this
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::setFrame(Steinberg::IPlugFrame *frame)
        {
            // TODO: implement this
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::canResize()
        {
            // TODO: implement this
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::checkSizeConstraint(Steinberg::ViewRect *rect)
        {
            // TODO: implement this
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::setContentScaleFactor(Steinberg::IPlugViewContentScaleSupport::ScaleFactor factor)
        {
            // TODO: implement this
            return Steinberg::kResultOk;
        }

    } /* namespace vst3 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_PLUGVIEW_H_ */
