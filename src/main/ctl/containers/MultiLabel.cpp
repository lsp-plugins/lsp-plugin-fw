/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 17 июн. 2021 г.
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
        CTL_FACTORY_IMPL_START(MultiLabel)
            status_t res;

            if (!name->equals_ascii("multilabel"))
                return STATUS_NOT_FOUND;

            tk::MultiLabel *w = new tk::MultiLabel(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::MultiLabel *wc  = new ctl::MultiLabel(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(MultiLabel)

        //-----------------------------------------------------------------
        const ctl_class_t MultiLabel::metadata     = { "MultiLabel", &Widget::metadata };

        MultiLabel::MultiLabel(ui::IWrapper *wrapper, tk::MultiLabel *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        MultiLabel::~MultiLabel()
        {
        }

        status_t MultiLabel::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            return STATUS_OK;
        }

        void MultiLabel::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::MultiLabel *lbl  = tk::widget_cast<tk::MultiLabel>(wWidget);
            if (lbl != NULL)
            {
                set_constraints(lbl->constraints(), name, value);
                set_param(lbl->bearing(), "bearing", name, value);
                set_param(lbl->hover(), "hover", name, value);
            }

            Widget::set(ctx, name, value);
        }

        status_t MultiLabel::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::MultiLabel *grp  = tk::widget_cast<tk::MultiLabel>(wWidget);
            return (grp != NULL) ? grp->add(child->widget()) : STATUS_BAD_STATE;
        }

    } /* namespace ctl */
} /* namespace lsp */


