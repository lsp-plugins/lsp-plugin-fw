/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 22 июн. 2025 г.
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
        CTL_FACTORY_IMPL_START(Tab)
            status_t res;

            if (!name->equals_ascii("tab"))
                return STATUS_NOT_FOUND;

            tk::Tab *w = new tk::Tab(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Tab *wc  = new ctl::Tab(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Tab)

        //-----------------------------------------------------------------
        const ctl_class_t Tab::metadata     = { "Tab", &Widget::metadata };

        Tab::Tab(ui::IWrapper *wrapper, tk::Tab *tab):
            Widget(wrapper, tab)
        {
            pClass      = &metadata;
        }

        Tab::~Tab()
        {
        }

        status_t Tab::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Tab *tab = tk::widget_cast<tk::Tab>(wWidget);
            if (tab != NULL)
            {
                sText.init(pWrapper, tab->text());
            }

            return STATUS_OK;
        }

        void Tab::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Tab *tab = tk::widget_cast<tk::Tab>(wWidget);
            if (tab != NULL)
            {
                sText.set("text", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        status_t Tab::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::Tab *tab = tk::widget_cast<tk::Tab>(wWidget);
            return (tab != NULL) ? tab->add(child->widget()) : STATUS_BAD_STATE;
        }

    } /* namespace ctl */
} /* namespace lsp */



