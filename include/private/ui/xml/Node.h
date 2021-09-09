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

#ifndef PRIVATE_UI_XML_NODE_H_
#define PRIVATE_UI_XML_NODE_H_

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * XML node handler
             */
            class Node
            {
                private:
                    Node & operator = (const Node &);
                    Node(const Node &);

                protected:
                    UIContext      *pContext;
                    Node           *pParent;

                public:
                    explicit Node(UIContext *ctx, Node *parent);
                    virtual ~Node();

                public:
                    /**
                     * Get pointer to the UI context
                     * @return pointer to the UI context
                     */
                    inline UIContext       *context()       { return pContext; }

                    /**
                     * Get pointer to the parent node
                     * @return pointer to the parent node
                     */
                    inline Node            *parent()        { return pParent; }

                public:
                    /**
                     * Lookup for child node
                     * @param child pointer to store child node, should be NULL if handler not created
                     * @param name node name
                     * @return error if node was not found
                     */
                    virtual status_t        lookup(Node **child, const LSPString *name);

                    /** Called when XML handler is set
                     *
                     */
                    virtual status_t        enter(const LSPString * const *atts);

                    /** Call on tag open
                     *
                     * @param child pointer to pass child node handler if necessary
                     * @param name tag name
                     * @param atts NULL-terminated list of attributes
                     * @return handler of tag sub-structure or NULL
                     */
                    virtual status_t        start_element(const LSPString *name, const LSPString * const *atts);

                    /** Call on tag close
                     *
                     * @param name tag name
                     */
                    virtual status_t        end_element(const LSPString *name);

                    /** Called by child on leave() event
                     *
                     * @param child child that has been fully parsed
                     */
                    virtual status_t        completed(Node *child);

                    /** Called when there will be no more data
                     *
                     */
                    virtual status_t        leave();
            };
        }
    }
}



#endif /* PRIVATE_UI_XML_NODE_H_ */
