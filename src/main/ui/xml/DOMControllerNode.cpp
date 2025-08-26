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

#include <lsp-plug.in/common/debug.h>

#include <private/ui/xml/DOMControllerNode.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            DOMControllerNode::DOMControllerNode(UIContext *ctx, Node *parent, ctl::DOMController *ctl): Node(ctx, parent)
            {
                pController = ctl;
            }

            DOMControllerNode::~DOMControllerNode()
            {
                if (pController != NULL)
                    pController = NULL;
            }

            status_t DOMControllerNode::enter(const LSPString * const *atts)
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

                pController->begin(pContext);
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
                    pController->set(pContext, atts[0]->get_utf8(), xvalue.get_utf8());
                }

                // Push the state of overrides
                if ((res = pContext->overrides()->push(1)) != STATUS_OK)
                {
                    lsp_error("Error entering new attribute override state: %d", int(res));
                    return res;
                }

                return STATUS_OK;
            }

            status_t DOMControllerNode::leave()
            {
                status_t res;

                pController->end(pContext);

                // Pop state of overrides
                if ((res = pContext->overrides()->pop()) != STATUS_OK)
                {
                    lsp_error("Error restoring override state: %d", int(res));
                    return res;
                }

                return Node::leave();
            }

        } /* namespac xml */
    } /* namespace ui */
} /* namespace lsp */


