/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_EXPRESSION_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_EXPRESSION_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/expr/Expression.h>
#include <lsp-plug.in/expr/Variables.h>
#include <lsp-plug.in/runtime/LSPString.h>

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl/util/Property.h>

namespace lsp
{
    namespace ctl
    {
        enum
        {
            EXPR_FLAGS_MULTIPLE = expr::Expression::FLAG_MULTIPLE,
            EXPR_FLAGS_STRING   = expr::Expression::FLAG_STRING
        };

        /**
         * Computed expression
         */
        class Expression: public ctl::Property
        {
            private:
                Expression & operator = (const Expression &);

            protected:
                ui::IPortListener          *pListener;

            protected:
                virtual void    on_updated(ui::IPort *port);

            public:
                explicit Expression();

                void            init(ui::IWrapper *wrapper, ui::IPortListener *listener);

            public:
                float           evaluate_float(float dfl = 0.0f);
                ssize_t         evaluate_int(ssize_t dfl = 0);
                bool            evaluate_bool(bool dfl = false);

                float           evaluate();
                float           evaluate(size_t idx);
                inline status_t evaluate(expr::value_t *value)                                              { return Property::evaluate(value);         }
                inline status_t evaluate(size_t idx, expr::value_t *value)                                  { return Property::evaluate(idx, value);    }

                inline size_t   results()                       { return sExpr.results(); }
                float           result(size_t idx);

                inline bool     parse(const char *expr, size_t flags = expr::Expression::FLAG_NONE)         { return Property::parse(expr, flags);      }
                inline bool     parse(const LSPString *expr, size_t flags = expr::Expression::FLAG_NONE)    { return Property::parse(expr, flags);      }
                inline bool     parse(io::IInSequence *expr, size_t flags = expr::Expression::FLAG_NONE)    { return Property::parse(expr, flags);      }
        };
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_EXPRESSION_H_ */
