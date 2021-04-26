/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 апр. 2021 г.
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
        CTL_FACTORY_IMPL_START(Box)
            status_t res;
            ssize_t orientation = -1;

            if (name->equals_ascii("hbox"))
                orientation = tk::O_HORIZONTAL;
            else if (name->equals_ascii("vbox"))
                orientation = tk::O_VERTICAL;
            else if (!name->equals_ascii("box"))
                return STATUS_NOT_FOUND;

            tk::Box *w = new tk::Box(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->add_widget(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Box *wc  = new ctl::Box(context->wrapper(), w, orientation);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Box)

        //-----------------------------------------------------------------
        const ctl_class_t Box::metadata     = { "Box", &Widget::metadata };

        Box::Box(ui::IWrapper *src, tk::Box *widget, ssize_t orientation): Widget(src, widget)
        {
            pClass          = &metadata;
            enOrientation   = orientation;
        }

        Box::~Box()
        {
        }

        void Box::set(const char *name, const char *value)
        {
            tk::Box *box    = tk::widget_cast<tk::Box>(wWidget);
            if (box != NULL)
            {
                set_constraints(box->constraints(), name, value);
                set_param(box->spacing(), "spacing", name, value);
                set_param(box->homogeneous(), "homogeneous", name, value);
                set_param(box->homogeneous(), "hgen", name, value);

                // Set orientation
                if (enOrientation < 0)
                {
                    if (set_orientation(box->orientation(), name, value))
                        enOrientation = box->orientation()->get();
                }
            }

            Widget::set(name, value);
        }

        status_t Box::add(ctl::Widget *child)
        {
            tk::Box *box    = tk::widget_cast<tk::Box>(wWidget);
            return (box != NULL) ? box->add(child->widget()) : STATUS_BAD_STATE;
        }

    } /* namespace ctl */
} /* namespace lsp */


