/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 13 мая 2021 г.
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

namespace lsp
{
    namespace ctl
    {
        Graph::Graph(ui::IWrapper *wrapper, tk::Graph *widget): Widget(wrapper, widget)
        {
        }

        Graph::~Graph()
        {
        }

        status_t Graph::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            return STATUS_OK;
        }

        void Graph::set(const char *name, const char *value)
        {
            return Widget::set(name, value);
        }

        status_t Graph::add(ctl::Widget *child)
        {
            // TODO
            return false;
        }
    }
}


