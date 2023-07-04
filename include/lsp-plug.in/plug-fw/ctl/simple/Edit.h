/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 15 апр. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_EDIT_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_EDIT_H_

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
         * Edit widget controller
         */
        class Edit: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ctl::Color          sColor;
                ctl::Color          sBorderColor;
                ctl::Color          sBorderGapColor;
                ctl::Color          sCursorColor;
                ctl::Color          sTextColor;
                ctl::Color          sTextSelectedColor;

                ctl::Integer        sBorderSize;
                ctl::Integer        sBorderGapSize;
                ctl::Integer        sBorderRadius;

            public:
                explicit Edit(ui::IWrapper *wrapper, tk::Edit *widget);
                virtual ~Edit() override;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_EDIT_H_ */
