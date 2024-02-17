/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 мая 2021 г.
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
        CTL_FACTORY_IMPL_START(Knob)
            status_t res;
            if (!name->equals_ascii("knob"))
                return STATUS_NOT_FOUND;

            tk::Knob *w = new tk::Knob(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Knob *wc  = new ctl::Knob(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Knob)


        //-----------------------------------------------------------------
        const ctl_class_t Knob::metadata = { "Knob", &Widget::metadata };

        Knob::Knob(ui::IWrapper *wrapper, tk::Knob *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            pScaleEnablePort= NULL;

            nFlags          = 0;
            fDefault        = 0.0f;
            fStep           = 1.0f;
            fAStep          = 10.0f;
            fDStep          = 0.1f;
            fBalance        = 0.0f;

            fDefaultValue   = 0.0f;
        }

        Knob::~Knob()
        {
        }

        status_t Knob::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Knob *knob = tk::widget_cast<tk::Knob>(wWidget);
            if (knob != NULL)
            {
                // Initialize color controllers
                sColor.init(pWrapper, knob->color());
                sScaleColor.init(pWrapper, knob->scale_color());
                sBalanceColor.init(pWrapper, knob->balance_color());
                sHoleColor.init(pWrapper, knob->hole_color());
                sTipColor.init(pWrapper, knob->tip_color());
                sBalanceTipColor.init(pWrapper, knob->balance_tip_color());
                sMeterColor.init(pWrapper, knob->meter_color());
                sMeterVisible.init(pWrapper, knob->meter_active());
                sEditable.init(pWrapper, knob->editable());

                sMin.init(pWrapper, this);
                sMax.init(pWrapper, this);
                sMeterMin.init(pWrapper, this);
                sMeterMax.init(pWrapper, this);

                // Bind slots
                knob->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
                knob->slots()->bind(tk::SLOT_MOUSE_DBL_CLICK, slot_dbl_click, this);

                // Set value
                pScaleEnablePort = pWrapper->port(UI_ENABLE_KNOB_SCALE_ACTIONS_PORT);
                if (pScaleEnablePort != NULL)
                    pScaleEnablePort->bind(this);
            }

            return STATUS_OK;
        }

        void Knob::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Knob *knob = tk::widget_cast<tk::Knob>(wWidget);
            if (knob != NULL)
            {
                bind_port(&pPort, "id", name, value);
                bind_port(&pScaleEnablePort, "scale.active.id", name, value);

                sColor.set("color", name, value);
                sScaleColor.set("scolor", name, value);
                sScaleColor.set("scale.color", name, value);
                sBalanceColor.set("bcolor", name, value);
                sBalanceColor.set("balance.color", name, value);
                sHoleColor.set("hcolor", name, value);
                sHoleColor.set("hole.color", name, value);
                sTipColor.set("tcolor", name, value);
                sTipColor.set("tip.color", name, value);
                sBalanceTipColor.set("btcolor", name, value);
                sBalanceTipColor.set("balance.tip.color", name, value);
                sMeterColor.set("meter.color", name, value);
                sMeterColor.set("mcolor", name, value);

                set_expr(&sMeterMin, "meter.min", name, value);
                set_expr(&sMeterMin, "mmin", name, value);
                set_expr(&sMeterMax, "meter.max", name, value);
                set_expr(&sMeterMax, "mmax", name, value);

                sMeterVisible.set("meter.visibility", name, value);
                sMeterVisible.set("meter.v", name, value);
                sMeterVisible.set("mvisibility", name, value);

                sEditable.set("editable", name, value);

                if (!strcmp(name, "min"))
                {
                    sMin.parse(value);
                    nFlags     |= KF_MIN;
                }
                if (!strcmp(name, "max"))
                {
                    sMax.parse(value);
                    nFlags     |= KF_MAX;
                }
                if (set_value(&fStep, "step", name, value))
                    nFlags     |= KF_STEP;

                if (set_value(&fStep, "dfl", name, value))
                    nFlags     |= KF_DFL;
                if (set_value(&fStep, "default", name, value))
                    nFlags     |= KF_DFL;

                if (set_value(&fAStep, "astep", name, value))
                    nFlags     |= KF_ASTEP;
                if (set_value(&fAStep, "step.accel", name, value))
                    nFlags     |= KF_ASTEP;
                if (set_value(&fDStep, "dstep", name, value))
                    nFlags     |= KF_DSTEP;
                if (set_value(&fDStep, "step.decel", name, value))
                    nFlags     |= KF_DSTEP;

                if (set_value(&fBalance, "bal", name, value))
                    nFlags     |= KF_BAL_SET;
                else if (set_value(&fBalance, "balance", name, value))
                    nFlags     |= KF_BAL_SET;

                bool log = false;
                if (set_value(&log, "log", name, value))
                    nFlags      = lsp_setflag(nFlags, KF_LOG, log) | KF_LOG_SET;
                else if (set_value(&log, "logarithmic", name, value))
                    nFlags      = lsp_setflag(nFlags, KF_LOG, log) | KF_LOG_SET;

                bool cycling = false;
                if (set_value(&cycling, "cycling", name, value))
                    nFlags      = lsp_setflag(nFlags, KF_CYCLIC, cycling) | KF_CYCLIC_SET;

                set_size_range(knob->size(), "size", name, value);

                set_param(knob->scale(), "scale.size", name, value);
                set_param(knob->scale(), "ssize", name, value);
                set_param(knob->balance_color_custom(), "bcolor.custom", name, value);
                set_param(knob->balance_color_custom(), "balance.color.custom", name, value);
                set_param(knob->flat(), "flat", name, value);
                set_param(knob->scale_marks(), "smarks", name, value);
                set_param(knob->scale_marks(), "scale.marks", name, value);
                set_param(knob->hole_size(), "hole.size", name, value);
                set_param(knob->gap_size(), "gap.size", name, value);
                set_param(knob->balance_tip_size(), "balance.tip.size", name, value);
                set_param(knob->balance_tip_size(), "btsize", name, value);

                set_param(knob->scale_brightness(), "scale.brightness", name, value);
                set_param(knob->scale_brightness(), "scale.bright", name, value);
                set_param(knob->scale_brightness(), "sbrightness", name, value);
                set_param(knob->scale_brightness(), "sbright", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Knob::submit_value()
        {
            tk::Knob *knob  = tk::widget_cast<tk::Knob>(wWidget);
            if (knob == NULL)
                return;

            float value     = knob->value()->get();
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
                double base     = (p->unit == meta::U_GAIN_AMP) ? M_LN10 * 0.05 : M_LN10 * 0.1;
                double thresh   = (p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                value           = exp(value * base);
                if (value < thresh)
                    value           = 0.0f;
            }
            else if (is_discrete_unit(p->unit)) // Integer type
            {
                value          = truncf(value);
            }
            else if (nFlags & KF_LOG)  // Float and other values, logarithmic
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

        void Knob::set_default_value()
        {
            tk::Knob *knob = tk::widget_cast<tk::Knob>(wWidget);
            if ((knob == NULL) || (pPort == NULL))
                return;

            pPort->set_default();
            pPort->notify_all(ui::PORT_USER_EDIT);
        }

        void Knob::sync_scale_state()
        {
            bool scale_enable = pScaleEnablePort->value() >= 0.5f;
            tk::Knob *knob    = tk::widget_cast<tk::Knob>(wWidget);
            if (knob != NULL)
                knob->scale_active()->set(scale_enable);
        }

        void Knob::commit_value(size_t flags)
        {
            tk::Knob *knob = tk::widget_cast<tk::Knob>(wWidget);
            if (knob == NULL)
                return;

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

            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
            if (p != NULL)
                xp          = *p;
            p           = &xp;

            // Override default values
            if (nFlags & KF_MIN)
            {
                xp.min          = sMin.evaluate_float();
                xp.flags       |= meta::F_LOWER;
            }
            if (nFlags & KF_MAX)
            {
                xp.max          = sMax.evaluate_float();
                xp.flags       |= meta::F_UPPER;
            }
            if (nFlags & KF_STEP)
            {
                xp.step         = fStep;
                xp.flags       |= meta::F_STEP;
            }
            if (nFlags & KF_DFL)
                xp.start        = fDefault;
            if (nFlags & KF_CYCLIC_SET)
                xp.flags        = lsp_setflag(xp.flags, meta::F_CYCLIC, nFlags & KF_CYCLIC);
            if (nFlags & KF_LOG_SET)
                xp.flags        = lsp_setflag(xp.flags, meta::F_LOG, nFlags & KF_LOG);
            else
                nFlags          = lsp_setflag(nFlags, KF_LOG, xp.flags & meta::F_LOG);

            float min = 0.0f, max = 1.0f, step = 0.01f, balance = 0.0f;
            float value     = (pPort != NULL) ? pPort->value() : xp.start;
            float mtr_min = 0.0f, mtr_max = 1.0f;

            if (meta::is_gain_unit(p->unit)) // Gain
            {
                double base     = (p->unit == meta::U_GAIN_AMP) ? 20.0 / M_LN10 : 10.0 / M_LN10;

                min             = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                max             = (p->flags & meta::F_UPPER) ? p->max : GAIN_AMP_P_12_DB;
                float dfl       = (nFlags & KF_BAL_SET) ? fBalance : min;
                mtr_min         = (sMeterMin.valid()) ? sMeterMin.evaluate_float() : min;
                mtr_max         = (sMeterMax.valid()) ? sMeterMax.evaluate_float() : min;

                step            = base * log((p->flags & meta::F_STEP) ? p->step + 1.0f : 1.01f) * 0.1f;
                double thresh   = ((p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);

                min             = (fabs(min)   < thresh)    ? (base * log(thresh) - step) : (base * log(min));
                max             = (fabs(max)   < thresh)    ? (base * log(thresh) - step) : (base * log(max));
                double db_dfl   = (fabs(dfl)   < thresh)    ? (base * log(thresh) - step) : (base * log(dfl));
                value           = (fabs(value) < thresh)    ? (base * log(thresh) - step) : (base * log(value));
                mtr_min         = (fabs(mtr_min) < thresh)  ? (base * log(thresh) - step) : (base * log(mtr_min));
                mtr_max         = (fabs(mtr_max) < thresh)  ? (base * log(thresh) - step) : (base * log(mtr_max));

                balance         = lsp_xlimit(db_dfl, min, max);
                value           = lsp_xlimit(value, min, max);
                mtr_min         = lsp_xlimit(mtr_min, min, max);
                mtr_max         = lsp_xlimit(mtr_max, min, max);

                step           *= 10.0f;
                fDefaultValue   = base * log(p->start);
            }
            else if (meta::is_discrete_unit(p->unit)) // Integer type
            {
                min             = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                max             = (p->unit == meta::U_ENUM) ? min + meta::list_size(p->items) - 1.0f :
                                  (p->flags & meta::F_UPPER) ? p->max : 1.0f;
                float dfl       = (nFlags & KF_BAL_SET) ? fBalance : p->min;
                mtr_min         = (sMeterMin.valid()) ? sMeterMin.evaluate_float() : min;
                mtr_max         = (sMeterMax.valid()) ? sMeterMax.evaluate_float() : min;

                balance         = lsp_xlimit(dfl, min, max);
                value           = lsp_xlimit(value, min, max);
                mtr_min         = lsp_xlimit(mtr_min, min, max);
                mtr_max         = lsp_xlimit(mtr_max, min, max);
                ssize_t istep   = (p->flags & meta::F_STEP) ? p->step : 1;

                step            = (istep == 0) ? 1.0f : istep;
                fDefaultValue   = p->start;
            }
            else if (meta::is_log_rule(p))  // Float and other values, logarithmic
            {
                float xmin      = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                float xmax      = (p->flags & meta::F_UPPER) ? p->max : GAIN_AMP_P_12_DB;
                float xdfl      = (nFlags & KF_BAL_SET) ? fBalance : min;
                float xmtr_min  = (sMeterMin.valid()) ? sMeterMin.evaluate_float() : xmin;
                float xmtr_max  = (sMeterMax.valid()) ? sMeterMax.evaluate_float() : xmin;
                float thresh    = ((p->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB);

                step            = log((p->flags & meta::F_STEP) ? p->step + 1.0f : 1.01f);
                min             = (fabs(xmin) < thresh)     ? log(thresh) - step : log(xmin);
                max             = (fabs(xmax) < thresh)     ? log(thresh) - step : log(xmax);
                float dfl       = (fabs(xdfl) < thresh)     ? log(thresh) - step : log(xdfl);
                value           = (fabs(value) < thresh)    ? log(thresh) - step : log(value);
                mtr_min         = (fabs(xmtr_min) < thresh) ? log(thresh) - step : log(xmtr_min);
                mtr_max         = (fabs(xmtr_max) < thresh) ? log(thresh) - step : log(xmtr_max);

                balance         = lsp_xlimit(dfl, min, max);
                value           = lsp_xlimit(value, min, max);

                step           *= 10.0f;
                fDefaultValue   = log(p->start);
            }
            else // Float and other values, non-logarithmic
            {
                min             = (p->flags & meta::F_LOWER) ? p->min : 0.0f;
                max             = (p->flags & meta::F_UPPER) ? p->max : 1.0f;
                float dfl       = (nFlags & KF_BAL_SET) ? fBalance : min;
                mtr_min         = (sMeterMin.valid()) ? sMeterMin.evaluate_float() : min;
                mtr_max         = (sMeterMax.valid()) ? sMeterMax.evaluate_float() : min;

                balance         = lsp_xlimit(dfl, min, max);
                value           = lsp_xlimit(value, min, max);
                mtr_min         = lsp_xlimit(mtr_min, min, max);
                mtr_max         = lsp_xlimit(mtr_max, min, max);

                step            = (p->flags & meta::F_STEP) ? p->step * 10.0f : (max - min) * 0.1f;
                fDefaultValue   = p->start;
            }

            // Initialize knob
            knob->cycling()->set(p->flags & meta::F_CYCLIC);
            if (flags & KF_MIN)
                knob->value()->set_min(min);
            if (flags & KF_MAX)
                knob->value()->set_max(max);
            if (flags & KF_VALUE)
                knob->value()->set((flags & KF_DFL) ? fDefaultValue : value);
            knob->meter_min()->set(mtr_min);
            knob->meter_max()->set(mtr_max);
            knob->step()->set(step);
            knob->balance()->set(balance);

            if (nFlags & KF_ASTEP)
                knob->step()->set_accel(fAStep);
            if (nFlags & KF_DSTEP)
                knob->step()->set_decel(fDStep);
        }

        void Knob::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);
            size_t k_flags = 0;

            if (sMin.depends(port))
                k_flags    |= KF_MIN;
            if (sMax.depends(port))
                k_flags    |= KF_MAX;
            if (sMeterMin.depends(port))
                k_flags    |= KF_METER_MIN;
            if (sMeterMax.depends(port))
                k_flags    |= KF_METER_MAX;

            if (pPort != NULL)
            {
                if (port == pPort)
                    k_flags    |= KF_VALUE;
            }

            if (k_flags)
                commit_value(k_flags);

            sync_scale_state();
        }

        void Knob::end(ui::UIContext *ctx)
        {
            // Call parent controller
            Widget::end(ctx);
            commit_value(KF_MIN | KF_MAX | KF_VALUE | KF_DFL);
            sync_scale_state();
        }

        status_t Knob::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Knob *_this    = static_cast<ctl::Knob *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

        status_t Knob::slot_dbl_click(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Knob *_this    = static_cast<ctl::Knob *>(ptr);
            if (_this != NULL)
                _this->set_default_value();
            return STATUS_OK;
        }
    } /* namespace ctl */
} /* namespace lsp */
