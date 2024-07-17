/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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

        LedChannel::LedChannel(ui::IWrapper *wrapper, tk::LedMeterChannel *widget):
            Widget(wrapper, widget),
            sPropValueColor(&sProperties),
            sPropYellowZoneColor(&sProperties),
            sPropRedZoneColor(&sProperties)
        {
            pClass          = &metadata;

            pParent         = NULL;
            pPort           = NULL;
            nFlags          = 0;
            nType           = MT_PEAK;
            fMin            = 0.0f;
            fMax            = 0.0f;
            fBalance        = 0.0f;
            fValue          = 0.0f;
            fMaxValue       = 0.0f;
            fRms            = 0.0f;
            fReport         = 0.0f;
            fAttack         = 0.1f;
            fRelease        = 0.25f;
            bLog            = false;
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
                sHeaderVisible.init(pWrapper, lmc->header_visible());

                sPropValueColor.bind("normal.color", lmc->style());
                sPropYellowZoneColor.bind("yellow.color", lmc->style());
                sPropRedZoneColor.bind("red.color", lmc->style());

                sPropValueColor.set("meter_normal");
                sPropYellowZoneColor.set("meter_yellow");
                sPropRedZoneColor.set("meter_red");

                sColor.init(pWrapper, lmc->color());
                sValueColor.init(pWrapper, &sPropValueColor);
                sYellowZoneColor.init(pWrapper, &sPropYellowZoneColor);
                sRedZoneColor.init(pWrapper, &sPropRedZoneColor);
                sBalanceColor.init(pWrapper, lmc->balance_color());

                sTimer.bind(lmc->display());
                sTimer.set_handler(update_meter, this);

                lmc->slots()->bind(tk::SLOT_SHOW, slot_show, this);
                lmc->slots()->bind(tk::SLOT_HIDE, slot_hide, this);
                lmc->slots()->bind(tk::SLOT_MOUSE_CLICK, slot_mouse_click, this);
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
                bind_port(&pPort, "id", name, value);

                sActivity.set("activity", name, value);
                sActivity.set("active", name, value);
                sReversive.set("reversive", name, value);
                sPeakVisible.set("peak.visibility", name, value);
                sBalanceVisible.set("balance.visibility", name, value);
                sTextVisible.set("text.visibility", name, value);
                sHeaderVisible.set("header.visibility", name, value);

                sColor.set("color", name, value);
                sValueColor.set("value.color", name, value);
                sYellowZoneColor.set("yellow.color", name, value);
                sRedZoneColor.set("red.color", name, value);
                sBalanceColor.set("balance.color", name, value);
                sBalanceColor.set("bal.color", name, value);

                set_constraints(lmc->constraints(), name, value);
                set_font(lmc->font(), "font", name, value);

                set_param(lmc->min_segments(), "segments.min", name, value);
                set_param(lmc->min_segments(), "segmin", name, value);
                set_param(lmc->border(), "border", name, value);
                set_param(lmc->angle(), "angle", name, value);
                set_param(lmc->reversive(), "reversive", name, value);
                set_param(lmc->reversive(), "rev", name, value);

                set_value(&fAttack, "attack", name, value);
                set_value(&fAttack, "att", name, value);
                set_value(&fRelease, "release", name, value);
                set_value(&fRelease, "rel", name, value);

                if (set_value(&fMin, "min", name, value))
                    nFlags     |= MF_MIN;
                if (set_value(&fMax, "max", name, value))
                    nFlags     |= MF_MAX;
                if (set_value(&fBalance, "balance", name, value))
                    nFlags     |= MF_BALANCE;
                if (set_value(&bLog, "logarithmic", name, value))
                    nFlags     |= MF_LOG;
                if (set_value(&bLog, "log", name, value))
                    nFlags     |= MF_LOG;

                // Set type of led channel
                if (!strcmp(name, "type"))
                {
                    if (!strcasecmp(value, "peak"))
                        nType       = MT_PEAK;
                    else if (!strcasecmp(value, "rms_peak"))
                        nType       = MT_RMS_PEAK;
                    else if (!strcasecmp(value, "vu"))
                        nType       = MT_VU;
                    else if (!strcasecmp(value, "vumeter"))
                        nType       = MT_VU;
                }
            }

            return Widget::set(ctx, name, value);
        }

        void LedChannel::end(ui::UIContext *ctx)
        {
            sync_channel();
            sync_colors();
        }

        void LedChannel::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            tk::LedMeterChannel *lmc = tk::widget_cast<tk::LedMeterChannel>(wWidget);
            if (lmc != NULL)
            {
                if ((port != NULL) && (port == pPort))
                    fReport     = port->value();
            }
        }

        void LedChannel::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);
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
                    fValue      = (fValue <= v) ? v : fValue + fRelease * (v - fValue);
                else
                    fValue      = (fValue > v) ? v : fValue + fRelease * (v - fValue);
            }
            else
                fValue      = (fValue < v) ? v : fValue + fRelease * (v - fValue);

            fRms        += (av > fRms) ? fAttack * (av - fRms) :  fRelease * (av - fRms);

            // Limit RMS value
            if (fRms < 0.0f)
                fRms        = 0.0f;

            // Update meter
            fMaxValue           = lsp_max(fMaxValue, fValue);
            const float value   = calc_value(fValue);

            if (nType == MT_RMS_PEAK)
            {
                lmc->peak()->set(value);
                lmc->value()->set(calc_value(fRms));
                set_meter_text(lmc->text(), fRms);
            }
            else
            {
                lmc->value()->set(calc_value(fValue));
                set_meter_text(lmc->text(), fValue);
            }

            lmc->header_value()->set(calc_value(fMaxValue));
            set_meter_text(lmc->header(), fMaxValue);
        }

        void LedChannel::set_meter_text(tk::String *dst, float value)
        {
            float avalue = fabs(value);
            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;

            // Update the value
            if ((p != NULL) && (meta::is_decibel_unit(p->unit)))
            {
                if (avalue >= GAIN_AMP_MAX)
                {
                    dst->set_raw("+inf");
                    return;
                }
                else if (avalue < GAIN_AMP_MIN)
                {
                    dst->set_raw("-inf");
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
            dst->set_raw(buf);
        }

        float LedChannel::calc_value(float value)
        {
            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
            if (p == NULL)
                return 0.0f;

            bool xlog = (nFlags & MF_LOG) && (bLog);
            if (!xlog)
                xlog = meta::is_log_rule(p);

            if ((xlog) && (value < GAIN_AMP_M_120_DB))
                value   = GAIN_AMP_M_120_DB;

            float mul = (p->unit == meta::U_GAIN_AMP) ? 20.0f/M_LN10 :
                        (p->unit == meta::U_GAIN_POW) ? 10.0f/M_LN10 :
                        1.0f;

            return (xlog) ? mul * logf(fabs(value)) : value;
        }

        void LedChannel::sync_channel()
        {
            tk::LedMeterChannel *lmc = tk::widget_cast<tk::LedMeterChannel>(wWidget);
            if (lmc == NULL)
                return;

            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;

            float min, max, balance, value;

            // Calculate minimum
            if ((p != NULL) && (nFlags & MF_MIN))
                min = calc_value(fMin);
            else if ((p != NULL) && (p->flags & meta::F_LOWER))
                min = calc_value(p->min);
            else
                min = 0.0f;

            // Calculate maximum
            if ((p != NULL) && (nFlags & MF_MAX))
                max = calc_value(fMax);
            else if ((p != NULL) && (p->flags & meta::F_UPPER))
                max = calc_value(p->max);
            else
                max = 1.0f;

            // Calculate balance
            if (pPort != NULL)
                fValue      = pPort->value();
            fReport     = fValue;

            if (nFlags & MF_BALANCE)
            {
                balance = calc_value(fBalance);
                fValue      = fBalance;
                fReport     = fBalance;

                lmc->balance()->set(balance);
            }

            value   = calc_value(fValue);

            lmc->value()->set_all(value, min, max);

            // Launch the timer
            if (lmc->visibility()->get())
                sTimer.launch(-1, 50); // Schedule at 20 hz rate
        }

        void LedChannel::sync_colors()
        {
            tk::LedMeterChannel *lmc = tk::widget_cast<tk::LedMeterChannel>(wWidget);
            if (lmc == NULL)
                return;

            // Clear value ranges
            tk::ColorRanges *crv[4];

            crv[0]      = lmc->value_ranges();
            crv[1]      = lmc->peak_ranges();
            crv[2]      = lmc->text_ranges();
            crv[3]      = lmc->header_ranges();

            lsp::Color c(sPropValueColor.color());
            lmc->value_color()->set(&c);
            lmc->peak_color()->set(&c);
            lmc->text_color()->set(&c);
            lmc->header_color()->set(&c);
            float light = c.lightness();

            // For each component: update color ranges
            for (size_t i=0; i<4; ++i)
            {
                tk::ColorRange *cr;
                tk::ColorRanges *crs = crv[i];
                crs->clear();

                if ((nType == MT_VU) || ((nType == MT_RMS_PEAK)))
                {
                    cr = crs->append();
                    cr->set_range(0.0f, 120.0f);
                    cr->set(sPropRedZoneColor);

                    cr = crs->append();
                    cr->set_range(-6.0f, 0.0f);
                    cr->set_color(sPropYellowZoneColor);

                    c.lightness(0.8f * light);
                    cr = crs->append();
                    cr->set_range(-48.0f, -24.0f);
                    cr->set_color(c);

                    c.lightness(0.6f * light);
                    cr = crs->append();
                    cr->set_range(-96.0f, -48.0f);
                    cr->set_color(c);

                    c.lightness(0.4f * light);
                    cr = crs->append();
                    cr->set_range(-120.0f, -96.0f);
                    cr->set_color(c);
                }
            }
        }

        void LedChannel::on_mouse_click(const ws::event_t *ev)
        {
            tk::LedMeterChannel *lmc = tk::widget_cast<tk::LedMeterChannel>(wWidget);
            if (lmc == NULL)
                return;

            if (lmc->is_header(ev->nLeft, ev->nTop))
            {
                if (pParent != NULL)
                    pParent->cleanup_header();
                else
                    cleanup_header();
            }
        }

        status_t LedChannel::update_meter(ws::timestamp_t sched, ws::timestamp_t time, void *arg)
        {
            if (arg == NULL)
                return STATUS_OK;
            LedChannel *_this = static_cast<LedChannel *>(arg);
            _this->update_peaks(time);

            return STATUS_OK;
        }

        status_t LedChannel::slot_show(tk::Widget *sender, void *ptr, void *data)
        {
            LedChannel *_this = static_cast<LedChannel *>(ptr);
            if (_this != NULL)
                _this->sTimer.launch(-1, 50); // Schedule at 20 hz rate
            return STATUS_OK;
        }

        status_t LedChannel::slot_hide(tk::Widget *sender, void *ptr, void *data)
        {
            LedChannel *_this = static_cast<LedChannel *>(ptr);
            if (_this != NULL)
                _this->sTimer.cancel();
            return STATUS_OK;
        }

        void LedChannel::property_changed(tk::Property *prop)
        {
            if (sPropValueColor.is(prop))
                sync_colors();
            if (sPropYellowZoneColor.is(prop))
                sync_colors();
            if (sPropRedZoneColor.is(prop))
                sync_colors();
        }

        status_t LedChannel::slot_mouse_click(tk::Widget *sender, void *ptr, void *data)
        {
            LedChannel *self = static_cast<LedChannel *>(ptr);
            if (self != NULL)
                self->on_mouse_click(static_cast<ws::event_t *>(data));

            return STATUS_OK;
        }

        void LedChannel::cleanup_header()
        {
            fMaxValue = 0.0f;

            tk::LedMeterChannel *lmc = tk::widget_cast<tk::LedMeterChannel>(wWidget);
            if (lmc == NULL)
                return;

            lmc->header_value()->set(calc_value(fMaxValue));
            set_meter_text(lmc->header(), fMaxValue);
        }

        void LedChannel::set_parent(ctl::LedMeter *meter)
        {
            pParent     = meter;
        }

    } /* namespace ctl */
} /* namespace lsp */
