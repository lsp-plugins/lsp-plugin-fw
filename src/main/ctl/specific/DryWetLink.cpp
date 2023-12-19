/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 19 дек. 2023 г.
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
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(DryWetLink)
            status_t res;

            if (!name->equals_ascii("drywet"))
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

            ctl::DryWetLink *wc  = new ctl::DryWetLink(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(DryWetLink)

        //-----------------------------------------------------------------
        const ctl_class_t DryWetLink::metadata      = { "DryWetLink", &Button::metadata };

        DryWetLink::DryWetLink(ui::IWrapper *wrapper, tk::Button *widget):
            Button(wrapper, widget)
        {
            pClass          = &metadata;

            pDry            = NULL;
            pWet            = NULL;
        }

        DryWetLink::~DryWetLink()
        {
        }

        void DryWetLink::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            bind_port(&pDry, "dry.id", name, value);
            bind_port(&pWet, "wet.id", name, value);

            Button::set(ctx, name, value);
        }

        float DryWetLink::get_gain(ui::IPort *port)
        {
            const meta::port_t *meta = (port != NULL) ? port->metadata() : NULL;
            if (meta == NULL)
                return 0.0f;

            float min = 0.0f, max = 1.0f, value = port->value();
            meta::get_port_parameters(meta, &min, &max, NULL);

            if (meta->unit == meta::U_DB)
                value       = dspu::db_to_gain(value);

            return value;
        }

        void DryWetLink::set_gain(ui::IPort *port, float gain)
        {
            const meta::port_t *meta = (port != NULL) ? port->metadata() : NULL;
            if (meta == NULL)
                return;

            float min = 0.0f, max = 1.0f;
            meta::get_port_parameters(meta, &min, &max, NULL);

            if (meta->unit == meta::U_DB)
                gain    = dspu::gain_to_db(gain);
            gain        = lsp_limit(gain, min, max);

            // Update port value if it differs
            if (port->value() != gain)
            {
                port->set_value(gain);
                port->notify_all(ui::PORT_NONE);
            }
        }

        void DryWetLink::sync_value(ui::IPort *dst, ui::IPort *src)
        {
            // Check that link is enabled
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if ((btn == NULL) || (!btn->down()->get()))
                return;
            if ((dst == NULL) || (src == NULL))
                return;

            // Link the gain values
            float src_gain  = get_gain(src);
            float dst_gain  = lsp_max(0.0f, 1.0f - src_gain);
            set_gain(dst, dst_gain);
        }

        void DryWetLink::notify(ui::IPort *port, size_t flags)
        {
            Button::notify(port, flags);

            if ((port != NULL) && (flags & ui::PORT_USER_EDIT))
            {
                if (port == pDry)
                    sync_value(pWet, port);
                if (port == pWet)
                    sync_value(pDry, port);
            }
        }
    } /* namespace ctl */
} /* namespace lsp */



