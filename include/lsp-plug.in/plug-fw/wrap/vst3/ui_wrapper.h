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

#ifndef PLUG_IN_PLUG_FW_WRAP_VST3_UI_WRAPPER_H_
#define PLUG_IN_PLUG_FW_WRAP_VST3_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/ui.h>

#include <steinberg/vst3.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/string_buf.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/sync.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/controller.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ui_ports.h>

namespace lsp
{
    namespace vst3
    {
        class Controller;

        #include <steinberg/vst3/base/WarningsPush.h>
        class UIWrapper:
            public ui::IWrapper,
            public vst3::IUISync,
            public Steinberg::IDependent,
            public Steinberg::IPlugView,
            public Steinberg::IPlugViewContentScaleSupport
        {
            protected:
                uatomic_t                           nRefCounter;            // Reference counter
                vst3::Controller                   *pController;            // Controller
                Steinberg::IPlugFrame              *pPlugFrame;             // Plugin frame
                float                               fScalingFactor;         // Scaling factor
                uatomic_t                           nPlayPositionReq;
                uatomic_t                           nPlayPositionResp;

                lltl::parray<vst3::UIPort>          vSync;                  // Synchronization ports

            #ifdef VST_USE_RUNLOOP_IFACE
                Steinberg::Linux::IRunLoop         *pRunLoop;               // Run loop interface
                Steinberg::Linux::ITimerHandler    *pTimer;                 // Timer handler
            #endif /* VST_USE_RUNLOOP_IFACE */

            protected:
                static status_t                     slot_ui_resize(tk::Widget *sender, void *ptr, void *data);
                static status_t                     slot_ui_show(tk::Widget *sender, void *ptr, void *data);
                static status_t                     slot_ui_realized(tk::Widget *sender, void *ptr, void *data);
                static status_t                     slot_ui_close(tk::Widget *sender, void *ptr, void *data);
                static status_t                     slot_display_idle(tk::Widget *sender, void *ptr, void *data);

            protected:
                vst3::UIPort                       *create_port(const meta::port_t *port, const char *postfix);
                void                                query_resize(const ws::rectangle_t *r);
                void                                sync_with_controller();
                void                                sync_with_dsp();
                void                                sync_kvt_state(core::KVTStorage *kvt);
                void                                do_destroy();

            public:
                explicit UIWrapper(vst3::Controller *controller, ui::Module *ui, resource::ILoader *loader);
                UIWrapper(const UIWrapper &) = delete;
                UIWrapper(UIWrapper &&) = delete;
                virtual ~UIWrapper() override;

                UIWrapper & operator = (const UIWrapper &) = delete;
                UIWrapper & operator = (UIWrapper &&) = delete;

                virtual status_t                    init(void *root_widget) override;
                virtual void                        destroy() override;

            public:
                Steinberg::tresult                  show_about_box();
                Steinberg::tresult                  show_help();
                void                                commit_position(const plug::position_t *pos);
                void                                set_play_position(wssize_t position, wssize_t length);

            public: // ui::Wrapper
                virtual core::KVTStorage           *kvt_lock() override;
                virtual core::KVTStorage           *kvt_trylock() override;
                virtual bool                        kvt_release() override;
                virtual void                        dump_state_request() override;
                virtual const meta::package_t      *package() const override;
                virtual status_t                    play_file(const char *file, wsize_t position, bool release) override;
                virtual float                       ui_scaling_factor(float scaling) override;
                virtual void                        main_iteration() override;
                virtual meta::plugin_format_t       plugin_format() const override;
                virtual const core::ShmState       *shm_state() override;

            public: // vst3::IUISync
                virtual void                        sync_ui() override;

            public: // Steinberg::FUnknown
                virtual Steinberg::tresult          PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;
                virtual Steinberg::uint32           PLUGIN_API addRef() override;
                virtual Steinberg::uint32           PLUGIN_API release() override;

            public: // Steinberg::IDependent
                virtual void                        PLUGIN_API update(Steinberg::FUnknown *changedUnknown, Steinberg::int32 message) override;

            public: // Steinberg::IPlugView
                virtual Steinberg::tresult          PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override;
                virtual Steinberg::tresult          PLUGIN_API attached(void *parent, Steinberg::FIDString type) override;
                virtual Steinberg::tresult          PLUGIN_API removed() override;
                virtual Steinberg::tresult          PLUGIN_API onWheel(float distance) override;
                virtual Steinberg::tresult          PLUGIN_API onKeyDown(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers) override;
                virtual Steinberg::tresult          PLUGIN_API onKeyUp(Steinberg::char16 key, Steinberg::int16 keyCode, Steinberg::int16 modifiers) override;
                virtual Steinberg::tresult          PLUGIN_API getSize(Steinberg::ViewRect *size) override;
                virtual Steinberg::tresult          PLUGIN_API onSize(Steinberg::ViewRect *newSize) override;
                virtual Steinberg::tresult          PLUGIN_API onFocus(Steinberg::TBool state) override;
                virtual Steinberg::tresult          PLUGIN_API setFrame(Steinberg::IPlugFrame *frame) override;
                virtual Steinberg::tresult          PLUGIN_API canResize() override;
                virtual Steinberg::tresult          PLUGIN_API checkSizeConstraint(Steinberg::ViewRect *rect) override;

            public: // Steinberg::IPlugViewContentScaleSupport
                virtual Steinberg::tresult          PLUGIN_API setContentScaleFactor(Steinberg::IPlugViewContentScaleSupport::ScaleFactor factor) override;

        };
        #include <steinberg/vst3/base/WarningsPop.h>

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* PLUG_IN_PLUG_FW_WRAP_VST3_UI_WRAPPER_H_ */
