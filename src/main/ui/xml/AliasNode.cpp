/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 июл. 2021 г.
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

#include <private/ui/xml/AliasNode.h>
#include <private/ui/xml/NodeFactory.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            //-----------------------------------------------------------------
            NODE_FACTORY_IMPL_START(AliasNode)
                if (!name->equals_ascii("ui:alias"))
                    return STATUS_NOT_FOUND;

                *child = new AliasNode(context, parent);
                return (*child != NULL) ? STATUS_OK : STATUS_NO_MEM;
            NODE_FACTORY_IMPL_END(AliasNode)

            //-----------------------------------------------------------------
            AliasNode::AliasNode(UIContext *ctx, Node *parent): Node(ctx, parent)
            {
            }

            status_t AliasNode::enter(const LSPString * const *atts)
            {
                enum parse_flags_t
                {
                    PF_ID_SET = 1 << 0,
                    PF_VALUE_SET = 1 << 1
                };

                status_t res;
                size_t flags = 0;
                LSPString v_id, v_value;

                for ( ; *atts != NULL; atts += 2)
                {
                    const LSPString *name   = atts[0];
                    const LSPString *value  = atts[1];

                    if ((name == NULL) || (value == NULL))
                        continue;

                    if (name->equals_ascii("id"))
                    {
                        if ((res = pContext->eval_string(&v_id, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression for attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        flags      |= PF_ID_SET;
                    }
                    else if (name->equals_ascii("value"))
                    {
                        if ((res = pContext->eval_string(&v_value, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        flags      |= PF_VALUE_SET;
                    }
                    else
                    {
                        lsp_error("Unknown attribute: '%s' for ui:alias tag", name->get_utf8());
                        return STATUS_CORRUPTED;
                    }
                }

                if (flags != (PF_ID_SET | PF_VALUE_SET))
                {
                    lsp_error("Not all attributes are set for ui:alias tag");
                    return STATUS_CORRUPTED;
                }

                // Set port alias
                if ((res = pContext->wrapper()->set_port_alias(&v_id, &v_value)) != STATUS_OK)
                    lsp_error("Error creating alias id='%s' to value='%s', error=%d", v_id.get_native(), v_value.get_native(), int(res));

                return res;
            }

        }
    }
}


