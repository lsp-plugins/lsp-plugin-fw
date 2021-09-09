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

#include <private/ui/xml/AttributeNode.h>
#include <private/ui/xml/NodeFactory.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            //-----------------------------------------------------------------
            NODE_FACTORY_IMPL_START(AttributeNode)
                if ((!name->equals_ascii("ui:attributes"))
                    && (!name->equals_ascii("ui:with")))
                    return STATUS_NOT_FOUND;

                *child = new AttributeNode(context, parent);
                return (*child != NULL) ? STATUS_OK : STATUS_NO_MEM;
            NODE_FACTORY_IMPL_END(AttributeNode)

            //-----------------------------------------------------------------
            AttributeNode::AttributeNode(UIContext *ctx, Node *parent) :
                Node(ctx, parent),
                sHandler(ctx->wrapper()->resources(), parent)
            {
            }

            AttributeNode::~AttributeNode()
            {
            }

            status_t AttributeNode::enter(const LSPString * const *atts)
            {
                status_t res;
                bool depth_set = false;
                ssize_t depth = -1;

                // Scan for 'Recursion' property
                for (const LSPString * const *p = atts ; *p != NULL; p += 2)
                {
                    const LSPString *name   = p[0];
                    if (name == NULL)
                    {
                        lsp_error("Got NULL attribute name");
                        return STATUS_BAD_ARGUMENTS;
                    }

                    // Check for 'ui:recursion" attribute
                    if (name->equals_ascii("ui:depth"))
                    {
                        // Check if attribute has already been set
                        if (depth_set)
                        {
                            lsp_error("Duplicate attribute '%s'", name->get_native());
                            return STATUS_BAD_FORMAT;
                        }
                        depth_set = true;

                        // Parse value
                        const LSPString *value  = p[1];
                        if (value == NULL)
                        {
                            lsp_error("Got NULL value for attribute '%s'", name->get_native());
                            return STATUS_BAD_ARGUMENTS;
                        }

                        // Evaluate value
                        if ((res = pContext->eval_int(&depth, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                    }
                }

                // Push the state of overrides
                if ((res = pContext->overrides()->push(0)) != STATUS_OK)
                {
                    lsp_error("Error entering new attribute override state: %d", int(res));
                    return res;
                }

                // Apply overrides
                LSPString xvalue;

                for (const LSPString * const *p = atts ; *p != NULL; p += 2)
                {
                    // Get name and value
                    const LSPString *name   = p[0];
                    const LSPString *value  = p[1];

                    // Skip 'ui:recursion" attribute
                    if (name->equals_ascii("ui:depth"))
                        continue;

                    // Compute value
                    if ((res = pContext->eval_string(&xvalue, value)) != STATUS_OK)
                    {
                        lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                        return res;
                    }

                    // Apply override settings
                    if ((res = pContext->overrides()->set(name, &xvalue, depth)) != STATUS_OK)
                    {
                        lsp_error("Error overriding attribute '%s' by value '%s'", name->get_native(), xvalue.get_native());
                        return res;
                    }
                }

                return STATUS_OK;
            }

            status_t AttributeNode::start_element(const LSPString *name, const LSPString * const *atts)
            {
                return sHandler.start_element(name, atts);
            }

            status_t AttributeNode::end_element(const LSPString *name)
            {
                return sHandler.end_element(name);
            }

            status_t AttributeNode::leave()
            {
                status_t res;

                // Restore the state of overrides
                if ((res = pContext->overrides()->pop()) != STATUS_OK)
                {
                    lsp_error("Error restoring override state: %d", int(res));
                    return res;
                }

                // Do not call parent for leave()
                return STATUS_OK;
            }

        }
    }
}


