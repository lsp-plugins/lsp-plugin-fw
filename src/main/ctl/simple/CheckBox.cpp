/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-sampler
 * Created on: 17 дек. 2022 г.
 *
 * lsp-plugins-sampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-sampler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-sampler. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {

        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(CheckBox)
            status_t res;

            if (!name->equals_ascii("check"))
                return STATUS_NOT_FOUND;

            tk::CheckBox *w = new tk::CheckBox(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::CheckBox *wc  = new ctl::CheckBox(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(CheckBox)

        //-----------------------------------------------------------------
        const ctl_class_t CheckBox::metadata            = { "CheckBox", &Widget::metadata };

        CheckBox::CheckBox(ui::IWrapper *wrapper, tk::CheckBox *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            fValue          = 0;
            pPort           = NULL;
            bInvert         = false;
        }

        CheckBox::~CheckBox()
        {
        }

        status_t CheckBox::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::CheckBox *ck   = tk::widget_cast<tk::CheckBox>(wWidget);
            if (ck != NULL)
            {
                sBorderSize.init(pWrapper, ck->border_size());
                sBorderRadius.init(pWrapper, ck->border_radius());
                sBorderGapSize.init(pWrapper, ck->border_gap_size());
                sCheckRadius.init(pWrapper, ck->check_radius());
                sCheckGapSize.init(pWrapper, ck->check_gap_size());
                sCheckMinSize.init(pWrapper, ck->check_min_size());

                sColor.init(pWrapper, ck->color());
                sHoverColor.init(pWrapper, ck->hover_color());
                sFillColor.init(pWrapper, ck->fill_color());
                sFillHoverColor.init(pWrapper, ck->fill_hover_color());
                sBorderColor.init(pWrapper, ck->border_color());
                sBorderHoverColor.init(pWrapper, ck->border_hover_color());
                sBorderGapColor.init(pWrapper, ck->border_gap_color());
                sBorderGapHoverColor.init(pWrapper, ck->border_gap_hover_color());

                // Bind slots
                ck->slots()->bind(tk::SLOT_SUBMIT, slot_submit, this);
            }

            return STATUS_OK;
        }

        void CheckBox::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::CheckBox *ck   = tk::widget_cast<tk::CheckBox>(wWidget);
            if (ck != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sBorderSize.set("border.size", name, value);
                sBorderSize.set("bsize", name, value);
                sBorderRadius.set("border.radius", name, value);
                sBorderRadius.set("bradius", name, value);
                sBorderGapSize.set("border.gap.size", name, value);
                sBorderGapSize.set("bgap.size", name, value);
                sCheckRadius.set("check.radius", name, value);
                sCheckGapSize.set("check.gap.size", name, value);
                sCheckGapSize.set("cgap.size", name, value);
                sCheckMinSize.set("check.min.size", name, value);

                sColor.set("color", name, value);
                sHoverColor.set("hover.color", name, value);
                sHoverColor.set("hcolor", name, value);
                sFillColor.set("fill.color", name, value);
                sFillHoverColor.set("fill.hover.color", name, value);
                sFillHoverColor.set("fill.hcolor", name, value);

                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sBorderHoverColor.set("border.hover.color", name, value);
                sBorderHoverColor.set("border.hcolor", name, value);
                sBorderGapColor.set("border.gap.color", name, value);
                sBorderGapColor.set("bgap.color", name, value);
                sBorderGapHoverColor.set("border.gap.hover.color", name, value);
                sBorderGapHoverColor.set("border.gap.hcolor", name, value);
                sBorderGapHoverColor.set("bgap.hover.color", name, value);
                sBorderGapHoverColor.set("bgap.hcolor", name, value);

                set_constraints(ck->constraints(), name, value);
                set_value(&bInvert, "invert", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void CheckBox::commit_value(float value)
        {
            tk::CheckBox *ck   = tk::widget_cast<tk::CheckBox>(wWidget);
            if (ck == NULL)
                return;

            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
            float half = ((p != NULL) && (p->unit != meta::U_BOOL)) ? (p->min + p->max) * 0.5f : 0.5f;
            ck->checked()->set((value >= half) ^ (bInvert));
        }

        void CheckBox::submit_value()
        {
            tk::CheckBox *ck   = tk::widget_cast<tk::CheckBox>(wWidget);
            if (ck == NULL)
                return;

            bool checked    = ck->checked()->get();
            lsp_trace("checkbox checked=%s", (checked) ? "true" : "false");

            const meta::port_t *p   = (pPort != NULL) ? pPort->metadata() : NULL;
            float min       = ((p != NULL) && (p->unit != meta::U_BOOL)) ? p->min : 0.0f;
            float max       = ((p != NULL) && (p->unit != meta::U_BOOL)) ? p->max : 1.0f;
            float value     = (checked ^ bInvert) ? max : min;

            if (pPort != NULL)
            {
                pPort->set_value(value);
                pPort->notify_all(ui::PORT_USER_EDIT);
            }
        }

        void CheckBox::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);
            if ((port == pPort) && (pPort != NULL))
                commit_value(pPort->value());
        }

        void CheckBox::end(ui::UIContext *ctx)
        {
            commit_value((pPort != NULL) ? pPort->value() : fValue);
            Widget::end(ctx);
        }

        status_t CheckBox::slot_submit(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::CheckBox *_this      = static_cast<ctl::CheckBox *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


