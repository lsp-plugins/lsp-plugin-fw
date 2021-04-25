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

            tk::Box *box = new tk::Box(context->display());
            if (box == NULL)
                return STATUS_NO_MEM;
            if ((res = context->add_widget(box)) != STATUS_OK)
            {
                delete box;
                return res;
            }

            if ((res = box->init()) != STATUS_OK)
                return res;

            ctl::Box *cbox  = new ctl::Box(context->wrapper(), box, orientation);
            if (cbox == NULL)
                return STATUS_NO_MEM;

            *ctl = cbox;
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

                if (!strcmp(name, "spacing"))
                    PARSE_INT(value, box->spacing()->set(__))
                else if (!strcmp(name, "homogeneous"))
                    PARSE_BOOL(value, box->homogeneous()->set(__))
                else if (!strcmp(name, "hgen"))
                    PARSE_BOOL(value, box->homogeneous()->set(__))
                else if ((enOrientation < 0) && (!strcmp(name, "hor")))
                {
                    PARSE_BOOL(value, box->orientation()->set((__) ? tk::O_HORIZONTAL : tk::O_VERTICAL));
                    enOrientation = box->orientation()->get();
                }
                else if ((enOrientation < 0) && (!strcmp(name, "horizontal")))
                {
                    PARSE_BOOL(value, box->orientation()->set((__) ? tk::O_HORIZONTAL : tk::O_VERTICAL));
                    enOrientation = box->orientation()->get();
                }
                else if ((enOrientation < 0) && (!strcmp(name, "vert")))
                {
                    PARSE_BOOL(value, box->orientation()->set((__) ? tk::O_VERTICAL : tk::O_HORIZONTAL));
                    enOrientation = box->orientation()->get();
                }
                else if ((enOrientation < 0) && (!strcmp(name, "vertical")))
                {
                    PARSE_BOOL(value, box->orientation()->set((__) ? tk::O_VERTICAL : tk::O_HORIZONTAL));
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


