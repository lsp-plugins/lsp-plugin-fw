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

#ifndef PRIVATE_UI_XML_WIDGETNODE_H_
#define PRIVATE_UI_XML_WIDGETNODE_H_

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/ui.h>

#include <private/ui/xml/Node.h>
#include <private/ui/xml/NodeFactory.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * Node for configuring properties of the widget controller from the XML document
             */
            class WidgetNode: public Node
            {
                private:
                    WidgetNode & operator = (const WidgetNode *);
                    WidgetNode(const WidgetNode &);

                private:
                    ctl::Widget            *pWidget;
                    WidgetNode             *pChild;
                    Node                   *pSpecial;

                public:
                    explicit WidgetNode(UIContext *ctx, ctl::Widget *widget);
                    virtual ~WidgetNode();

                public:
                    virtual status_t        lookup(Node **child, const LSPString *name);

                    virtual status_t        enter();

                    virtual status_t        start_element(Node **child, const LSPString *name, const LSPString * const *atts);

                    virtual status_t        completed(Node *child);

                    virtual status_t        leave();
            };

        }
    }
}

#endif /* PRIVATE_UI_XML_WIDGETNODE_H_ */
