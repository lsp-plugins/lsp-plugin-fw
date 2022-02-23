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

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Switch)
            status_t res;

            if (!name->equals_ascii("switch"))
                return STATUS_NOT_FOUND;

            tk::Switch *w = new tk::Switch(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Switch *wc  = new ctl::Switch(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Switch)

        //-----------------------------------------------------------------
        const ctl_class_t Switch::metadata          = { "Switch", &Widget::metadata };

        Switch::Switch(ui::IWrapper *wrapper, tk::Switch *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            fValue          = 0;
            pPort           = NULL;
            bInvert         = false;
        }

        Switch::~Switch()
        {
        }

        status_t Switch::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Switch *sw   = tk::widget_cast<tk::Switch>(wWidget);
            if (sw != NULL)
            {
                sColor.init(pWrapper, sw->color());
                sTextColor.init(pWrapper, sw->text_color());
                sBorderColor.init(pWrapper, sw->border_color());
                sHoleColor.init(pWrapper, sw->hole_color());

                // Bind slots
                sw->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
            }

            return STATUS_OK;
        }

        void Switch::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Switch *sw   = tk::widget_cast<tk::Switch>(wWidget);
            if (sw != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sColor.set("color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sHoleColor.set("hole.color", name, value);
                sHoleColor.set("hcolor", name, value);

                set_size_range(sw->size(), "size", name, value);

                set_param(sw->border(), "border", name, value);
                set_param(sw->aspect(), "aspect", name, value);
                set_param(sw->angle(), "angle", name, value);

                set_value(&bInvert, "invert", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Switch::commit_value(float value)
        {
            tk::Switch *sw   = tk::widget_cast<tk::Switch>(wWidget);
            if (sw == NULL)
                return;

            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
            float half = ((p != NULL) && (p->unit != meta::U_BOOL)) ? (p->min + p->max) * 0.5f : 0.5f;
            sw->down()->set((value >= half) ^ (bInvert));
        }

        void Switch::submit_value()
        {
            tk::Switch *sw   = tk::widget_cast<tk::Switch>(wWidget);
            if (sw == NULL)
                return;

            bool down       = sw->down()->get();
            lsp_trace("switch clicked down=%s", (down) ? "true" : "false");

            const meta::port_t *p   = (pPort != NULL) ? pPort->metadata() : NULL;
            float min       = ((p != NULL) && (p->unit != meta::U_BOOL)) ? p->min : 0.0f;
            float max       = ((p != NULL) && (p->unit != meta::U_BOOL)) ? p->max : 1.0f;
            float value     = (down ^ bInvert) ? max : min;

            if (pPort != NULL)
            {
                pPort->set_value(value);
                pPort->notify_all();
            }
        }

        void Switch::notify(ui::IPort *port)
        {
            Widget::notify(port);

            if ((port == pPort) && (pPort != NULL))
                commit_value(pPort->value());
        }

        void Switch::end(ui::UIContext *ctx)
        {
            commit_value((pPort != NULL) ? pPort->value() : fValue);
            Widget::end(ctx);
        }

        status_t Switch::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Switch *_this      = static_cast<ctl::Switch *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

    }
}


