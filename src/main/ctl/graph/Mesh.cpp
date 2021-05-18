/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 мая 2021 г.
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
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Mesh)
            status_t res;

            if (!name->equals_ascii("mesh"))
                return STATUS_NOT_FOUND;

            tk::GraphMesh *w = new tk::GraphMesh(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->add_widget(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Mesh *wc  = new ctl::Mesh(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Mesh)

        //-----------------------------------------------------------------
        const ctl_class_t Mesh::metadata        = { "Mesh", &Widget::metadata };

        Mesh::Mesh(ui::IWrapper *wrapper, tk::GraphMesh *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
        }

        Mesh::~Mesh()
        {
        }

        status_t Mesh::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphMesh *gm   = tk::widget_cast<tk::GraphMesh>(wWidget);
            if (gm != NULL)
            {
            }

            return STATUS_OK;
        }

        void Mesh::set(const char *name, const char *value)
        {
            tk::GraphMesh *gm   = tk::widget_cast<tk::GraphMesh>(wWidget);
            if (gm != NULL)
            {
            }

            return Widget::set(name, value);
        }

        void Mesh::notify(ui::IPort *port)
        {
            Widget::notify(port);
        }

        void Mesh::end()
        {
            Widget::end();

        }
    }
}


