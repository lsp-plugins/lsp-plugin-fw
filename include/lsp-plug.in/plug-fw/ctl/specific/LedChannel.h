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
                    MF_LOG      = 1 << 2,
                    MF_LOG_SET  = 1 << 3,
                    MF_BALANCE  = 1 << 4,
                    MF_REV      = 1 << 5,
                    MF_ACTIVITY = 1 << 6
                };

                enum type_t
                {
                    MT_PEAK,
                    MT_VU,
                    MT_RMS_PEAK
                };

            protected:
                ui::IPort          *pPort;
                size_t              nFlags;
                size_t              nType;
                float               fMin;
                float               fMax;
                float               fBalance;
                float               fValue;
                float               fRms;
                float               fReport;

                ctl::Boolean        sActivity;
                ctl::Boolean        sReversive;
                ctl::Boolean        sPeakVisible;
                ctl::Boolean        sBalanceVisible;
                ctl::Boolean        sTextVisible;

                tk::Timer           sTimer;

            protected:
                static status_t     update_meter(ws::timestamp_t sched, ws::timestamp_t time, void *arg);

            protected:
                void                update_peaks(ws::timestamp_t ts);
                float               calc_value(float value);
                void                set_meter_text(tk::LedMeterChannel *lmc, float value);
                void                sync_channel();

            public:
                explicit LedChannel(ui::IWrapper *wrapper, tk::LedMeterChannel *widget);
                virtual ~LedChannel();

                virtual status_t    init();
                virtual void        destroy();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        notify(ui::IPort *port);
                virtual void        end(ui::UIContext *ctx);
                virtual void        schema_reloaded();
        };
    } // namespace ctl
} // namespace lsp


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_LEDCHANNEL_H_ */
