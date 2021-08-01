/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 авг. 2021 г.
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
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/stdlib/stdio.h>

#define METER_ATT       0.1f
#define METER_REL       0.25f

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(LedChannel)
            status_t res;

            if (!name->equals_ascii("ledchannel"))
                return STATUS_NOT_FOUND;

            tk::LedMeterChannel *w = new tk::LedMeterChannel(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::LedChannel *wc  = new ctl::LedChannel(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(LedChannel)

        //-----------------------------------------------------------------
        const ctl_class_t LedChannel::metadata          = { "LedChannel", &Widget::metadata };

        LedChannel::LedChannel(ui::IWrapper *wrapper, tk::LedMeterChannel *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
            pPort           = NULL;
            nFlags          = 0;
            nType           = MT_PEAK;
            fMin            = 0.0f;
            fMax            = 0.0f;
            fBalance        = 0.0f;
            fValue          = 0.0f;
            fRms            = 0.0f;
            fReport         = 0.0f;
        }

        LedChannel::~LedChannel()
        {
        }

        status_t LedChannel::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::LedMeterChannel *lmc = tk::widget_cast<tk::LedMeterChannel>(wWidget);
            if (lmc != NULL)
            {
                sActivity.init(pWrapper, lmc->active());
                sReversive.init(pWrapper, lmc->reversive());
                sPeakVisible.init(pWrapper, lmc->peak_visible());
                sBalanceVisible.init(pWrapper, lmc->balance_visible());
                sTextVisible.init(pWrapper, lmc->text_visible());
//                LSP_TK_PROPERTY(RangeFloat,         value,              &sValue)
//                LSP_TK_PROPERTY(Float,              peak,               &sPeak)
//                LSP_TK_PROPERTY(Float,              balance,            &sBalance)
//
//                LSP_TK_PROPERTY(Color,              color,              &sColor)
//                LSP_TK_PROPERTY(Color,              value_color,        &sValueColor)
//                LSP_TK_PROPERTY(Color,              peak_color,         &sPeakColor)
//                LSP_TK_PROPERTY(Color,              text_color,         &sTextColor)
//                LSP_TK_PROPERTY(Color,              balance_color,      &sBalanceColor)
//
//                LSP_TK_PROPERTY(ColorRanges,        text_ranges,        &sTextRanges)
//                LSP_TK_PROPERTY(ColorRanges,        value_ranges,       &sValueRanges)
//                LSP_TK_PROPERTY(ColorRanges,        peak_ranges,        &sPeakRanges)
//
//                LSP_TK_PROPERTY(String,             text,               &sText)
//                LSP_TK_PROPERTY(String,             estimation_text,    &sEstText)

                sTimer.bind(lmc->display());
                sTimer.set_handler(update_meter, this);
            }

            return STATUS_OK;
        }

        void LedChannel::destroy()
        {
            // Cancel timer
            sTimer.cancel();
        }

        void LedChannel::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::LedMeterChannel *lmc = tk::widget_cast<tk::LedMeterChannel>(wWidget);
            if (lmc != NULL)
            {
                sActivity.set("activity", name, value);
                sActivity.set("active", name, value);
                sReversive.set("reversive", name, value);
                sPeakVisible.set("peak.visibility", name, value);
                sBalanceVisible.set("balance.visibility", name, value);
                sTextVisible.set("text.visibility", name, value);

                set_constraints(lmc->constraints(), name, value);
                set_font(lmc->font(), "font", name, value);

                set_param(lmc->min_segments(), "segments.min", name, value);
                set_param(lmc->min_segments(), "segmin", name, value);
                set_param(lmc->border(), "border", name, value);
                set_param(lmc->angle(), "angle", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void LedChannel::end(ui::UIContext *ctx)
        {
            sync_channel();
        }

        void LedChannel::notify(ui::IPort *port)
        {
            Widget::notify(port);

            tk::LedMeterChannel *lmc = tk::widget_cast<tk::LedMeterChannel>(wWidget);
            if (lmc != NULL)
            {
                if ((port != NULL) && (port == pPort))
                    fReport     = port->value();
            }
        }

        void LedChannel::schema_reloaded()
        {
            Widget::schema_reloaded();

            // TODO: reload colors

            sync_channel();
        }

        void LedChannel::update_peaks(ws::timestamp_t ts)
        {
            tk::LedMeterChannel *lmc = tk::widget_cast<tk::LedMeterChannel>(wWidget);
            if (lmc == NULL)
                return;

            float v         = fReport;
            float av        = fabs(v);

            // Peak value
            if (nFlags & MF_BALANCE)
            {
                if (v > fBalance)
                    fValue      = (fValue <= v) ? v : fValue + METER_REL * (v - fValue);
                else
                    fValue      = (fValue > v) ? v : fValue + METER_REL * (v - fValue);
            }
            else
                fValue      = (fValue < v) ? v : fValue + METER_REL * (v - fValue);

            fRms        += (av > fRms) ? METER_ATT * (av - fRms) :  METER_REL * (av - fRms);

            // Limit RMS value
            if (fRms < 0.0f)
                fRms        = 0.0f;

            // Update meter
            if (nType == MT_RMS_PEAK)
            {
                lmc->peak()->set(calc_value(fValue));
                lmc->value()->set(calc_value(fRms));
                set_meter_text(lmc, fRms);
            }
            else
            {
                lmc->value()->set(calc_value(fRms));
                set_meter_text(lmc, fValue);
            }
        }

        void LedChannel::set_meter_text(tk::LedMeterChannel *lmc, float value)
        {
            float avalue = fabs(value);
            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;

            // Update the value
            if ((p != NULL) && (meta::is_decibel_unit(p->unit)))
            {
                if (avalue >= GAIN_AMP_MAX)
                {
                    lmc->text()->set_raw("+inf");
                    return;
                }
                else if (avalue < GAIN_AMP_MIN)
                {
                    lmc->text()->set_raw("-inf");
                    return;
                }

                value       = logf(fabs(value)) * ((p->unit == meta::U_GAIN_POW) ? 10.0f : 20.0f) / M_LN10;
                avalue      = fabs(value);
            }

            // Now we are able to format values
            char buf[40];

            if (isnan(avalue))
                strcpy(buf, "nan");
            else if (avalue < 10.0f)
                ::snprintf(buf, sizeof(buf), "%.2f", value);
            else if (avalue < 100.0f)
                ::snprintf(buf, sizeof(buf), "%.1f", value);
            else
                ::snprintf(buf, sizeof(buf), "%ld", long(value));
            buf[sizeof(buf)-1] = '\0';

            // Update text of the meter
            lmc->text()->set_raw(buf);
        }

        float LedChannel::calc_value(float value)
        {
            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
            if (p == NULL)
                return 0.0f;

            bool xlog = (nFlags & MF_LOG_SET) && (nFlags & MF_LOG);

            if (!xlog)
                xlog = meta::is_decibel_unit(p->unit) || (p->flags & meta::F_LOG);

            if ((xlog) && (value < GAIN_AMP_M_120_DB))
                value   = GAIN_AMP_M_120_DB;

            float mul = (p->unit == meta::U_GAIN_AMP) ? 20.0f/M_LN10 :
                        (p->unit == meta::U_GAIN_POW) ? 10.0f/M_LN10 :
                        1.0f;

            return (xlog) ? mul * logf(fabs(value)) : value;
        }

        void LedChannel::sync_channel()
        {
            // TODO
        }

        status_t LedChannel::update_meter(ws::timestamp_t sched, ws::timestamp_t time, void *arg)
        {
            if (arg == NULL)
                return STATUS_OK;
            LedChannel *_this = static_cast<LedChannel *>(arg);
            _this->update_peaks(time);

            return STATUS_OK;
        }
    }
}


