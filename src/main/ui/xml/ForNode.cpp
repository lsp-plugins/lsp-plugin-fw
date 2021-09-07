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

#include <private/ui/xml/ForNode.h>
#include <private/ui/xml/NodeFactory.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            //-----------------------------------------------------------------
            NODE_FACTORY_IMPL_START(ForNode)
                if (!name->equals_ascii("ui:for"))
                    return STATUS_NOT_FOUND;

                *child = new ForNode(context, parent);
                return (*child != NULL) ? STATUS_OK : STATUS_NO_MEM;
            NODE_FACTORY_IMPL_END(ForNode)

            //-----------------------------------------------------------------
            ForNode::ForNode(UIContext *ctx, Node *parent) : PlaybackNode(ctx, parent)
            {
                pID         = NULL;
                nFirst      = 0;
                nLast       = 0;
                nStep       = 1;
            }


            ForNode::~ForNode()
            {
                if (pID != NULL)
                {
                    delete pID;
                    pID     = NULL;
                }
            }

            status_t ForNode::iterate(ssize_t value)
            {
                status_t res;
                if ((res = pContext->vars()->set_int(pID, value)) != STATUS_OK)
                    return res;
                return playback();
            }

            status_t ForNode::init(const LSPString * const *atts)
            {
                bool increment_set = false;
                status_t res;

                for ( ; *atts != NULL; atts += 2)
                {
                    const LSPString *name   = atts[0];
                    const LSPString *value  = atts[1];

                    if ((name == NULL) || (value == NULL))
                        continue;

                    if (name->equals_ascii("id"))
                    {
                        if (pID != NULL)
                            return STATUS_CORRUPTED;
                        LSPString tmp;
                        if ((res = pContext->eval_string(&tmp, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        if ((pID = tmp.release()) == NULL)
                            return STATUS_NO_MEM;
                    }
                    else if (name->equals_ascii("first"))
                    {
                        if ((res = pContext->eval_int(&nFirst, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                    }
                    else if (name->equals_ascii("last"))
                    {
                        if ((res = pContext->eval_int(&nLast, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                    }
                    else if (name->equals_ascii("step"))
                    {
                        if ((res = pContext->eval_int(&nStep, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        increment_set = true;
                    }
                    else
                    {
                        lsp_error("Unknown attribute: %s", name->get_utf8());
                        return STATUS_CORRUPTED;
                    }
                }

                // Compute increment
                if (!increment_set)
                    nStep       = (nFirst <= nLast) ? 1 : -1;

                return STATUS_OK;
            }

            status_t ForNode::execute()
            {
                status_t res;
                if (pID == NULL)
                    return STATUS_OK;

                // Create new scope
                if ((res = pContext->push_scope()) != STATUS_OK)
                    return res;

                // Perform a loop
                if (nFirst <= nLast)
                {
                    for (ssize_t value = nFirst; value <= nLast; value += nStep)
                        if ((res = iterate(value)) != STATUS_OK)
                            break;
                }
                else
                {
                    for (ssize_t value = nFirst; value >= nLast; value += nStep)
                        if ((res = iterate(value)) != STATUS_OK)
                            break;
                }

                // Pop scope and return
                return (res == STATUS_OK) ? pContext->pop_scope() : res;
            }

        }
    }
}


