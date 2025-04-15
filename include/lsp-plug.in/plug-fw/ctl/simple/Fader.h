/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 июл. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_FADER_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_FADER_H_

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
         * Fader control
         */
        class Fader: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                enum fader_flags_t
                {
                    FF_MIN          = 1 << 0,
                    FF_MAX          = 1 << 1,
                    FF_DFL          = 1 << 2,
                    FF_VALUE        = 1 << 3,
                    FF_STEP         = 1 << 4,
                    FF_LOG          = 1 << 5,
                    FF_LOG_SET      = 1 << 6,
                    FF_BAL_SET      = 1 << 7
                };

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

                ctl::Expression     sMin;
                ctl::Expression     sMax;

                ui::IPort          *pPort;

                size_t              nFlags;
                float               fDefault;
                float               fStep;
                float               fAStep;
                float               fDStep;
                float               fBalance;

                float               fDefaultValue;

            protected:
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dbl_click(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                submit_value();
                void                set_default_value();
                void                commit_value(size_t flags);

            public:
                explicit Fader(ui::IWrapper *wrapper, tk::Fader *widget);
                Fader(const Fader &) = delete;
                Fader(Fader &&) = delete;
                virtual ~Fader() override;

                Fader & operator = (const Fader &) = delete;
                Fader & operator = (Fader &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
        };
    } /* namespace ctl */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_FADER_H_ */
