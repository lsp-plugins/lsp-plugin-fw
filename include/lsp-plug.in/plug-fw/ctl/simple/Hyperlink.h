/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 июн. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_HYPERLINK_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_HYPERLINK_H_

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
         * The hyperlink to some resource
         */
        class Hyperlink: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                LCString            sText;
                LCString            sUrl;

                ctl::Color          sColor;
                ctl::Color          sHoverColor;
                ctl::Color          sInactiveColor;
                ctl::Color          sInactiveHoverColor;

            public:
                explicit Hyperlink(ui::IWrapper *wrapper, tk::Hyperlink *widget);
                Hyperlink(const Hyperlink &) = delete;
                Hyperlink(Hyperlink &&) = delete;
                virtual ~Hyperlink() override;

                Hyperlink & operator = (const Hyperlink &) = delete;
                Hyperlink & operator = (Hyperlink &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_HYPERLINK_H_ */
