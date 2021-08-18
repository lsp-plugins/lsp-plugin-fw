/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 авг. 2021 г.
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
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Axis)
            status_t res;

            if (!name->equals_ascii("dot"))
                return STATUS_NOT_FOUND;

            tk::GraphDot *w = new tk::GraphDot(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Dot *wc    = new ctl::Dot(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Axis)

        //-----------------------------------------------------------------
        const ctl_class_t Dot::metadata        = { "Dot", &Widget::metadata };

        Dot::Dot(ui::IWrapper *wrapper, tk::GraphDot *widget): Widget(wrapper, widget)
        {
            pClass      = &metadata;

            init_param(&sX);
            init_param(&sY);
            init_param(&sZ);
        }

        Dot::~Dot()
        {
        }

        void Dot::init_param(param_t *p)
        {
            p->nFlags       = 0;
            p->fMin         = 0;
            p->fMax         = 0;
            p->fDefault     = 0;
            p->fStep        = 0.0f;
            p->fAStep       = 10.0f;
            p->fDStep       = 0.1f;

            p->pPort        = NULL;
        }

        status_t Dot::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphDot *gd = tk::widget_cast<tk::GraphDot>(wWidget);
            if (gd != NULL)
            {
                sX.sEditable.init(pWrapper, gd->heditable());
                sY.sEditable.init(pWrapper, gd->veditable());
                sZ.sEditable.init(pWrapper, gd->zeditable());

                sSize.init(pWrapper, gd->size());
                sHoverSize.init(pWrapper, gd->hover_size());
                sBorderSize.init(pWrapper, gd->border_size());
                sHoverBorderSize.init(pWrapper, gd->hover_border_size());
                sGap.init(pWrapper, gd->gap());
                sHoverGap.init(pWrapper, gd->hover_gap());

                sColor.init(pWrapper, gd->color());
                sHoverColor.init(pWrapper, gd->hover_color());
                sBorderColor.init(pWrapper, gd->border_color());
                sHoverBorderColor.init(pWrapper, gd->hover_border_color());
                sGapColor.init(pWrapper, gd->gap_color());
                sHoverGapColor.init(pWrapper, gd->hover_gap_color());

                // Bind slots
                gd->slots()->bind(tk::SLOT_SUBMIT, slot_change, this);
            }

            return STATUS_OK;
        }

        void Dot::set_param(param_t *p, const char *prefix, const char *name, const char *value)
        {
            char s[0x80]; // Should be enough

            snprintf(s, sizeof(s), "%s.id", prefix);
            bind_port(&p->pPort, s, name, value);

            snprintf(s, sizeof(s), "%s.value", prefix);
            set_expr(&p->sExpr, s, name, value);

            snprintf(s, sizeof(s), "%s.editable", prefix);
            p->sEditable.set(s, name, value);

            snprintf(s, sizeof(s), "%s.min", prefix);
            if (set_value(&p->fMin, s, name, value))
                p->nFlags      |= DF_MIN;
            snprintf(s, sizeof(s), "%s.max", prefix);
            if (set_value(&p->fMax, s, name, value))
                p->nFlags      |= DF_MAX;
            snprintf(s, sizeof(s), "%s.default", prefix);
            if (set_value(&p->fDefault, s, name, value))
                p->nFlags      |= DF_DFL;
            snprintf(s, sizeof(s), "%s.dfl", prefix);
            if (set_value(&p->fDefault, s, name, value))
                p->nFlags      |= DF_DFL;

            bool log = false;
            snprintf(s, sizeof(s), "%s.log", prefix);
            if (set_value(&log, s, name, value))
                p->nFlags       = lsp_setflag(p->nFlags, DF_LOG, log) | DF_LOG_SET;
            snprintf(s, sizeof(s), "%s.logarithmic", prefix);
            if (set_value(&log, s, name, value))
                p->nFlags       = lsp_setflag(p->nFlags, DF_LOG, log) | DF_LOG_SET;

            snprintf(s, sizeof(s), "%s.step", prefix);
            if (set_value(&p->fStep, s, name, value))
                p->nFlags      |= DF_STEP;
            snprintf(s, sizeof(s), "%s.astep", prefix);
            if (set_value(&p->fAStep, s, name, value))
                p->nFlags      |= DF_ASTEP;
            snprintf(s, sizeof(s), "%s.dstep", prefix);
            if (set_value(&p->fDStep, s, name, value))
                p->nFlags      |= DF_DSTEP;
        }

        void Dot::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphDot *gd = tk::widget_cast<tk::GraphDot>(wWidget);
            if (gd != NULL)
            {
                set_param(&sX, "hor", name, value);
                set_param(&sX, "h", name, value);
                set_param(&sX, "x", name, value);

                set_param(&sY, "vert", name, value);
                set_param(&sY, "v", name, value);
                set_param(&sY, "y", name, value);

                set_param(&sZ, "scroll", name, value);
                set_param(&sZ, "s", name, value);
                set_param(&sZ, "z", name, value);

                sSize.set("size", name, value);
                sHoverSize.set("hover.size", name, value);
                sBorderSize.set("border.size", name, value);
                sBorderSize.set("bsize", name, value);
                sHoverBorderSize.set("hover.border.size", name, value);
                sHoverBorderSize.set("hover.bsize", name, value);
                sGap.set("gap.size", name, value);
                sGap.set("gsize", name, value);
                sHoverGap.set("hover.gap.size", name, value);
                sHoverGap.set("hover.gsize", name, value);

                sColor.set("color", name, value);
                sHoverColor.set("hover.color", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sHoverBorderColor.set("hover.border.color", name, value);
                sHoverBorderColor.set("hover.bcolor", name, value);
                sGapColor.set("gap.color", name, value);
                sGapColor.set("gcolor", name, value);
                sHoverGapColor.set("hover.gap.color", name, value);
                sHoverGapColor.set("hover.gcolor", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Dot::notify(ui::IPort *port)
        {
            Widget::notify(port);
        }

        void Dot::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            configure_param(&sX, false);
            configure_param(&sY, false);
            configure_param(&sZ, true);

            commit_value(&sX, sX.pPort);
            commit_value(&sY, sY.pPort);
            commit_value(&sZ, sZ.pPort);
        }

        void Dot::schema_reloaded()
        {
            Widget::schema_reloaded();

            sColor.reload();
            sHoverColor.reload();
            sBorderColor.reload();
            sHoverBorderColor.reload();
            sGapColor.reload();
            sHoverGapColor.reload();
        }

        status_t Dot::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            Dot *_this          = static_cast<Dot *>(ptr);
            if (_this != NULL)
                _this->submit_values();
            return STATUS_OK;
        }

        void Dot::submit_values()
        {
            tk::GraphDot *gd = tk::widget_cast<tk::GraphDot>(wWidget);
            if (gd == NULL)
                return;

            submit_value(&sX, gd->hvalue()->get());
            submit_value(&sY, gd->vvalue()->get());
            submit_value(&sZ, gd->zvalue()->get());
        }

        void Dot::commit_value(param_t *p, ui::IPort *port)
        {
            float value;

            if ((p->pPort != NULL) && (p->pPort == port))
                value       = p->pPort->value();
            else if (p->sExpr.depends(port))
                value       = p->sExpr.evaluate();
            else
                return;

            // TODO: deploy value to widget

        }

        void Dot::submit_value(param_t *p, float value)
        {
            if (!p->sEditable.value())
                return;
            if (p->pPort == NULL)
                return;

            // TODO: apply value from widget
        }

        void Dot::configure_param(param_t *p, bool allow_log)
        {
            tk::GraphDot *gd = tk::widget_cast<tk::GraphDot>(wWidget);
            if (gd == NULL)
                return;

            p->nFlags   = lsp_setflag(p->nFlags, DF_LOG_ALLOWED, allow_log);

//            gd->hvalue();
//            gd->hstep();

            // TODO: add other parameters configuration
        }

    } // namespace ctl
} // namespace lsp


