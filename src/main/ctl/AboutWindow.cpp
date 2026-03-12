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

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/common/debug.h>

#include <private/ui/xml/RootNode.h>
#include <private/ui/xml/Handler.h>

namespace lsp
{
    namespace ctl
    {
        const ctl_class_t AboutWindow::metadata = { "AboutWindow", &Window::metadata };

        AboutWindow::AboutWindow(ui::IWrapper *src):
            ctl::Window(src, NULL)
        {
            pClass      = &metadata;
        }

        status_t AboutWindow::init()
        {
            status_t res;

            // Create window
            tk::Window *w = new tk::Window(pWrapper->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = pWrapper->controller()->widgets()->add(w)) != STATUS_OK)
            {
                w->destroy();
                delete w;
            }
            LSP_STATUS_ASSERT(w->init());
            w->actions()->set_actions(ws::WA_DIALOG | ws::WA_RESIZE | ws::WA_CLOSE);
            wWidget = w;

            lsp_trace("Registered window ptr=%p", static_cast<tk::Widget *>(w));

            // Initialize controller
            LSP_STATUS_ASSERT(ctl::Window::init());

            // Build UI
            ui::UIContext uctx(pWrapper, controllers(), widgets());
            LSP_STATUS_ASSERT(init_ui_context(&uctx, pWrapper->package(), pWrapper->metadata()));

            // Parse the XML document
            ui::xml::RootNode root(&uctx, "window", this);
            ui::xml::Handler handler(pWrapper->resources());
            LSP_STATUS_ASSERT(handler.parse_resource(LSP_BUILTIN_PREFIX "ui/about.xml", &root));

            // Do post-initialization
            return post_init();
        }

        status_t AboutWindow::post_init()
        {
            // Bind slots
            tk::Widget *btn = widgets()->find("submit");
            if (btn != NULL)
                btn->slots()->bind(tk::SLOT_SUBMIT, slot_close, this);
            wWidget->slots()->bind(tk::SLOT_CLOSE, slot_close, this);

            // Bind shortcuts
            bind_shortcut(ws::WSK_ESCAPE, tk::KM_NONE, slot_close);
            bind_shortcut(ws::WSK_RETURN, tk::KM_NONE, slot_close);
            bind_shortcut(ws::WSK_KEYPAD_ENTER, tk::KM_NONE, slot_close);

            return STATUS_OK;
        }

        tk::handler_id_t AboutWindow::bind_shortcut(ws::code_t key, size_t mod, tk::event_handler_t handler)
        {
            tk::Window * const wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return -STATUS_BAD_STATE;

            tk::Shortcut * const scut = wnd->shortcuts()->append(key, mod);
            if (scut == NULL)
                return -STATUS_NO_MEM;
            return scut->slot()->bind(handler, this);
        }

        void AboutWindow::show(tk::Widget *over)
        {
            tk::Window * const wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd != NULL)
                wnd->show(over);
        }

        status_t AboutWindow::slot_close(tk::Widget *sender, void *ptr, void *data)
        {
            AboutWindow * const self = static_cast<AboutWindow *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            self->wWidget->visibility()->set(false);
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


