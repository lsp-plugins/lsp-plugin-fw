/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 апр. 2021 г.
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

#ifndef PRIVATE_UI_XML_NODEFACTORY_H_
#define PRIVATE_UI_XML_NODEFACTORY_H_

#include <lsp-plug.in/plug-fw/ui.h>

#include <private/ui/xml/Node.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * XML node factory
             */
            class NodeFactory
            {
                private:
                    NodeFactory &operator = (const NodeFactory &);
                    NodeFactory(const NodeFactory &);

                private:
                    static NodeFactory *pRoot;
                    NodeFactory        *pNext;

                public:
                    explicit NodeFactory();
                    virtual ~NodeFactory();

                public:
                    /**
                     * Get the next factory in the chain
                     * @return next factory in the factory chain or NULL if there is no more factories
                     */
                    inline NodeFactory *next()          { return pNext;     }

                    /**
                     * Get the first node factory in the chain
                     * @return the first node factory in the chain
                     */
                    inline static NodeFactory *root()   { return pRoot;     }

                public:
                    /**
                     * Create element
                     *
                     * @param context UI context
                     * @param parent the pointer to parent node
                     * @param child the pointer to store the pointer to created node
                     * @param name name of the node
                     * @return status of operation, STATUS_NOT_FOUND if there is no supported node for this factory
                     */
                    virtual status_t    create(Node **child, UIContext *context, Node *parent, const LSPString *name);
            };

            #define NODE_FACTORY_IMPL_START(fname) \
                class fname ## Factory: public ::lsp::ui::xml::NodeFactory \
                { \
                    public: \
                        explicit fname ## Factory() {} \
                        virtual ~fname ## Factory() {} \
                    \
                    public: \
                        virtual status_t create(Node **child, UIContext *context, Node *parent, const LSPString *name) \
                        {

            #define NODE_FACTORY_IMPL_END(fname) \
                        } \
                }; \
                \
                static fname ## Factory  fname ## FactoryInstance; /* Variable */
        }
    }
}

#endif /* PRIVATE_UI_XML_NODEFACTORY_H_ */
