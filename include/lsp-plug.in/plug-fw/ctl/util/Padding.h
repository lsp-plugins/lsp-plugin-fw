/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 13 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_PADDING_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_PADDING_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/expr/Expression.h>
#include <lsp-plug.in/runtime/LSPString.h>

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl/util/Property.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * Padding controller
         */
        class Padding: public ui::IPortListener, public ui::ISchemaListener
        {
            protected:
                enum pad_t
                {
                    PAD_ALL,
                    PAD_LEFT,
                    PAD_RIGHT,
                    PAD_TOP,
                    PAD_BOTTOM,
                    PAD_HORIZONTAL,
                    PAD_VERTICAL,

                    PAD_COUNT
                };

            protected:
                ui::IWrapper       *pWrapper;
                tk::Padding        *pPadding;
                ctl::Expression    *vExpr[PAD_COUNT];

            protected:
                void                apply_change(size_t index, expr::value_t *value);

            public:
                explicit Padding();
                Padding(const Padding &) = delete;
                Padding(Padding &&) = delete;
                virtual ~Padding() override;

                Padding &operator = (const Padding &) = delete;
                Padding &operator = (Padding &) = delete;

                status_t            init(ui::IWrapper *wrapper, tk::Padding *padding);

            public:
                bool                set(const char *param, const char *name, const char *value);

                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_PADDING_H_ */
