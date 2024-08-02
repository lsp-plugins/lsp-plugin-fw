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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_LEDCHANNEL_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_LEDCHANNEL_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        class LedMeter;

        /**
         * Led Meter channel widget controller
         */
        class LedChannel: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                enum flags_t
                {
                    MF_MIN      = 1 << 0,
                    MF_MAX      = 1 << 1,
                    MF_LOG      = 1 << 3,
                    MF_BALANCE  = 1 << 4
                };

                enum type_t
                {
                    MT_PEAK,
                    MT_VU,
                    MT_RMS_PEAK
                };

            protected:
                ctl::LedMeter      *pParent;
                ui::IPort          *pPort;
                size_t              nFlags;
                size_t              nType;
                float               fMin;
                float               fMax;
                float               fBalance;
                float               fValue;
                float               fMaxValue;
                float               fRms;
                float               fReport;
                float               fAttack;
                float               fRelease;
                bool                bLog;

                tk::prop::Color     sPropValueColor;
                tk::prop::Color     sPropYellowZoneColor;
                tk::prop::Color     sPropRedZoneColor;

                ctl::Boolean        sActivity;
                ctl::Boolean        sReversive;
                ctl::Boolean        sPeakVisible;
                ctl::Boolean        sBalanceVisible;
                ctl::Boolean        sTextVisible;
                ctl::Boolean        sHeaderVisible;

                ctl::Color          sColor;
                ctl::Color          sValueColor;
                ctl::Color          sRedZoneColor;
                ctl::Color          sYellowZoneColor;
                ctl::Color          sBalanceColor;

                tk::Timer           sTimer;

            protected:
                static status_t     update_meter(ws::timestamp_t sched, ws::timestamp_t time, void *arg);
                static status_t     slot_show(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_hide(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_click(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                update_peaks(ws::timestamp_t ts);
                float               calc_value(float value);
                void                set_meter_text(tk::String *dst, float value);
                void                sync_channel();
                void                sync_colors();
                void                on_mouse_click(const ws::event_t *ev);

            protected:
                virtual void        property_changed(tk::Property *prop) override;

            public:
                explicit LedChannel(ui::IWrapper *wrapper, tk::LedMeterChannel *widget);
                LedChannel(const LedChannel &) = delete;
                LedChannel(LedChannel &&) = delete;
                virtual ~LedChannel() override;

                LedChannel & operator = (const LedChannel &) = delete;
                LedChannel & operator = (LedChannel &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;

            public:
                void                cleanup_header();
                void                set_parent(ctl::LedMeter *meter);
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_LEDCHANNEL_H_ */
