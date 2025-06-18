/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                    ctl::Widget            *pWidget;
                    WidgetNode             *pChild;

                public:
                    explicit WidgetNode(UIContext *ctx, Node *parent, ctl::Widget *widget);
                    WidgetNode(const WidgetNode &) = delete;
                    WidgetNode(WidgetNode &&) = delete;
                    WidgetNode & operator = (const WidgetNode &) = delete;
                    WidgetNode & operator = (WidgetNode &&) = delete;

                    virtual ~WidgetNode() override;

                public:
                    virtual status_t        lookup(Node **child, const LSPString *name) override;
                    virtual status_t        enter(const LSPString * const *atts) override;
                    virtual status_t        completed(Node *child) override;
                    virtual status_t        leave() override;
            };

        } /* namespace xml */
    } /* namespace ui */
} /* namespace lsp */

#endif /* PRIVATE_UI_XML_WIDGETNODE_H_ */
