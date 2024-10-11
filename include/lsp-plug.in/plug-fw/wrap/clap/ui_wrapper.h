/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 янв. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/wrap/clap/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/clap/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/clap/ports.h>
#include <lsp-plug.in/plug-fw/wrap/clap/ui_ports.h>
#include <lsp-plug.in/plug-fw/wrap/clap/wrapper.h>
#include <lsp-plug.in/plug-fw/plug.h>

#ifndef PLATFORM_WINDOWS
    #define LSP_CLAP_OWN_EVENT_LOOP
#endif /* PLATFORM_WINDOWS */

namespace lsp
{
    namespace clap
    {
        class Wrapper;

        class UIWrapper: public ui::IWrapper
        {
            private:
                clap::Wrapper                  *pWrapper;       // CLAP Wrapper
                HostExtensions                 *pExt;           // Host extensions
                float                           fScaling;       // Scaling factor
                ipc::Mutex                      sMutex;         // Main loop mutex
                void                           *pParent;        // Parent window handle
                ws::IWindow                    *pTransientFor;  // TransientFor window
                bool                            bUIInitialized; // UI initialized flag
                bool                            bRequestProcess;// Request the process() call flag
                bool                            bUIActive;      // UI is active flag
                bool                            bRealizeActive; // Realize is active

            #ifdef LSP_CLAP_OWN_EVENT_LOOP
                ipc::Thread                    *pUIThread;      // Thread that performs the UI event loop
            #endif /* LSP_CLAP_OWN_EVENT_LOOP */

            protected:
                static status_t                 slot_ui_resize(tk::Widget *sender, void *ptr, void *data);
                static status_t                 slot_ui_show(tk::Widget *sender, void *ptr, void *data);
                static status_t                 slot_ui_realized(tk::Widget *sender, void *ptr, void *data);
                static status_t                 slot_ui_close(tk::Widget *sender, void *ptr, void *data);
                static status_t                 slot_display_idle(tk::Widget *sender, void *ptr, void *data);

                static void                    *to_native_handle(const clap_window_t *window);

            #ifdef LSP_CLAP_OWN_EVENT_LOOP
                static status_t                 event_loop(void *arg);
            #endif /* LSP_CLAP_OWN_EVENT_LOOP */

            protected:
                clap::UIPort                   *create_port(const meta::port_t *port, const char *postfix);
                bool                            start_event_loop();
                void                            stop_event_loop();
                void                            tranfet_ui_to_dsp();
                void                            transfer_dsp_to_ui();
                bool                            initialize_ui();
                void                            do_destroy();

            public:
                explicit UIWrapper(ui::Module *ui, clap::Wrapper *wrapper);
                virtual ~UIWrapper() override;

                virtual status_t                init(void *root_widget) override;
                virtual void                    destroy() override;

            public: // ui::IWrapper
                virtual core::KVTStorage       *kvt_lock() override;
                virtual core::KVTStorage       *kvt_trylock() override;
                virtual bool                    kvt_release() override;
                virtual void                    dump_state_request() override;
                virtual const meta::package_t  *package() const override;
                virtual status_t                play_file(const char *file, wsize_t position, bool release) override;
                virtual float                   ui_scaling_factor(float scaling) override;
                virtual void                    main_iteration() override;
                virtual bool                    accept_window_size(tk::Window *wnd, size_t width, size_t height) override;
                virtual meta::plugin_format_t   plugin_format() const override;
                virtual const core::ShmState   *shm_state() override;

            public: // CLAP API
                bool                            set_scale(double scale);
                bool                            get_size(uint32_t *width, uint32_t *height);
                bool                            can_resize();
                bool                            get_resize_hints(clap_gui_resize_hints_t *hints);
                bool                            adjust_size(uint32_t *width, uint32_t *height);
                bool                            set_size(uint32_t width, uint32_t height);
                bool                            set_parent(const clap_window_t *window);
                bool                            set_transient(const clap_window_t *window);
                void                            suggest_title(const char *title);
                bool                            show();
                bool                            hide();

            public: // Miscellaneious functions
                bool                            ui_active() const;
        };
    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_UI_WRAPPER_H_ */
