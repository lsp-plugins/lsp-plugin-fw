/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 мая 2025 г.
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

#ifndef PRIVATE_UI_XML_DOMCONTROLLERNODE_H_
#define PRIVATE_UI_XML_DOMCONTROLLERNODE_H_

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
            class DOMControllerNode: public Node
            {
                private:
                    ctl::DOMController     *pController;

                public:
                    explicit DOMControllerNode(UIContext *ctx, Node *parent, ctl::DOMController *ctl);
                    DOMControllerNode(const DOMControllerNode &) = delete;
                    DOMControllerNode(DOMControllerNode &&) = delete;
                    DOMControllerNode & operator = (const DOMControllerNode &) = delete;
                    DOMControllerNode & operator = (DOMControllerNode &&) = delete;

                    virtual ~DOMControllerNode() override;

                public: // xml::Node
                    virtual status_t        enter(const LSPString * const *atts) override;
                    virtual status_t        leave() override;
            };

        } /* namespace xml */
    } /* namespace ui */
} /* namespace lsp */



#endif /* PRIVATE_UI_XML_DOMCONTROLLERNODE_H_ */
