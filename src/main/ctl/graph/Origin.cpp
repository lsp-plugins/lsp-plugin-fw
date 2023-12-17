/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 мая 2021 г.
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
        CTL_FACTORY_IMPL_START(Origin)
            status_t res;

            if (!name->equals_ascii("origin"))
                return STATUS_NOT_FOUND;

            tk::GraphOrigin *w = new tk::GraphOrigin(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Origin *wc  = new ctl::Origin(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Origin)

        //-----------------------------------------------------------------
        const ctl_class_t Origin::metadata      = { "Origin", &Widget::metadata };

        Origin::Origin(ui::IWrapper *wrapper, tk::GraphOrigin *widget): Widget(wrapper, widget)
        {
            pClass      = &metadata;
        }

        Origin::~Origin()
        {
        }

        status_t Origin::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphOrigin *go = tk::widget_cast<tk::GraphOrigin>(wWidget);
            if (go != NULL)
            {
                sSmooth.init(pWrapper, go->smooth());
                sLeft.init(pWrapper, this);
                sTop.init(pWrapper, this);
                sRadius.init(pWrapper, go->radius());
                sColor.init(pWrapper, go->color());
            }

            return STATUS_OK;
        }

        void Origin::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphOrigin *go = tk::widget_cast<tk::GraphOrigin>(wWidget);
            if (go != NULL)
            {
                if ((set_expr(&sLeft, "left", name, value)) ||
                    (set_expr(&sLeft, "hpos", name, value)))
                    go->left()->set(sLeft.evaluate_float());
                if ((set_expr(&sTop, "top", name, value)) ||
                    (set_expr(&sTop, "vpos", name, value)))
                    go->top()->set(sTop.evaluate_float());

                set_param(go->priority(), "priority", name, value);
                set_param(go->priority_group(), "priority_group", name, value);
                set_param(go->priority_group(), "pgroup", name, value);

                sSmooth.set("smooth", name, value);
                sRadius.set("radius", name, value);
                sColor.set("color", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Origin::trigger_expr()
        {
            tk::GraphOrigin *go = tk::widget_cast<tk::GraphOrigin>(wWidget);
            if (go == NULL)
                return;

            if (sLeft.valid())
                go->left()->set(sLeft.evaluate_float());
            if (sTop.valid())
                go->top()->set(sTop.evaluate_float());
        }

        void Origin::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            tk::GraphOrigin *go = tk::widget_cast<tk::GraphOrigin>(wWidget);
            if (go != NULL)
            {
                if (sLeft.depends(port))
                    go->left()->set(sLeft.evaluate_float());
                if (sTop.depends(port))
                    go->top()->set(sTop.evaluate_float());
            }
        }

        void Origin::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);
        }

    } /* namespace ctl */
} /* namespace lsp */
