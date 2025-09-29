/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 сент. 2025 г.
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
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Embed)
            status_t res;

            if (!name->equals_ascii("embed"))
                return STATUS_NOT_FOUND;

            tk::GraphEmbed *w = new tk::GraphEmbed(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Embed *wc = new ctl::Embed(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Embed)

        //-----------------------------------------------------------------
        const ctl_class_t Embed::metadata      = { "GraphEmbed", &Widget::metadata };

        Embed::Embed(ui::IWrapper *wrapper, tk::GraphEmbed *widget): Widget(wrapper, widget)
        {
            pClass      = &metadata;
        }

        Embed::~Embed()
        {
        }

        status_t Embed::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphEmbed *ge = tk::widget_cast<tk::GraphEmbed>(wWidget);
            if (ge != NULL)
            {
                sXAxis.init(pWrapper, ge->haxis());
                sYAxis.init(pWrapper, ge->vaxis());
                sHStartValue.init(pWrapper, ge->hvalue_start());
                sVStartValue.init(pWrapper, ge->vvalue_start());
                sHEndValue.init(pWrapper, ge->hvalue_end());
                sVEndValue.init(pWrapper, ge->vvalue_end());
                sLayout.init(pWrapper, ge->layout());
                sTransparency.init(pWrapper, ge->transparency());
            }

            return STATUS_OK;
        }

        void Embed::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphEmbed *ge = tk::widget_cast<tk::GraphEmbed>(wWidget);
            if (ge != NULL)
            {
                set_param(ge->origin(), "origin", name, value);
                set_param(ge->origin(), "center", name, value);
                set_param(ge->origin(), "o", name, value);

                sXAxis.set("haxis", name, value);
                sXAxis.set("xaxis", name, value);
                sXAxis.set("ox", name, value);

                sYAxis.set("vaxis", name, value);
                sYAxis.set("yaxis", name, value);
                sYAxis.set("oy", name, value);

                sHStartValue.set("start.hvalue", name, value);
                sHStartValue.set("start.h", name, value);
                sHStartValue.set("start.x", name, value);
                sHStartValue.set("sx", name, value);

                sVStartValue.set("start.vert", name, value);
                sVStartValue.set("start.v", name, value);
                sVStartValue.set("start.y", name, value);
                sVStartValue.set("sy", name, value);

                sHEndValue.set("end.hvalue", name, value);
                sHEndValue.set("end.h", name, value);
                sHEndValue.set("end.x", name, value);
                sHEndValue.set("ex", name, value);

                sVEndValue.set("end.vert", name, value);
                sVEndValue.set("end.v", name, value);
                sVEndValue.set("end.y", name, value);
                sVEndValue.set("ey", name, value);

                sLayout.set(name, value);

                sTransparency.set("transparency", name, value);
                sTransparency.set("alpha", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        status_t Embed::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::GraphEmbed *ge = tk::widget_cast<tk::GraphEmbed>(wWidget);
            return (ge != NULL) ? ge->add(child->widget()) : STATUS_BAD_STATE;
        }

    } /* namespace ctl */
} /* namespace lsp */




