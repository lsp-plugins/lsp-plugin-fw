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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_BOOLEAN_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_BOOLEAN_H_

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
        /**
         * Simple expression-based boolean property
         */
        class Boolean: public ctl::Property, public ui::ISchemaListener
        {
            protected:
                tk::Boolean    *pProp;

            protected:
                void            apply_changes();

            protected:
                virtual void    on_updated(ui::IPort *port) override;

            public:
                explicit        Boolean();
                Boolean(const Boolean &) = delete;
                Boolean(Boolean &&) = delete;
                virtual        ~Boolean() override;

                Boolean & operator = (const Boolean &) = delete;
                Boolean & operator = (Boolean &&) = delete;

                void            init(ui::IWrapper *wrapper, tk::Boolean *prop);

            public:
                bool            set(const char *prop, const char *name, const char *value);
                inline bool     value() const   { return pProp->get();  }
                virtual void    reloaded(const tk::StyleSheet *sheet) override;
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_BOOLEAN_H_ */
