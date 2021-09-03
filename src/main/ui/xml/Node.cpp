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

#include <private/ui/xml/Node.h>
#include <private/ui/xml/NodeFactory.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            Node::Node(UIContext *ctx)
            {
                pContext        = ctx;
            }

            Node::~Node()
            {
            }

            status_t Node::init(const LSPString * const *atts)
            {
                return STATUS_OK;
            }

            const LSPString *Node::find_attribute(const LSPString * const *atts, const LSPString *name)
            {
                for ( ; *atts != NULL; atts += 2)
                {
                    if (atts[0]->equals(name))
                        return atts[1];
                }

                return NULL;
            }

            const LSPString *Node::find_attribute(const LSPString * const *atts, const char *name)
            {
                LSPString tmp;
                if (!tmp.set_utf8(name))
                    return NULL;
                return find_attribute(atts, &tmp);
            }

            status_t Node::find_meta_node(Node **child, const LSPString *name, const LSPString * const *atts)
            {
                status_t res;
                if (!name->starts_with_ascii("ui:"))
                    return STATUS_NOT_FOUND;

                // Try to instantiate proper node handler
                for (NodeFactory *f  = NodeFactory::root(); f != NULL; f   = f->next())
                {
                    if ((res = f->create(child, pContext, this, name, atts)) == STATUS_OK)
                        return res;
                    if (res != STATUS_NOT_FOUND)
                        return res;
                }

                lsp_error("Unknown meta-tag: <%s>", name->get_native());
                return STATUS_CORRUPTED;
            }

            status_t Node::enter()
            {
                return STATUS_OK;
            }

            status_t Node::start_element(Node **child, const LSPString *name, const LSPString * const *atts)
            {
                return STATUS_OK;
            }

            status_t Node::end_element(const LSPString *name)
            {
                return STATUS_OK;
            }

            status_t Node::quit()
            {
                return STATUS_OK;
            }

            status_t Node::completed(Node *child)
            {
                return STATUS_OK;
            }

            status_t Node::execute()
            {
                return STATUS_OK;
            }
        }
    }
}


