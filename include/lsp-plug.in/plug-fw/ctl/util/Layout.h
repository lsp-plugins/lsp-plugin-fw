/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-para-equalizer
 * Created on: 13 февр. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_LAYOUT_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_LAYOUT_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ctl
    {
        class Layout: public ui::IPortListener, public ui::ISchemaListener
        {
            protected:
                ctl::Expression     sHAlign;
                ctl::Expression     sVAlign;
                ctl::Expression     sHScale;
                ctl::Expression     sVScale;
                tk::Layout         *pLayout;
                ui::IWrapper       *pWrapper;

            protected:
                void            apply_changes();
                bool            parse_and_apply(ctl::Expression *expr, const char *value);

            public:
                explicit Layout();
                virtual ~Layout() override;

            public:
                void            init(ui::IWrapper *wrapper, tk::Layout *layout);

            public:
                bool            set(const char *name, const char *value);

            public:
                virtual void    notify(ui::IPort *port) override;
                virtual void    reloaded(const tk::StyleSheet *sheet) override;
        };

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_LAYOUT_H_ */
