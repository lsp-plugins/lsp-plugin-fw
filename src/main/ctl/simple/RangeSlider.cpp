/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 окт. 2025 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/meta/func.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(RangeSlider)
            status_t res;
            if (!name->equals_ascii("range"))
                return STATUS_NOT_FOUND;

            tk::RangeSlider *w = new tk::RangeSlider(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::RangeSlider *wc  = new ctl::RangeSlider(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(RangeSlider)

        //-----------------------------------------------------------------
        const ctl_class_t RangeSlider::metadata = { "RangeSlider", &Widget::metadata };

        RangeSlider::RangeSlider(ui::IWrapper *wrapper, tk::RangeSlider *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            nFlags          = 0;
            fStep           = 1.0f;
            fAStep          = 10.0f;
            fDStep          = 0.1f;

            for (size_t i=0; i<PT_TOTAL; ++i)
            {
                param_t *p      = &vParams[i];

                p->pPort            = NULL;
                p->fMin             = 0.0f;
                p->fMax             = 0.0f;
                p->fValue           = 0.0f;
                p->fDefault         = 0.0f;
                p->bHasDefault      = false;
            }
        }

        RangeSlider::~RangeSlider()
        {
        }

        status_t RangeSlider::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::RangeSlider *rs = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs != NULL)
            {
                // Initialize color controllers
                sBtnColor.init(pWrapper, rs->button_color());
                sBtnBorderColor.init(pWrapper, rs->button_border_color());
                sScaleColor.init(pWrapper, rs->scale_color());
                sScaleBorderColor.init(pWrapper, rs->scale_border_color());
                sBalanceColor.init(pWrapper, rs->balance_color());

                sInactiveBtnColor.init(pWrapper, rs->inactive_button_color());
                sInactiveBtnBorderColor.init(pWrapper, rs->inactive_button_border_color());
                sInactiveScaleColor.init(pWrapper, rs->inactive_scale_color());
                sInactiveScaleBorderColor.init(pWrapper, rs->inactive_scale_border_color());
                sInactiveBalanceColor.init(pWrapper, rs->inactive_balance_color());

                for (size_t i=0; i<PT_TOTAL; ++i)
                {
                    param_t * const rp = &vParams[i];
                    rp->sMin.init(pWrapper, this);
                    rp->sMax.init(pWrapper, this);
                }

                // Bind slots
                rs->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
                rs->slots()->bind(tk::SLOT_MOUSE_DBL_CLICK, slot_dbl_click, this);
            }

            return STATUS_OK;
        }

        bool RangeSlider::bind_params(size_t type, const char *prefix, const char *name, const char *value)
        {
            if (!(name = match_prefix(prefix, name)))
                return false;

            param_t * const rp = &vParams[type];

            bind_port(&rp->pPort, "id", name, value);

            switch (type)
            {
                case PT_MIN:
                    if (!strcmp(name, ""))
                        rp->sMin.parse(value);
                    break;

                case PT_MAX:
                    if (!strcmp(name, ""))
                        rp->sMax.parse(value);
                    break;

                case PT_RANGE:
                    if (!strcmp(name, ""))
                        rp->sMin.parse(value);
                    break;

                default:
                    if (!strcmp(name, "min"))
                        rp->sMin.parse(value);
                    if (!strcmp(name, "max"))
                        rp->sMax.parse(value);

                    if (set_value(&rp->fDefault, "dfl", name, value))
                        rp->bHasDefault     = true;
                    if (set_value(&rp->fDefault, "default", name, value))
                        rp->bHasDefault     = true;
                    break;
            }

            return true;
        }

        void RangeSlider::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::RangeSlider *rs = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs != NULL)
            {
                bind_params(PT_MIN, "min", name, value);
                bind_params(PT_MAX, "max", name, value);
                bind_params(PT_BEGIN, "start", name, value);
                bind_params(PT_BEGIN, "begin", name, value);
                bind_params(PT_END, "end", name, value);
                bind_params(PT_RANGE, "range", name, value);
                bind_params(PT_RANGE, "distance", name, value);

                set_value(&fAStep, "astep", name, value);
                set_value(&fAStep, "step.accel", name, value);
                set_value(&fDStep, "dstep", name, value);
                set_value(&fDStep, "step.decel", name, value);

                if (set_value(&fStep, "step", name, value))
                    nFlags         |= GF_STEP;

                bool log = false;
                if (set_value(&log, "log", name, value))
                    nFlags      = lsp_setflag(nFlags, GF_LOG, log) | GF_LOG_SET;
                else if (set_value(&log, "logarithmic", name, value))
                    nFlags      = lsp_setflag(nFlags, GF_LOG, log) | GF_LOG_SET;

                sBtnColor.set("color", name, value);
                sBtnColor.set("button.color", name, value);
                sBtnColor.set("btncolor", name, value);
                sBtnBorderColor.set("button.border.color", name, value);
                sBtnBorderColor.set("btnborder.color", name, value);
                sScaleColor.set("scale.color", name, value);
                sScaleColor.set("scolor", name, value);
                sScaleBorderColor.set("scale.border.color", name, value);
                sScaleBorderColor.set("sborder.color", name, value);
                sBalanceColor.set("balance.color", name, value);
                sBalanceColor.set("bcolor", name, value);

                sInactiveBtnColor.set("inactive.color", name, value);
                sInactiveBtnColor.set("inactive.button.color", name, value);
                sInactiveBtnColor.set("inactive.btncolor", name, value);
                sInactiveBtnBorderColor.set("inactive.button.border.color", name, value);
                sInactiveBtnBorderColor.set("inactive.btnborder.color", name, value);
                sInactiveScaleColor.set("inactive.scale.color", name, value);
                sInactiveScaleColor.set("inactive.scolor", name, value);
                sInactiveScaleBorderColor.set("inactive.scale.border.color", name, value);
                sInactiveScaleBorderColor.set("inactive.sborder.color", name, value);
                sInactiveBalanceColor.set("inactive.balance.color", name, value);
                sInactiveBalanceColor.set("inactive.bcolor", name, value);

                set_size_range(rs->size(), "size", name, value);

                set_size_range(rs->button_width(), "button.size", name, value);
                set_size_range(rs->button_width(), "btnsize", name, value);

                set_param(rs->button_aspect(), "button.aspect", name, value);
                set_param(rs->button_aspect(), "btna", name, value);

                set_param(rs->button_pointer(), "button.pointer", name, value);
                set_param(rs->button_pointer(), "bpointer", name, value);

                set_param(rs->angle(), "angle", name, value);
                set_param(rs->scale_width(), "scale.width", name, value);
                set_param(rs->scale_width(), "swidth", name, value);
                set_param(rs->scale_border(), "scale.border", name, value);
                set_param(rs->scale_border(), "sborder", name, value);
                set_param(rs->scale_radius(), "scale.radius", name, value);
                set_param(rs->scale_radius(), "sradius", name, value);
                set_param(rs->scale_gradient(), "scale.gradient", name, value);
                set_param(rs->scale_gradient(), "sgradient", name, value);

                set_param(rs->button_border(), "button.border", name, value);
                set_param(rs->button_border(), "btnborder", name, value);
                set_param(rs->button_radius(), "button.radius", name, value);
                set_param(rs->button_radius(), "btnradius", name, value);
                set_param(rs->button_gradient(), "button.gradient", name, value);
                set_param(rs->button_gradient(), "btngradient", name, value);

                set_param(rs->scale_brightness(), "scale.brightness", name, value);
                set_param(rs->scale_brightness(), "scale.bright", name, value);
                set_param(rs->scale_brightness(), "sbrightness", name, value);
                set_param(rs->scale_brightness(), "sbright", name, value);

                set_param(rs->balance_color_custom(), "bcolor.custom", name, value);
                set_param(rs->balance_color_custom(), "balance.color.custom", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void RangeSlider::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            if (nFlags & GF_CHANGING)
                return;
            nFlags |= GF_CHANGING;
            lsp_finally { nFlags &= ~GF_CHANGING; };

            // Check that absolute minimum and maximum have changed
            size_t nf_flags = 0;

            // Parse absolute minimum
            {
                param_t * const rp = &vParams[PT_MIN];
                if (rp->sMin.depends(port))
                {
                    rp->fMin        = rp->sMin.evaluate_float();
                    nf_flags       |= NF_MIN;
                }
                else if (rp->pPort == port)
                {
                    rp->fMin        = rp->pPort->value();
                    nf_flags       |= NF_MIN;
                }
            }
            // Parse absolute maximum
            {
                param_t * const rp = &vParams[PT_MAX];
                if (rp->sMax.depends(port))
                {
                    rp->fMax        = rp->sMax.evaluate_float();
                    nf_flags       |= NF_MAX;
                }
                else if (rp->pPort == port)
                {
                    rp->fMax        = rp->pPort->value();
                    nf_flags       |= NF_MAX;
                }
            }
            // Parse range
            {
                param_t * const rp = &vParams[PT_RANGE];
                if (rp->sMax.depends(port))
                {
                    rp->fMin        = rp->sMin.evaluate_float();
                    nf_flags       |= NF_RANGE;
                }
                else if (rp->pPort == port)
                {
                    rp->fMin        = rp->pPort->value();
                    nf_flags       |= NF_RANGE;
                }
            }

            // Commit new values
            for (size_t i=PT_BEGIN; i<=PT_END; ++i)
            {
                param_t * const rp = &vParams[i];
                if (rp->sMin.depends(port))
                {
                    rp->fMin        = rp->sMin.evaluate_float();
                    nf_flags       |= NF_MIN;
                }
                if (rp->sMax.depends(port))
                {
                    rp->fMax        = rp->sMax.evaluate_float();
                    nf_flags       |= NF_MAX;
                }

                rp->fValue      = rp->pPort->value();
                if (rp->pPort == port)
                    nf_flags       |= (i == PT_BEGIN) ? NF_BEGIN : NF_END;
            }

            // Commit and synchronize values
            if (nf_flags != 0)
            {
                commit_values(nf_flags);

                bool changed[PT_TOTAL];
                for (size_t i=PT_BEGIN; i<=PT_END; ++i)
                {
                    changed[i] = false;

                    param_t * const rp  = &vParams[i];
                    if ((rp->pPort != NULL) && (rp->pPort->value() != rp->fValue))
                    {
                        rp->pPort->set_value(rp->fValue);
                        changed[i] = true;
                    }
                }

                // Notify about changes
                for (size_t i=PT_BEGIN; i<=PT_END; ++i)
                {
                    if (changed[i])
                        vParams[i].pPort->notify_all(ui::PORT_USER_EDIT);
                }
            }
        }

        void RangeSlider::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            // Parse minimum and maximum for begin and end
            for (size_t i=PT_BEGIN; i<=PT_END; ++i)
            {
                param_t * const rp = &vParams[i];
                if (rp->pPort != NULL)
                {
                    const meta::port_t * const meta = rp->pPort->metadata();
                    rp->fMin        = (meta->flags & meta::F_LOWER) ? meta->min : 0.0f;
                    rp->fMax        = (meta->flags & meta::F_UPPER) ? meta->max : 1.0f;
                    rp->fValue      = rp->pPort->value();
                }
                if (rp->sMin.valid())
                    rp->fMin        = rp->sMin.evaluate_float();
                if (rp->sMax.valid())
                    rp->fMax        = rp->sMax.evaluate_float();
            }

            // Parse absolute minimum
            {
                param_t * const rp = &vParams[PT_MIN];
                rp->fMin        = lsp_min(vParams[PT_BEGIN].fMin, vParams[PT_END].fMin);
                if (rp->sMin.valid())
                    rp->fMin        = rp->sMin.evaluate_float();
                else if (rp->pPort != NULL)
                    rp->fMin        = rp->pPort->value();
            }
            // Parse absolute maximum
            {
                param_t * const rp = &vParams[PT_MAX];
                rp->fMax        = lsp_max(vParams[PT_BEGIN].fMax, vParams[PT_END].fMax);
                if (rp->sMax.valid())
                    rp->fMax        = rp->sMax.evaluate_float();
                else if (rp->pPort != NULL)
                    rp->fMax        = rp->pPort->value();
            }
            // Parse range
            {
                param_t * const rp = &vParams[PT_RANGE];
                rp->fMin        = 0.0f;
                if (rp->sMin.valid())
                    rp->fMin        = rp->sMin.evaluate_float();
                else if (rp->pPort != NULL)
                    rp->fMin        = rp->pPort->value();
            }

            commit_values(NF_MIN | NF_MAX | NF_RANGE | NF_BEGIN | NF_END);
        }

        float RangeSlider::get_slider_value(tk::RangeSlider *rs, size_t type)
        {
            switch (type)
            {
                case PT_MIN: return rs->limits()->min();
                case PT_MAX: return rs->limits()->max();
                case PT_BEGIN: return rs->values()->min();
                case PT_END: return rs->values()->max();
                case PT_RANGE: return rs->distance()->get();
                default:
                    break;
            }
            return rs->values()->min();
        }

        void RangeSlider::set_slider_value(tk::RangeSlider *rs, size_t type, float value)
        {
            switch (type)
            {
                case PT_MIN:
                    rs->limits()->set_min(value);
                    break;
                case PT_MAX:
                    rs->limits()->set_max(value);
                    break;
                case PT_BEGIN:
                    rs->values()->set_min(value);
                    break;
                case PT_END:
                    rs->values()->set_max(value);
                    break;
                case PT_RANGE:
                    rs->distance()->set(value);
                    break;
                default:
                    break;
            }
        }

        void RangeSlider::submit_values(size_t flags)
        {
            tk::RangeSlider *rs = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs == NULL)
                return;

            // Process values
            bool changed[PT_TOTAL];
            for (size_t i=PT_BEGIN; i<=PT_END; ++i)
            {
                changed[i]              = false;
                if (!(flags & (1u << i)))
                    continue;

                param_t * const rp      = &vParams[i];
                float value             = get_slider_value(rs, i);
                const meta::port_t *p   = (rp->pPort != NULL) ? rp->pPort->metadata() : NULL;
                if (p == NULL)
                {
                    if ((rp->pPort != NULL) && (rp->pPort->value() != value))
                    {
                        rp->pPort->set_value(value);
                        changed[i]      = true;
                    }
                    continue;
                }

                if (is_gain_unit(p->unit)) // Gain
                {
                    float base      = (p->unit == meta::U_GAIN_AMP) ? M_LN10 * 0.05 : M_LN10 * 0.1;
                    float thresh    = (p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                    value           = expf(value * base);
                    if (value < thresh)
                        value           = 0.0f;
                }
                else if (is_discrete_unit(p->unit)) // Integer type
                {
                    value          = truncf(value);
                }
                else if (nFlags & GF_LOG)  // Float and other values, logarithmic
                {
                    double thresh   = (p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                    value           = exp(value);
                    float min       = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                    if ((min <= 0.0f) && (value < thresh))
                        value           = 0.0f;
                }

                if ((rp->pPort != NULL) && (rp->pPort->value() != value))
                {
                    rp->pPort->set_value(value);
                    changed[i]      = true;
                }
            }

            // Notify about changes
            for (size_t i=PT_BEGIN; i<=PT_END; ++i)
            {
                if (changed[i])
                    vParams[i].pPort->notify_all(ui::PORT_USER_EDIT);
            }
        }

        void RangeSlider::set_default_values()
        {
            if (nFlags & GF_CHANGING)
                return;

            tk::RangeSlider *rs = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs == NULL)
                return;

            nFlags |= GF_CHANGING;
            lsp_finally { nFlags &= ~GF_CHANGING; };

            // Process values
            bool changed[PT_TOTAL];
            for (size_t i=PT_BEGIN; i<=PT_END; ++i)
            {
                changed[i]              = false;

                param_t * const rp      = &vParams[i];
                const meta::port_t *p   = (rp->pPort != NULL) ? rp->pPort->metadata() : NULL;
                const float dfl         = (rp->pPort != NULL) ? rp->pPort->default_value() : (rp->bHasDefault) ? rp->fDefault : p->start;
                float value             = dfl;

                if (p != NULL)
                {
                    if (is_gain_unit(p->unit)) // Decibels
                    {
                        double base = (p->unit == meta::U_GAIN_AMP) ? 20.0 / M_LN10 : 10.0 / M_LN10;

                        if (value < GAIN_AMP_M_120_DB)
                            value           = GAIN_AMP_M_120_DB;

                        value   = base * log(value);
                    }
                    else if (nFlags & GF_LOG)
                    {
                        if (value < GAIN_AMP_M_120_DB)
                            value           = GAIN_AMP_M_120_DB;
                        value   = logf(value);
                    }
                }

                set_slider_value(rs, i, value);
                if ((rp->pPort != NULL) && (dfl != rp->pPort->value()))
                {
                    rp->fValue              = dfl;
                    changed[i]              = true;
                    rp->pPort->set_value(dfl);
                }
            }

            // Notify about changes
            for (size_t i=PT_BEGIN; i<=PT_END; ++i)
            {
                if (changed[i])
                    vParams[i].pPort->notify_all(ui::PORT_USER_EDIT);
            }
        }

        bool RangeSlider::is_log_range(ui::IPort *p)
        {
            if (nFlags & GF_LOG_SET)
                return nFlags & GF_LOG;

            if (p == NULL)
                return false;

            const meta::port_t * const meta     = p->metadata();
            if (meta == NULL)
                return false;

            return meta::is_log_rule(meta) || meta::is_gain_unit(meta->unit);
        }

        void RangeSlider::commit_values(size_t flags)
        {
            // Ensure that widget is set
            tk::RangeSlider *rs = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs == NULL)
                return;

            // Initialize configuration
            param_t * const begin   = &vParams[PT_BEGIN];
            param_t * const end     = &vParams[PT_END];
            float abs_min           = vParams[PT_MIN].fMin;
            float abs_max           = vParams[PT_MAX].fMax;
            float abs_range         = vParams[PT_RANGE].fMin;

            // Apply value constraints
            if (flags & (NF_MIN | NF_MAX))
            {
                for (size_t i=PT_BEGIN; i<=PT_END; ++i)
                {
                    param_t * const rp      = &vParams[i];
                    rp->fValue              = lsp_xlimit(rp->fValue, rp->fMin, rp->fMax);
                    rp->fValue              = lsp_xlimit(rp->fValue, abs_min, abs_max);
                }
            }
            if (flags & NF_RANGE)
            {
                if ((begin->pPort != NULL) && (end->pPort != NULL))
                {
                    if (flags & NF_BEGIN)
                    {
                        if (is_log_range(begin->pPort))
                            end->fValue     = lsp_max(begin->fValue * logf(abs_range), end->fValue);
                        else
                            end->fValue     = lsp_max(begin->fValue + abs_range, end->fValue);
                        end->fValue     = lsp_xlimit(end->fValue, end->fMin, end->fMax);
                        end->fValue     = lsp_xlimit(end->fValue, abs_min, abs_max);
                    }
                    else if (flags & NF_END)
                    {
                        if (is_log_range(end->pPort))
                            begin->fValue   = lsp_min(end->fValue / logf(abs_range), begin->fValue);
                        else
                            begin->fValue   = lsp_min(end->fValue - abs_range, begin->fValue);
                        begin->fValue   = lsp_xlimit(begin->fValue, begin->fMin, begin->fMax);
                        begin->fValue   = lsp_xlimit(begin->fValue, abs_min, abs_max);
                    }
                }
            }
            float v_begin           = begin->fValue;
            float v_end             = end->fValue;
            const meta::port_t *p   = (begin->pPort != NULL) ? begin->pPort->metadata() : NULL;
            if (p == NULL)
                p                       = (end->pPort != NULL) ? end->pPort->metadata() : NULL;
            const meta::unit_t unit = (p != NULL) ? p->unit : meta::U_NONE;
            const bool has_step     = (nFlags & GF_STEP) || ((p != NULL) && (p->flags & meta::F_STEP));
            float step              = (nFlags & GF_STEP) ? fStep : (has_step) ? p->step : 0.0f;

            if (meta::is_gain_unit(unit)) // Decibels
            {
                const float base        = (unit == meta::U_GAIN_AMP) ? 20.0f / M_LN10 : 10.0f / M_LN10;
                const float thresh      = ((p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);
                step                    = base * logf((has_step) ? fStep + 1.0f : 1.01f) * 0.1f;

                abs_min                 = (fabsf(abs_min) < thresh) ? (base * logf(thresh) - step) : (base * logf(abs_min));
                abs_max                 = (fabsf(abs_max) < thresh) ? (base * logf(thresh) - step) : (base * logf(abs_max));
                abs_range               = (base * logf(abs_range));
                v_begin                 = (fabsf(v_begin) < thresh) ? (base * logf(thresh) - step) : (base * logf(v_begin));
                v_end                   = (fabsf(v_end) < thresh)   ? (base * logf(thresh) - step) : (base * logf(v_end));

                step                   *= 10.0f;
            }
            else if (meta::is_log_rule(p))  // Float and other values, logarithmic
            {
                const float thresh      = ((p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);

                step                    = logf((p->flags & meta::F_STEP) ? p->step + 1.0f : 1.01f);
                abs_min                 = (fabsf(abs_min) < thresh)     ? logf(thresh) - step : logf(abs_min);
                abs_max                 = (fabsf(abs_max) < thresh)     ? logf(thresh) - step : logf(abs_max);
                abs_range               = logf(abs_range);
                v_begin                 = (fabsf(v_begin) < thresh)     ? logf(thresh) - step : logf(v_begin);
                v_end                   = (fabsf(v_end) < thresh)       ? logf(thresh) - step : logf(v_end);

                step                   *= 10.0f;
            }

            // Initialize fader
            switch (flags & (NF_MIN | NF_MAX))
            {
                case NF_MIN | NF_MAX:
                    rs->limits()->set(abs_min, abs_max);
                    break;
                case NF_MIN:
                    rs->limits()->set_min(abs_min);
                    break;
                case NF_MAX:
                    rs->limits()->set_max(abs_max);
                    break;
                default:
                    break;
            }
            if (flags & NF_RANGE)
                rs->distance()->set(abs_range);
            if (flags & (NF_BEGIN | NF_END))
                rs->values()->set(v_begin, v_end);
            rs->step()->set(step);
        }

        status_t RangeSlider::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::RangeSlider *_this = static_cast<ctl::RangeSlider *>(ptr);
            const size_t *ev_flags  = static_cast<size_t *>(data);
            size_t flags            = 0;
            if (ev_flags != NULL)
            {
                flags                   = lsp_setflag(flags, 1 << PT_END, *ev_flags & tk::RangeSlider::CHANGE_MAX);
                flags                   = lsp_setflag(flags, 1 << PT_BEGIN, *ev_flags & tk::RangeSlider::CHANGE_MIN);
            }
            else
                flags                   = 1 << PT_END;
            if (_this != NULL)
                _this->submit_values(flags);
            return STATUS_OK;
        }

        status_t RangeSlider::slot_dbl_click(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::RangeSlider *_this   = static_cast<ctl::RangeSlider *>(ptr);
            if (_this != NULL)
                _this->set_default_values();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */






