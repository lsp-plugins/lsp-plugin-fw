/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_COLOR_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_COLOR_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl/util/Expression.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * Color controller
         */
        class Color: public ui::IPortListener
        {
            private:
                Color & operator = (const Color &);

            protected:
                enum component_t
                {
                    C_VALUE,
                    C_R, C_G, C_B, C_H, C_S, C_L, C_A,
                    C_TOTAL
                };

            protected:
                LSPString           sPrefix;            // Prefix name
                tk::Color          *pColor;             // Color
                ui::IWrapper       *pWrapper;           // Wrapper
                ctl::Expression    *vExpr[C_TOTAL];     // Expression

            protected:
                void                apply_change(size_t index, expr::value_t *value);

            public:
                explicit Color();
                virtual ~Color();

                status_t            init(ui::IWrapper *wrapper, tk::Color *color);

            public:
                bool                set(const char *prefix, const char *name, const char *value);

                /**
                 * Re-evaluate all expressions assigned to the controller
                 */
                void                reload();

            public:
                virtual void        notify(ui::IPort *port);
        };
    }
}


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_COLOR_H_ */
