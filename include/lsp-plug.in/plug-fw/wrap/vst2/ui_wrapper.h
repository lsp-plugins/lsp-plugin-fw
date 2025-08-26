/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ui_ports.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/ipc/Thread.h>

#ifndef PLATFORM_WINDOWS
    #define LSP_VST2_ALT_EVENT_LOOP
#endif /* PLATFORM_WINDOWS */

namespace lsp
{
    namespace vst2
    {
        class Wrapper;

        class UIWrapper: public ui::IWrapper
        {
            private:
                vst2::Wrapper                      *pWrapper;       // VST Wrapper
                size_t                              nKeyState;      // State of the keys
                ERect                               sRect;
            #ifdef LSP_VST2_ALT_EVENT_LOOP
                ipc::Mutex                          sMutex;         // UI barrier mutex
                ipc::Thread                        *pIdleThread;    // Thread that simulates effEditIdle
            #endif /* LSP_VST2_ALT_EVENT_LOOP */

            protected:
                static status_t                 slot_ui_resize(tk::Widget *sender, void *ptr, void *data);
                static status_t                 slot_ui_show(tk::Widget *sender, void *ptr, void *data);
                static status_t                 slot_ui_realize(tk::Widget *sender, void *ptr, void *data);
                static status_t                 slot_display_idle(tk::Widget *sender, void *ptr, void *data);

                static status_t                 eff_edit_idle(void *arg);

            #ifdef LSP_VST2_ALT_EVENT_LOOP
                static status_t                 event_loop(void *arg);
            #endif /* LSP_VST2_ALT_EVENT_LOOP */

            protected:
                void                            transfer_dsp_to_ui();
                bool                            start_event_loop();
                void                            stop_event_loop();
                void                            do_destroy();
                vst2::UIPort                   *create_port(const meta::port_t *port, const char *postfix);

            public:
                explicit UIWrapper(ui::Module *ui, vst2::Wrapper *wrapper);
                virtual ~UIWrapper() override;

                virtual status_t                init(void *root_widget) override;
                virtual void                    destroy() override;

            public: // ui::IWrapper
                virtual core::KVTStorage       *kvt_lock() override;
                virtual core::KVTStorage       *kvt_trylock() override;
                virtual bool                    kvt_release() override;
                virtual void                    dump_state_request() override;
                virtual void                    main_iteration() override;
                virtual const meta::package_t  *package() const override;
                virtual status_t                play_file(const char *file, wsize_t position, bool release) override;
                virtual meta::plugin_format_t   plugin_format() const override;
                virtual const core::ShmState   *shm_state() override;
                virtual void                    send_preset_state(const core::preset_state_t *state) override;

            public:
                bool                            show_ui();
                void                            hide_ui();
                void                            resize_ui();
                void                            idle_ui();
                ERect                          *ui_rect();
                size_t                          key_state() const;
                size_t                          set_key_state(size_t state);

            public:
                static UIWrapper               *create(vst2::Wrapper *wrapper, void *root_widget);
        };
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_UI_WRAPPER_H_ */
