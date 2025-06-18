/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 июн. 2021 г.
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
        CTL_FACTORY_IMPL_START(Hyperlink)
            status_t res;

            if (!name->equals_ascii("hlink"))
                return STATUS_NOT_FOUND;

            tk::Hyperlink *w = new tk::Hyperlink(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Hyperlink *wc  = new ctl::Hyperlink(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Hyperlink)

        //-----------------------------------------------------------------
        const ctl_class_t Hyperlink::metadata     = { "Hyperlink", &Widget::metadata };

        Hyperlink::Hyperlink(ui::IWrapper *wrapper, tk::Hyperlink *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        Hyperlink::~Hyperlink()
        {
        }

        status_t Hyperlink::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Hyperlink *lnk = tk::widget_cast<tk::Hyperlink>(wWidget);
            if (lnk != NULL)
            {
                sText.init(pWrapper, lnk->text());
                sUrl.init(pWrapper, lnk->url());
                sColor.init(pWrapper, lnk->color());
                sHoverColor.init(pWrapper, lnk->hover_color());
                sInactiveColor.init(pWrapper, lnk->inactive_color());
                sInactiveHoverColor.init(pWrapper, lnk->inactive_hover_color());
            }

            return STATUS_OK;
        }

        void Hyperlink::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Hyperlink *lnk = tk::widget_cast<tk::Hyperlink>(wWidget);
            if (lnk != NULL)
            {
                sText.set("text", name, value);
                sUrl.set("url", name, value);
                sColor.set("color", name, value);
                sHoverColor.set("hover.color", name, value);
                sHoverColor.set("hcolor", name, value);
                sInactiveColor.set("inactive.color", name, value);
                sInactiveHoverColor.set("inactive.hover.color", name, value);
                sInactiveHoverColor.set("inactive.hcolor", name, value);

                set_constraints(lnk->constraints(), name, value);
                set_font(lnk->font(), "font", name, value);
                set_text_layout(lnk->text_layout(), name, value);

                set_param(lnk->text_adjust(), "text.adjust", name, value);
                set_param(lnk->follow(), "follow", name, value);
            }

            return Widget::set(ctx, name, value);
        }

    } /* namespace ctl */
} /* namespace lsp */


