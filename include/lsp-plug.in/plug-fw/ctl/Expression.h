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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_EXPRESSION_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_EXPRESSION_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/expr/Expression.h>
#include <lsp-plug.in/expr/Variables.h>
#include <lsp-plug.in/runtime/LSPString.h>

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ctl
    {
        enum
        {
            EXPR_FLAGS_MULTIPLE = expr::Expression::FLAG_MULTIPLE,
            EXPR_FLAGS_STRING   = expr::Expression::FLAG_STRING
        };

        class Expression: public ui::IPortListener
        {
            private:
                Expression & operator = (const Expression &);

            protected:
                class ExprResolver: public ui::PortResolver
                {
                    protected:
                        Expression *pExpr;

                    public:
                        explicit ExprResolver(Expression *expr);

                    public:
                        virtual status_t on_resolved(const LSPString *name, ui::IPort *p);

                        virtual status_t resolve(expr::value_t *value, const char *name, size_t num_indexes, const ssize_t *indexes);

                        virtual status_t resolve(expr::value_t *value, const LSPString *name, size_t num_indexes, const ssize_t *indexes);
                };

            protected:
                expr::Expression            sExpr;
                expr::Variables             sVars;
                expr::Parameters            sParams;
                ExprResolver                sResolver;
                ui::IWrapper               *pWrapper;
                ui::IPortListener          *pListener;
                lltl::parray<ui::IPort>     vDependencies;
            #ifdef LSP_TRACE
                LSPString                   sText;
            #endif /* LSP_TRACE */

            protected:
                void            do_destroy();
                void            drop_dependencies();
                status_t        on_resolved(const LSPString *name, ui::IPort *p);

            public:
                explicit Expression();
                virtual ~Expression();

            public:
                virtual void    notify(ui::IPort *port);

            public:
                void            init(ui::IWrapper *wrapper, ui::IPortListener *listener);
                void            destroy();

                inline expr::Parameters *params()               { return &sParams; };

                float           evaluate();
                float           evaluate(size_t idx);
                status_t        evaluate(expr::value_t *value);
                status_t        evaluate(size_t idx, expr::value_t *value);

                inline size_t   results()                       { return sExpr.results(); }
                float           result(size_t idx);
                bool            parse(const char *expr, size_t flags = expr::Expression::FLAG_NONE);
                bool            parse(const LSPString *expr, size_t flags = expr::Expression::FLAG_NONE);
                bool            parse(io::IInSequence *expr, size_t flags = expr::Expression::FLAG_NONE);
                inline bool     valid() const                   { return sExpr.valid(); };
                inline bool     depends(ui::IPort *port) const  { return vDependencies.contains(port); }

            #ifdef LSP_TRACE
                inline const char *text() const                 { return sText.get_utf8(); }
            #endif /* LSP_TRACE */
        };
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_EXPRESSION_H_ */
