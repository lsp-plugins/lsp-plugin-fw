/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 23 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_LCSTRING_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_LCSTRING_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl/util/Expression.h>
#include <lsp-plug.in/plug-fw/ctl/util/Property.h>
#include <lsp-plug.in/expr/Parameters.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * Localized string controller with parameter substitution
         */
        class LCString: public ui::IPortListener
        {
            protected:
                typedef struct param_t
                {
                    ctl::Expression         sExpr;
                    LSPString               sValue;
                    bool                    bInitialized;
                } param_t;

            protected:
                ui::IWrapper               *pWrapper;
                tk::String                 *pProp;
                bool                        bEvaluate;
                lltl::pphash<char, param_t> vParams;

            protected:
                void            do_destroy();
                void            bind_metadata(expr::Parameters *params);
                void            update_text(ui::IPort *port);
                bool            add_parameter(const char *name, const char *expr);
                bool            init_expressions();

            public:
                explicit        LCString();
                LCString(const LCString &) = delete;
                LCString(LCString &&) = delete;
                virtual         ~LCString() override;

                LCString &operator = (const LCString &) = delete;
                LCString &operator = (LCString &&) = delete;

                void            init(ui::IWrapper *wrapper, tk::String *prop);
                void            destroy();

            public:
                bool            set(const char *param, const char *name, const char *value);
                expr::Parameters *params();

            public:
                virtual void    notify(ui::IPort *port, size_t flags) override;
                virtual void    sync_metadata(ui::IPort *port) override;
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_LCSTRING_H_ */
