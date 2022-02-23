/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 17 мая 2021 г.
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

#define TMP_BUF_SIZE        128

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Text)
            status_t res;

            if (!name->equals_ascii("text"))
                return STATUS_NOT_FOUND;

            tk::GraphText *w = new tk::GraphText(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Text *wc   = new ctl::Text(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Text)

        //-----------------------------------------------------------------
        const ctl_class_t Text::metadata        = { "Text", &Widget::metadata };

        Text::Text(ui::IWrapper *wrapper, tk::GraphText *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
        }

        Text::~Text()
        {
        }

        status_t Text::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphText *gt   = tk::widget_cast<tk::GraphText>(wWidget);
            if (gt != NULL)
            {
                sColor.init(pWrapper, gt->color());
                sHValue.init(pWrapper, gt->hvalue());
                sVValue.init(pWrapper, gt->vvalue());
                sText.init(pWrapper, gt->text());
            }

            return STATUS_OK;
        }

        void Text::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphText *gt   = tk::widget_cast<tk::GraphText>(wWidget);
            if (gt != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sColor.set("color", name, value);
                set_layout(gt->layout(), NULL, name, value);
                set_text_layout(gt->text_layout(), name, value);

                sHValue.set("hval", name, value);
                sHValue.set("xval", name, value);
                sHValue.set("x", name, value);

                sVValue.set("vval", name, value);
                sVValue.set("yval", name, value);
                sVValue.set("y", name, value);

                sText.set("text", name, value);

                set_param(gt->haxis(), "basis", name, value);
                set_param(gt->haxis(), "xaxis", name, value);
                set_param(gt->haxis(), "ox", name, value);

                set_param(gt->vaxis(), "parallel", name, value);
                set_param(gt->vaxis(), "yaxis", name, value);
                set_param(gt->vaxis(), "oy", name, value);

                set_param(gt->origin(), "origin", name, value);
                set_param(gt->origin(), "center", name, value);
                set_param(gt->origin(), "o", name, value);

                set_param(gt->text_adjust(), "text.adjust", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Text::trigger_expr()
        {
            tk::GraphText *gt   = tk::widget_cast<tk::GraphText>(wWidget);
            if (gt == NULL)
                return;

            // Get port metadata and value
            char buf[TMP_BUF_SIZE];
            const meta::port_t *mdata   = (pPort != NULL) ? pPort->metadata() : NULL;
            if (mdata != NULL)
            {
                meta::format_value(buf, TMP_BUF_SIZE, mdata, pPort->value(), -1);
                gt->text()->params()->set_cstring("value", buf);
            }
        }

        void Text::notify(ui::IPort *port)
        {
            Widget::notify(port);

            if ((pPort == port) && (pPort != NULL))
                trigger_expr();
        }

        void Text::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
            trigger_expr();
        }
    }
}
