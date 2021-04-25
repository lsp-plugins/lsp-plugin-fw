/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_EMBEDDING_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_EMBEDDING_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl/util/Expression.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * Color controller
         */
        class Embedding: public ui::IPortListener
        {
            private:
                Embedding & operator = (const Embedding &);

            protected:
                enum component_t
                {
                    E_ALL, E_H, E_V,
                    E_L, E_R, E_T, E_B,
                    E_TOTAL
                };

            protected:
                LSPString           sPrefix;            // Prefix name
                tk::Embedding      *pEmbedding;         // Embedding
                ui::IWrapper       *pWrapper;           // Wrapper
                ctl::Expression    *vExpr[E_TOTAL];     // Expression

            public:
                explicit Embedding();
                virtual ~Embedding();

                status_t            init(ui::IWrapper *wrapper, tk::Embedding *embed, const char *prefix);
                status_t            init(ui::IWrapper *wrapper, tk::Embedding *embed, const LSPString *prefix);

            public:
                void                set(const char *name, const char *value);

            public:
                virtual void        notify(ui::IPort *port);
        };
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_EMBEDDING_H_ */
