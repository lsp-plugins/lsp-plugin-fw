/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 мая 2021 г.
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
        CTL_FACTORY_IMPL_START(Cell)
            if (!name->equals_ascii("cell"))
                return STATUS_NOT_FOUND;

            ctl::Cell *wc  = new ctl::Cell(context->wrapper());
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Cell)

        //-----------------------------------------------------------------
        const ctl_class_t Cell::metadata        = { "Cell", &Widget::metadata };

        Cell::Cell(ui::IWrapper *wrapper): Widget(wrapper, NULL)
        {
            pClass          = &metadata;

            cChild          = NULL;
            nRows           = 1;
            nCols           = 1;
        }

        Cell::~Cell()
        {
            for (size_t i=0, n=vParams.size(); i<n; ++i)
            {
                char *p = vParams.uget(i);
                if (p != NULL)
                    ::free(p);
            }
            vParams.flush();
        }

        void Cell::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            if (set_value(&nRows, "rows", name, value))
                return;
            else if (set_value(&nCols, "cols", name, value))
                return;

            // Duplicate name and value
            char *kname     = strdup(name);
            if (kname == NULL)
                return;
            char *kvalue    = strdup(value);
            if (kvalue == NULL)
            {
                free(kname);
                return;
            }

            // Add 2 parameters
            char **dst = vParams.add_n(2);
            if (!dst)
            {
                free(kname);
                free(kvalue);
                return;
            }

            dst[0]      = kname;
            dst[1]      = kvalue;
        }

        status_t Cell::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            if (cChild != NULL)
                return STATUS_ALREADY_EXISTS;

            cChild      = child;

            // Apply settings to the child
            if (child != NULL)
            {
                for (size_t i=0, n=vParams.size(); i<n; i += 2)
                {
                    const char *name    = vParams.uget(i);
                    const char *value   = vParams.uget(i+1);

                    if ((name != NULL) && (value != NULL))
                        child->set(ctx, name, value);
                }
            }

            return STATUS_OK;
        }

        tk::Widget *Cell::widget()
        {
            return (cChild != NULL) ? cChild->widget() : wWidget;
        }
    }
}


