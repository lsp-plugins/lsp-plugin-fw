/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 июн. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_WINDOW_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_WINDOW_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        class Window: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ctl::Registry           sControllers;
                tk::Registry            sWidgets;
                ctl::LCString           sTitle;

            protected:
                tk::handler_id_t        bind_slot(const char *widget_id, tk::slot_t id, tk::event_handler_t handler);
                tk::handler_id_t        bind_shortcut(tk::Window *wnd, ws::code_t key, size_t mod, tk::event_handler_t handler);

            public:
                explicit Window(ui::IWrapper *src, tk::Window *window);
                virtual ~Window() override;

                virtual status_t        init() override;
                virtual void            destroy() override;

            public:
                inline ctl::Registry   *controllers()   { return &sControllers; }
                inline tk::Registry    *widgets()       { return &sWidgets;     }

            public: // ctl::DOMController
                virtual void            begin(ui::UIContext *ctx) override;
                virtual void            set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual status_t        add(ui::UIContext *ctx, ctl::Widget *child) override;
                virtual void            end(ui::UIContext *ctx) override;
                virtual void            reloaded(const tk::StyleSheet *sheet) override;
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_WINDOW_H_ */
