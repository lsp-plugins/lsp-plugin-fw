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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_BOX_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_BOX_H_

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
         * Simple container: vertical or horizontal box
         */
        class Box: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ssize_t             enOrientation;

            public:
                explicit Box(ui::IWrapper *src, tk::Box *widget, ssize_t orientation = -1);
                virtual ~Box();

                virtual status_t    init();

            public:

                virtual void        set(const char *name, const char *value);

                virtual status_t    add(ctl::Widget *child);
        };
    }
}


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_BOX_H_ */
