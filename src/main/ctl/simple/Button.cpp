/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 мая 2021 г.
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
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Button)
            status_t res;

            if (!name->equals_ascii("button"))
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

            ctl::Button *wc  = new ctl::Button(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Button)

        //-----------------------------------------------------------------
        const ctl_class_t Button::metadata     = { "Button", &Widget::metadata };

        Button::Button(ui::IWrapper *wrapper, tk::Button *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            fValue          = 0.0f;
            fDflValue       = 0.0f;
            bValueSet       = false;
            pPort           = NULL;
        }

        Button::~Button()
        {
        }

        status_t Button::init()
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

                sInactiveColor.init(pWrapper, btn->inactive_color());
                sInactiveTextColor.init(pWrapper, btn->inactive_text_color());
                sInactiveBorderColor.init(pWrapper, btn->inactive_border_color());
                sInactiveHoverColor.init(pWrapper, btn->inactive_hover_color());
                sInactiveTextHoverColor.init(pWrapper, btn->inactive_text_hover_color());
                sInactiveBorderHoverColor.init(pWrapper, btn->inactive_border_hover_color());
                sInactiveDownColor.init(pWrapper, btn->inactive_down_color());
                sInactiveTextDownColor.init(pWrapper, btn->inactive_text_down_color());
                sInactiveBorderDownColor.init(pWrapper, btn->inactive_border_down_color());
                sInactiveDownHoverColor.init(pWrapper, btn->inactive_down_hover_color());
                sInactiveTextDownHoverColor.init(pWrapper, btn->inactive_text_down_hover_color());
                sInactiveBorderDownHoverColor.init(pWrapper, btn->inactive_border_down_hover_color());

                sHoleColor.init(pWrapper, btn->hole_color());

                sEditable.init(pWrapper, btn->editable());
                sTextPad.init(pWrapper, btn->text_padding());
                sText.init(pWrapper, btn->text());
                sTextClip.init(pWrapper, btn->text_clip());

                // Bind slots
                btn->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
            }

            return STATUS_OK;
        }

        void Button::set(ui::UIContext *ctx, const char *name, const char *value)
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

                sInactiveColor.set("inactive.color", name, value);
                sInactiveTextColor.set("inactive.text.color", name, value);
                sInactiveTextColor.set("inactive.tcolor", name, value);
                sInactiveBorderColor.set("inactive.border.color", name, value);
                sInactiveBorderColor.set("inactive.bcolor", name, value);
                sInactiveHoverColor.set("inactive.hover.color", name, value);
                sInactiveHoverColor.set("inactive.hcolor", name, value);
                sInactiveTextHoverColor.set("inactive.text.hover.color", name, value);
                sInactiveTextHoverColor.set("inactive.thcolor", name, value);
                sInactiveBorderHoverColor.set("inactive.border.hover.color", name, value);
                sInactiveBorderHoverColor.set("inactive.bhcolor", name, value);
                sInactiveDownColor.set("inactive.down.color", name, value);
                sInactiveDownColor.set("inactive.dcolor", name, value);
                sInactiveTextDownColor.set("inactive.text.down.color", name, value);
                sInactiveTextDownColor.set("inactive.tdcolor", name, value);
                sInactiveBorderDownColor.set("inactive.border.down.color", name, value);
                sInactiveBorderDownColor.set("inactive.bdcolor", name, value);
                sInactiveDownHoverColor.set("inactive.down.hover.color", name, value);
                sInactiveDownHoverColor.set("inactive.dhcolor", name, value);
                sInactiveTextDownHoverColor.set("inactive.text.down.hover.color", name, value);
                sInactiveTextDownHoverColor.set("inactive.tdhcolor", name, value);
                sInactiveBorderDownHoverColor.set("inactive.border.down.hover.color", name, value);
                sInactiveBorderDownHoverColor.set("inactive.bdhcolor", name, value);

                sHoleColor.set("hole.color", name, value);

                sEditable.set("editable", name, value);
                sTextPad.set("text.padding", name, value);
                sTextPad.set("text.pad", name, value);
                sTextPad.set("tpadding", name, value);
                sTextPad.set("tpad", name, value);
                sHover.set("hover", name, value);
                sText.set("text", name, value);
                sTextClip.set("text.clip", name, value);

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
                set_param(btn->mode(), "mode", name, value);
                set_text_layout(btn->text_layout(), name, value);

                if (set_value(&fDflValue, "value", name, value))
                {
                    bValueSet = true;
                    commit_value(fDflValue);
                    fDflValue = fValue;
                }
            }

            Widget::set(ctx, name, value);
        }

        void Button::commit_value(float value)
        {
//            lsp_trace("commit value=%f", value);
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn == NULL)
                return;

            const meta::port_t *mdata = (pPort != NULL) ? pPort->metadata() : NULL;

            if (mdata != NULL)
            {
                fValue      = value;

                float min   = (mdata->flags & meta::F_LOWER) ? mdata->min : 0.0;
                float max   = (mdata->flags & meta::F_UPPER) ? mdata->max : min + 1.0f;

                if (mdata->unit == meta::U_ENUM)
                {
                    if (bValueSet)
                        btn->down()->set(fValue == fDflValue);
                    else
                        btn->down()->set(false);
                }
                else if (!meta::is_trigger_port(mdata))
                {
                    if (bValueSet)
                        btn->down()->set(fValue == fDflValue);
                    else
                        btn->down()->set(fabs(value - max) < fabs(value - min));
                }
                else
                {
                    fValue      = (value >= 0.5f) ? 1.0f : 0.0f;
                    if (bValueSet)
                        btn->down()->set(fValue == fDflValue);
                    else
                        btn->down()->set(fValue >= 0.5f);
                }
            }
            else
            {
                fValue      = (value >= 0.5f) ? 1.0f : 0.0f;
                if (bValueSet)
                    btn->down()->set(fValue == fDflValue);
                else
                    btn->down()->set(fValue >= 0.5f);
            }
        }

        void Button::submit_value()
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn == NULL)
                return;

            //lsp_trace("button is down=%s", (btn->down()->get()) ? "true" : "false");

            float value     = next_value(btn->down()->get());
            if (value == fValue)
            {
                if (bValueSet)
                    btn->down()->set(value == fDflValue);
                return;
            }

            if (pPort != NULL)
            {
                lsp_trace("Setting %s = %f", pPort->id(), value);
                pPort->set_value(value);
                pPort->notify_all(ui::PORT_USER_EDIT);
            }
        }

        float Button::next_value(bool down)
        {
            const meta::port_t *mdata = (pPort != NULL) ? pPort->metadata() : NULL;
            if (mdata == NULL)
                return (fValue >= 0.5f) ? 0.0f : 1.0f;

            // Analyze event
            if (down)
            {
                if (mdata->unit == meta::U_ENUM)
                    return (bValueSet) ? fDflValue : fValue;
                else if (bValueSet)
                    return fDflValue;
            }

            // Get minimum and maximum
            float min   = (mdata->flags & meta::F_LOWER) ? mdata->min : 0.0;
            float max   = (mdata->flags & meta::F_UPPER) ? mdata->max : min + 1.0f;
            float step  = (mdata->flags & meta::F_STEP) ? mdata->step : 1.0;
            if ((mdata->unit == meta::U_ENUM) && (mdata->items != NULL))
            {
                if (bValueSet)
                    return fDflValue;

                max     = mdata->min + meta::list_size(mdata->items) - 1.0f;
            }
            else if (bValueSet)
                return fDflValue;

            float value = fValue + step;
            if (value > max)
                value = min;
            else if (value < min)
                value = max;

            return value;
        }

        void Button::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            if ((port == pPort) && (pPort != NULL))
                commit_value(pPort->value());
        }

        void Button::end(ui::UIContext *ctx)
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn == NULL)
                return;

            if (pPort != NULL)
            {
                const meta::port_t *mdata = pPort->metadata();
                if (mdata != NULL)
                {
                    if (meta::is_trigger_port(mdata))
                        btn->mode()->set_trigger();
                    else if (mdata->unit != meta::U_ENUM)
                        btn->mode()->set_toggle();
                    else if (bValueSet)
                        btn->mode()->set_toggle();
                }

                commit_value(pPort->value());
            }
            else
                commit_value(fValue);

            Widget::end(ctx);
        }

        status_t Button::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Button *_this      = static_cast<ctl::Button *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }
    } /* namespace ctl */
} /* namespace lsp */




