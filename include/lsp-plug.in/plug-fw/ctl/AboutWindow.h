/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 февр. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_ABOUTWINDOW_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_ABOUTWINDOW_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/ctl/Window.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * The About window controller
         */
        class AboutWindow: public ctl::Window
        {
            public:
                static const ctl_class_t metadata;

            protected:
                static status_t     slot_close(tk::Widget *sender, void *ptr, void *data);

            protected:
                status_t            post_init();
                tk::handler_id_t    bind_shortcut(ws::code_t key, size_t mod, tk::event_handler_t handler);

            public:
                explicit AboutWindow(ui::IWrapper *src);
                AboutWindow(const AboutWindow &) = delete;
                AboutWindow(AboutWindow &&) = delete;

                AboutWindow & operator = (const AboutWindow &) = delete;
                AboutWindow & operator = (AboutWindow &&) = delete;

                /**
                 * Init controller
                 *
                 */
                virtual status_t    init() override;

            public:
                /**
                 * Show plugin window
                 * @param over the widget to show window over
                 */
                void                show(tk::Widget *over);
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_ABOUTWINDOW_H_ */
