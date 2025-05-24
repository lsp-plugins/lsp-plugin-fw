/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        void Widget::PropListener::notify(tk::Property *prop)
        {
            if (pWidget != NULL)
                pWidget->property_changed(prop);
        }

        //---------------------------------------------------------------------
        const ctl_class_t Widget::metadata = { "Widget", NULL };

        Widget::Widget(ui::IWrapper *wrapper, tk::Widget *widget):
            ctl::DOMController(wrapper),
            ui::IPortListener(),
            sProperties(this)
        {
            pClass          = &metadata;

            wWidget         = widget;
        }

        Widget::~Widget()
        {
            do_destroy();
        }

        void Widget::destroy()
        {
            do_destroy();
            ctl::DOMController::destroy();
        }

        void Widget::do_destroy()
        {
            if (pWrapper != NULL)
                pWrapper->remove_schema_listener(this);
            sProperties.unbind();

            pWrapper    = NULL;
            wWidget     = NULL;
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

        bool Widget::set_text_fitness(tk::TextFitness *f, const char *param, const char *name, const char *value)
        {
            if (f == NULL)
                return false;
            if (!(name = match_prefix(param, name)))
                return false;

            if (name[0] == '\0')                    PARSE_FLOAT(value, f->set(__));
            else if (!strcmp(name, "hfit"))         PARSE_FLOAT(value, f->set_hfit(__));
            else if (!strcmp(name, "h"))            PARSE_FLOAT(value, f->set_hfit(__));
            else if (!strcmp(name, "vfit"))         PARSE_FLOAT(value, f->set_vfit(__));
            else if (!strcmp(name, "v"))            PARSE_FLOAT(value, f->set_vfit(__));
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
            else if (!strcmp(name, "reduce"))       PARSE_BOOL(value, alloc->set_reduce(__));
            else if (!strcmp(name, "hreduce"))      PARSE_BOOL(value, alloc->set_hreduce(__));
            else if (!strcmp(name, "vreduce"))      PARSE_BOOL(value, alloc->set_vreduce(__));
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
            else if (!strcmp(name, "size.min"))     PARSE_INT(value, c->set_min(__));
            else if (!strcmp(name, "size.max"))     PARSE_INT(value, c->set_max(__));
            else return false;

            return true;
        }

        bool Widget::set_layout(tk::Layout *l, const char *param, const char *name, const char *value)
        {
            if (l == NULL)
                return false;
            if (!(name = match_prefix(param, name)))
                return false;

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
            if (l == NULL)
                return false;

            if      (!strcmp(name, "htext"))        PARSE_FLOAT(value, l->set_halign(__));
            else if (!strcmp(name, "text.halign"))  PARSE_FLOAT(value, l->set_halign(__));
            else if (!strcmp(name, "text.h"))       PARSE_FLOAT(value, l->set_halign(__));
            else if (!strcmp(name, "vtext"))        PARSE_FLOAT(value, l->set_valign(__));
            else if (!strcmp(name, "text.valign"))  PARSE_FLOAT(value, l->set_valign(__));
            else if (!strcmp(name, "text.v"))       PARSE_FLOAT(value, l->set_valign(__));
            else return false;

            return true;
        }

        bool Widget::set_text_layout(tk::TextLayout *l, const char *param, const char *name, const char *value)
        {
            if (l == NULL)
                return false;
            if (!(name = match_prefix(param, name)))
                return false;

            if      (!strcmp(name, "htext"))        PARSE_FLOAT(value, l->set_halign(__));
            else if (!strcmp(name, "halign"))       PARSE_FLOAT(value, l->set_halign(__));
            else if (!strcmp(name, "h"))            PARSE_FLOAT(value, l->set_halign(__));
            else if (!strcmp(name, "vtext"))        PARSE_FLOAT(value, l->set_valign(__));
            else if (!strcmp(name, "valign"))       PARSE_FLOAT(value, l->set_valign(__));
            else if (!strcmp(name, "v"))            PARSE_FLOAT(value, l->set_valign(__));
            else return false;

            return true;
        }

        bool Widget::set_expr(ctl::Expression *expr, const char *param, const char *name, const char *value)
        {
            if (expr == NULL)
                return false;
            if (strcmp(name, param))
                return false;

            if (!expr->parse(value))
                lsp_warn("Failed to parse expression for attribute '%s': %s", name, value);
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

        bool Widget::set_param(tk::Enum *e, const char *param, const char *name, const char *value)
        {
            if (e == NULL)
                return false;
            return e->parse(value) == STATUS_OK;
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

        bool Widget::link_port(ui::IPort **port, const char *id)
        {
            ui::IPort *oldp = *port;
            ui::IPort *newp = pWrapper->port(id);
            if (oldp == newp)
                return true;

            if (oldp != NULL)
                oldp->unbind(this);
            if (newp != NULL)
                newp->bind(this);

            *port           = newp;

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
            if (oldp == newp)
                return true;

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

        status_t Widget::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t Widget::init()
        {
            status_t res = ctl::DOMController::init();
            if (res != STATUS_OK)
                return res;

            pWrapper->add_schema_listener(this);

            if (wWidget != NULL)
            {
                sActivity.init(pWrapper, wWidget->active());
                sBgColor.init(pWrapper, wWidget->bg_color());
                sInactiveBgColor.init(pWrapper, wWidget->inactive_bg_color());
                sBgInherit.init(pWrapper, wWidget->bg_inherit());
                sPadding.init(pWrapper, wWidget->padding());
                sVisibility.init(pWrapper, wWidget->visibility());
                sBrightness.init(pWrapper, wWidget->brightness());
                sInactiveBrightness.init(pWrapper, wWidget->inactive_brightness());
                sBgBrightness.init(pWrapper, wWidget->bg_brightness());
                sInactiveBgBrightness.init(pWrapper, wWidget->inactive_bg_brightness());
                sPointer.init(pWrapper, wWidget->pointer());
            }

            return STATUS_OK;
        }


        void Widget::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            ctl::DOMController::set(ctx, name, value);

            if (wWidget != NULL)
            {
                set_param(wWidget->scaling(), "scaling", name, value);
                set_param(wWidget->font_scaling(), "font.scaling", name, value);
                set_param(wWidget->tag(), "ui:tag", name, value);
                set_allocation(wWidget->allocation(), name, value);

                if (!strcmp(name, "ui:id"))
                    ctx->widgets()->map(value, wWidget);
                if (!strcmp(name, "ui:group"))
                    ctx->widgets()->map_group(value, wWidget);

                if (!strcmp(name, "ui:style"))
                    assign_styles(wWidget, value, true);
                if (!strcmp(name, "ui:inject"))
                    assign_styles(wWidget, value, false);
            }

            sActivity.set("activity", name, value);
            sVisibility.set("visibility", name, value);
            sVisibility.set("visible", name, value);
            sBrightness.set("brightness", name, value);
            sBrightness.set("bright", name, value);
            sInactiveBrightness.set("inactive.brightness", name, value);
            sInactiveBrightness.set("inactive.bright", name, value);
            sBgBrightness.set("bg.brightness", name, value);
            sBgBrightness.set("bg.bright", name, value);
            sInactiveBgBrightness.set("inactive.bg.brightness", name, value);
            sInactiveBgBrightness.set("inactive.bg.bright", name, value);
            sPointer.set("pointer", name, value);

            sPadding.set("pad", name, value);
            sPadding.set("padding", name, value);
            if (sBgColor.set("bg", name, value))
            {
                if (wWidget != NULL)
                    wWidget->bg_inherit()->set(false);
            }
            if (sBgColor.set("bg.color", name, value))
            {
                if (wWidget != NULL)
                    wWidget->bg_inherit()->set(false);
            }
            sInactiveBgColor.set("inactive.bg", name, value);
            sInactiveBgColor.set("inactive.bg.color", name, value);
            sBgInherit.set("bg.inherit", name, value);
            sBgInherit.set("ibg", name, value);
        }

        void Widget::begin(ui::UIContext *ctx)
        {
        }

        void Widget::end(ui::UIContext *ctx)
        {
        }

        void Widget::notify(ui::IPort *port, size_t flags)
        {
        }

        void Widget::reloaded(const tk::StyleSheet *sheet)
        {
        }

        void Widget::property_changed(tk::Property *prop)
        {
        }

    } /* namespace ctl */
} /* namespace lsp */



