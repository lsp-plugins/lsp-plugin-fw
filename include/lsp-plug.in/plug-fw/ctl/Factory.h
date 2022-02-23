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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_FACTORY_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ctl/Widget.h>

namespace lsp
{
    namespace ui
    {
        class UIContext;
    }

    namespace ctl
    {
        /**
         * XML node factory
         */
        class Factory
        {
            private:
                static Factory *pRoot;
                Factory        *pNext;

            public:
                explicit Factory();
                virtual ~Factory();

            public:
                /**
                 * Get the next factory in the chain
                 * @return next factory in the factory chain or NULL if there is no more factories
                 */
                inline Factory *next()          { return pNext;     }

                /**
                 * Get the first node factory in the chain
                 * @return the first node factory in the chain
                 */
                inline static Factory *root()   { return pRoot;     }

            public:
                /**
                 * Create element
                 *
                 * @param ctl the pointer to store the pointer to created controller
                 * @param name name of the node
                 * @return status of operation, STATUS_NOT_FOUND if there is no supported node for this factory
                 */
                virtual status_t    create(Widget **ctl, ui::UIContext *context, const LSPString *name);
        };

        #define CTL_FACTORY_IMPL_START(fname) \
            class fname ## Factory: public ::lsp::ctl::Factory \
            { \
                public: \
                    explicit fname ## Factory() {} \
                    virtual ~fname ## Factory() {} \
                \
                public: \
                    virtual status_t create(Widget **ctl, ui::UIContext *context, const LSPString *name) \
                    {

        #define CTL_FACTORY_IMPL_END(fname) \
                    } \
            }; \
            \
            static fname ## Factory  fname ## FactoryInstance; /* Variable */
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_FACTORY_H_ */
