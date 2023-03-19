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

#include <lsp-plug.in/common/debug.h>

#include <private/ui/xml/RootNode.h>
#include <private/ui/xml/WidgetNode.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            RootNode::RootNode(UIContext *ctx, const char *name, ctl::Widget *widget): Node(ctx, NULL)
            {
                pWidget     = widget;
                sName.set_utf8(name);
            }

            RootNode::~RootNode()
            {
                pWidget = NULL;

                sName.clear();
            }

            status_t RootNode::lookup(Node **child, const LSPString *name)
            {
                // Check that root tag is valid
                if (!name->equals(&sName))
                {
                    lsp_error("expected root element <%s>", sName.get_native());
                    return STATUS_CORRUPTED;
                }

                // Create and initialize widget
                ctl::Widget *widget     = pWidget;
                if (widget == NULL) // If there is no widget, instantiate it
                    widget  =  pContext->create_controller(name);

                // No handler?
                if (widget == NULL)
                {
                    *child = NULL;
                    return STATUS_OK;
                }

                // Remember the root widget
                pContext->ui()->set_root(widget->widget());

                // Create child handler
                *child = new WidgetNode(pContext, this, widget);
                return (*child != NULL) ? STATUS_OK : STATUS_NO_MEM;
            }

        }
    }
}


