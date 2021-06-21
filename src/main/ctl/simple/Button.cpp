/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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
            sEditable.destroy();
        }

        status_t Button::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn != NULL)
            {
                sColor.init(pWrapper, btn->color());
                sHoverColor.init(pWrapper, btn->hover_color());
                sLedColor.init(pWrapper, btn->led_color());
                sTextColor.init(pWrapper, btn->text_color());
                sHoverTextColor.init(pWrapper, btn->hover_text_color());
                sLedTextColor.init(pWrapper, btn->led_text_color());
                sEditable.init(pWrapper, this);
                sTextPad.init(pWrapper, btn->text_padding());
                sText.init(pWrapper, btn->text());

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
                sHoverColor.set("hover.color", name, value);
                sHoverColor.set("hcolor", name, value);
                sLedColor.set("led.color", name, value);
                sLedColor.set("lcolor", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sHoverTextColor.set("hover.text.color", name, value);
                sHoverTextColor.set("htcolor", name, value);
                sLedTextColor.set("led.text.color", name, value);
                sLedTextColor.set("ltcolor", name, value);
                sHoleColor.set("hole.color", name, value);
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
                set_expr(&sEditable, "editable", name, value);
                set_text_layout(btn->text_layout(), name, value);
            }

            Widget::set(ctx, name, value);
        }

        void Button::commit_value(float value)
        {
            lsp_trace("commit value=%f", value);
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
                    btn->down()->set(fabs(value - max) < fabs(value - min));
            }
            else
            {
                fValue      = (value >= 0.5f) ? 1.0f : 0.0f;
                btn->down()->set(fValue >= 0.5f);
            }
        }

        void Button::submit_value()
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn == NULL)
                return;

            lsp_trace("button is down=%s", (btn->down()->get()) ? "true" : "false");

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
                pPort->notify_all();
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

            float value = fValue + step;
            if (value > max)
                value = min;
            else if (value < min)
                value = max;

            return value;
        }

        void Button::trigger_expr()
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn == NULL)
                return;

            if (sEditable.valid())
                btn->editable()->set(sEditable.evaluate() >= 0.5f);
        }

        void Button::notify(ui::IPort *port)
        {
            Widget::notify(port);

            if ((port == pPort) && (pPort != NULL))
                commit_value(pPort->value());

            // Trigger expressions
            trigger_expr();
        }

        void Button::end(ui::UIContext *ctx)
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn == NULL)
                return;

            if (btn != NULL)
            {
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
            }

            trigger_expr();
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




