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

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ctl
    {
        const ctl_class_t Window::metadata  = { "Window", &Widget::metadata };

        Window::Window(ui::IWrapper *src, tk::Window *window): Widget(src, window)
        {
            pClass      = &metadata;
        }

        Window::~Window()
        {
            sControllers.destroy();
            sWidgets.destroy();
        }

        status_t Window::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd != NULL)
            {
                sTitle.init(pWrapper, wnd->title());
            }

            return STATUS_OK;
        }

        void Window::destroy()
        {
            sControllers.destroy();
            sWidgets.destroy();

            Widget::destroy();
        }

        void Window::begin(ui::UIContext *ctx)
        {
            Widget::begin(ctx);
        }

        void Window::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd != NULL)
            {
                sTitle.set("title", name, value);

                set_constraints(wnd->constraints(), name, value);
                set_layout(wnd->layout(), NULL, name, value);
                set_param(wnd->border_size(), "border", name, value);
            }

            Widget::set(ctx, name, value);
        }

        status_t Window::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            return (wnd != NULL) ? wnd->add(child->widget()) : STATUS_BAD_STATE;
        }

        void Window::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
        }

        tk::handler_id_t Window::bind_slot(const char *widget_id, tk::slot_t id, tk::event_handler_t handler)
        {
            tk::Widget *w = sWidgets.find(widget_id);
            if (w == NULL)
            {
                lsp_warn("Not found widget: ui:id=%s", widget_id);
                return -STATUS_NOT_FOUND;
            }

            return w->slots()->bind(id, handler, self());
        }

        tk::handler_id_t Window::bind_shortcut(tk::Window *wnd, ws::code_t key, size_t mod, tk::event_handler_t handler)
        {
            tk::Shortcut *scut = wnd->shortcuts()->append(key, mod);
            if (scut == NULL)
                return -STATUS_NO_MEM;
            return scut->slot()->bind(handler, self());
        }

        void Window::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);

            // Call for resize
            if (wWidget != NULL)
                wWidget->query_resize();
        }

    } /* namespace ctl */
} /* namespace lsp */


