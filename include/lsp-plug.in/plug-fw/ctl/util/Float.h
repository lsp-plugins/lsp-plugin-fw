/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_FLOAT_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_FLOAT_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl/util/Property.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * Simple expression-based floating-point property
         */
        class Float: public ctl::Property, public ui::ISchemaListener
        {
            protected:
                tk::Float      *pProp;

            protected:
                void            apply_changes();

            protected:
                virtual void    on_updated(ui::IPort *port, size_t flags) override;

            public:
                explicit        Float();
                Float(const Float &) = delete;
                Float(Float &&) = delete;
                virtual         ~Float() override;

                Float & operator = (const Float &) = delete;
                Float & operator = (Float &&) = delete;

                void            init(ui::IWrapper *wrapper, tk::Float *prop);

            public:
                bool            set(const char *prop, const char *name, const char *value);
                inline float    value() const   { return pProp->get();  }
                virtual void    reloaded(const tk::StyleSheet *sheet) override;
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_FLOAT_H_ */
