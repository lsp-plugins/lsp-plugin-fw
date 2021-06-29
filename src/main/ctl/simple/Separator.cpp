/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 21 мая 2021 г.
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
        CTL_FACTORY_IMPL_START(Separator)
            status_t res;
            ssize_t orientation = -1;

            if (name->equals_ascii("hsep"))
                orientation = tk::O_HORIZONTAL;
            else if (name->equals_ascii("vsep"))
                orientation = tk::O_VERTICAL;
            else if (!name->equals_ascii("sep"))
                return STATUS_NOT_FOUND;

            tk::Separator *w = new tk::Separator(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Separator *wc  = new ctl::Separator(context->wrapper(), w, orientation);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Separator)

        //-----------------------------------------------------------------
        const ctl_class_t Separator::metadata   = { "Separator", &Widget::metadata };

        Separator::Separator(ui::IWrapper *wrapper, tk::Separator *widget, ssize_t orientation): Widget(wrapper, widget)
        {
            nOrientation    = orientation;
        }

        Separator::~Separator()
        {
        }

        status_t Separator::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Separator *sp = tk::widget_cast<tk::Separator>(wWidget);
            if (sp != NULL)
            {
                sColor.init(pWrapper, sp->color());
                if (nOrientation >= 0)
                    sp->orientation()->set(tk::orientation_t(nOrientation));
            }

            return STATUS_OK;
        }

        void Separator::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Separator *sp = tk::widget_cast<tk::Separator>(wWidget);
            if (sp != NULL)
            {
                sColor.set("color", name, value);
                if (nOrientation < 0)
                {
                    if (set_orientation(sp->orientation(), name, value))
                        nOrientation    = sp->orientation()->get();
                }

                set_size_range(sp->size(), "size", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Separator::schema_reloaded()
        {
            Widget::schema_reloaded();

            sColor.reload();
        }
    } // namespace ctl
} // namespace lsp
