/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 31 июл. 2021 г.
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
        CTL_FACTORY_IMPL_START(LedMeter)
            status_t res;

            if (!name->equals_ascii("ledmeter"))
                return STATUS_NOT_FOUND;

            tk::LedMeter *w = new tk::LedMeter(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::LedMeter *wc  = new ctl::LedMeter(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(LedMeter)

        //-----------------------------------------------------------------
        const ctl_class_t LedMeter::metadata            = { "LedMeter", &Widget::metadata };

        LedMeter::LedMeter(ui::IWrapper *wrapper, tk::LedMeter *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        LedMeter::~LedMeter()
        {
        }

        status_t LedMeter::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::LedMeter *lm = tk::widget_cast<tk::LedMeter>(wWidget);
            if (lm != NULL)
            {
                sEstText.init(pWrapper, lm->estimation_text());
                sColor.init(pWrapper, lm->color());
            }

            return STATUS_OK;
        }

        void LedMeter::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::LedMeter *lm = tk::widget_cast<tk::LedMeter>(wWidget);
            if (lm != NULL)
            {
                set_constraints(lm->constraints(), name, value);
                set_font(lm->font(), "font", name, value);

                sEstText.set("estimation_text", name, value);
                sEstText.set("etext", name, value);

                set_param(lm->border(), "border", name, value);
                set_param(lm->angle(), "angle", name, value);
                set_param(lm->stereo_groups(), "stereo_groups", name, value);
                set_param(lm->stereo_groups(), "stereo", name, value);
                set_param(lm->stereo_groups(), "sgroups", name, value);
                set_param(lm->text_visible(), "text.visible", name, value);
                set_param(lm->text_visible(), "tvisible", name, value);
                set_param(lm->header_visible(), "header.visible", name, value);
                set_param(lm->header_visible(), "hvisible", name, value);
                set_param(lm->min_channel_width(), "channel_width.min", name, value);
                set_param(lm->min_channel_width(), "cwidth.min", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        status_t LedMeter::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::LedMeterChannel *lmc = (child != NULL) ? tk::widget_cast<tk::LedMeterChannel>(child->widget()) : NULL;
            if (lmc == NULL)
                return STATUS_BAD_ARGUMENTS;

            tk::LedMeter *lm = tk::widget_cast<tk::LedMeter>(wWidget);
            if (lm == NULL)
                return STATUS_BAD_STATE;

            LedChannel *lc = ctl_cast<LedChannel>(child);
            if (lc != NULL)
            {
                vChildren.add(lc);
                lc->set_parent(this);
            }

            return lm->items()->add(lmc);
        }

        void LedMeter::cleanup_header()
        {
            for (size_t i=0, n = vChildren.size(); i<n; ++i)
            {
                LedChannel *lc = vChildren.uget(i);
                if (lc != NULL)
                    lc->cleanup_header();
            }
        }

    } /* namespace ctl */
} /* namespace lsp */


