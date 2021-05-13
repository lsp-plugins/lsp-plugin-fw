/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 апр. 2021 г.
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
        CTL_FACTORY_IMPL_START(Grid)
            status_t res;

            if (!name->equals_ascii("grid"))
                return STATUS_NOT_FOUND;

            tk::Grid *w = new tk::Grid(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->add_widget(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Grid *wc  = new ctl::Grid(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Grid)

        //-----------------------------------------------------------------
        const ctl_class_t Grid::metadata     = { "Grid", &Widget::metadata };

        Grid::Grid(ui::IWrapper *wrapper, tk::Grid *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        Grid::~Grid()
        {
        }

        status_t Grid::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Grid *grd  = tk::widget_cast<tk::Grid>(wWidget);
            if (grd != NULL)
            {
                sRows.init(pWrapper, grd->rows());
                sCols.init(pWrapper, grd->columns());
            }

            return STATUS_OK;
        }

        void Grid::set(const char *name, const char *value)
        {
            tk::Grid *grd  = tk::widget_cast<tk::Grid>(wWidget);
            if (grd != NULL)
            {
                set_param(grd->hspacing(), "hspacing", name, value);
                set_param(grd->vspacing(), "vspacing", name, value);
                set_param(grd->hspacing(), "spacing", name, value);
                set_param(grd->vspacing(), "spacing", name, value);
                set_constraints(grd->constraints(), name, value);
                set_orientation(grd->orientation(), name, value);

                // Legacy property
                if (!strcmp(name, "transpose"))
                    PARSE_BOOL(value, grd->orientation()->set((__) ? tk::O_VERTICAL : tk::O_HORIZONTAL))
            }

            sRows.set("rows", name, value);
            sCols.set("cols", name, value);
            sCols.set("columns", name, value);

            Widget::set(name, value);
        }

        status_t Grid::add(ctl::Widget *child)
        {
            tk::Grid *grd   = tk::widget_cast<tk::Grid>(wWidget);
            if (grd == NULL)
                return STATUS_BAD_STATE;

            ctl::Cell *cell = ctl::ctl_cast<ctl::Cell>(child);
            if (cell != NULL)
                return grd->add(cell->widget(), cell->rows(), cell->columns());

            return grd->add(child->widget());
        }

    } /* namespace ctl */
} /* namespace lsp */



