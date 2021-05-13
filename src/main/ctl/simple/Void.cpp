/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 мая 2021 г.
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
        CTL_FACTORY_IMPL_START(Void)
            status_t res;

            if (!name->equals_ascii("void"))
                return STATUS_NOT_FOUND;

            tk::Void *w = new tk::Void(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->add_widget(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Void *wc  = new ctl::Void(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Void)

        //-----------------------------------------------------------------
        const ctl_class_t Void::metadata    = { "Void", &Widget::metadata };

        Void::Void(ui::IWrapper *wrapper, tk::Void *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        Void::~Void()
        {
        }

        status_t Void::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Void *vd = tk::widget_cast<tk::Void>(wWidget);
            if (vd != NULL)
            {
                sColor.init(pWrapper, vd->color());
            }

            return STATUS_OK;
        }

        void Void::set(const char *name, const char *value)
        {
            tk::Void *vd = tk::widget_cast<tk::Void>(wWidget);
            if (vd != NULL)
            {
                sColor.set("color", name, value);
                set_param(vd->fill(), "cfill", name, value);
                set_constraints(vd->constraints(), name, value);
            }

            return Widget::set(name, value);
        }
    }
}
