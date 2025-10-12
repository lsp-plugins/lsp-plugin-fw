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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_RANGESLIDER_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_RANGESLIDER_H_

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
         * Range slider control
         */
        class RangeSlider: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                enum param_type_t
                {
                    PT_MIN,
                    PT_MAX,
                    PT_RANGE,
                    PT_BEGIN,
                    PT_END,

                    PT_TOTAL
                };

                enum notify_flags_t
                {
                    NF_MIN          = 1 << 0,
                    NF_MAX          = 1 << 1,
                    NF_RANGE        = 1 << 2,
                    NF_BEGIN        = 1 << 3,
                    NF_END          = 1 << 4
                };

                enum global_flags_t
                {
                    GF_LOG          = 1 << 0,
                    GF_LOG_SET      = 1 << 1,
                    GF_STEP         = 1 << 2,
                    GF_CHANGING     = 1 << 3
                };

                typedef struct param_t
                {
                    ui::IPort          *pPort;
                    float               fMin;
                    float               fMax;
                    float               fValue;
                    float               fDefault;
                    bool                bHasDefault;

                    ctl::Expression     sMin;
                    ctl::Expression     sMax;
                } param_t;

            protected:
                ctl::Color          sBtnColor;
                ctl::Color          sBtnBorderColor;
                ctl::Color          sScaleColor;
                ctl::Color          sScaleBorderColor;
                ctl::Color          sBalanceColor;
                ctl::Color          sInactiveBtnColor;
                ctl::Color          sInactiveBtnBorderColor;
                ctl::Color          sInactiveScaleColor;
                ctl::Color          sInactiveScaleBorderColor;
                ctl::Color          sInactiveBalanceColor;

                param_t             vParams[PT_TOTAL];

                size_t              nFlags;
                float               fStep;
                float               fAStep;
                float               fDStep;

            protected:
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dbl_click(tk::Widget *sender, void *ptr, void *data);

            protected:
                static float        get_slider_value(tk::RangeSlider *rs, size_t type);
                static void         set_slider_value(tk::RangeSlider *rs, size_t type, float value);

            protected:
                void                submit_values(size_t flags);
                void                set_default_values();
                void                commit_values(size_t flags);
                bool                bind_params(size_t type, const char *prefix, const char *name, const char *value);
                bool                is_log_range(ui::IPort *p);

            public:
                explicit RangeSlider(ui::IWrapper *wrapper, tk::RangeSlider *widget);
                RangeSlider(const RangeSlider &) = delete;
                RangeSlider(RangeSlider &&) = delete;
                virtual ~RangeSlider() override;

                RangeSlider & operator = (const RangeSlider &) = delete;
                RangeSlider & operator = (RangeSlider &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
        };
    } /* namespace ctl */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_RANGESLIDER_H_ */
