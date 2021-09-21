/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 сент. 2021 г.
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
#include <lsp-plug.in/runtime/system.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(TempoTap)
            status_t res;

            if (!name->equals_ascii("ttap"))
                return STATUS_NOT_FOUND;

            tk::Button *w = new tk::Button(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::TempoTap *wc   = new ctl::TempoTap(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(TempoTap)

        //-----------------------------------------------------------------
        const ctl_class_t TempoTap::metadata     = { "TempoTap", &Widget::metadata };

        TempoTap::TempoTap(ui::IWrapper *wrapper, tk::Button *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            nThresh         = 1000;
            nLastTap        = 0;
            fTempo          = 0.0f;
        }

        TempoTap::~TempoTap()
        {
        }

        status_t TempoTap::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn != NULL)
            {
                sColor.init(pWrapper, btn->color());
                sTextColor.init(pWrapper, btn->text_color());
                sBorderColor.init(pWrapper, btn->border_color());
                sHoverColor.init(pWrapper, btn->hover_color());
                sTextHoverColor.init(pWrapper, btn->text_hover_color());
                sBorderHoverColor.init(pWrapper, btn->border_hover_color());
                sDownColor.init(pWrapper, btn->down_color());
                sTextDownColor.init(pWrapper, btn->text_down_color());
                sBorderDownColor.init(pWrapper, btn->border_down_color());
                sDownHoverColor.init(pWrapper, btn->down_hover_color());
                sTextDownHoverColor.init(pWrapper, btn->text_down_hover_color());
                sBorderDownHoverColor.init(pWrapper, btn->border_down_hover_color());
                sHoleColor.init(pWrapper, btn->hole_color());

                sEditable.init(pWrapper, btn->editable());
                sTextPad.init(pWrapper, btn->text_padding());
                sText.init(pWrapper, btn->text());

                // Bind slots
                btn->slots()->bind(tk::SLOT_CHANGE, slot_change, this);

                // Set trigger mode
                inject_style(btn, "TempoTap");
                btn->mode()->set_trigger();
            }

            return STATUS_OK;
        }

        void TempoTap::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sColor.set("color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sHoverColor.set("hover.color", name, value);
                sHoverColor.set("hcolor", name, value);
                sTextHoverColor.set("text.hover.color", name, value);
                sTextHoverColor.set("thcolor", name, value);
                sBorderHoverColor.set("border.hover.color", name, value);
                sBorderHoverColor.set("bhcolor", name, value);
                sDownColor.set("down.color", name, value);
                sDownColor.set("dcolor", name, value);
                sTextDownColor.set("text.down.color", name, value);
                sTextDownColor.set("tdcolor", name, value);
                sBorderDownColor.set("border.down.color", name, value);
                sBorderDownColor.set("bdcolor", name, value);
                sDownHoverColor.set("down.hover.color", name, value);
                sDownHoverColor.set("dhcolor", name, value);
                sTextDownHoverColor.set("text.down.hover.color", name, value);
                sTextDownHoverColor.set("tdhcolor", name, value);
                sBorderDownHoverColor.set("border.down.hover.color", name, value);
                sBorderDownHoverColor.set("bdhcolor", name, value);

                sHoleColor.set("hole.color", name, value);

                sEditable.set("editable", name, value);
                sTextPad.set("text.padding", name, value);
                sTextPad.set("text.pad", name, value);
                sTextPad.set("tpadding", name, value);
                sTextPad.set("tpad", name, value);
                sHover.set("hover", name, value);
                sText.set("text", name, value);

                set_font(btn->font(), "font", name, value);
                set_constraints(btn->constraints(), name, value);
                set_param(btn->led(), "led", name, value);
                set_param(btn->hole(), "hole", name, value);
                set_param(btn->flat(), "flat", name, value);
                set_param(btn->text_clip(), "text.clip", name, value);
                set_param(btn->text_adjust(), "text.adjust", name, value);
                set_param(btn->text_clip(), "tclip", name, value);
                set_param(btn->font_scaling(), "font.scaling", name, value);
                set_param(btn->font_scaling(), "font.scale", name, value);
                set_text_layout(btn->text_layout(), name, value);
            }

            Widget::set(ctx, name, value);
        }

        void TempoTap::end(ui::UIContext *ctx)
        {
            if (pPort != NULL)
            {
                const meta::port_t *meta = pPort->metadata();
                if ((meta != NULL) && (meta->flags & meta::F_LOWER))
                    nThresh     = (121 * 1000) / (meta->min);
            }

            Widget::end(ctx);
        }

        uint64_t TempoTap::time()
        {
            system::time_t time;
            system::get_time(&time);

            return (time.seconds * 1000) + (time.nanos / 1000000);
        }

        void TempoTap::submit_value()
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if ((btn == NULL) || (btn->down()->get()))
                return;

            // Estimate delay between two sequential taps
            uint64_t t  = time();
            int64_t d   = t - nLastTap;
            nLastTap    = t;

            lsp_trace("button tap time=%lld, delta=%lld", (long long)t, (long long)d);

            if ((d >= nThresh) || (d <= 0))
            {
                fTempo   = 0.0f;
                return;
            }

            // Now calculate tempo
            float tempo = 60000.0f / d;
            fTempo = (fTempo <= 0) ? tempo : fTempo * 0.5f + tempo * 0.5f;
            lsp_trace("tempo = %.3f", fTempo);

            // Update port value
            if (pPort != NULL)
            {
                pPort->set_value(fTempo);
                pPort->notify_all();
            }
        }

        status_t TempoTap::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            TempoTap *_this    = static_cast<TempoTap *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

    } // namespace ctl
} // namespace lsp


