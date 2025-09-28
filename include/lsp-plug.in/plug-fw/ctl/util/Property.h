/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_PROPERTY_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_PROPERTY_H_

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
        /**
         * Some property controller, simplified expression
         */
        class Property: public ui::IPortListener
        {
            protected:
                class PropResolver: public ui::PortResolver
                {
                    protected:
                        Property *pProp;

                    public:
                        explicit PropResolver(Property *prop);

                    public:
                        virtual status_t on_resolved(const LSPString *name, ui::IPort *p) override;
                        virtual status_t resolve(expr::value_t *value, const char *name, size_t num_indexes, const ssize_t *indexes) override;
                        virtual status_t resolve(expr::value_t *value, const LSPString *name, size_t num_indexes, const ssize_t *indexes) override;
                        virtual status_t call(expr::value_t *value, const char *name, size_t num_args, const expr::value_t *args) override;
                        virtual status_t call(expr::value_t *value, const LSPString *name, size_t num_args, const expr::value_t *args) override;
                };

            protected:
                expr::Expression            sExpr;
                expr::Variables             sVars;
                expr::Parameters            sParams;
                PropResolver                sResolver;
                ui::IWrapper               *pWrapper;
                expr::Resolver             *pResolver;
                lltl::parray<ui::IPort>     vDependencies;

            protected:
                void            do_destroy();
                void            drop_dependencies();
                status_t        on_resolved(const LSPString *name, ui::IPort *p);
                virtual void    on_updated(ui::IPort *port, size_t flags);

            protected:
                status_t        evaluate(expr::value_t *value);
                status_t        evaluate(size_t idx, expr::value_t *value);
                bool            parse(const char *expr, size_t flags = expr::Expression::FLAG_NONE);
                bool            parse(const LSPString *expr, size_t flags = expr::Expression::FLAG_NONE);
                bool            parse(io::IInSequence *expr, size_t flags = expr::Expression::FLAG_NONE);

            public:
                explicit Property();
                Property(const Property &) = delete;
                Property(Property &&) = delete;
                virtual ~Property() override;

                Property & operator = (const Property &) = delete;
                Property & operator = (Property &&) = delete;

                void            init(ui::IWrapper *wrapper, expr::Resolver *resolver = NULL);
                virtual void    destroy();

            public:
                virtual void    notify(ui::IPort *port, size_t flags) override;

            public:
                inline bool     valid() const                   { return sExpr.valid(); };
                inline bool     depends(ui::IPort *port) const  { return vDependencies.contains(port); }
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_PROPERTY_H_ */
