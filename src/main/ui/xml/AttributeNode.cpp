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

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            AttributeNode::AttributeNode(UIContext *ctx, Node *handler) : PlaybackNode(ctx, handler)
            {
                nLevel     = 0;
                nRecursion = 0;
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

            status_t AttributeNode::init(const LSPString * const *atts)
            {
                status_t res;

                // Generate list of appended properties
                for ( ; *atts != NULL; atts += 2)
                {
                    const LSPString *name   = atts[0];
                    const LSPString *value  = atts[1];

                    if ((name == NULL) || (value == NULL))
                        continue;

                    if ((*atts)->equals_ascii("ui:recursion"))
                    {
                        if ((res = pContext->eval_int(&nRecursion, value)) != STATUS_OK)
                            return res;
                    }

                    // Process name
                    LSPString *xname        = name->clone();
                    if (xname == NULL)
                        return STATUS_NO_MEM;
                    else if (!vAtts.add(xname))
                    {
                        delete xname;
                        return STATUS_NO_MEM;
                    }

                    // Process value
                    LSPString *xattr        = new LSPString();
                    if (xattr == NULL)
                        return STATUS_NO_MEM;
                    else if (!vAtts.add(xattr))
                    {
                        delete xattr;
                        return STATUS_NO_MEM;
                    }

                    // Evaluate string
                    if ((res = pContext->eval_string(xattr, value)) != STATUS_OK)
                        return res;
                }

                return STATUS_OK;
            }

            status_t AttributeNode::playback_start_element(lsp::xml::IXMLHandler *handler, const LSPString *name, const LSPString * const *atts)
            {
                size_t level = nLevel++;

                // Skip parameter substitution for control tags
                if (name->starts_with_ascii("ui:"))
                    return PlaybackNode::playback_start_element(handler, name, atts);

                lltl::parray<LSPString> tmp;

                // Need to override attributes?
                if ((nRecursion < 0) || (level <= size_t(nRecursion)))
                {
                    // Copy attributes
                    for (size_t i=0; atts[i] != NULL; ++i)
                        if (!tmp.add(const_cast<LSPString *>(atts[i])))
                            return STATUS_NO_MEM;

                    // Append unexisting attributes
                    LSPString **vatts = vAtts.array();
                    for (size_t i=0, n=vAtts.size(); i<n; i += 2)
                    {
                        LSPString *name   = vatts[i];
                        LSPString *value  = vatts[i+1];

                        // Check for duplicate
                        for (size_t j=0; atts[j] != NULL; j+=2)
                            if (atts[j]->equals(name))
                            {
                                name = NULL;
                                break;
                            }

                        // Append property if it does not exist
                        if (name == NULL)
                            continue;

                        if (!tmp.add(name))
                            return STATUS_NO_MEM;
                        if (!tmp.add(value))
                            return STATUS_NO_MEM;
                    }

                    // Append argument terminator
                    if (!tmp.add(static_cast<LSPString *>(NULL)))
                        return STATUS_NO_MEM;

                    // Override properties with our own list
                    atts = tmp.array();
                }
                return PlaybackNode::playback_start_element(handler, name, atts);
            }

            status_t AttributeNode::playback_end_element(lsp::xml::IXMLHandler *handler, const LSPString *name)
            {
                --nLevel;
                return PlaybackNode::playback_end_element(handler, name);
            }
        }
    }
}


