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
                nFirst      = 0;
                nLast       = 0;
                nStep       = 1;
                nFlags      = 0;
            }


            ForNode::~ForNode()
            {
            }

            status_t ForNode::iterate(const expr::value_t *value)
            {
                status_t res;
                if (nFlags & F_ID_SET)
                {
                    if ((res = pContext->vars()->set(&sID, value)) != STATUS_OK)
                        return res;
                }
                return playback();
            }

            status_t ForNode::enter(const LSPString * const *atts)
            {
                status_t res;
                ssize_t count = 0;

                for ( ; *atts != NULL; atts += 2)
                {
                    const LSPString *name   = atts[0];
                    const LSPString *value  = atts[1];

                    if ((name == NULL) || (value == NULL))
                        continue;

                    // Parse different parameter for 'for' loop
                    if (name->equals_ascii("id"))
                    {
                        if (nFlags & F_ID_SET)
                        {
                            lsp_error("Duplicate attribute '%s': %s", name->get_native(), value->get_native());
                            return STATUS_BAD_FORMAT;
                        }
                        if ((res = pContext->eval_string(&sID, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        nFlags |= F_ID_SET;
                    }
                    else if (name->equals_ascii("first"))
                    {
                        if (nFlags & F_FIRST_SET)
                        {
                            lsp_error("Duplicate attribute '%s': %s", name->get_native(), value->get_native());
                            return STATUS_BAD_FORMAT;
                        }
                        if ((res = pContext->eval_int(&nFirst, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        nFlags |= F_FIRST_SET;
                    }
                    else if (name->equals_ascii("last"))
                    {
                        if (nFlags & F_LAST_SET)
                        {
                            lsp_error("Duplicate attribute '%s': %s", name->get_native(), value->get_native());
                            return STATUS_BAD_FORMAT;
                        }
                        if ((res = pContext->eval_int(&nLast, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        nFlags |= F_LAST_SET;
                    }
                    else if (name->equals_ascii("step"))
                    {
                        if (nFlags & F_STEP_SET)
                        {
                            lsp_error("Duplicate attribute '%s': %s", name->get_native(), value->get_native());
                            return STATUS_BAD_FORMAT;
                        }
                        if ((res = pContext->eval_int(&nStep, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        if (nStep == 0)
                        {
                            lsp_error("Zero 'step' value: %lld", (long long)(nStep));
                            return res;
                        }
                        nFlags |= F_STEP_SET;
                    }
                    else if (name->equals_ascii("count"))
                    {
                        if (nFlags & F_COUNT_SET)
                        {
                            lsp_error("Duplicate attribute '%s': %s", name->get_native(), value->get_native());
                            return STATUS_BAD_FORMAT;
                        }
                        if ((res = pContext->eval_int(&count, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        if (count < 0)
                        {
                            lsp_error("Negative 'count' value: %lld", (long long)(count));
                            return res;
                        }
                        nFlags |= F_COUNT_SET;
                    }
                    else if (name->equals_ascii("list"))
                    {
                        if (nFlags & F_LIST_SET)
                        {
                            lsp_error("Duplicate attribute '%s': %s", name->get_native(), value->get_native());
                            return STATUS_BAD_FORMAT;
                        }
                        if ((res = pContext->eval_string(&sList, value)) != STATUS_OK)
                        {
                            lsp_error("Could not evaluate expression attribute '%s': %s", name->get_native(), value->get_native());
                            return res;
                        }
                        nFlags |= F_LIST_SET;
                    }
                    else
                    {
                        lsp_error("Unknown attribute: %s", name->get_utf8());
                        return STATUS_CORRUPTED;
                    }
                }

                // Check that 'id' attribute is set and other flags are missing
                if (nFlags & F_LIST_SET)
                {
                    if (nFlags & F_FIRST_SET)
                    {
                        lsp_error("Conflicting attributes 'list' and 'first', one should be omitted");
                        return STATUS_BAD_FORMAT;
                    }
                    else if (nFlags & F_LAST_SET)
                    {
                        lsp_error("Conflicting attributes 'list' and 'last', one should be omitted");
                        return STATUS_BAD_FORMAT;
                    }
                    else if (nFlags & F_COUNT_SET)
                    {
                        lsp_error("Conflicting attributes 'list' and 'count', one should be omitted");
                        return STATUS_BAD_FORMAT;
                    }
                    else if (nFlags & F_STEP_SET)
                    {
                        lsp_error("Conflicting attributes 'list' and 'step', one should be omitted");
                        return STATUS_BAD_FORMAT;
                    }
                }

                // Check that there are no conflicting values
                if ((nFlags & (F_FIRST_SET | F_LAST_SET | F_COUNT_SET)) == (F_FIRST_SET | F_LAST_SET | F_COUNT_SET))
                {
                    lsp_error("Conflicting attributes 'first', 'last' and 'count', one should be omitted");
                    return STATUS_BAD_FORMAT;
                }

                // Compute increment
                if (!(nFlags & F_STEP_SET))
                {
                    if ((nFlags & (F_FIRST_SET | F_LAST_SET)) == (F_FIRST_SET | F_LAST_SET))
                        nStep       = (nFirst <= nLast) ? 1 : -1;
                    else
                        nStep       = 1;
                }

                // Now check that 'count' is set and compute first or last
                if (nFlags & F_COUNT_SET)
                {
                    if (nFlags & F_LAST_SET)
                        nFirst  = nLast  - (count - 1) * nStep;
                    else
                        nLast   = nFirst + (count - 1) * nStep;
                }

                return STATUS_OK;
            }

            status_t ForNode::leave()
            {
                status_t res;
                expr::value_t value;

                // Create new scope
                if ((res = pContext->push_scope()) != STATUS_OK)
                    return res;

                expr::init_value(&value);

                if (nFlags & F_LIST_SET)
                {
                    expr::Expression expr;

                    // Evaluate the list
                    if ((res = pContext->evaluate(&expr, &sList, expr::Expression::FLAG_MULTIPLE)) == STATUS_OK)
                    {
                        // Now we can iterate over expression results
                        for (size_t i=0, n=expr.results(); i<n; ++i)
                        {
                            // Read the evaluation result
                            if ((res = expr.result(&value, i)) != STATUS_OK)
                            {
                                lsp_error("Error evaluating list expression: %s", sList.get_native());
                                break;
                            }

                            // Iterate
                            if ((res = iterate(&value)) != STATUS_OK)
                                break;
                        }
                    }
                    else
                    {
                        lsp_error("Error evaluating list expression: %s", sList.get_native());
                    }
                }
                else
                {
                    // Perform a loop from 'first' to 'last' depending on the sign of 'step' value
                    if (nStep > 0)
                    {
                        for (ssize_t x = nFirst; x <= nLast; x += nStep)
                        {
                            expr::set_value_int(&value, x);
                            if ((res = iterate(&value)) != STATUS_OK)
                                break;
                        }
                    }
                    else
                    {
                        for (ssize_t x = nFirst; x >= nLast; x += nStep)
                        {
                            expr::set_value_int(&value, x);
                            if ((res = iterate(&value)) != STATUS_OK)
                                break;
                        }
                    }
                }

                expr::destroy_value(&value);

                // Pop scope and return
                return (res == STATUS_OK) ? pContext->pop_scope() : res;
            }

        }
    }
}


