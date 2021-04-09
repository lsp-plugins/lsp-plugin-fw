/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 апр. 2021 г.
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

#ifndef PRIVATE_UI_PORTRESOLVER_H_
#define PRIVATE_UI_PORTRESOLVER_H_

#include <lsp-plug.in/expr/Resolver.h>
#include <lsp-plug.in/expr/Expression.h>

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ui
    {
        /**
         * Port resolver, resolves port by the port name
         */
        class PortResolver: public expr::Resolver
        {
            private:
                PortResolver & operator = (const PortResolver &);

            protected:
                ui::IWrapper       *pWrapper;

            public:
                explicit PortResolver(ui::IWrapper *wrapper);
                virtual ~PortResolver();

            public:
                virtual status_t    on_resolved(const LSPString *name, ui::IPort *p);

                virtual status_t    on_resolved(const char *name, ui::IPort *p);

                virtual status_t    resolve(expr::value_t *value, const char *name, size_t num_indexes = 0, const ssize_t *indexes = NULL);

                virtual status_t    resolve(expr::value_t *value, const LSPString *name, size_t num_indexes = 0, const ssize_t *indexes = NULL);
        };
    }
}

#endif /* PRIVATE_UI_PORTRESOLVER_H_ */
