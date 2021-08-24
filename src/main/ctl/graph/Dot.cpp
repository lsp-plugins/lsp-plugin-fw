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
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Dot)
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
        CTL_FACTORY_IMPL_END(Dot)

        //-----------------------------------------------------------------
        const ctl_class_t Dot::metadata        = { "Dot", &Widget::metadata };

        Dot::Dot(ui::IWrapper *wrapper, tk::GraphDot *widget): Widget(wrapper, widget)
        {
            pClass      = &metadata;

            init_param(&sX, widget->hvalue(), widget->hstep());
            init_param(&sY, widget->vvalue(), widget->vstep());
            init_param(&sZ, widget->zvalue(), widget->zstep());
        }

        Dot::~Dot()
        {
        }

        void Dot::init_param(param_t *p, tk::RangeFloat *value, tk::StepFloat *step)
        {
            p->nFlags       = 0;
            p->fMin         = 0.0f;
            p->fMax         = 1.0f;
            p->fStep        = 0.0f;
            p->fAStep       = 10.0f;
            p->fDStep       = 0.1f;

            p->pPort        = NULL;
            p->pValue       = value;
            p->pStep        = step;
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
                gd->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
                gd->slots()->bind(tk::SLOT_MOUSE_DBL_CLICK, slot_dbl_click, this);
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
            snprintf(s, sizeof(s), "%s", prefix);
            set_expr(&p->sExpr, s, name, value);

            snprintf(s, sizeof(s), "%s.editable", prefix);
            p->sEditable.set(s, name, value);

            snprintf(s, sizeof(s), "%s.min", prefix);
            if (set_value(&p->fMin, s, name, value))
                p->nFlags      |= DF_MIN;
            snprintf(s, sizeof(s), "%s.max", prefix);
            if (set_value(&p->fMax, s, name, value))
                p->nFlags      |= DF_MAX;

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

            commit_value(&sX, port, false);
            commit_value(&sY, port, false);
            commit_value(&sZ, port, false);
        }

        void Dot::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            configure_param(&sX, true);
            configure_param(&sY, true);
            configure_param(&sZ, false);

            commit_value(&sX, sX.pPort, true);
            commit_value(&sY, sY.pPort, true);
            commit_value(&sZ, sZ.pPort, true);
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

        void Dot::submit_values()
        {
            tk::GraphDot *gd = tk::widget_cast<tk::GraphDot>(wWidget);
            if (gd == NULL)
                return;

            submit_value(&sX, gd->hvalue()->get());
            submit_value(&sY, gd->vvalue()->get());
            submit_value(&sZ, gd->zvalue()->get());
        }

        void Dot::submit_default_values()
        {
            tk::GraphDot *gd = tk::widget_cast<tk::GraphDot>(wWidget);
            if (gd == NULL)
                return;

            submit_value(&sX, sX.fDefault);
            submit_value(&sY, sY.fDefault);
            submit_value(&sZ, sZ.fDefault);
        }

        void Dot::commit_value(param_t *p, ui::IPort *port, bool force)
        {
            float value;

            if ((p->pPort != NULL) && (p->pPort == port))
                value       = p->pPort->value();
            else if ((p->sExpr.depends(port)) || (force))
                value       = p->sExpr.evaluate();
            else
                return;

            const meta::port_t *x = (p->pPort != NULL) ? p->pPort->metadata() : NULL;
            if (x == NULL)
            {
                if (!(p->nFlags & DF_MIN))
                    p->pValue->set_min(value);
                if (!(p->nFlags & DF_MAX))
                    p->pValue->set_max(value);
                p->pValue->set(value);
                return;
            }
            else if (p->nFlags & DF_AXIS)
            {
                p->pValue->set(value);
                return;
            }

            // Advanced setup
            if (meta::is_gain_unit(x->unit)) // Decibels
            {
                double base = (x->unit == meta::U_GAIN_AMP) ? 20.0 / M_LN10 : 10.0 / M_LN10;

                if (value < GAIN_AMP_M_120_DB)
                    value           = GAIN_AMP_M_120_DB;

                p->pValue->set(base * log(value));
            }
            else if (meta::is_discrete_unit(x->unit)) // Integer type
            {
                float ov    = truncf(p->pValue->get());
                float nv    = truncf(value);
                if (ov != nv)
                    p->pValue->set(nv);
            }
            else if (p->nFlags & DF_LOG)
            {
                if (value < GAIN_AMP_M_120_DB)
                    value           = GAIN_AMP_M_120_DB;
                p->pValue->set(log(value));
            }
            else
                p->pValue->set(value);
        }

        void Dot::submit_value(param_t *p, float value)
        {
            if (!p->sEditable.value())
                return;
            if (p->pPort == NULL)
                return;

            const meta::port_t *x = (p->pPort != NULL) ? p->pPort->metadata() : NULL;
            if (x == NULL)
            {
                if (p->pPort != NULL)
                {
                    p->pPort->set_value(value);
                    p->pPort->notify_all();
                }
                return;
            }

            if (!(p->nFlags & DF_AXIS))
            {
                if (meta::is_gain_unit(x->unit)) // Gain
                {
                    float base     = (x->unit == meta::U_GAIN_AMP) ? M_LN10 * 0.05 : M_LN10 * 0.1;
                    value           = exp(value * base);
                    float min       = (x->flags & meta::F_LOWER) ? x->min : 0.0f;
                    float thresh    = ((x->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);
                    if ((min <= 0.0f) && (value < logf(thresh)))
                        value           = 0.0f;
                }
                else if (meta::is_discrete_unit(x->unit)) // Integer type
                {
                    value          = truncf(value);
                }
                else if (p->nFlags & DF_LOG)  // Float and other values, logarithmic
                {
                    value           = exp(value);
                    float min       = (x->flags & meta::F_LOWER) ? x->min : 0.0f;
                    float thresh    = ((x->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);
                    if ((min <= 0.0f) && (value < logf(thresh)))
                        value           = 0.0f;
                }
            }

            p->pPort->set_value(value);
            p->pPort->notify_all();
        }

        void Dot::configure_param(param_t *p, bool axis)
        {
            tk::GraphDot *gd = tk::widget_cast<tk::GraphDot>(wWidget);
            if (gd == NULL)
                return;

            p->nFlags   = lsp_setflag(p->nFlags, DF_AXIS, axis);

            meta::port_t xp;

            xp.id           = NULL;
            xp.name         = NULL;
            xp.unit         = meta::U_NONE;
            xp.role         = meta::R_CONTROL;
            xp.flags        = meta::F_OUT | meta::F_LOWER | meta::F_UPPER | meta::F_STEP;
            xp.min          = 0.0f;
            xp.max          = 1.0f;
            xp.start        = 0.0f;
            xp.step         = 0.01f;
            xp.items        = NULL;
            xp.members      = NULL;

            const meta::port_t *x = (p->pPort != NULL) ? p->pPort->metadata() : NULL;
            if (x != NULL)
                xp              = *x;
            x               = &xp;

            if (p->nFlags & DF_MIN)
            {
                xp.min          = p->fMin;
                xp.flags       |= meta::F_LOWER;
            }
            if (p->nFlags & DF_MAX)
            {
                xp.max          = p->fMax;
                xp.flags       |= meta::F_UPPER;
            }
            if (p->nFlags & DF_STEP)
            {
                xp.step         = p->fStep;
                xp.flags       |= meta::F_STEP;
            }

            if (p->nFlags & DF_LOG_SET)
                xp.flags        = lsp_setflag(p->nFlags, meta::F_LOG, p->nFlags & DF_LOG);

            float min = 0.0f, max = 1.0f, step = 0.01f;

            if (!(p->nFlags & DF_AXIS))
            {
                if (meta::is_gain_unit(x->unit)) // Gain
                {
                    float base      = (x->unit == meta::U_GAIN_AMP) ? 20.0 / M_LN10 : 10.0 / M_LN10;

                    min             = (x->flags & meta::F_LOWER) ? x->min : 0.0f;
                    max             = (x->flags & meta::F_UPPER) ? x->max : GAIN_AMP_P_12_DB;

                    step            = base * log((x->flags & meta::F_STEP) ? x->step + 1.0f : 1.01f) * 0.1f;
                    float thresh    = ((x->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);

                    min             = (fabs(min) < thresh) ? (base * log(thresh) - step) : (base * log(min));
                    max             = (fabs(max) < thresh) ? (base * log(thresh) - step) : (base * log(max));

                    step           *= 10.0f;
                    p->fDefault     = base * log(x->start);
                }
                else if (meta::is_discrete_unit(x->unit)) // Integer type
                {
                    min             = (x->flags & meta::F_LOWER) ? x->min : 0.0f;
                    max             = (x->unit == meta::U_ENUM) ? min + meta::list_size(x->items) - 1.0f :
                                      (x->flags & meta::F_UPPER) ? x->max : 1.0f;
                    ssize_t istep   = (x->flags & meta::F_STEP) ? x->step : 1;

                    step            = (istep == 0) ? 1.0f : istep;
                    p->fDefault     = x->start;
                }
                else if (meta::is_log_rule(x))  // Float and other values, logarithmic
                {
                    float xmin      = (x->flags & meta::F_LOWER) ? x->min : 0.0f;
                    float xmax      = (x->flags & meta::F_UPPER) ? x->max : GAIN_AMP_P_12_DB;
                    float thresh    = ((x->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);

                    step            = log((x->flags & meta::F_STEP) ? x->step + 1.0f : 1.01f);
                    min             = (fabs(xmin) < thresh) ? log(thresh) - step : log(xmin);
                    max             = (fabs(xmax) < thresh) ? log(thresh) - step : log(xmax);

                    step           *= 10.0f;
                    p->fDefault     = log(x->start);
                }
                else // Float and other values, non-logarithmic
                {
                    min             = (x->flags & meta::F_LOWER) ? x->min : 0.0f;
                    max             = (x->flags & meta::F_UPPER) ? x->max : 1.0f;

                    step            = (x->flags & meta::F_STEP) ? x->step * 10.0f : (max - min) * 0.1f;
                    p->fDefault     = x->start;
                }
            }
            else
            {
                min             = (x->flags & meta::F_LOWER) ? x->min : 0.0f;
                max             = (x->flags & meta::F_UPPER) ? x->max : 1.0f;

                step            = (x->flags & meta::F_STEP) ? x->step * 10.0f : (max - min) * 0.1f;
                p->fDefault     = x->start;
            }

            // Initialize parameters
            p->pValue->set_all(p->fDefault, min, max);
            p->pStep->set((p->nFlags & DF_AXIS) ? 1.0f : step);

            if (p->nFlags & DF_ASTEP)
                p->pStep->set_accel(p->fAStep);
            if (p->nFlags & DF_DSTEP)
                p->pStep->set_decel(p->fDStep);
        }

        status_t Dot::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            Dot *_this          = static_cast<Dot *>(ptr);
            if (_this != NULL)
                _this->submit_values();
            return STATUS_OK;
        }

        status_t Dot::slot_dbl_click(tk::Widget *sender, void *ptr, void *data)
        {
            Dot *_this          = static_cast<Dot *>(ptr);
            if (_this != NULL)
                _this->submit_default_values();
            return STATUS_OK;
        }
    } // namespace ctl
} // namespace lsp


