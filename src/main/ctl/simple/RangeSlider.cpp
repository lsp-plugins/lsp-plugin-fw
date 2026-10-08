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
                {
                    sBegin.sMin.sExpr.parse(value);
                    sEnd.sMin.sExpr.parse(value);
                }
                else if (!strcmp(name, "max"))
                {
                    sBegin.sMax.sExpr.parse(value);
                    sEnd.sMax.sExpr.parse(value);
                }

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

            const bool is_log   = (nFlags & GF_LOG_SET) ? (nFlags & GF_LOG) : meta::is_log_rule(meta);

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
            else if (is_log)  // Float and other values, logarithmic
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
            const meta::port_t * const meta = (p != NULL) ? p->metadata() : NULL;
            if (meta == NULL)
                return value;

            const bool is_log       = (nFlags & GF_LOG_SET) ? (nFlags & GF_LOG) : meta::is_log_rule(meta);
            const bool has_step     = (nFlags & GF_STEP) || (meta->flags & meta::F_STEP);
            float step              = (nFlags & GF_STEP) ? fStep : (has_step) ? meta->step : 0.0f;
            const float thresh      = (meta->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;

            if (meta::is_gain_unit(meta->unit)) // Decibels
            {
                const float base        = (meta->unit == meta::U_GAIN_AMP) ? 20.0 / M_LN10 : 10.0 / M_LN10;
                step                    = base * logf((has_step) ? fStep + 1.0f : 1.01f) * 0.1f;

                value                   = (fabsf(value) < thresh) ? (base * logf(thresh) - step) : (base * logf(value));
            }
            else if (is_log)
            {
                step                    = logf((meta->flags & meta::F_STEP) ? meta->step + 1.0f : 1.01f);

                value                   = (fabsf(value) < thresh)       ? logf(thresh) - step : logf(value);
            }

            return value;
        }

        float RangeSlider::encode_range(ui::IPort *p, float range)
        {
            const meta::port_t * const meta = (p != NULL) ? p->metadata() : NULL;
            if (meta == NULL)
                return range;

            const bool is_log   = (nFlags & GF_LOG_SET) ? (nFlags & GF_LOG) : meta::is_log_rule(meta);

            if (meta::is_gain_unit(meta->unit)) // Decibels
            {
                const float base        = (meta->unit == meta::U_GAIN_AMP) ? 20.0 / M_LN10 : 10.0 / M_LN10;
                range                   = base*logf(range);
            }
            else if (is_log)
                range                   = logf(range);

            return range;
        }

        float RangeSlider::get_step(ui::IPort *p)
        {
            const meta::port_t * const meta = (p != NULL) ? p->metadata() : NULL;
            const bool has_step     = (nFlags & GF_STEP) || ((meta != NULL) && (meta->flags & meta::F_STEP));
            float step              = (nFlags & GF_STEP) ? fStep : (has_step) ? meta->step : 0.0f;

            if ((meta::is_gain_unit(meta->unit)) || (meta::is_log_rule(meta))) // Decibels
                step                   *= 10.0f;

            return step;
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
            if (v->pPort != NULL)
            {
                v->fValue           = v->pPort->value();
                return true;
            }
            if (v->sExpr.valid())
            {
                v->fValue           = v->sExpr.evaluate_float();
                return true;
            }
            return false;
        }

        void RangeSlider::end_param(param_t *p)
        {
            const meta::port_t * const meta = (p->sValue.pPort != NULL) ? p->sValue.pPort->metadata() : NULL;
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

            if ((wWidget != NULL) && (wWidget->tag()->get() == 102))
                lsp_trace("debug");

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
                (sBegin.sValue.pPort != NULL) &&
                ((new_begin != old_begin) ||
                 (new_begin != sBegin.sValue.pPort->value()));
            const bool end_ch      =
                (sEnd.sValue.pPort != NULL) &&
                ((new_end != old_end) ||
                 (new_end != sEnd.sValue.pPort->value()));

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
            const bool begin_ch     =
                (sBegin.sValue.pPort != NULL) &&
                (sBegin.sValue.pPort->value() != begin_dfl);
            const bool end_ch       =
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
                rs->end()->set(encode_value(sEnd.sValue.pPort, end_dfl));
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

        void RangeSlider::limit_param(param_t *p, float min, float max)
        {
            p->sValue.fValue    = lsp_xlimit(p->sValue.fValue, p->sMin.fValue, p->sMax.fValue);
            p->sValue.fValue    = lsp_xlimit(p->sValue.fValue, min, max);
        }

        void RangeSlider::commit_values(size_t flags)
        {
            // Ensure that widget is set
            tk::RangeSlider * const rs  = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs == NULL)
                return;

            if ((wWidget != NULL) && (wWidget->tag()->get() == 102))
                lsp_trace("debug");

            // Initialize configuration
            sBegin.sValue.fValue    = lsp_xlimit(sBegin.sValue.fValue, sBegin.sMin.fValue, sBegin.sMax.fValue);
            sEnd.sValue.fValue      = lsp_xlimit(sEnd.sValue.fValue, sEnd.sMin.fValue, sEnd.sMax.fValue);

            const float abs_min     = lsp_min(sBegin.sMin.fValue, sEnd.sMin.fValue);
            const float abs_max     = lsp_max(sBegin.sMax.fValue, sEnd.sMax.fValue);
            const float abs_range   = sRange.fValue;

            limit_param(&sBegin, abs_min, abs_max);
            limit_param(&sEnd, abs_min, abs_max);

            // Apply value constraints
            if ((sBegin.sValue.pPort != NULL) && (sEnd.sValue.pPort != NULL))
            {
                if (flags & NF_BEGIN)
                {
                    if (is_log_range(sBegin.sValue.pPort))
                        sEnd.sValue.fValue      = lsp_max(sBegin.sValue.fValue * logf(abs_range), sEnd.sValue.fValue);
                    else
                        sEnd.sValue.fValue      = lsp_max(sBegin.sValue.fValue + abs_range, sEnd.sValue.fValue);
                    limit_param(&sEnd, abs_min, abs_max);
                }
                else if (flags & NF_END)
                {
                    if (is_log_range(sBegin.sValue.pPort))
                        sBegin.sValue.fValue    = lsp_min(sEnd.sValue.fValue / logf(abs_range), sBegin.sValue.fValue);
                    else
                        sBegin.sValue.fValue    = lsp_min(sEnd.sValue.fValue - abs_range, sBegin.sValue.fValue);
                    limit_param(&sBegin, abs_min, abs_max);
                }
            }

            if ((wWidget != NULL) && (wWidget->tag()->get() == 102))
                lsp_trace("debug");

            // Set step
            ui::IPort * const refp  = (sBegin.sValue.pPort != NULL) ? sBegin.sValue.pPort : sEnd.sValue.pPort;
            const float v_step      = get_step(refp);
            rs->step()->set(v_step);

            // Initialize slider
            if (flags & (NF_BEGIN | NF_BEGIN_MIN | NF_BEGIN_MAX))
            {
                ui::IPort * const p = sBegin.sValue.pPort;
                const float v_begin = (flags & NF_BEGIN) ? encode_value(p, sBegin.sValue.fValue) : rs->begin()->get();
                const float v_min   = (flags & NF_BEGIN_MIN) ? encode_value(p, sBegin.sMin.fValue) : rs->begin()->min();
                const float v_max   = (flags & NF_BEGIN_MAX) ? encode_value(p, sBegin.sMax.fValue) : rs->begin()->max();

                rs->begin()->set_all(v_begin, v_min, v_max);
            }
            if (flags & (NF_END | NF_END_MIN | NF_END_MAX))
            {
                ui::IPort * const p = sEnd.sValue.pPort;
                const float v_end   = (flags & NF_END) ? encode_value(p, sEnd.sValue.fValue) : rs->end()->get();
                const float v_min   = (flags & NF_END_MIN) ? encode_value(p, sEnd.sMin.fValue) : rs->end()->min();
                const float v_max   = (flags & NF_END_MAX) ? encode_value(p, sEnd.sMax.fValue) : rs->end()->max();

                rs->end()->set_all(v_end, v_min, v_max);
            }

            if (flags & NF_RANGE)
            {
                const float range       = encode_range(refp, abs_range);
                rs->distance()->set(range);
            }
        }

        status_t RangeSlider::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::RangeSlider * const self   = static_cast<ctl::RangeSlider *>(ptr);
            const size_t * const ev_flags   = static_cast<size_t *>(data);
            size_t flags            = 0;
            if (ev_flags != NULL)
            {
                flags                   = lsp_setflag(flags, NF_END, *ev_flags & tk::RangeSlider::CHANGE_MAX);
                flags                   = lsp_setflag(flags, NF_BEGIN, *ev_flags & tk::RangeSlider::CHANGE_MIN);
            }
            else
                flags                   = NF_END;
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






