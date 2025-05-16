/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 мая 2021 г.
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
        CTL_FACTORY_IMPL_START(Align)
            status_t res;

            if (!name->equals_ascii("align"))
                return STATUS_NOT_FOUND;

            tk::Align *w = new tk::Align(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Align *wc  = new ctl::Align(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Align)

        //-----------------------------------------------------------------
        const ctl_class_t Align::metadata   = { "Align", &Widget::metadata };

        Align::Align(ui::IWrapper *wrapper, tk::Align *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        Align::~Align()
        {
        }

        status_t Align::init()
        {
            LSP_STATUS_ASSERT(Widget::init());
            tk::Align *alg = tk::widget_cast<tk::Align>(wWidget);
            if (alg != NULL)
            {
                sHAlign.init(pWrapper, this);
                sVAlign.init(pWrapper, this);
                sHScale.init(pWrapper, this);
                sVScale.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void Align::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Align *alg = tk::widget_cast<tk::Align>(wWidget);
            if (alg != NULL)
            {
                set_expr(&sHAlign, "align", name, value);
                set_expr(&sVAlign, "align", name, value);
                set_expr(&sHAlign, "halign", name, value);
                set_expr(&sVAlign, "valign", name, value);

                set_expr(&sHScale, "scale", name, value);
                set_expr(&sVScale, "scale", name, value);
                set_expr(&sHScale, "hscale", name, value);
                set_expr(&sVScale, "vscale", name, value);

                set_constraints(alg->constraints(), name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Align::update_alignment()
        {
            tk::Align *alg = tk::widget_cast<tk::Align>(wWidget);
            if (alg == NULL)
                return;

            if (sHAlign.valid())
                alg->layout()->set_halign(sHAlign.evaluate());
            if (sVAlign.valid())
                alg->layout()->set_valign(sVAlign.evaluate());
            if (sHScale.valid())
                alg->layout()->set_hscale(sHScale.evaluate());
            if (sVScale.valid())
                alg->layout()->set_vscale(sVScale.evaluate());
        }

        status_t Align::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::Align *alg = tk::widget_cast<tk::Align>(wWidget);
            return (alg != NULL) ? alg->add(child->widget()) : STATUS_BAD_STATE;
        }

        void Align::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            if ((sHAlign.depends(port)) ||
                (sVAlign.depends(port)) ||
                (sHScale.depends(port)) ||
                (sVScale.depends(port)))
                update_alignment();
        }

        void Align::end(ui::UIContext *ctx)
        {
            update_alignment();
            Widget::end(ctx);
        }

        void Align::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);
            update_alignment();
        }

    } /* namespace ctl */
} /* namespace lsp */


