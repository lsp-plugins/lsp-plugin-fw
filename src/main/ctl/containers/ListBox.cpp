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

#include <lsp-plug.in/plug-fw/ctl.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/meta/func.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(ListBox)
            status_t res;

            if (!name->equals_ascii("list"))
                return STATUS_NOT_FOUND;

            tk::ListBox *w = new tk::ListBox(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::ListBox *wc  = new ctl::ListBox(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(ListBox)

        //-----------------------------------------------------------------
        const ctl_class_t ListBox::metadata    = { "ListBox", &Widget::metadata };

        ListBox::ListBox(ui::IWrapper *wrapper, tk::ListBox *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        ListBox::~ListBox()
        {
        }

        status_t ListBox::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::ListBox *lbox = tk::widget_cast<tk::ListBox>(wWidget);
            if (lbox != NULL)
            {
                sHScroll.init(pWrapper, lbox->hscroll_mode());
                sVScroll.init(pWrapper, lbox->vscroll_mode());
                // TODO
            }

            return STATUS_OK;
        }

        void ListBox::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::ListBox *lbox = tk::widget_cast<tk::ListBox>(wWidget);
            if (lbox != NULL)
            {
                set_param(lbox->border_size(), "border.size", name, value);
                set_param(lbox->border_size(), "bsize", name, value);
                set_param(lbox->border_gap(), "border.gap", name, value);
                set_param(lbox->border_gap(), "bgap", name, value);
                set_param(lbox->border_radius(), "border.radius", name, value);
                set_param(lbox->border_radius(), "bradius", name, value);

                sHScroll.set(name, "hscroll", value);
                sVScroll.set(name, "vscroll", value);

                set_font(lbox->font(), "font", name, value);
                set_constraints(lbox->constraints(), name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void ListBox::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
        }

    } /* namespace ctl */
} /* namespace lsp */


