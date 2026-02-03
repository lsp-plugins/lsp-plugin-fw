/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 3 февр. 2026 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(ListBox)
            status_t res;

            if ((!name->equals_ascii("sarea")) || (!name->equals_ascii("scrollarea")))
                return STATUS_NOT_FOUND;

            tk::ScrollArea *w = new tk::ScrollArea(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::ScrollArea *wc     = new ctl::ScrollArea(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(ListBox)

        //-----------------------------------------------------------------
        const ctl_class_t ScrollArea::metadata      = { "ScrollArea", &Widget::metadata };

        ScrollArea::ScrollArea(ui::IWrapper *wrapper, tk::ScrollArea *widget): Widget(wrapper, widget)
        {
            pClass  = &metadata;
        }

        ScrollArea::~ScrollArea()
        {
        }

        status_t ScrollArea::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::ScrollArea * const sa = tk::widget_cast<tk::ScrollArea>(wWidget);
            if (sa != NULL)
            {
                sHScroll.init(pWrapper, sa->hscroll_mode());
                sVScroll.init(pWrapper, sa->vscroll_mode());
                // TODO
            }

            return STATUS_OK;
        }

        void ScrollArea::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::ScrollArea * const sa = tk::widget_cast<tk::ScrollArea>(wWidget);
            if (sa != NULL)
            {
                sHScroll.set(name, "hscroll", value);
                sVScroll.set(name, "vscroll", value);

                set_constraints(sa->constraints(), name, value);
            }

            return Widget::set(ctx, name, value);
        }

        status_t ScrollArea::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::ScrollArea *alg = tk::widget_cast<tk::ScrollArea>(wWidget);
            return (alg != NULL) ? alg->add(child->widget()) : STATUS_BAD_STATE;
        }

        void ScrollArea::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
        }

    } /* namespace ctl */
} /* namespace lsp */


