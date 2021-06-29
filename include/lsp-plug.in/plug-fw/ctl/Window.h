/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 июн. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_WINDOW_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_WINDOW_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        class Window: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ctl::Registry           sControllers;
                tk::Registry            sWidgets;
                ctl::LCString           sTitle;

            public:
                explicit Window(ui::IWrapper *src, tk::Window *window);
                virtual ~Window();

                virtual status_t        init();
                virtual void            destroy();

            public:
                inline ctl::Registry   *controllers()   { return &sControllers; }
                inline tk::Registry    *widgets()       { return &sWidgets;     }

            public:
                virtual void            begin(ui::UIContext *ctx);

                virtual void            set(ui::UIContext *ctx, const char *name, const char *value);

                virtual status_t        add(ui::UIContext *ctx, ctl::Widget *child);

                virtual void            end(ui::UIContext *ctx);

                virtual void            schema_reloaded();
        };
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_WINDOW_H_ */
