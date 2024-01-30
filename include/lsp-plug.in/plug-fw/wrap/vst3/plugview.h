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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_PLUGVIEW_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_PLUGVIEW_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/ui_wrapper.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        class UIWrapper;

        class PluginView:
            public Steinberg::IDependent,
            public Steinberg::IPlugView,
            public Steinberg::IPlugViewContentScaleSupport
        {
            protected:
                volatile uatomic_t                  nRefCounter;        // Reference counter
                UIWrapper                          *pWrapper;           // The UI wrapper
                tk::Window                         *wWindow;            // The main window
                ctl::Window                        *pWindow;            // The controller for window
                Steinberg::IPlugFrame              *pPlugFrame;         // Plugin frame

            public:
                PluginView(UIWrapper *wrapper);
                PluginView(const PluginView &) = delete;
                PluginView(PluginView &&) = delete;
                virtual ~PluginView();

                PluginView & operator = (const PluginView &) = delete;
                PluginView & operator = (PluginView &&) = delete;

                status_t init();

            public:
                status_t                    accept_window_size(tk::Window *wnd, size_t width, size_t height);
                Steinberg::tresult          show_about_box();
                Steinberg::tresult          show_help();

            public: // Steinberg::FUnknown
                virtual Steinberg::tresult  PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;
                virtual Steinberg::uint32   PLUGIN_API addRef() override;
                virtual Steinberg::uint32   PLUGIN_API release() override;

            public: // Steinberg::IDependent
                virtual void                PLUGIN_API update(Steinberg::FUnknown *changedUnknown, Steinberg::int32 message) override;

            public: // Steinberg::IPlugView
                virtual Steinberg::tresult  PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override;
                virtual Steinberg::tresult  PLUGIN_API attached(void *parent, Steinberg::FIDString type) override;
                virtual Steinberg::tresult  PLUGIN_API removed() override;
                virtual Steinberg::tresult  PLUGIN_API onWheel(float distance) override;
                virtual Steinberg::tresult  PLUGIN_API onKeyDown(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers) override;
                virtual Steinberg::tresult  PLUGIN_API onKeyUp(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers) override;
                virtual Steinberg::tresult  PLUGIN_API getSize(Steinberg::ViewRect *size) override;
                virtual Steinberg::tresult  PLUGIN_API onSize(Steinberg::ViewRect *newSize) override;
                virtual Steinberg::tresult  PLUGIN_API onFocus(Steinberg::TBool state) override;
                virtual Steinberg::tresult  PLUGIN_API setFrame(Steinberg::IPlugFrame *frame) override;
                virtual Steinberg::tresult  PLUGIN_API canResize() override;
                virtual Steinberg::tresult  PLUGIN_API checkSizeConstraint(Steinberg::ViewRect *rect) override;

            public: // Steinberg::IPlugViewContentScaleSupport
                virtual Steinberg::tresult  PLUGIN_API setContentScaleFactor(Steinberg::IPlugViewContentScaleSupport::ScaleFactor factor) override;
        };

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_PLUGVIEW_H_ */
