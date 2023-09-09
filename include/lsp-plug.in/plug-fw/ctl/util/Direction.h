/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 июн. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_DIRECTION_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_DIRECTION_H_

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
         * Direction controller
         */
        class Direction: public ui::IPortListener, public ui::ISchemaListener
        {
            protected:
                enum dir_t
                {
                    DIR_DX,
                    DIR_DY,
                    DIR_ANGLE,
                    DIR_DANGLE,
                    DIR_LENGTH,

                    DIR_COUNT
                };

            protected:
                ui::IWrapper       *pWrapper;
                tk::Vector2D       *pDirection;
                ctl::Expression    *vExpr[DIR_COUNT];

            protected:
                void                apply_change(size_t index, expr::value_t *value);

            public:
                explicit Direction();
                Direction(const Direction &) = delete;
                Direction(Direction &&) = delete;
                virtual ~Direction() override;

                Direction &operator = (const Direction &) = delete;
                Direction &operator = (Direction &&) = delete;

                status_t            init(ui::IWrapper *wrapper, tk::Vector2D *direction);

            public:
                bool                set(const char *param, const char *name, const char *value);

                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_DIRECTION_H_ */
