/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/debug.h>

#include <private/ui/xml/DOMControllerNode.h>
#include <private/ui/xml/WidgetNode.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            WidgetNode::WidgetNode(UIContext *ctx, Node *parent, ctl::Widget *widget): Node(ctx, parent)
            {
                pWidget     = widget;
                pChild      = NULL;
            }

            WidgetNode::~WidgetNode()
            {
                if (pChild != NULL)
                    pChild = NULL;
            }

            status_t WidgetNode::lookup(Node **child, const LSPString *name)
            {
                status_t res = Node::lookup(child, name);
                if (res != STATUS_OK)
                    return res;
                if (*child != NULL)
                    return STATUS_OK;

                // Create controller
                ctl::Controller *ctl    = pContext->create_controller(name);

                // Process it as widget if it is a widget
                ctl::Widget *widget     = ctl::ctl_cast<ctl::Widget>(ctl);
                if (widget != NULL)
                {
                    // Create widget handler
                    pChild = new WidgetNode(pContext, this, widget);
                    if (pChild == NULL)
                        return STATUS_NO_MEM;

                    *child = pChild;
                    return STATUS_OK;
                }

                // Process it as a DOM controller if it is a DOM controller
                ctl::DOMController *dom_ctl = ctl::ctl_cast<ctl::DOMController>(ctl);
                if (dom_ctl != NULL)
                {
                    // Create DOM controller handler
                    DOMControllerNode *node = new DOMControllerNode(pContext, this, dom_ctl);
                    *child = node;
                    return (node != NULL) ? STATUS_OK : STATUS_NO_MEM;
                }

                return STATUS_OK;
            }

            status_t WidgetNode::enter(const LSPString * const *atts)
            {
                status_t res;
                lltl::parray<LSPString> tmp;

                // Build list of overridden attributes
                if ((res = pContext->overrides()->build(&tmp, atts)) != STATUS_OK)
                {
                    lsp_error("Error building overridden attributes: %d", int(res));
                    return res;
                }
                atts = tmp.array();

                // Apply overridden widget attributes
                LSPString xvalue;

                pWidget->begin(pContext);
                for ( ; *atts != NULL; atts += 2)
                {
                    // Evaluate attribute value
                    if ((res = pContext->eval_string(&xvalue, atts[1])) != STATUS_OK)
                    {
                        lsp_error(
                            "Error evaluating expression for attribute '%s': %s",
                            atts[0]->get_native(), atts[1]->get_native()
                        );
                        return res;
                    }

                    // Set widget attribute
                    pWidget->set(pContext, atts[0]->get_utf8(), xvalue.get_utf8());
                }

                // Push the state of overrides
                if ((res = pContext->overrides()->push(1)) != STATUS_OK)
                {
                    lsp_error("Error entering new attribute override state: %d", int(res));
                    return res;
                }

                return STATUS_OK;
            }

            status_t WidgetNode::leave()
            {
                status_t res;

                pWidget->end(pContext);

                // Pop state of overrides
                if ((res = pContext->overrides()->pop()) != STATUS_OK)
                {
                    lsp_error("Error restoring override state: %d", int(res));
                    return res;
                }

                return Node::leave();
            }

            status_t WidgetNode::completed(Node *child)
            {
                status_t res = STATUS_OK;

                // Forget the child at exit
                lsp_finally { pChild = NULL; };

                // Link the child widget together with parent widget
                if ((child != pChild) || (pChild == NULL))
                    return res;

                ctl::Widget *w = pChild->pWidget;
                if ((pWidget == NULL) || (w == NULL))
                    return res;

                ctl::Overlay *ov = ctl::ctl_cast<ctl::Overlay>(w);
                if (ov != NULL)
                {
                    // Need to add the widget to the list of overlays
                    res = (pContext->overlays()->add(ov)) ? STATUS_OK : STATUS_NO_MEM;
                    if (res != STATUS_OK)
                        lsp_error(
                            "Error while trying to register overlay widget of type '%s'",
                            w->get_class()->name,
                            pWidget->get_class()->name);
                }
                else
                {
                    res = pWidget->add(pContext, w);
                    if (res != STATUS_OK)
                        lsp_error(
                            "Error while trying to add widget of type '%s' as child for '%s'",
                            w->get_class()->name,
                            pWidget->get_class()->name);
                }

                return res;
            }

        } /* namespac xml */
    } /* namespace ui */
} /* namespace lsp */


