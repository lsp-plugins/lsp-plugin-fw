/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 янв. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_PLUGVIEW_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_PLUGVIEW_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
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
            nRefCounter = 1;
            pWrapper    = safe_acquire(wrapper);
            pPlugFrame  = NULL;
        }

        PluginView::~PluginView()
        {
            safe_release(pWrapper);

            if (pWrapper->pFactory != NULL)
                pWrapper->pFactory->unregister_ui_sync(this);
        }

        status_t PluginView::init()
        {
            // Attach event handler to the wrapper
            pWrapper->attach_ui(this);

            // Register self as UI sync stuff
            pWrapper->pFactory->register_ui_sync(this);

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

        status_t PluginView::accept_window_size(tk::Window *wnd, size_t width, size_t height)
        {
            if (pWrapper->wWindow != wnd)
                return STATUS_NOT_FOUND;

// TODO
//            if (pPlugFrame != NULL)
//                pPlugFrame->resizeView(this, newSize);

            return STATUS_OK;
        }

        Steinberg::tresult PluginView::show_about_box()
        {
            ctl::PluginWindow *wnd = ctl::ctl_cast<ctl::PluginWindow>(pWrapper->pWindow);
            if (wnd == NULL)
                return Steinberg::kResultFalse;

            status_t res = wnd->show_about_window();
            return (res == STATUS_OK) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
        }

        Steinberg::tresult PluginView::show_help()
        {
            ctl::PluginWindow *wnd = ctl::ctl_cast<ctl::PluginWindow>(pWrapper->pWindow);
            if (wnd == NULL)
                return Steinberg::kResultFalse;

            status_t res = wnd->show_plugin_manual();
            return (res == STATUS_OK) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
        }

        void PluginView::query_resize(const ws::rectangle_t *r)
        {
            if (pPlugFrame == NULL)
                return;

            Steinberg::ViewRect newSize;
            newSize.left        = 0;
            newSize.top         = 0;
            newSize.right       = r->nWidth;
            newSize.bottom      = r->nHeight;

            pPlugFrame->resizeView(this, &newSize);
        }

        void PLUGIN_API PluginView::update(Steinberg::FUnknown *changedUnknown, Steinberg::int32 message)
        {
            // Nothing to do
        }

        Steinberg::tresult PLUGIN_API PluginView::isPlatformTypeSupported(Steinberg::FIDString type)
        {
            bool supported = false;
#if defined(PLATFORM_WINDOWS)
            supported = (strcmp(type, Steinberg::kPlatformTypeHWND) == 0);
#elif defined(PLATFORM_MACOSX)
#else
            supported = (strcmp(type, Steinberg::kPlatformTypeX11EmbedWindowID) == 0);
#endif
            return (supported) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API PluginView::attached(void *parent, Steinberg::FIDString type)
        {
            if (isPlatformTypeSupported(type) != Steinberg::kResultTrue)
                return Steinberg::kResultFalse;

            // Show the window
            if (pWrapper->wWindow != NULL)
                pWrapper->wWindow->show();

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::removed()
        {
            // Hide the window
            if (pWrapper->wWindow != NULL)
                pWrapper->wWindow->hide();

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::onWheel(float distance)
        {
            // No, please!
            return Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API PluginView::onKeyDown(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers)
        {
            // No, please!
            return Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API PluginView::onKeyUp(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers)
        {
            // No, please!
            return Steinberg::kResultFalse;
        }

        Steinberg::tresult PLUGIN_API PluginView::getSize(Steinberg::ViewRect *size)
        {
            if (pWrapper->wWindow == NULL)
                return Steinberg::kResultFalse;

            ws::rectangle_t r;
            pWrapper->wWindow->get_padded_rectangle(&r);

            // Obtain the window parameters
            if (pWrapper->wWindow->visibility()->get())
            {
                ws::rectangle_t rr;
                rr.nWidth       = 0;
                rr.nHeight      = 0;

                if (pWrapper->wWindow->get_screen_rectangle(&rr) != STATUS_OK)
                    return false;

                // Return result
                size->left      = rr.nLeft;
                size->top       = rr.nTop;
                size->right     = rr.nLeft + rr.nWidth;
                size->bottom    = rr.nTop  + rr.nHeight;
            }
            else
            {
                ws::size_limit_t sr;
                pWrapper->wWindow->get_size_limits(&sr);

                size->left      = 0;
                size->top       = 0;
                size->right     = lsp_min(sr.nMinWidth, 32);
                size->bottom    = lsp_min(sr.nMinHeight, 32);
            }

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::onSize(Steinberg::ViewRect *newSize)
        {
            Steinberg::tresult res = checkSizeConstraint(newSize);
            if (res != Steinberg::kResultOk)
                return res;

            pWrapper->wWindow->position()->set(newSize->left, newSize->top);
            pWrapper->wWindow->size()->set(newSize->right - newSize->left, newSize->bottom - newSize->top);

            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API PluginView::onFocus(Steinberg::TBool state)
        {
            // No, please
            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::setFrame(Steinberg::IPlugFrame *frame)
        {
            safe_release(pPlugFrame);
            pPlugFrame  = safe_acquire(frame);

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::canResize()
        {
            return Steinberg::kResultTrue;
        }

        Steinberg::tresult PLUGIN_API PluginView::checkSizeConstraint(Steinberg::ViewRect *rect)
        {
            if (pWrapper->wWindow == NULL)
                return Steinberg::kResultFalse;

            ws::rectangle_t sr, dr;
            sr.nLeft    = rect->left;
            sr.nTop     = rect->top;
            sr.nWidth   = rect->right  - rect->left;
            sr.nHeight  = rect->bottom - rect->top;

            dr = sr;

            pWrapper->wWindow->constraints()->apply(&dr, pWrapper->wWindow->scaling()->get());

            // Update the rect if it was constrained
            if ((dr.nWidth != sr.nWidth) || (dr.nHeight != sr.nHeight))
            {
                rect->right  = rect->left + dr.nWidth;
                rect->bottom = rect->top  + dr.nHeight;
            }

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginView::setContentScaleFactor(Steinberg::IPlugViewContentScaleSupport::ScaleFactor factor)
        {
            pWrapper->set_scaling_factor(factor * 100.0f);

            ctl::PluginWindow *wnd = ctl::ctl_cast<ctl::PluginWindow>(pWrapper->pWindow);
            if (wnd != NULL)
                wnd->host_scaling_changed();

            return Steinberg::kResultOk;
        }

        void PluginView::sync_ui()
        {
            if (pWrapper->pDisplay != NULL)
                pWrapper->pDisplay->main_iteration();
        }

    } /* namespace vst3 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_PLUGVIEW_H_ */
