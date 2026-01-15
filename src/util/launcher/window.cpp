/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 15 янв. 2026 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/util/launcher/window.h>

namespace lsp
{
    namespace launcher
    {
        Window::Window(ui::IWrapper *src, tk::Window *window)
            : ctl::Window(src, window)
        {
        }

        Window::~Window()
        {
        }

        status_t Window::init()
        {
            return ctl::Window::init();
        }

        void Window::destroy()
        {
            ctl::Window::destroy();
        }

        status_t Window::post_init()
        {
            return STATUS_OK;
        }
    } /* namespace launcher */
} /* namespace lsp */

