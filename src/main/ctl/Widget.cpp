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

        Widget::Widget(ui::IWrapper *wrapper, tk::Widget *widget)
        {
            pClass          = &metadata;
            pWrapper        = wrapper;
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

        const char *Widget::match_prefix(const char *prefix, const char *name)
        {
            // If there is no prefix, just return the name unmodified
            if ((prefix == NULL) || (name == NULL))
                return name;

            // Check that prefix matches
            size_t len = strlen(prefix);
            if (strncmp(name, prefix, len))
                return NULL;

            // Check that there is dot after prefix
            name += len;
            if (name[0] == '\0')
                return name;
            else if (name[0] == '.')
                return &name[1];

            return NULL;
        }

        bool Widget::set_font(tk::Font *f, const char *param, const char *name, const char *value)
        {
            // Does the prefix match?
            size_t len = ::strlen(param);
            if (strncmp(name, param, len))
                return false;

            name += len;

            if      (!strcmp(name, ".name"))        f->set_name(value);
            else if (!strcmp(name, ".size"))        PARSE_FLOAT(value, f->set_size(__));
            else if (!strcmp(name, ".sz"))          PARSE_FLOAT(value, f->set_size(__));
            else if (!strcmp(name, ".bold"))        PARSE_BOOL(value, f->set_bold(__));
            else if (!strcmp(name, ".b"))           PARSE_BOOL(value, f->set_bold(__));
            else if (!strcmp(name, ".italic"))      PARSE_BOOL(value, f->set_italic(__));
            else if (!strcmp(name, ".i"))           PARSE_BOOL(value, f->set_italic(__));
            else if (!strcmp(name, ".underline"))   PARSE_BOOL(value, f->set_underline(__));
            else if (!strcmp(name, ".u"))           PARSE_BOOL(value, f->set_underline(__));
            else if (!strcmp(name, ".antialiasing"))f->set_antialiasing(value);
            else if (!strcmp(name, ".antialias"))   f->set_antialiasing(value);
            else if (!strcmp(name, ".a"))           f->set_antialiasing(value);
            else return false;

            return true;
        }

        bool Widget::set_size_range(tk::SizeRange *r, const char *param, const char *name, const char *value)
        {
            if (r == NULL)
                return false;
            if (!(name = match_prefix(param, name)))
                return false;

            if (name[0] == '\0')                    PARSE_FLOAT(value, r->set(__));
            else if (!strcmp(name, "min"))          PARSE_FLOAT(value, r->set_min(__));
            else if (!strcmp(name, "max"))          PARSE_FLOAT(value, r->set_max(__));
            else return false;

            return true;
        }

        bool Widget::set_allocation(tk::Allocation *alloc, const char *name, const char *value)
        {
            if (alloc == NULL)
                return false;

            if      (!strcmp(name, "fill"))         PARSE_BOOL(value, alloc->set_fill(__));
            else if (!strcmp(name, "hfill"))        PARSE_BOOL(value, alloc->set_hfill(__));
            else if (!strcmp(name, "vfill"))        PARSE_BOOL(value, alloc->set_vfill(__));
            else if (!strcmp(name, "expand"))       PARSE_BOOL(value, alloc->set_expand(__));
            else if (!strcmp(name, "hexpand"))      PARSE_BOOL(value, alloc->set_hexpand(__));
            else if (!strcmp(name, "vexpand"))      PARSE_BOOL(value, alloc->set_vexpand(__));
            else return false;

            return true;
        }

        bool Widget::set_constraints(tk::SizeConstraints *c, const char *name, const char *value)
        {
            if (c == NULL)
                return false;

            if      (!strcmp(name, "width"))        PARSE_INT(value, c->set_width(__));
            else if (!strcmp(name, "wmin"))         PARSE_INT(value, c->set_min_width(__));
            else if (!strcmp(name, "width.min"))    PARSE_INT(value, c->set_min_width(__));
            else if (!strcmp(name, "wmax"))         PARSE_INT(value, c->set_max_width(__));
            else if (!strcmp(name, "width.max"))    PARSE_INT(value, c->set_max_width(__));
            else if (!strcmp(name, "min_width"))    PARSE_INT(value, c->set_min_width(__));
            else if (!strcmp(name, "max_width"))    PARSE_INT(value, c->set_max_width(__));
            else if (!strcmp(name, "height"))       PARSE_INT(value, c->set_height(__));
            else if (!strcmp(name, "hmin"))         PARSE_INT(value, c->set_min_height(__));
            else if (!strcmp(name, "height.min"))   PARSE_INT(value, c->set_min_height(__));
            else if (!strcmp(name, "hmax"))         PARSE_INT(value, c->set_max_height(__));
            else if (!strcmp(name, "height.max"))   PARSE_INT(value, c->set_max_height(__));
            else if (!strcmp(name, "min_height"))   PARSE_INT(value, c->set_min_height(__));
            else if (!strcmp(name, "max_height"))   PARSE_INT(value, c->set_max_height(__));
            else if (!strcmp(name, "size"))         PARSE_INT(value, c->set_all(__));
            else return false;

            return true;
        }

        bool Widget::set_layout(tk::Layout *l, const char *name, const char *value)
        {
            if      (!strcmp(name, "align"))        PARSE_FLOAT(value, l->set_align(__));
            else if (!strcmp(name, "halign"))       PARSE_FLOAT(value, l->set_halign(__));
            else if (!strcmp(name, "valign"))       PARSE_FLOAT(value, l->set_valign(__));
            else if (!strcmp(name, "scale"))        PARSE_FLOAT(value, l->set_scale(__));
            else if (!strcmp(name, "hscale"))       PARSE_FLOAT(value, l->set_hscale(__));
            else if (!strcmp(name, "vscale"))       PARSE_FLOAT(value, l->set_vscale(__));
            else return false;

            return true;
        }

        bool Widget::set_arrangement(tk::Arrangement *a, const char *param, const char *name, const char *value)
        {
            if (a == NULL)
                return false;
            if (!(name = match_prefix(param, name)))
                return false;

            if      (!strcmp(name, "align"))        PARSE_FLOAT(value, a->set(__));
            else if (!strcmp(name, "halign"))       PARSE_FLOAT(value, a->set_halign(__));
            else if (!strcmp(name, "hpos"))         PARSE_FLOAT(value, a->set_halign(__));
            else if (!strcmp(name, "valign"))       PARSE_FLOAT(value, a->set_valign(__));
            else if (!strcmp(name, "vpos"))         PARSE_FLOAT(value, a->set_valign(__));
            else return false;

            return true;
        }

        bool Widget::set_alignment(tk::Alignment *a, const char *param, const char *name, const char *value)
        {
            if (a == NULL)
                return false;
            if (!(name = match_prefix(param, name)))
                return false;

            if (!strcmp(name, "align"))             PARSE_FLOAT(value, a->set_align(__));
            else if (!strcmp(name, "scale"))        PARSE_FLOAT(value, a->set_scale(__));
            else return false;

            return true;
        }

        bool Widget::set_text_layout(tk::TextLayout *l, const char *name, const char *value)
        {
            if      (!strcmp(name, "htext"))        PARSE_FLOAT(value, l->set_halign(__));
            else if (!strcmp(name, "vtext"))        PARSE_FLOAT(value, l->set_valign(__));
            else if (!strcmp(name, "text.halign"))  PARSE_FLOAT(value, l->set_halign(__));
            else if (!strcmp(name, "text.valign"))  PARSE_FLOAT(value, l->set_valign(__));
            else return false;

            return true;
        }

        bool Widget::set_expr(ctl::Expression *expr, const char *param, const char *name, const char *value)
        {
            if (expr == NULL)
                return false;
            if (strcmp(name, param))
                return false;

            expr->parse(value);
            return true;
        }

        bool Widget::set_param(tk::Boolean *b, const char *param, const char *name, const char *value)
        {
            if (b == NULL)
                return false;
            if (strcmp(param, name))
                return false;
            PARSE_BOOL(value, b->set(__));
            return true;
        }

        bool Widget::set_param(tk::Integer *i, const char *param, const char *name, const char *value)
        {
            if (i == NULL)
                return false;
            if (strcmp(param, name))
                return false;
            PARSE_INT(value, i->set(__));
            return true;
        }

        bool Widget::set_param(tk::Float *f, const char *param, const char *name, const char *value)
        {
            if (f == NULL)
                return false;
            if (strcmp(param, name))
                return false;
            PARSE_FLOAT(value, f->set(__));
            return true;
        }

        bool Widget::set_value(bool *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;
            if (strcmp(param, name))
                return false;
            PARSE_BOOL(value, *v = __);
            return true;
        }

        bool Widget::set_value(ssize_t *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;
            if (strcmp(param, name))
                return false;
            PARSE_INT(value, *v = __);
            return true;
        }

        bool Widget::set_value(size_t *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;
            if (strcmp(param, name))
                return false;
            PARSE_UINT(value, *v = __);
            return true;
        }

        bool Widget::set_value(float *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;
            if (strcmp(param, name))
                return false;
            PARSE_FLOAT(value, *v = __);
            return true;
        }

        bool Widget::set_value(LSPString *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;
            if (strcmp(param, name))
                return false;

            v->set_utf8(value);
            return true;
        }

        bool Widget::bind_port(ui::IPort **port, const char *param, const char *name, const char *value)
        {
            if (strcmp(param, name))
                return false;
            if (port == NULL)
                return false;

            ui::IPort *oldp = *port;
            ui::IPort *newp = pWrapper->port(value);

            if (oldp != NULL)
                oldp->unbind(this);
            if (newp != NULL)
                newp->bind(this);

            *port           = newp;

            return true;
        }

        bool Widget::set_embedding(tk::Embedding *e, const char *name, const char *value)
        {
            if (e == NULL)
                return false;

            if      (!strcmp(name, "embed"))        PARSE_BOOL(value, e->set(__));
            else if (!strcmp(name, "embed.h"))      PARSE_BOOL(value, e->set_horizontal(__));
            else if (!strcmp(name, "embed.hor"))    PARSE_BOOL(value, e->set_horizontal(__));
            else if (!strcmp(name, "embed.v"))      PARSE_BOOL(value, e->set_vertical(__));
            else if (!strcmp(name, "embed.vert"))   PARSE_BOOL(value, e->set_vertical(__));
            else if (!strcmp(name, "embed.l"))      PARSE_BOOL(value, e->set_left(__));
            else if (!strcmp(name, "embed.left"))   PARSE_BOOL(value, e->set_left(__));
            else if (!strcmp(name, "embed.r"))      PARSE_BOOL(value, e->set_right(__));
            else if (!strcmp(name, "embed.right"))  PARSE_BOOL(value, e->set_right(__));
            else if (!strcmp(name, "embed.t"))      PARSE_BOOL(value, e->set_top(__));
            else if (!strcmp(name, "embed.top"))    PARSE_BOOL(value, e->set_top(__));
            else if (!strcmp(name, "embed.b"))      PARSE_BOOL(value, e->set_bottom(__));
            else if (!strcmp(name, "embed.bottom")) PARSE_BOOL(value, e->set_bottom(__));
            else return false;

            return true;
        }

        bool Widget::set_orientation(tk::Orientation *o, const char *name, const char *value)
        {
            if (!strcmp(name, "hor"))
                PARSE_BOOL(value, o->set((__) ? tk::O_HORIZONTAL : tk::O_VERTICAL));
            else if (!strcmp(name, "horizontal"))
                PARSE_BOOL(value, o->set((__) ? tk::O_HORIZONTAL : tk::O_VERTICAL));
            else if (!strcmp(name, "vert"))
                PARSE_BOOL(value, o->set((__) ? tk::O_VERTICAL : tk::O_HORIZONTAL));
            else if (!strcmp(name, "vertical"))
                PARSE_BOOL(value, o->set((__) ? tk::O_VERTICAL : tk::O_HORIZONTAL));
            else if (!strcmp(name, "orientation"))
                o->parse(value);
            else
                return false;

            return true;
        }

        void Widget::inject_style(tk::Widget *widget, const char *style_name)
        {
            tk::Style *style = widget->display()->schema()->get(style_name);
            if (style != NULL)
                widget->style()->inject_parent(style);
        }

        void Widget::add_parent_style(tk::Widget *widget, const char *style_name)
        {
            tk::Style *style = widget->display()->schema()->get(style_name);
            if (style != NULL)
                widget->style()->add_parent(style);
        }

        void Widget::revoke_style(tk::Widget *widget, const char *style_name)
        {
            tk::Style *style = widget->display()->schema()->get(style_name);
            if (style != NULL)
                widget->style()->remove_parent(style);
        }

        status_t Widget::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t Widget::init()
        {
            if (wWidget != NULL)
            {
                sBgColor.init(pWrapper, wWidget->bg_color());
                sBgInherit.init(pWrapper, wWidget->bg_inherit());
                sPadding.init(pWrapper, wWidget->padding());
                sVisibility.init(pWrapper, wWidget->visibility());
            }

            return STATUS_OK;
        }


        void Widget::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            if (wWidget == NULL)
                return;

            if (!strcmp(name, "ui:id"))
                ctx->widgets()->map(value, wWidget);
            if (!strcmp(name, "ui:style"))
                assign_styles(wWidget, value, true);
            if (!strcmp(name, "ui:inject"))
                assign_styles(wWidget, value, false);

            sVisibility.set("visibility", name, value);
            sVisibility.set("visible", name, value);

            set_param(wWidget->brightness(), "brightness", name, value);
            set_param(wWidget->brightness(), "bright", name, value);
            set_param(wWidget->scaling(), "scaling", name, value);
            set_allocation(wWidget->allocation(), name, value);

            sPadding.set("pad", name, value);
            sPadding.set("padding", name, value);
            if (sBgColor.set("bg", name, value))
                wWidget->bg_inherit()->set(false);
            if (sBgColor.set("bg.color", name, value))
                wWidget->bg_inherit()->set(false);
            sBgInherit.set("bg.inherit", name, value);
            sBgInherit.set("ibg", name, value);
        }

        bool Widget::assign_styles(tk::Widget *widget, const char *style_list, bool remove_parents)
        {
            if (widget == NULL)
                return false;

            tk::Style *s = widget->style();
            if (s == NULL)
                return false;

            LSPString cname, text;
            if (!text.set_utf8(style_list))
                return false;

            if (remove_parents)
                s->remove_all_parents();

            // Parse list of
            ssize_t first = 0, last = -1, len = text.length();

            while (true)
            {
                last = text.index_of(first, ',');
                if (last < 0)
                {
                    last = len;
                    break;
                }

                if (!cname.set(&text, first, last))
                    return false;

                add_parent_style(widget, cname.get_utf8());
                first = last + 1;
            }

            // Last token pending?
            if (last > first)
            {
                if (!cname.set(&text, first, last))
                    return false;
                add_parent_style(widget, cname.get_utf8());
            }

            return true;
        }

        void Widget::begin(ui::UIContext *ctx)
        {
        }

        void Widget::end(ui::UIContext *ctx)
        {
        }

        void Widget::notify(ui::IPort *port)
        {
        }

        void Widget::destroy()
        {
            pWrapper    = NULL;
            wWidget     = NULL;
        }

    }
} /* namespace lsp */



