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

                public:
                    explicit Node(UIContext *ctx);
                    virtual ~Node();

                    virtual status_t    init(const LSPString * const *atts);

                protected:
                    static const LSPString *find_attribute(const LSPString * const *atts, const LSPString *name);
                    static const LSPString *find_attribute(const LSPString * const *atts, const char *name);

                public:
                    /** Called when XML handler is set
                     *
                     */
                    virtual status_t        enter();

                    /** Call on tag open
                     *
                     * @param child pointer to pass child node handler if necessary
                     * @param name tag name
                     * @param atts NULL-terminated list of attributes
                     * @return handler of tag sub-structure or NULL
                     */
                    virtual status_t        start_element(Node **child, const LSPString *name, const LSPString * const *atts);

                    /** Call on tag close
                     *
                     * @param name tag name
                     */
                    virtual status_t        end_element(const LSPString *name);

                    /**
                     * Execute the body of node (if required)
                     * @return status of operation
                     */
                    virtual status_t        execute();

                    /** Called when there will be no more data
                     *
                     */
                    virtual status_t        quit();

                    /** Called when child has been fully parsed
                     *
                     * @param child child that has been fully parsed
                     */
                    virtual status_t        completed(Node *child);
            };
        }
    }
}



#endif /* PRIVATE_UI_XML_NODE_H_ */
