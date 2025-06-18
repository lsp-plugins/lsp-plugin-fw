/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 2 окт. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_FRACTION_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_FRACTION_H_

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
         * Fraction widget controller
         */
        class Fraction: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;
                ui::IPort          *pDen;

                float               fSig;
                float               fMaxSig;
                ssize_t             nDenomMin;
                ssize_t             nDenomMax;
                ssize_t             nNum;
                ssize_t             nDenom;

                ctl::Float          sAngle;
                ctl::Integer        sTextPad;
                ctl::Integer        sThick;

                ctl::Color          sColor;
                ctl::Color          sNumColor;
                ctl::Color          sDenColor;
                ctl::Color          sInactiveColor;
                ctl::Color          sInactiveNumColor;
                ctl::Color          sInactiveDenColor;

            protected:
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_submit(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                update_values(ui::IPort *port);
                void                submit_value();
                void                sync_numerator();
                status_t            add_list_item(tk::WidgetList<tk::ListBoxItem> *list, int i, const char *text);

            public:
                explicit Fraction(ui::IWrapper *wrapper, tk::Fraction *widget);
                Fraction(const Fraction &) = delete;
                Fraction(Fraction &&) = delete;
                virtual ~Fraction() override;

                Fraction & operator = (const Fraction &) = delete;
                Fraction & operator = (Fraction &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        end(ui::UIContext *ctx) override;

                virtual void        notify(ui::IPort *port, size_t flags) override;
        };

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_FRACTION_H_ */
