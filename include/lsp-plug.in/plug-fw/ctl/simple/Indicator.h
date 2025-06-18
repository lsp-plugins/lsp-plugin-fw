/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_INDICATOR_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_INDICATOR_H_

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
         * Led indicator control
         */
        class Indicator: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                enum format_type_t
                {
                    FT_UNKNOWN,
                    FT_FLOAT,
                    FT_INT,
                    FT_TIME
                };

                enum ind_flags_t
                {
                    IF_SIGN         = 1 << 0,
                    IF_PLUS         = 1 << 1,
                    IF_PAD_ZERO     = 1 << 2,
                    IF_FIXED_PREC   = 1 << 3,
                    IF_NO_ZERO      = 1 << 4,
                    IF_DOT          = 1 << 5,
                    IF_TOLERANCE    = 1 << 6
                };

                typedef struct fmt_t
                {
                    char        type;
                    size_t      digits;
                    ssize_t     precision;
                } fmt_t;

            protected:
                class PropListener: public tk::IStyleListener
                {
                    protected:
                        Indicator     *pIndicator;

                    public:
                        inline PropListener(Indicator *ind)     { pIndicator = ind; }
                        virtual void notify(tk::atom_t property) override;
                };

            protected:
                ctl::Color              sColor;
                ctl::Color              sTextColor;
                ctl::Color              sInactiveColor;
                ctl::Color              sInactiveTextColor;

                ctl::Padding            sIPadding;
                LSPString               sFormat;
                PropListener            sListener;

                format_type_t           enFormat;
                lltl::darray<fmt_t>     vFormat;
                size_t                  nDigits;
                size_t                  nFlags;

                ui::IPort              *pPort;

            protected:
                static bool         parse_long(char *p, char **ret, long *value);

            protected:
                bool                parse_format();
                bool                fmt_time(LSPString *buf, double value);
                bool                fmt_float(LSPString *buf, double value);
                bool                fmt_int(LSPString *buf, ssize_t value);
                bool                format(LSPString *buf, double value);
                void                commit_value(float value);

            public:
                explicit Indicator(ui::IWrapper *wrapper, tk::Indicator *widget);
                Indicator(const Indicator &) = delete;
                Indicator(Indicator &&) = delete;
                virtual ~Indicator() override;
                Indicator & operator = (const Indicator &) = delete;
                Indicator & operator = (Indicator &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
        };
    } /* namespace ctl */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_INDICATOR_H_ */
