/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
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

            construct_param(&sBegin);
            construct_param(&sEnd);
            construct_value(&sRange);
        }

        RangeSlider::~RangeSlider()
        {
        }

        status_t RangeSlider::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::RangeSlider * const rs = tk::widget_cast<tk::RangeSlider>(wWidget);
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

                init_param(&sBegin);
                init_param(&sEnd);
                init_value(&sRange);

                // Bind slots
                rs->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
                rs->slots()->bind(tk::SLOT_BEGIN_EDIT, slot_begin_edit, this);
                rs->slots()->bind(tk::SLOT_END_EDIT, slot_end_edit, this);
                rs->slots()->bind(tk::SLOT_MOUSE_DBL_CLICK, slot_dbl_click, this);
            }

            return STATUS_OK;
        }

        void RangeSlider::construct_value(value_t *v)
        {
            v->pPort           = NULL;
            v->fValue           = 0.0f;
        }

        void RangeSlider::construct_param(param_t *p)
        {
            construct_value(&p->sMin);
            construct_value(&p->sMax);
            construct_value(&p->sValue);
            p->fDefault         = 0.0f;
            p->bHasDefault      = false;
        }

        void RangeSlider::init_value(value_t *v)
        {
            v->sExpr.init(pWrapper, this);
        }

        void RangeSlider::init_param(param_t *p)
        {
            init_value(&p->sMin);
            init_value(&p->sMax);
            init_value(&p->sValue);
        }

        bool RangeSlider::bind_param(param_t *p, const char *prefix, const char *name, const char *value)
        {
            if (!(name = match_prefix(prefix, name)))
                return false;

            bool result = true;

            if (!strcmp(name, ""))
                p->sValue.sExpr.parse(value);
            else if (!strcmp(name, "min"))
                p->sMin.sExpr.parse(value);
            else if (!strcmp(name, "max"))
                p->sMax.sExpr.parse(value);
            else if (set_value(&p->fDefault, "dfl", name, value))
                p->bHasDefault     = true;
            else if (set_value(&p->fDefault, "default", name, value))
                p->bHasDefault     = true;
            else
            {
                result =
                    (bind_port(&p->sValue.pPort, "id", name, value)) ||
                    (bind_port(&p->sMin.pPort, "min.id", name, value)) ||
                    (bind_port(&p->sMax.pPort, "max.id", name, value));
            }

            return result;
        }

        bool RangeSlider::bind_value(value_t *v, const char *prefix, const char *name, const char *value)
        {
            if (!(name = match_prefix(prefix, name)))
                return false;

            bool result = true;
            if (!strcmp(name, ""))
                v->sExpr.parse(value);
            else
                result = bind_port(&v->pPort, "id", name, value);

            return result;
        }

        void RangeSlider::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::RangeSlider *rs = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs != NULL)
            {
                if (!strcmp(name, "min"))
                    sBegin.sMin.sExpr.parse(value);
                else if (!strcmp(name, "max"))
                    sEnd.sMax.sExpr.parse(value);

                bind_param(&sBegin, "start", name, value);
                bind_param(&sBegin, "begin", name, value);
                bind_param(&sEnd, "end", name, value);
                bind_value(&sRange, "range", name, value);
                bind_value(&sRange, "distance", name, value);

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

        float RangeSlider::decode_value(ui::IPort *p, float value)
        {
            const meta::port_t *meta    = (p != NULL) ? p->metadata() : NULL;
            if (meta == NULL)
                return value;

            if (is_gain_unit(meta->unit)) // Gain
            {
                float base      = (meta->unit == meta::U_GAIN_AMP) ? float(M_LN10 * 0.05f) : float(M_LN10 * 0.1f);
                float thresh    = (meta->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                value           = expf(value * base);
                if (value < thresh)
                    value           = 0.0f;
            }
            else if (is_discrete_unit(meta->unit)) // Integer type
            {
                value          = truncf(value);
            }
            else if (nFlags & GF_LOG)  // Float and other values, logarithmic
            {
                double thresh   = (meta->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                value           = exp(value);
                float min       = (meta->flags & meta::F_LOWER) ? meta->min : 0.0f;
                if ((min <= 0.0f) && (value < thresh))
                    value           = 0.0f;
            }

            return value;
        }

        float RangeSlider::encode_value(ui::IPort *p, float value)
        {
            const meta::port_t *meta    = (p != NULL) ? p->metadata() : NULL;
            if (meta == NULL)
                return value;

            if (is_gain_unit(meta->unit)) // Decibels
            {
                double base = (meta->unit == meta::U_GAIN_AMP) ? 20.0 / M_LN10 : 10.0 / M_LN10;
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

            return value;
        }

        size_t RangeSlider::notify_value(value_t *v, ui::IPort *port, size_t nf_flag)
        {
            if (v->sExpr.depends(port))
            {
                v->fValue           = v->sExpr.evaluate_float();
                return nf_flag;
            }
            if (v->pPort != NULL)
            {
                v->fValue           = v->pPort->value();
                if (v->pPort == port)
                    return nf_flag;
            }
            return 0;
        }

        size_t RangeSlider::notify_param(param_t *p, ui::IPort *port)
        {
            const size_t min_flag   = (p == &sBegin) ? NF_BEGIN_MIN : NF_END_MIN;
            const size_t max_flag   = (p == &sBegin) ? NF_BEGIN_MAX : NF_END_MAX;
            const size_t val_flag   = (p == &sBegin) ? NF_BEGIN : NF_END;

            return
                notify_value(&p->sMin, port, min_flag) |
                notify_value(&p->sMax, port, max_flag) |
                notify_value(&p->sValue, port, val_flag);
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
            nf_flags |= notify_param(&sBegin, port);
            nf_flags |= notify_param(&sEnd, port);
            nf_flags |= notify_value(&sRange, port, NF_RANGE);

            // Commit and synchronize values
            if (nf_flags != 0)
            {
                // Commit values
                commit_values(nf_flags);

                const float begin_ch    =
                    (sBegin.sValue.pPort != NULL) &&
                    (sBegin.sValue.pPort->value() != sBegin.sValue.fValue);
                const float end_ch      =
                    (sEnd.sValue.pPort != NULL) &&
                    (sEnd.sValue.pPort->value() != sEnd.sValue.fValue);

                // Start port editing
                if (begin_ch)
                    sBegin.sValue.pPort->begin_edit();
                if (end_ch)
                    sEnd.sValue.pPort->begin_edit();

                // Update port settings
                if (begin_ch)
                    sBegin.sValue.pPort->set_value(sBegin.sValue.fValue);
                if (end_ch)
                    sEnd.sValue.pPort->set_value(sEnd.sValue.fValue);

                // Notify about changes
                if (begin_ch)
                    sBegin.sValue.pPort->notify_all(ui::PORT_USER_EDIT);
                if (end_ch)
                    sEnd.sValue.pPort->notify_all(ui::PORT_USER_EDIT);

                // Notify about end of edit
                if (begin_ch)
                    sBegin.sValue.pPort->end_edit();
                if (end_ch)
                    sEnd.sValue.pPort->end_edit();
            }
        }

        bool RangeSlider::end_value(value_t *v)
        {
            if (v->sExpr.valid())
            {
                v->fValue           = v->sExpr.evaluate_float();
                return true;
            }
            if (v->pPort != NULL)
            {
                v->fValue           = v->pPort->value();
                return true;
            }
            return false;
        }

        void RangeSlider::end_param(param_t *p)
        {
            const meta::port_t * const meta = (p->sValue.pPort != NULL) ? meta : NULL;
            if (!end_value(&p->sMin))
                p->sMin.fValue  = ((meta != NULL) && (meta->flags & meta::F_LOWER)) ? meta->min : 0.0f;
            if (!end_value(&p->sMax))
                p->sMax.fValue  = ((meta != NULL) && (meta->flags & meta::F_UPPER)) ? meta->max : 1.0f;
            if (!end_value(&p->sValue))
            {
                if (p->bHasDefault)
                    p->sValue.fValue    = p->fDefault;
                else
                    p->sValue.fValue    = (meta != NULL) ? meta->start: 0.5f;
            }
        }

        void RangeSlider::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            // Parse minimum and maximum for begin and end
            end_param(&sBegin);
            end_param(&sEnd);
            end_value(&sRange);

            commit_values(
                NF_RANGE |
                NF_BEGIN | NF_BEGIN_MIN | NF_BEGIN_MAX |
                NF_END | NF_END_MIN | NF_END_MAX);
        }

        void RangeSlider::submit_values(float begin, float end, bool begin_ch, bool end_ch, size_t flags)
        {
            // Start port editing
            if (begin_ch)
                sBegin.sValue.pPort->begin_edit();
            if (end_ch)
                sEnd.sValue.pPort->begin_edit();

            // Update port settings
            if (begin_ch)
                sBegin.sValue.pPort->set_value(begin);
            if (end_ch)
                sEnd.sValue.pPort->set_value(end);

            // Notify about changes
            if (begin_ch)
                sBegin.sValue.pPort->notify_all(flags);
            if (end_ch)
                sEnd.sValue.pPort->notify_all(flags);

            // Notify about end of edit
            if (begin_ch)
                sBegin.sValue.pPort->end_edit();
            if (end_ch)
                sEnd.sValue.pPort->end_edit();
        }

        void RangeSlider::submit_values(size_t flags)
        {
            tk::RangeSlider * const rs  = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs == NULL)
                return;

            // Store previous values
            float old_begin     = (sBegin.sValue.pPort != NULL)    ? sBegin.sValue.pPort->value() : 0.0f;
            float old_end       = (sEnd.sValue.pPort != NULL)      ? sEnd.sValue.pPort->value()   : 0.0f;
            float new_begin     = (flags & NF_BEGIN)    ? decode_value(sBegin.sValue.pPort, rs->begin()->get())   : old_begin;
            float new_end       = (flags & NF_END)      ? decode_value(sEnd.sValue.pPort, rs->end()->get())       : old_end;

            // Begin editing
            const bool begin_ch    =
                (new_begin != old_begin) &&
                (sBegin.sValue.pPort != NULL) &&
                (sBegin.sValue.pPort->value() != old_begin);
            const bool end_ch      =
                (new_end != old_end) &&
                (sEnd.sValue.pPort != NULL) &&
                (sEnd.sValue.pPort->value() != old_end);

            submit_values(new_begin, new_end, begin_ch, end_ch, ui::PORT_USER_EDIT);
        }
        
        float RangeSlider::calc_default_value(param_t *p)
        {
            const meta::port_t *meta    = (p->sValue.pPort!= NULL) ? p->sValue.pPort->metadata() : NULL;
            const float dfl         = (p->sValue.pPort != NULL) ?
                p->sValue.pPort->default_value() :
                (p->bHasDefault) ? p->fDefault : meta->start;
            return dfl;
        }
        
        void RangeSlider::set_default_values()
        {
            if (nFlags & GF_CHANGING)
                return;

            tk::RangeSlider * const rs = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs == NULL)
                return;

            nFlags |= GF_CHANGING;
            lsp_finally { nFlags &= ~GF_CHANGING; };

            // Begin editing
            const float begin_dfl   = calc_default_value(&sBegin);
            const float end_dfl     = calc_default_value(&sEnd);
            const float begin_ch    =
                (sBegin.sValue.pPort != NULL) &&
                (sBegin.sValue.pPort->value() != begin_dfl);
            const float end_ch      =
                (sEnd.sValue.pPort != NULL) &&
                (sEnd.sValue.pPort->value() != end_dfl);

            // Update slider if needd
            if (begin_ch)
            {
                rs->begin()->set(encode_value(sBegin.sValue.pPort, begin_dfl));
                sBegin.sValue.fValue        = begin_dfl;
            }
            if (end_ch)
            {
                rs->begin()->set(encode_value(sEnd.sValue.pPort, end_dfl));
                sEnd.sValue.fValue          = end_dfl;
            }

            submit_values(begin_dfl, end_dfl, begin_ch, end_ch, ui::PORT_USER_EDIT);
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

            if ((wWidget != NULL) && (wWidget->tag()->get() == 102))
                lsp_trace("debug");

            // Initialize configuration
            param_t * const begin   = &vParams[PT_BEGIN];
            param_t * const end     = &vParams[PT_END];
            float abs_min           = vParams[PT_MIN].fMin;
            float abs_max           = vParams[PT_MAX].fMax;
            float abs_range         = vParams[PT_RANGE].fMin;

            // Apply value constraints
            for (size_t i=PT_BEGIN; i<=PT_END; ++i)
            {
                param_t * const rp      = &vParams[i];
                rp->fValue              = lsp_xlimit(rp->fValue, rp->fMin, rp->fMax);
                rp->fValue              = lsp_xlimit(rp->fValue, abs_min, abs_max);
            }
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
            else if ((p != NULL) && (meta::is_log_rule(p)))  // Float and other values, logarithmic
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

            if ((wWidget != NULL) && (wWidget->tag()->get() == 102))
                lsp_trace("debug");

            // Initialize slider
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
            ctl::RangeSlider * const self   = static_cast<ctl::RangeSlider *>(ptr);
            const size_t * const ev_flags   = static_cast<size_t *>(data);
            size_t flags            = 0;
            if (ev_flags != NULL)
            {
                flags                   = lsp_setflag(flags, 1 << NF_END, *ev_flags & tk::RangeSlider::CHANGE_MAX);
                flags                   = lsp_setflag(flags, 1 << NF_BEGIN, *ev_flags & tk::RangeSlider::CHANGE_MIN);
            }
            else
                flags                   = 1 << NF_END;
            if (self != NULL)
                self->submit_values(flags);
            return STATUS_OK;
        }

        status_t RangeSlider::slot_begin_edit(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::RangeSlider * const self   = static_cast<ctl::RangeSlider *>(ptr);
            if (self != NULL)
            {
                if (self->sBegin.sValue.pPort != NULL)
                    self->sBegin.sValue.pPort->begin_edit();
                if (self->sEnd.sValue.pPort != NULL)
                    self->sEnd.sValue.pPort->begin_edit();
            }
            return STATUS_OK;
        }

        status_t RangeSlider::slot_end_edit(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::RangeSlider * const self   = static_cast<ctl::RangeSlider *>(ptr);
            if (self != NULL)
            {
                if (self->sBegin.sValue.pPort != NULL)
                    self->sBegin.sValue.pPort->end_edit();
                if (self->sEnd.sValue.pPort != NULL)
                    self->sEnd.sValue.pPort->end_edit();
            }
            return STATUS_OK;
        }

        status_t RangeSlider::slot_dbl_click(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::RangeSlider * const self   = static_cast<ctl::RangeSlider *>(ptr);
            if (self != NULL)
                self->set_default_values();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */






