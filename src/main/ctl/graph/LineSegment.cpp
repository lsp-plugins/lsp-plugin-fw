/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-gott-compressor
 * Created on: 18 июн. 2023 г.
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
        CTL_FACTORY_IMPL_START(LineSegment)
            status_t res;

            if (!name->equals_ascii("line"))
                return STATUS_NOT_FOUND;

            tk::GraphLineSegment *w = new tk::GraphLineSegment(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::LineSegment *wc    = new ctl::LineSegment(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(LineSegment)

        //-----------------------------------------------------------------
        const ctl_class_t LineSegment::metadata        = { "LineSegment", &Widget::metadata };

        LineSegment::LineSegment(ui::IWrapper *wrapper, tk::GraphLineSegment *widget): Widget(wrapper, widget)
        {
            pClass      = &metadata;

            init_param(&sX, widget->hvalue(), widget->hstep());
            init_param(&sY, widget->vvalue(), widget->vstep());
            init_param(&sZ, widget->zvalue(), widget->zstep());
        }

        LineSegment::~LineSegment()
        {
        }

        void LineSegment::init_param(param_t *p, tk::RangeFloat *value, tk::StepFloat *step)
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

        status_t LineSegment::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphLineSegment *gls = tk::widget_cast<tk::GraphLineSegment>(wWidget);
            if (gls != NULL)
            {
                sX.sEditable.init(pWrapper, gls->heditable());
                sY.sEditable.init(pWrapper, gls->veditable());
                sZ.sEditable.init(pWrapper, gls->zeditable());
                sX.sExpr.init(pWrapper, this);
                sY.sExpr.init(pWrapper, this);
                sZ.sExpr.init(pWrapper, this);

                sSmooth.init(pWrapper, gls->smooth());
                sWidth.init(pWrapper, gls->width());
                sHoverWidth.init(pWrapper, gls->hover_width());
                sLeftBorder.init(pWrapper, gls->left_border());
                sRightBorder.init(pWrapper, gls->right_border());
                sHoverLeftBorder.init(pWrapper, gls->hover_left_border());
                sHoverRightBorder.init(pWrapper, gls->hover_right_border());
                sBeginX.init(pWrapper, this);
                sBeginY.init(pWrapper, this);

                sColor.init(pWrapper, gls->color());
                sHoverColor.init(pWrapper, gls->hover_color());
                sLeftColor.init(pWrapper, gls->border_left_color());
                sRightColor.init(pWrapper, gls->border_right_color());
                sHoverLeftColor.init(pWrapper, gls->hover_border_left_color());
                sHoverRightColor.init(pWrapper, gls->hover_border_right_color());

                // Bind slots
                gls->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
                gls->slots()->bind(tk::SLOT_MOUSE_DBL_CLICK, slot_dbl_click, this);
            }

            return STATUS_OK;
        }

        void LineSegment::set_segment_param(param_t *p, const char *prefix, const char *name, const char *value)
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

        void LineSegment::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphLineSegment *gls = tk::widget_cast<tk::GraphLineSegment>(wWidget);
            if (gls != NULL)
            {
                set_segment_param(&sX, "hor", name, value);
                set_segment_param(&sX, "h", name, value);
                set_segment_param(&sX, "x", name, value);

                set_segment_param(&sY, "vert", name, value);
                set_segment_param(&sY, "v", name, value);
                set_segment_param(&sY, "y", name, value);

                set_segment_param(&sZ, "scroll", name, value);
                set_segment_param(&sZ, "s", name, value);
                set_segment_param(&sZ, "z", name, value);

                set_param(gls->haxis(), "basis", name, value);
                set_param(gls->haxis(), "xaxis", name, value);
                set_param(gls->haxis(), "ox", name, value);

                set_param(gls->vaxis(), "parallel", name, value);
                set_param(gls->vaxis(), "yaxis", name, value);
                set_param(gls->vaxis(), "oy", name, value);

                set_param(gls->origin(), "origin", name, value);
                set_param(gls->origin(), "center", name, value);
                set_param(gls->origin(), "o", name, value);

                set_expr(&sBeginX, "start.x", name, value);
                set_expr(&sBeginX, "begin.x", name, value);
                set_expr(&sBeginX, "sx", name, value);

                set_expr(&sBeginY, "start.y", name, value);
                set_expr(&sBeginY, "begin.y", name, value);
                set_expr(&sBeginY, "sy", name, value);

                sSmooth.set("smooth", name, value);
                sWidth.set("width", name, value);
                sHoverWidth.set("hwidth", name, value);
                sLeftBorder.set("lborder", name, value);
                sLeftBorder.set("left_border", name, value);
                sRightBorder.set("rborder", name, value);
                sRightBorder.set("right_border", name, value);

                sHoverLeftBorder.set("hlborder", name, value);
                sHoverLeftBorder.set("hover_left_border", name, value);
                sHoverRightBorder.set("hrborder", name, value);
                sHoverRightBorder.set("hover_right_border", name, value);

                sColor.set("color", name, value);
                sHoverColor.set("hcolor", name, value);
                sHoverColor.set("hover_color", name, value);
                sLeftColor.set("lcolor", name, value);
                sLeftColor.set("left_color", name, value);
                sRightColor.set("rcolor", name, value);
                sRightColor.set("right_color", name, value);
                sHoverLeftColor.set("hlcolor", name, value);
                sHoverLeftColor.set("hover_left_color", name, value);
                sHoverRightColor.set("hrcolor", name, value);
                sHoverRightColor.set("hover_right_color", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void LineSegment::notify(ui::IPort *port)
        {
            Widget::notify(port);

            commit_value(&sX, port, false);
            commit_value(&sY, port, false);
            commit_value(&sZ, port, false);

            tk::GraphLineSegment *gls = tk::widget_cast<tk::GraphLineSegment>(wWidget);
            if (gls != NULL)
            {
                if (sBeginX.depends(port))
                    gls->begin()->set_x(sBeginX.evaluate());
                if (sBeginY.depends(port))
                    gls->begin()->set_y(sBeginY.evaluate());
            }
        }

        void LineSegment::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            configure_param(&sX, true);
            configure_param(&sY, true);
            configure_param(&sZ, false);

            commit_value(&sX, sX.pPort, true);
            commit_value(&sY, sY.pPort, true);
            commit_value(&sZ, sZ.pPort, true);

            tk::GraphLineSegment *gls = tk::widget_cast<tk::GraphLineSegment>(wWidget);
            if (gls != NULL)
            {
                if (sBeginX.valid())
                    gls->begin()->set_x(sBeginX.evaluate());
                if (sBeginY.valid())
                    gls->begin()->set_y(sBeginY.evaluate());
            }
        }

        void LineSegment::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);

            commit_value(&sX, sX.pPort, true);
            commit_value(&sY, sY.pPort, true);
            commit_value(&sZ, sZ.pPort, true);
        }

        void LineSegment::submit_values()
        {
            tk::GraphLineSegment *gls = tk::widget_cast<tk::GraphLineSegment>(wWidget);
            if (gls == NULL)
                return;

            submit_value(&sX, gls->hvalue()->get());
            submit_value(&sY, gls->vvalue()->get());
            submit_value(&sZ, gls->zvalue()->get());
        }

        void LineSegment::submit_default_values()
        {
            tk::GraphLineSegment *gls = tk::widget_cast<tk::GraphLineSegment>(wWidget);
            if (gls == NULL)
                return;

            submit_value(&sX, sX.fDefault);
            submit_value(&sY, sY.fDefault);
            submit_value(&sZ, sZ.fDefault);
        }

        void LineSegment::commit_value(param_t *p, ui::IPort *port, bool force)
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

        void LineSegment::submit_value(param_t *p, float value)
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

        void LineSegment::configure_param(param_t *p, bool axis)
        {
            tk::GraphLineSegment *gls = tk::widget_cast<tk::GraphLineSegment>(wWidget);
            if (gls == NULL)
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
            else
                p->nFlags       = lsp_setflag(p->nFlags, DF_LOG, xp.flags & meta::F_LOG);

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

        status_t LineSegment::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            LineSegment *_this  = static_cast<LineSegment *>(ptr);
            if (_this != NULL)
                _this->submit_values();
            return STATUS_OK;
        }

        status_t LineSegment::slot_dbl_click(tk::Widget *sender, void *ptr, void *data)
        {
            LineSegment *_this  = static_cast<LineSegment *>(ptr);
            if (_this != NULL)
                _this->submit_default_values();
            return STATUS_OK;
        }
    } // namespace ctl
} // namespace lsp



