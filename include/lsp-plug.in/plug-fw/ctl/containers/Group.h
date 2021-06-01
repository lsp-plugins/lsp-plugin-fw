/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_GROUP_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_GROUP_H_

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
         * Simple container: group box
         */
        class Group: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ctl::Color          sTextColor;
                ctl::Color          sColor;
                ctl::Embedding      sEmbed;
                ctl::Padding        sIPadding;
                ctl::Padding        sTextPadding;
                ctl::LCString       sText;

            public:
                explicit Group(ui::IWrapper *wrapper, tk::Group *widget);
                virtual ~Group();

                virtual status_t    init();

            public:
                virtual void        set(const char *name, const char *value);

                virtual status_t    add(ctl::Widget *child);
        };
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_GROUP_H_ */
