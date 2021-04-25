/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 апр. 2021 г.
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
        const ctl_class_t Widget::metadata = { "Widget", NULL };

        Widget::Widget(ui::IWrapper *src, tk::Widget *widget)
        {
            pClass          = &metadata;
            pWrapper        = src;
            wWidget         = widget;
        }

        bool Widget::instance_of(const ctl_class_t *wclass) const
        {
            const ctl_class_t *wc = pClass;
            while (wc != NULL)
            {
                if (wc == wclass)
                    return true;
                wc = wc->parent;
            }

            return false;
        }

        Widget::~Widget()
        {
            destroy();
        }

        tk::Widget *Widget::widget()
        {
            return wWidget;
        };

//        void Widget::set_lc_attr(widget_attribute_t att, LSPLocalString *s, const char *name, const char *value)
//        {
//            // Get prefix
//            const char *prefix = widget_attribute(att);
//            size_t len = ::strlen(prefix);
//
//            // Prefix matches?
//            if (::strncmp(prefix, name, len) != 0)
//                return;
//
//            if (name[len] == ':') // Parameter ("prefix:")?
//                s->params()->add_cstring(&name[len+1], value);
//            else if (name[len] == '\0') // Key?
//            {
//                if (strchr(value, '.') == NULL) // Raw value with high probability?
//                    s->set_raw(value);
//                else
//                    s->set_key(value);
//            }
//        }

        bool Widget::set_padding(tk::Padding *pad, const char *name, const char *value)
        {
            if (pad == NULL)
                return false;

            if      (!strcmp(name, "pad"))          PARSE_UINT(value, pad->set_all(__))
            else if (!strcmp(name, "padding"))      PARSE_UINT(value, pad->set_all(__))
            else if (!strcmp(name, "hpad"))         PARSE_UINT(value, pad->set_horizontal(__))
            else if (!strcmp(name, "vpad"))         PARSE_UINT(value, pad->set_vertical(__))
            else if (!strcmp(name, "lpad"))         PARSE_UINT(value, pad->set_left(__))
            else if (!strcmp(name, "pad_left"))     PARSE_UINT(value, pad->set_left(__))
            else if (!strcmp(name, "rpad"))         PARSE_UINT(value, pad->set_right(__))
            else if (!strcmp(name, "pad_right"))    PARSE_UINT(value, pad->set_right(__))
            else if (!strcmp(name, "tpad"))         PARSE_UINT(value, pad->set_top(__))
            else if (!strcmp(name, "pad_top"))      PARSE_UINT(value, pad->set_top(__))
            else if (!strcmp(name, "bpad"))         PARSE_UINT(value, pad->set_bottom(__))
            else if (!strcmp(name, "pad_bottom"))   PARSE_UINT(value, pad->set_bottom(__))
            else return false;

            return true;
        }

        bool Widget::set_allocation(tk::Allocation *alloc, const char *name, const char *value)
        {
            if (alloc == NULL)
                return false;

            if      (!strcmp(name, "fill"))         PARSE_BOOL(value, alloc->set_fill(__))
            else if (!strcmp(name, "hfill"))        PARSE_BOOL(value, alloc->set_hfill(__))
            else if (!strcmp(name, "vfill"))        PARSE_BOOL(value, alloc->set_vfill(__))
            else if (!strcmp(name, "expand"))       PARSE_BOOL(value, alloc->set_expand(__))
            else if (!strcmp(name, "hexpand"))      PARSE_BOOL(value, alloc->set_hexpand(__))
            else if (!strcmp(name, "vexpand"))      PARSE_BOOL(value, alloc->set_vexpand(__))
            else return false;

            return true;
        }

        bool Widget::set_constraints(tk::SizeConstraints *c, const char *name, const char *value)
        {
            if (c == NULL)
                return false;

            if      (!strcmp(name, "width"))        PARSE_INT(value, c->set_width(__))
            else if (!strcmp(name, "wmin"))         PARSE_INT(value, c->set_min_width(__))
            else if (!strcmp(name, "wmax"))         PARSE_INT(value, c->set_max_width(__))
            else if (!strcmp(name, "min_width"))    PARSE_INT(value, c->set_min_width(__))
            else if (!strcmp(name, "max_width"))    PARSE_INT(value, c->set_max_width(__))
            else if (!strcmp(name, "height"))       PARSE_INT(value, c->set_height(__))
            else if (!strcmp(name, "hmin"))         PARSE_INT(value, c->set_min_height(__))
            else if (!strcmp(name, "hmax"))         PARSE_INT(value, c->set_max_height(__))
            else if (!strcmp(name, "min_height"))   PARSE_INT(value, c->set_min_height(__))
            else if (!strcmp(name, "max_height"))   PARSE_INT(value, c->set_max_height(__))
            else return false;

            return true;
        }

        void Widget::set(const char *name, const char *value)
        {
            if (wWidget == NULL)
                return;

            if (!strcmp(name, "visibility"))
                BIND_EXPR(sVisibility, value)
            else if (!strcmp(name, "visible"))
                BIND_EXPR(sVisibility, value)
            else if (!strcmp(name, "brightness"))
                sBright.parse(value);
            else if (!strcmp(name, "bright"))
                sBright.parse(value);
            else if (!strcmp(name, "ui:id"))
                pWrapper->ui()->map_widget(value, wWidget);

            set_padding(wWidget->padding(), name, value);
            set_allocation(wWidget->allocation(), name, value);
            sBgColor.set(name, value);
        }

        status_t Widget::add(ctl::Widget *child)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t Widget::init()
        {
            sVisibility.init(pWrapper, this);
            sBright.init(pWrapper, this);
            if (wWidget != NULL)
                sBgColor.init(pWrapper, wWidget->bg_color(), "bg");

            return STATUS_OK;
        }

        void Widget::begin()
        {
        }

        void Widget::end()
        {
            // Evaluate expression
            if (sVisibility.valid())
            {
                float value = sVisibility.evaluate();
                if (wWidget != NULL)
                    wWidget->visibility()->set(value >= 0.5f);
            }

            // Evaluate brightness
            if (sBright.valid())
            {
                float value = sBright.evaluate();
                wWidget->brightness()->set(value);
            }
        }

        void Widget::notify(ui::IPort *port)
        {
            if (wWidget == NULL)
                return;

            // Visibility
            if (sVisibility.depends(port))
            {
                float value = sVisibility.evaluate();
                wWidget->visibility()->set(value >= 0.5f);
            }

            // Brightness
            if (sBright.depends(port))
            {
                float value = sBright.evaluate();
                wWidget->brightness()->set(value);
            }
        }

        void Widget::destroy()
        {
            if ((pWrapper != NULL) && (wWidget != NULL))
                pWrapper->ui()->unmap_widget(wWidget);

            sVisibility.destroy();
            sBright.destroy();

            pWrapper    = NULL;
            wWidget     = NULL;
        }

    }
} /* namespace lsp */



