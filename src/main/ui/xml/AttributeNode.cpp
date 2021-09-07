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
            AttributeNode::AttributeNode(UIContext *ctx, Node *parent) : PlaybackNode(ctx, parent)
            {
                nDepth  = 0;
            }

            AttributeNode::~AttributeNode()
            {
                for (size_t i=0, n=vAtts.size(); i<n; ++i)
                {
                    LSPString *s = vAtts.uget(i);
                    if (s != NULL)
                        delete s;
                }
                vAtts.flush();
            }

            status_t AttributeNode::enter(const LSPString * const *atts)
            {
// TODO
//                status_t res;
//
//                // Generate list of appended properties
//                for ( ; *atts != NULL; atts += 2)
//                {
//                    const LSPString *name   = atts[0];
//                    const LSPString *value  = atts[1];
//
//                    if ((name == NULL) || (value == NULL))
//                        continue;
//
//                    if ((*atts)->equals_ascii("ui:depth"))
//                    {
//                        if ((res = pContext->eval_int(&nDepth, value)) != STATUS_OK)
//                        {
//                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
//                            return res;
//                        }
//                    }
//
//                    // Process name
//                    LSPString *xname        = name->clone();
//                    if (xname == NULL)
//                        return STATUS_NO_MEM;
//                    else if (!vAtts.add(xname))
//                    {
//                        delete xname;
//                        return STATUS_NO_MEM;
//                    }
//
//                    // Process value
//                    LSPString *xattr        = new LSPString();
//                    if (xattr == NULL)
//                        return STATUS_NO_MEM;
//                    else if (!vAtts.add(xattr))
//                    {
//                        delete xattr;
//                        return STATUS_NO_MEM;
//                    }
//
//                    // Evaluate string
//                    if ((res = pContext->eval_string(xattr, value)) != STATUS_OK)
//                    {
//                        lsp_error("Could not evaluate expression attribute '%s': %s", xname->get_native(), value->get_native());
//                        return res;
//                    }
//                }

                return STATUS_OK;
            }

        }
    }
}


