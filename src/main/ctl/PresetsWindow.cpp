/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 31 мар. 2024 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        const ctl_class_t PresetsWindow::metadata = { "PresetsWindow", &Window::metadata };

        PresetsWindow::PresetsWindow(ui::IWrapper *src, tk::Window *widget):
            ctl::Window(src, widget)
        {
            pClass      = &metadata;
        }

        PresetsWindow::~PresetsWindow()
        {
        }

        status_t PresetsWindow::init()
        {
            status_t res = ctl::Window::init();
            if (res != STATUS_OK)
                return res;

            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd != NULL)
            {
                // Bind slots
                tk::Widget *btn = widgets()->find("submit");
                if (btn != NULL)
                    btn->slots()->bind(tk::SLOT_SUBMIT, slot_window_close, this);

                wnd->slots()->bind(tk::SLOT_CLOSE, slot_window_close, this);
            }

            return STATUS_OK;
        }

        void PresetsWindow::destroy()
        {
            ctl::Window::destroy();
        }

        status_t PresetsWindow::slot_window_close(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            tk::Window *wnd = tk::widget_cast<tk::Window>(self->wWidget);
            if (wnd != NULL)
                wnd->visibility()->set(false);
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */

