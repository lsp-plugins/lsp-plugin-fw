/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 окт. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_PROGRESSBAR_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_PROGRESSBAR_H_

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
         * ProgressBar control
         */
        class ProgressBar: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;

                ctl::LCString       sText;
                ctl::Boolean        sShowText;
                ctl::Color          sBorderColor;
                ctl::Color          sBorderGapColor;
                ctl::Color          sColor;
                ctl::Color          sTextColor;
                ctl::Color          sInvColor;
                ctl::Color          sInvTextColor;
                ctl::Integer        sBorderSize;
                ctl::Integer        sBorderGapSize;
                ctl::Integer        sBorderRadius;
                ctl::Expression     sValue;
                ctl::Expression     sMin;
                ctl::Expression     sMax;
                ctl::Expression     sDefault;

            protected:
                void                sync_value();

            public:
                explicit ProgressBar(ui::IWrapper *wrapper, tk::ProgressBar *widget);
                virtual ~ProgressBar() override;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
        };

    } /* namespace ctl */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_PROGRESSBAR_H_ */
