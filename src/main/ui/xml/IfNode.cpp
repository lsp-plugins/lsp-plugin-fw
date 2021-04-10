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

#include <private/ui/xml/IfNode.h>
#include <private/ui/xml/NodeFactory.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            //-----------------------------------------------------------------
            NODE_FACTORY_IMPL_START(IfNode)
                if (!name->equals_ascii("ui:if"))
                    return STATUS_NOT_FOUND;

                Node *node = new IfNode(context, parent);
                if (node == NULL)
                    return STATUS_NO_MEM;

                status_t res = node->init(atts);
                if (res != STATUS_OK)
                {
                    delete node;
                    return res;
                }

                *child  = node;
                return STATUS_OK;
            NODE_FACTORY_IMPL_END(IfNode)

            //-----------------------------------------------------------------
            IfNode::IfNode(UIContext *ctx, Node *child): Node(ctx)
            {
                pContext    = ctx;
                pChild      = child;
                bPass       = true;
            }

            IfNode::~IfNode()
            {
                pContext    = NULL;
                pChild      = NULL;
            }

            status_t IfNode::init(const LSPString * const *atts)
            {
                status_t res;
                bool valid = false;

                for ( ; *atts != NULL; atts += 2)
                {
                    const LSPString *name   = atts[0];
                    const LSPString *value  = atts[1];

                    if ((name == NULL) || (value == NULL))
                        continue;

                    if (name->equals_ascii("test"))
                    {
                        if ((res = pContext->eval_bool(&bPass, value)) != STATUS_OK)
                            return res;
                        valid = true;
                    }
                    else
                    {
                        lsp_error("Unknown attribute: %s", name->get_utf8());
                        return STATUS_CORRUPTED;
                    }
                }

                if (!valid)
                {
                    lsp_error("Not all attributes are set");
                    return STATUS_CORRUPTED;
                }

                return STATUS_OK;
            }

            status_t IfNode::start_element(Node **child, const LSPString *name, const LSPString * const *atts)
            {
                return (bPass) ? pChild->start_element(child, name, atts) : STATUS_OK;
            }

            status_t IfNode::end_element(const LSPString *name)
            {
                return (bPass) ? pChild->end_element(name) : STATUS_OK;
            }

            status_t IfNode::completed(Node *child)
            {
                return (bPass) ? pChild->completed(child) : STATUS_OK;
            }

            status_t IfNode::quit()
            {
                return (bPass) ? pChild->quit() : STATUS_OK;
            }

            status_t IfNode::enter()
            {
                return (bPass) ? pChild->enter() : STATUS_OK;
            }

        }
    }
}



