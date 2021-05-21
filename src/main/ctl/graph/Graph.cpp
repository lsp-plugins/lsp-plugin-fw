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
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Graph)
            status_t res;

            if (!name->equals_ascii("graph"))
                return STATUS_NOT_FOUND;

            tk::Graph *w = new tk::Graph(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->add_widget(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Graph *wc  = new ctl::Graph(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Graph)

        //-----------------------------------------------------------------
        const ctl_class_t Graph::metadata       = { "Graph", &Widget::metadata };

        Graph::Graph(ui::IWrapper *wrapper, tk::Graph *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        Graph::~Graph()
        {
        }

        status_t Graph::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Graph *gr   = tk::widget_cast<tk::Graph>(wWidget);
            if (gr != NULL)
            {
                sColor.init(pWrapper, gr->color());
                sBorderColor.init(pWrapper, gr->border_color());
                sGlassColor.init(pWrapper, gr->glass_color());
                sBorderFlat.init(pWrapper, gr->border_flat());
                sIPadding.init(pWrapper, gr->ipadding());
            }

            return STATUS_OK;
        }

        void Graph::set(const char *name, const char *value)
        {
            tk::Graph *gr   = tk::widget_cast<tk::Graph>(wWidget);
            if (gr != NULL)
            {
                set_constraints(gr->constraints(), name, value);
                set_param(gr->border_size(), "border.size", name, value);
                set_param(gr->border_size(), "bsize", name, value);
                set_param(gr->border_radius(), "border.radius", name, value);
                set_param(gr->border_radius(), "bradius", name, value);
                set_param(gr->border_radius(), "brad", name, value);
                set_param(gr->glass(), "glass", name, value);

                sColor.set("color", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sGlassColor.set("glass.color", name, value);
                sGlassColor.set("gcolor", name, value);
                sBorderFlat.set("border.flat", name, value);
                sBorderFlat.set("bflat", name, value);
                sIPadding.set("ipadding", name, value);
                sIPadding.set("ipad", name, value);
            }

            return Widget::set(name, value);
        }

        status_t Graph::add(ctl::Widget *child)
        {
            tk::Graph *gr   = tk::widget_cast<tk::Graph>(wWidget);
            return (gr != NULL) ? gr->add(child->widget()) : STATUS_BAD_STATE;
        }
    }
}


