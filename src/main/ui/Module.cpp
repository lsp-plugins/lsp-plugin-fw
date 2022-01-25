/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ui
    {
        Module::Module(const meta::plugin_t *meta)
        {
            pMetadata       = meta;
            pWrapper        = NULL;
            pDisplay        = NULL;
            wRoot           = NULL;
        }

        Module::~Module()
        {
            do_destroy();
        }

        void Module::destroy()
        {
            do_destroy();
        }

        status_t Module::post_init()
        {
            return STATUS_OK;
        }

        status_t Module::pre_destroy()
        {
            return STATUS_OK;
        }

        void Module::do_destroy()
        {
            // Forget the root widget
            wRoot       = NULL;
        }

        status_t Module::init(IWrapper *wrapper, tk::Display *dpy)
        {
            pWrapper        = wrapper;
            pDisplay        = dpy;

            return STATUS_OK;
        }

        void Module::position_updated(const plug::position_t *pos)
        {
        }
    }
}


