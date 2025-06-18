/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 18 февр. 2025 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/common/debug.h>

#include <private/ui/xml/PortNode.h>
#include <private/ui/xml/NodeFactory.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            //-----------------------------------------------------------------
            NODE_FACTORY_IMPL_START(PortNode)
                if (!name->equals_ascii("ui:port"))
                    return STATUS_NOT_FOUND;

                *child = new PortNode(context, parent);
                return (*child != NULL) ? STATUS_OK : STATUS_NO_MEM;
            NODE_FACTORY_IMPL_END(PortNode)

            //-----------------------------------------------------------------
            PortNode::PortNode(UIContext *ctx, Node *parent): Node(ctx, parent)
            {
            }

            status_t PortNode::enter(const LSPString * const *atts)
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

                    if (value == NULL)
                    {
                        lsp_error("Not defined value for attribute '%s'", name->get_native());
                        return STATUS_CORRUPTED;
                    }

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
                ui::EvaluatedPort *port = new ui::EvaluatedPort(pContext->wrapper());
                if (port == NULL)
                {
                    lsp_error("Not enough memory to allocate evaluated port");
                    return STATUS_NO_MEM;
                }
                lsp_finally {
                    if (port != NULL)
                        delete port;
                };

                // Compile evaluated port
                if ((res = port->compile(&v_value)) != STATUS_OK)
                {
                    lsp_error("Error compiling expression for port='%s', error=%d, expression=%s",
                        v_id.get_native(), int(res), v_value.get_native());
                    return res;
                }

                // Add port to list of evaluated ports
                if ((res = pContext->wrapper()->add_evaluated_port(&v_id, port)) != STATUS_OK)
                {
                    lsp_error("Error registering evaluated port id='%s', error=%d", v_id.get_native(), int(res));
                    return res;
                }

                // Do not destroy evaluated port
                port    = NULL;

                return res;
            }

        } /* namespac xml */
    } /* namespace ui */
} /* namespace lsp */



