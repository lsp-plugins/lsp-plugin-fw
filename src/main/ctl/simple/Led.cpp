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

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Led)
            status_t res;

            if (!name->equals_ascii("led"))
                return STATUS_NOT_FOUND;

            tk::Led *w = new tk::Led(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Led *wc  = new ctl::Led(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Led)

        //-----------------------------------------------------------------
        const ctl_class_t Led::metadata         = { "Led", &Widget::metadata };

        Led::Led(ui::IWrapper *wrapper, tk::Led *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            fValue          = 0.0f;
            pPort           = NULL;
            fKey            = 1;
            bInvert         = false;
        }

        Led::~Led()
        {
        }

        status_t Led::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Led *led = tk::widget_cast<tk::Led>(wWidget);
            if (led != NULL)
            {
                sColor.init(pWrapper, led->color());
                sLightColor.init(pWrapper, led->led_color());
                sBorderColor.init(pWrapper, led->border_color());
                sLightBorderColor.init(pWrapper, led->led_border_color());
                sInactiveColor.init(pWrapper, led->inactive_color());
                sInactiveLightColor.init(pWrapper, led->inactive_led_color());
                sInactiveBorderColor.init(pWrapper, led->inactive_border_color());
                sInactiveLightBorderColor.init(pWrapper, led->inactive_led_border_color());

                sHoleColor.init(pWrapper, led->hole_color());

                sLight.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void Led::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Led *led = tk::widget_cast<tk::Led>(wWidget);
            if (led != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sColor.set("color", name, value);
                sLightColor.set("light.color", name, value);
                sLightColor.set("led.color", name, value);
                sLightColor.set("lcolor", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sLightBorderColor.set("light.bcolor", name, value);
                sLightBorderColor.set("led.bcolor", name, value);
                sLightBorderColor.set("lbcolor", name, value);

                sInactiveColor.set("inactive.color", name, value);
                sInactiveLightColor.set("inactive.light.color", name, value);
                sInactiveLightColor.set("inactive.led.color", name, value);
                sInactiveLightColor.set("inactive.lcolor", name, value);
                sInactiveBorderColor.set("inactive.border.color", name, value);
                sInactiveBorderColor.set("inactive.bcolor", name, value);
                sInactiveLightBorderColor.set("inactive.light.bcolor", name, value);
                sInactiveLightBorderColor.set("inactive.led.bcolor", name, value);
                sInactiveLightBorderColor.set("inactive.lbcolor", name, value);

                sHoleColor.set("hole.color", name, value);
                sHoleColor.set("hcolor", name, value);

                set_expr(&sLight, "light", name, value);

                set_constraints(led->constraints(), name, value);
                set_param(led->hole(), "hole", name, value);
                set_param(led->led(), "led", name, value);
                set_param(led->gradient(), "gradient", name, value);
                set_param(led->border_size(), "border.size", name, value);
                set_param(led->border_size(), "bsize", name, value);
                set_param(led->round(), "round", name, value);
                set_param(led->gradient(), "gradient", name, value);

                set_value(&fKey, "key", name, value);
                set_value(&fValue, "value", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Led::update_value()
        {
            tk::Led *led = tk::widget_cast<tk::Led>(wWidget);
            if (led == NULL)
                return;

            bool on = false;
            if (sLight.valid())
            {
                const float value = sLight.evaluate();
                on = value >= 0.5f;
            }
            else if (pPort != NULL)
            {
                const float value = pPort->value();
                const meta::port_t *meta = pPort->metadata();

                on = (meta->unit == meta::U_ENUM) ? (abs(value - fKey) <= FLOAT_CMP_PREC) : (value >= 0.5f);
            }
            else
                on = abs(fValue - fKey) <= FLOAT_CMP_PREC;

            // Update lighting
            led->on()->set(on ^ bInvert);
        }

        void Led::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            if (sLight.depends(port))
                update_value();

            if ((port == pPort) && (pPort != NULL))
                update_value();
        }

        void Led::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
            update_value();
        }

        void Led::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);
            update_value();
        }
    } /* namespace ctl */
} /* namespace lsp */

