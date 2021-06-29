/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 июн. 2021 г.
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
        CTL_FACTORY_IMPL_START(Bevel)
            status_t res;

            if (!name->equals_ascii("bevel"))
                return STATUS_NOT_FOUND;

            tk::Bevel *w = new tk::Bevel(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Bevel *wc  = new ctl::Bevel(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Bevel)

        //-----------------------------------------------------------------
        const ctl_class_t Bevel::metadata    = { "Bevel", &Widget::metadata };

        Bevel::Bevel(ui::IWrapper *wrapper, tk::Bevel *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        Bevel::~Bevel()
        {
        }

        status_t Bevel::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Bevel *bv = tk::widget_cast<tk::Bevel>(wWidget);
            if (bv != NULL)
            {
                sColor.init(pWrapper, bv->color());
                sBorderColor.init(pWrapper, bv->bg_color());
                sDirection.init(pWrapper, bv->direction());
                sBorder.init(pWrapper, bv->border());
            }

            return STATUS_OK;
        }

        void Bevel::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Bevel *bv = tk::widget_cast<tk::Bevel>(wWidget);
            if (bv != NULL)
            {
                sColor.set("color", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sDirection.set("dir", name, value);
                sDirection.set("direction", name, value);
                sBorder.set("border.size", name, value);
                sBorder.set("bsize", name, value);

                set_constraints(bv->constraints(), name, value);
                set_arrangement(bv->arrangement(), NULL, name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Bevel::schema_reloaded()
        {
            Widget::schema_reloaded();

            sColor.reload();
            sBorderColor.reload();
        }
    }
}



