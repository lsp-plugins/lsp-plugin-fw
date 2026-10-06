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
                enum notify_flags_t
                {
                    NF_RANGE        = 1 << 0,
                    NF_BEGIN        = 1 << 1,
                    NF_BEGIN_MIN    = 1 << 2,
                    NF_BEGIN_MAX    = 1 << 3,
                    NF_END          = 1 << 4,
                    NF_END_MIN      = 1 << 5,
                    NF_END_MAX      = 1 << 6,
                };

                enum global_flags_t
                {
                    GF_LOG          = 1 << 0,
                    GF_LOG_SET      = 1 << 1,
                    GF_STEP         = 1 << 2,
                    GF_CHANGING     = 1 << 3,
                };

                typedef struct value_t
                {
                    ui::IPort          *pPort;
                    float               fValue;
                    ctl::Expression     sExpr;
                } value_t;

                typedef struct param_t
                {
                    value_t             sMin;
                    value_t             sMax;
                    value_t             sValue;
                    float               fDefault;
                    bool                bHasDefault;
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

                param_t             sBegin;
                param_t             sEnd;
                value_t             sRange;

                size_t              nFlags;
                float               fStep;
                float               fAStep;
                float               fDStep;

            protected:
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_begin_edit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_end_edit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dbl_click(tk::Widget *sender, void *ptr, void *data);
                static void         construct_param(param_t *p);
                static void         construct_value(value_t *v);
                static float        decode_value(ui::IPort *p, float value);
                static float        encode_value(ui::IPort *p, float value);
                static float        calc_default_value(param_t *p);

            protected:
                void                init_param(param_t *p);
                void                init_value(value_t *v);
                void                submit_values(size_t flags);
                void                set_default_values();
                void                commit_values(size_t flags);
                bool                bind_param(param_t *p, const char *prefix, const char *name, const char *value);
                bool                bind_value(value_t *v, const char *prefix, const char *name, const char *value);
                bool                is_log_range(ui::IPort *p);
                size_t              notify_param(param_t *p, ui::IPort *port);
                size_t              notify_value(value_t *v, ui::IPort *port, size_t nf_flag);
                void                end_param(param_t *p);
                bool                end_value(value_t *v);
                void                submit_values(float begin, float end, bool begin_ch, bool end_ch, size_t flags);

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
