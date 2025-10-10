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
            fMin            = 0.0f;
            fMax            = 1.0f;
            fDistance       = 0.0f;

            for (size_t i=0; i<PT_TOTAL; ++i)
            {
                param_t *p      = &vParams[i];

                p->pPort            = NULL;
                p->nFlags           = 0;
                p->fDefault         = 0.0f;
                p->fDefaultValue    = 0.0f;
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

                sMin.init(pWrapper, this);
                sMax.init(pWrapper, this);

                // Bind slots
                rs->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
                rs->slots()->bind(tk::SLOT_MOUSE_DBL_CLICK, slot_dbl_click, this);
            }

            return STATUS_OK;
        }

        bool RangeSlider::bind_params(param_t *p, const char *prefix, const char *name, const char *value)
        {
            if (!(name = match_prefix(prefix, name)))
                return false;

            bind_port(&p->pPort, "id", name, value);

            if (!strcmp(name, "min"))
            {
                sMin.parse(value);
                p->nFlags      |= PF_MIN;
            }
            if (!strcmp(name, "max"))
            {
                sMax.parse(value);
                p->nFlags      |= PF_MAX;
            }

            if (set_value(&p->fDefault, "dfl", name, value))
                nFlags         |= PF_DFL;
            if (set_value(&p->fDefault, "default", name, value))
                nFlags         |= PF_DFL;

            return true;
        }

        void RangeSlider::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::RangeSlider *rs = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs != NULL)
            {
                bind_params(&vParams[PT_BEGIN], "start", name, value);
                bind_params(&vParams[PT_BEGIN], "begin", name, value);
                bind_params(&vParams[PT_END], "end", name, value);
                bind_params(&vParams[PT_RANGE], "range", name, value);

                set_value(&fAStep, "astep", name, value);
                set_value(&fAStep, "step.accel", name, value);
                set_value(&fDStep, "dstep", name, value);
                set_value(&fDStep, "step.decel", name, value);

                if (set_value(&fStep, "step", name, value))
                    nFlags         |= PF_STEP;

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

            size_t k_flags = 0;
            if (sMin.depends(port))
            {
                fMin        = sMin.evaluate_float();
                k_flags    |= NF_MIN | NF_VALUE;
            }
            if (sMax.depends(port))
            {
                fMax        = sMax.evaluate_float();
                k_flags    |= NF_MAX | NF_VALUE;
            }
            if (sDistance.depends(port))
            {
                fDistance   = sDistance.evaluate_float();
                k_flags    |= NF_RANGE | NF_VALUE;
            }

            for (size_t i=0; i<PT_TOTAL; ++i)
            {
                if (port == vParams[i].pPort)
                    k_flags    |= NF_VALUE;
            }

            if (k_flags)
                commit_values(k_flags);
        }

        void RangeSlider::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
            commit_values(NF_MIN | NF_MAX | NF_RANGE | NF_DFL | NF_VALUE);
        }

        void RangeSlider::submit_values()
        {
            if (nFlags & GF_CHANGING)
                return;

            tk::RangeSlider *rs = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (rs == NULL)
                return;

            nFlags |= GF_CHANGING;
            lsp_finally { nFlags &= ~GF_CHANGING; };

            float value     = rs->value()->get();
            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
            if (p == NULL)
            {
                if (pPort != NULL)
                {
                    pPort->set_value(value);
                    pPort->notify_all(ui::PORT_USER_EDIT);
                }
                return;
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
            else if (nFlags & FF_LOG)  // Float and other values, logarithmic
            {
                double thresh   = (p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                value           = exp(value);
                float min       = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                if ((min <= 0.0f) && (value < thresh))
                    value           = 0.0f;
            }

            if (pPort != NULL)
            {
                pPort->set_value(value);
                pPort->notify_all(ui::PORT_USER_EDIT);
            }
        }

        void RangeSlider::set_default_value()
        {
            if (nFlags & GF_CHANGING)
                return;

            tk::RangeSlider *fdr = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (fdr == NULL)
                return;

            nFlags |= GF_CHANGING;
            lsp_finally { nFlags &= ~GF_CHANGING; };

            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
            float dfl   = (pPort != NULL) ? pPort->default_value() : fDefaultValue;
            float value = dfl;

            if (p != NULL)
            {
                if (is_gain_unit(p->unit)) // Decibels
                {
                    double base = (p->unit == meta::U_GAIN_AMP) ? 20.0 / M_LN10 : 10.0 / M_LN10;

                    if (value < GAIN_AMP_M_120_DB)
                        value           = GAIN_AMP_M_120_DB;

                    value = base * log(value);
                }
                else if (nFlags & FF_LOG)
                {
                    if (value < GAIN_AMP_M_120_DB)
                        value           = GAIN_AMP_M_120_DB;
                    value = log(value);
                }
            }

            fdr->value()->set(value);
            if (pPort != NULL)
            {
                pPort->set_value(dfl);
                pPort->notify_all(ui::PORT_USER_EDIT);
            }
        }

        void RangeSlider::commit_values(size_t flags)
        {
            if (nFlags & GF_CHANGING)
                return;

            // Ensure that widget is set
            tk::RangeSlider *fdr = tk::widget_cast<tk::RangeSlider>(wWidget);
            if (fdr == NULL)
                return;

            nFlags |= GF_CHANGING;
            lsp_finally { nFlags &= ~GF_CHANGING; };

            // Ensure that port is set
            meta::port_t xp;

            xp.id           = NULL;
            xp.name         = NULL;
            xp.unit         = meta::U_NONE;
            xp.role         = meta::R_CONTROL;
            xp.flags        = meta::F_LOWER | meta::F_UPPER | meta::F_STEP;
            xp.min          = 0.0f;
            xp.max          = 1.0f;
            xp.start        = 0.0f;
            xp.step         = 0.01f;
            xp.items        = NULL;
            xp.members      = NULL;

            // Ensure that port is set
            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
            if (p != NULL)
                xp          = *p;
            p           = &xp;

            // Override default values
            if (nFlags & FF_MIN)
            {
                xp.min          = sMin.evaluate_float();
                xp.flags       |= meta::F_LOWER;
            }
            if (nFlags & FF_MAX)
            {
                xp.max          = sMax.evaluate_float();
                xp.flags       |= meta::F_UPPER;
            }
            if (nFlags & FF_STEP)
                xp.step         = fStep;
            if (nFlags & FF_DFL)
                xp.start        = fDefault;
            if (nFlags & FF_LOG_SET)
                xp.flags        = lsp_setflag(xp.flags, meta::F_LOG, nFlags & FF_LOG);
            else
                nFlags          = lsp_setflag(nFlags, FF_LOG, xp.flags & meta::F_LOG);

            float min = 0.0f, max = 1.0f, step = 0.01f, balance = 0.0f;
            float value     = (pPort != NULL) ? pPort->value() : xp.start;

            if (meta::is_gain_unit(p->unit)) // Decibels
            {
                float base      = (p->unit == meta::U_GAIN_AMP) ? 20.0f / M_LN10 : 10.0f / M_LN10;

                min             = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                max             = (p->flags & meta::F_UPPER) ? p->max : GAIN_AMP_P_12_DB;
                float dfl       = (nFlags & FF_BAL_SET) ? fBalance : min;

                step            = base * logf((p->flags & meta::F_STEP) ? p->step + 1.0f : 1.01f) * 0.1f;
                float thresh    = ((p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);

                min             = (fabsf(min) < thresh)     ? (base * logf(thresh) - step) : (base * logf(min));
                max             = (fabsf(max) < thresh)     ? (base * logf(thresh) - step) : (base * logf(max));
                float db_dfl    = (fabsf(max) < thresh)     ? (base * logf(thresh) - step) : (base * logf(dfl));
                value           = (fabsf(value) < thresh)   ? (base * logf(thresh) - step) : (base * logf(value));
                balance         = lsp_xlimit(db_dfl, min, max);

                step           *= 10.0f;
                fDefaultValue   = base * log(p->start);
            }
            else if (meta::is_discrete_unit(p->unit)) // Integer type
            {
                min             = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                max             = (p->unit == meta::U_ENUM) ? min + meta::list_size(p->items) - 1.0f :
                                  (p->flags & meta::F_UPPER) ? p->max : 1.0f;
                float dfl       = (nFlags & FF_BAL_SET) ? fBalance : p->min;
                balance         = lsp_xlimit(dfl, min, max);
                value           = lsp_xlimit(value, min, max);
                ssize_t istep   = (p->flags & meta::F_STEP) ? p->step : 1;

                step            = (istep == 0) ? 1.0f : istep;
                fDefaultValue   = p->start;
            }
            else if (meta::is_log_rule(p))  // Float and other values, logarithmic
            {
                float xmin      = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                float xmax      = (p->flags & meta::F_UPPER) ? p->max : GAIN_AMP_P_12_DB;
                float xdfl      = (nFlags & FF_BAL_SET) ? fBalance : min;
                float thresh    = ((p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);

                step            = logf((p->flags & meta::F_STEP) ? p->step + 1.0f : 1.01f);
                min             = (fabsf(xmin) < thresh)    ? logf(thresh) - step : logf(xmin);
                max             = (fabsf(xmax) < thresh)    ? logf(thresh) - step : logf(xmax);
                float dfl       = (fabsf(xdfl) < thresh)    ? logf(thresh) - step : logf(xdfl);
                value           = (fabsf(value) < thresh)   ? logf(thresh) - step : logf(value);
                balance         = lsp_xlimit(dfl, min, max);

                step           *= 10.0f;
                fDefaultValue   = logf(p->start);
            }
            else // Float and other values, non-logarithmic
            {
                min             = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                max             = (p->flags & meta::F_UPPER) ? p->max : 1.0f;
                float dfl       = (nFlags & FF_BAL_SET) ? fBalance : min;
                balance         = lsp_xlimit(dfl, min, max);
                value           = lsp_xlimit(value, min, max);

                step            = (p->flags & meta::F_STEP) ? p->step * 10.0f : (max - min) * 0.1f;
                fDefaultValue   = p->start;
            }

            // Initialize fader
            if (flags & FF_MIN)
                fdr->value()->set_min(min);
            if (flags & FF_MAX)
                fdr->value()->set_max(max);
            if (flags & FF_VALUE)
                fdr->value()->set((flags & FF_DFL) ? fDefaultValue : value);
            fdr->step()->set(step);
            fdr->balance()->set(balance);
        }

        status_t RangeSlider::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::RangeSlider *_this   = static_cast<ctl::RangeSlider *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

        status_t RangeSlider::slot_dbl_click(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::RangeSlider *_this   = static_cast<ctl::RangeSlider *>(ptr);
            if (_this != NULL)
                _this->set_default_value();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */






