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

#include <lsp-plug.in/io/InSequence.h>
#include <lsp-plug.in/io/InFileStream.h>
#include <lsp-plug.in/fmt/xml/PushParser.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/resource/ILoader.h>
#include <lsp-plug.in/common/debug.h>

#include <private/ui/xml/Handler.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            Handler::Handler(resource::ILoader *loader)
            {
                pLoader         = loader;
                sRoot.node      = NULL;
                sRoot.refs      = 0;
            }

            Handler::Handler(resource::ILoader *loader, Node *root)
            {
                pLoader         = loader;
                sRoot.node      = root;
                sRoot.refs      = 1;
            }

            Handler::~Handler()
            {
                // Cleanup stack
                for (ssize_t i=ssize_t(vStack.size()) - 1; i >= 0; --i)
                {
                    node_t *node = vStack.uget(i);
                    if (node->node != NULL)
                    {
                        delete node->node;
                        node->node = NULL;
                    }
                    node->refs  = 0;
                }

                vStack.flush();
                sRoot.node      = NULL;
                sRoot.refs      = 0;
            }

            void Handler::release_node(node_t *node)
            {
                // Do not release root node
                if (node == &sRoot)
                    return;

                // Release node element
                if (node->node != NULL)
                {
                    delete node->node;
                    node->node = NULL;
                }

                // Check if node is last in stack
                if (vStack.last() == node)
                    vStack.pop();

                return;
            }

            status_t Handler::start_element(const LSPString *name, const LSPString * const *atts)
            {
//                LSPString prop;
//                for (const LSPString * const *a = atts; *a != NULL; a += 2)
//                {
//                    prop.append(' ');
//                    prop.append(a[0]);
//                    prop.append('=');
//                    prop.append(a[1]);
//                }
//                lsp_trace("start: %s ->%s", name->get_utf8(), prop.get_utf8());

                node_t *top      = (vStack.size() > 0) ? vStack.last() : &sRoot;

                // Is there a handler for tag?
                if (top->node == NULL)
                {
                    ++top->refs;        // Just increment the number of references
                    return STATUS_OK;
                }

                // Call current node to lookup for nested node
                status_t res;
                Node *child      = NULL;
                if ((res = top->node->lookup(&child, name)) != STATUS_OK)
                {
                    lsp_error("Unknown XML node <%s>", name->get_utf8());
                    return res;
                }

                // No child node found?
                if (child == NULL)
                {
                    // Pass the 'start element' event to the node
                    if ((res = top->node->start_element(name, atts)) != STATUS_OK)
                        return res;

                    ++top->refs;    // Just increment the number of references
                    return STATUS_OK;
                }

                // Child node has been found - enter the node
                if ((res = child->enter(atts)) != STATUS_OK)
                {
                    delete child;
                    return res;
                }

                // Add node to stack
                if ((top = vStack.push()) == NULL)
                {
                    delete child;
                    return STATUS_NO_MEM;
                }

                top->node   = child;
                top->refs   = 1;

                return STATUS_OK;
            }

            status_t Handler::end_element(const LSPString *name)
            {
//                lsp_trace("end: %s", name->get_utf8());

                status_t res;
                node_t *top      = (vStack.size() > 0) ? vStack.last() : &sRoot;

                // If node is still alive, send 'end_element' event and return
                if ((--top->refs) > 0)
                {
                    if (top->node != NULL)
                        return top->node->end_element(name);
                    return STATUS_OK;
                }

                // Call 'leave' callback for the node
                if (top->node != NULL)
                {
                    if ((res = top->node->leave()) != STATUS_OK)
                        return res;
                }

                // Release the node
                release_node(top);

                return STATUS_OK;
            }

            status_t Handler::parse(io::IInStream *is, Node *root, size_t flags)
            {
                io::InSequence sq;

                // Wrap with sequence
                status_t res = sq.wrap(is, flags, "UTF-8");
                if (res != STATUS_OK)
                    return res;

                // Parse
                return parse(&sq, root, WRAP_CLOSE);
            }

            status_t Handler::parse(io::IInSequence *is, Node *root, size_t flags)
            {
                lsp::xml::PushParser parser;

                // Initialize
                sRoot.node  = root;
                sRoot.refs  = 1;

                // Parse
                return parser.parse_data(this, is, flags);
            }

            status_t Handler::parse_file(const LSPString *path, Node *root)
            {
                // Open file
                io::InFileStream ifs;
                status_t res = ifs.open(path);
                if (res != STATUS_OK)
                    return res;

                // Parse the file
                return parse(&ifs, root, WRAP_CLOSE);
            }

            status_t Handler::parse_file(const char *uri, Node *root)
            {
                LSPString tmp;
                if (!tmp.set_utf8(uri))
                    return STATUS_NO_MEM;
                return parse_file(&tmp, root);
            }

            status_t Handler::parse_resource(const LSPString *path, Node *root)
            {
                // Get the loader
                if (pLoader == NULL)
                    return STATUS_NOT_FOUND;

                // Find the resource
                lsp_trace("Reading resource: %s", path->get_native());
                io::IInStream  *is = pLoader->read_stream(path);
                if (is == NULL)
                    return STATUS_NOT_FOUND;

                // Parse the data
                return parse(is, root, WRAP_CLOSE | WRAP_DELETE);
            }

            status_t Handler::parse_resource(const char *uri, Node *root)
            {
                LSPString tmp;
                if (!tmp.set_utf8(uri))
                    return STATUS_NO_MEM;
                return parse_resource(&tmp, root);
            }

            status_t Handler::parse(const LSPString *uri, Node *root)
            {
                // Check for directive of using built-in resource
                if (uri->starts_with_ascii(LSP_BUILTIN_PREFIX))
                    return parse_resource(uri, root);

                status_t res = parse_resource(uri, root);
                if (res == STATUS_NOT_FOUND)
                    res         = parse_file(uri, root);

                return res;
            }

            status_t Handler::parse(const char *uri, Node *root)
            {
                LSPString tmp;
                if (!tmp.set_utf8(uri))
                    return STATUS_NO_MEM;
                return parse(&tmp, root);
            }
        }
    }
} /* namespace lsp */


