/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 18 февр. 2025 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUGIN_FW_INCLUDE_PRIVATE_UI_XML_PORTNODE_H_
#define LSP_PLUGIN_FW_INCLUDE_PRIVATE_UI_XML_PORTNODE_H_

#include <private/ui/xml/Node.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * The ui:port node which allows to define evaluated ports
             */
            class PortNode: public Node
            {
                public:
                    explicit PortNode(UIContext *ctx, Node *parent);
                    PortNode(const PortNode &) = delete;
                    PortNode(PortNode &&) = delete;
                    PortNode &operator = (const PortNode &) = delete;
                    PortNode &operator = (PortNode &&) = delete;

                public:
                    virtual status_t    enter(const LSPString * const *atts) override;
            };

        } /* namespace xml */
    } /* namespace ui */
} /* namespace lsp */



#endif /* LSP_PLUGIN_FW_INCLUDE_PRIVATE_UI_XML_PORTNODE_H_ */
