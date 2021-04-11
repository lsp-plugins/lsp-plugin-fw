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

#include <private/ui/xml/RootNode.h>
#include <private/ui/xml/WidgetNode.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            RootNode::RootNode(UIContext *ctx): Node(ctx)
            {
                pChild      = NULL;
            }

            RootNode::~RootNode()
            {
                if (pChild != NULL)
                {
                    delete pChild;
                    pChild = NULL;
                }
            }

            status_t RootNode::start_element(Node **child, const LSPString *name, const LSPString * const *atts)
            {
                // Check that root tag is valid
                if (!name->equals_ascii("plugin"))
                {
                    lsp_error("expected root element <plugin>");
                    return STATUS_CORRUPTED;
                }

                // Create and initialize widget
                ctl::Widget *widget     = pContext->create_widget(name, atts);
                if (widget == NULL)
                    return STATUS_OK;       // No handler

                // Create handler
                pChild = new WidgetNode(pContext, widget);
                if (pChild == NULL)
                    return STATUS_NO_MEM;
                *child  = pChild;

                return STATUS_OK;
            }

        }
    }
}


