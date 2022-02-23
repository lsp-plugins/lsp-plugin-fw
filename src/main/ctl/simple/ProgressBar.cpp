/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 окт. 2021 г.
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
        CTL_FACTORY_IMPL_START(ProgressBar)
            status_t res;
            if (!name->equals_ascii("progress"))
                return STATUS_NOT_FOUND;

            tk::ProgressBar *w = new tk::ProgressBar(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::ProgressBar *wc  = new ctl::ProgressBar(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(ProgressBar)

        //-----------------------------------------------------------------
        const ctl_class_t ProgressBar::metadata     = { "ProgressBar", &Widget::metadata };

        ProgressBar::ProgressBar(ui::IWrapper *wrapper, tk::ProgressBar *widget): Widget(wrapper, widget)
        {
            pClass      = &metadata;

            pPort       = NULL;
        }

        ProgressBar::~ProgressBar()
        {
        }

        status_t ProgressBar::init()
        {
            status_t res = Widget::init();
            if (res != STATUS_OK)
                return res;

            tk::ProgressBar *pb = tk::widget_cast<tk::ProgressBar>(wWidget);
            if (pb != NULL)
            {
                pb->text()->set("labels.values.x_pc");

                sText.init(pWrapper, pb->text());
                sShowText.init(pWrapper, pb->show_text());
                sBorderColor.init(pWrapper, pb->border_color());
                sBorderGapColor.init(pWrapper, pb->border_gap_color());
                sColor.init(pWrapper, pb->color());
                sTextColor.init(pWrapper, pb->text_color());
                sInvColor.init(pWrapper, pb->inv_color());
                sInvTextColor.init(pWrapper, pb->inv_text_color());
                sBorderSize.init(pWrapper, pb->border_size());
                sBorderGapSize.init(pWrapper, pb->border_gap_size());
                sBorderRadius.init(pWrapper, pb->border_radius());

                sValue.init(pWrapper, this);
                sMin.init(pWrapper, this);
                sMax.init(pWrapper, this);
                sDefault.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void ProgressBar::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::ProgressBar *pb = tk::widget_cast<tk::ProgressBar>(wWidget);
            if (pb != NULL)
            {
                bind_port(&pPort, "id", name, value);

                set_constraints(pb->constraints(), name, value);
                set_text_layout(pb->text_layout(), name, value);
                set_font(pb->font(), "font", name, value);

                sText.set("text", name, value);
                sShowText.set("text.visibility", name, value);
                sShowText.set("tvisibility", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sBorderGapColor.set("border.gap.color", name, value);
                sBorderGapColor.set("gap.color", name, value);
                sBorderGapColor.set("gcolor", name, value);
                sColor.set("color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sInvColor.set("color.inv", name, value);
                sInvTextColor.set("text.color.inv", name, value);
                sInvTextColor.set("tcolor.inv", name, value);

                sBorderSize.set("border.size", name, value);
                sBorderSize.set("bsize", name, value);
                sBorderGapSize.set("border.gap.size", name, value);
                sBorderGapSize.set("gap.size", name, value);
                sBorderGapSize.set("gsize", name, value);
                sBorderRadius.set("border.radius", name, value);
                sBorderRadius.set("bradius", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void ProgressBar::notify(ui::IPort *port)
        {
            Widget::notify(port);

            if (((pPort != NULL) && (port == pPort)) ||
                (sMin.depends(port)) ||
                (sMax.depends(port)) ||
                (sValue.depends(port)) ||
                (sDefault.depends(port)))
                sync_value();
        }

        void ProgressBar::end(ui::UIContext *ctx)
        {
            sync_value();
            Widget::end(ctx);
        }

        void ProgressBar::sync_value()
        {
            tk::ProgressBar *pb = tk::widget_cast<tk::ProgressBar>(wWidget);
            if (pb != NULL)
            {
                const meta::port_t *meta = (pPort != NULL) ? pPort->metadata() : NULL;

                float min = 0.0f, max = 1.0f, value = 0.0f, dfl = 0.0f;

                if (sDefault.valid())
                    dfl     = sDefault.evaluate_float(0.0f);
                else if (meta != NULL)
                    dfl     = meta->start;
                if (sMin.valid())
                    min     = sMin.evaluate_float(dfl);
                else if ((meta != NULL) && (meta->flags & meta::F_LOWER))
                    min     = meta->min;
                if (sMax.valid())
                    max     = sMax.evaluate_float(dfl);
                else if ((meta != NULL) && (meta->flags & meta::F_UPPER))
                    max     = meta->max;
                if (sValue.valid())
                    value   = sValue.evaluate_float(dfl);
                else if (pPort != NULL)
                    value   = pPort->value();

                // Update value
                pb->value()->set_all(value, min, max);
                pb->text()->params()->set_float("value", value);
            }
        }

    } /* namespace ctl */
} /* namespace lsp */

