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

#include <private/ui/xml/WidgetNode.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            WidgetNode::WidgetNode(UIContext *ctx, ctl::Widget *widget): Node(ctx)
            {
                pWidget     = widget;
                pChild      = NULL;
                pSpecial    = NULL;
            }

            WidgetNode::~WidgetNode()
            {
                if (pChild != NULL)
                    pChild = NULL;
            }

            status_t WidgetNode::enter()
            {
                pWidget->begin();
                return STATUS_OK;
            }

            status_t WidgetNode::start_element(Node **child, const LSPString *name, const LSPString * const *atts)
            {
                status_t res;

                // Check that
                if (name->starts_with_ascii("ui:"))
                {
                    // Try to instantiate proper node handler
                    for (NodeFactory *f  = NodeFactory::root(); f != NULL; f   = f->next())
                    {
                        if ((res = f->create(&pSpecial, pContext, this, name, atts)) == STATUS_OK)
                        {
                            *child  = pSpecial;
                            return res;
                        }
                        if (res != STATUS_NOT_FOUND)
                            return res;
                    }

                    lsp_error("Unknown meta-tag: <%s>", name->get_native());
                    return STATUS_CORRUPTED;
                }

                // Create and initialize widget
                ctl::Widget *widget         = pContext->create_widget(name, atts);
                if (widget == NULL)
                    return STATUS_OK;       // No handler

                // Create handler
                pChild = new WidgetNode(pContext, widget);
                if (pChild == NULL)
                    return STATUS_NO_MEM;

                *child = pChild;
                return STATUS_OK;
            }

            status_t WidgetNode::quit()
            {
                pWidget->end();
                return STATUS_OK;
            }

            status_t WidgetNode::completed(Node *child)
            {
                status_t res = STATUS_OK;
                if ((child == pChild) && (pChild != NULL))
                {
                    if ((pWidget != NULL) && (pChild->pWidget != NULL))
                    {
                        ctl::Widget *w = pChild->pWidget;
                        if (w != NULL)
                            res = pWidget->add(w);
                    }

                    // Remove child
                    delete pChild;
                    pChild  = NULL;
                }
                else if ((child == pSpecial) && (pSpecial != NULL))
                {
                    Node *special = pSpecial;
                    pSpecial = NULL;

                    res = special->execute();
                    delete special;
                }

                return res;
            }

        }
    }
}


