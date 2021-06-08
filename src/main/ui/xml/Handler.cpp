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
            }

            Handler::Handler(resource::ILoader *loader, Node *root)
            {
                pLoader         = loader;
                vNodes.add(root);
            }

            Handler::~Handler()
            {
                vNodes.flush();
                drop_element();
            }

            status_t Handler::start_element(const LSPString *name, const LSPString * const *atts)
            {
                Node *top        = vNodes.last();
                Node *child      = NULL;
        //        lsp_trace("start: %s", name->get_utf8());

                // Analyze
                if (top != NULL)
                {
                    status_t res = top->start_element(&child, name, atts);
                    if ((res == STATUS_OK) && (child != NULL))
                        res = child->enter();

                    if (res != STATUS_OK)
                        return res;
                }

                return (vNodes.push(child)) ? STATUS_OK : STATUS_NO_MEM;
            }

            status_t Handler::end_element(const LSPString *name)
            {
                status_t res;
                Node *node = NULL, *top = NULL;

                //lsp_trace("end: %s", name->get_utf8());

                // Obtain handlers
                if (!vNodes.pop(&node))
                    return STATUS_CORRUPTED;
                top     = vNodes.last();

                // Call callbacks
                if (node != NULL)
                {
                    if ((res = node->quit()) != STATUS_OK)
                        return res;
                }
                if (top != NULL)
                {
                    if ((res = top->completed(node)) != STATUS_OK)
                        return res;
                    if ((res = top->end_element(name)) != STATUS_OK)
                        return res;
                }

                return STATUS_OK;
            }

            void Handler::drop_element()
            {
                for (size_t i=0, n=vElement.size(); i<n; ++i)
                {
                    LSPString *s = vElement.uget(i);
                    if (s != NULL)
                        delete s;
                }
                vElement.flush();
            }

            status_t Handler::parse(io::IInStream *is, Node *root, size_t flags)
            {
                lsp::xml::PushParser parser;
                io::InSequence sq;

                // Wrap with sequence
                status_t res = sq.wrap(is, flags, "UTF-8");
                if (res != STATUS_OK)
                    return res;

                // Initialize
                sPath.clear();
                drop_element();
                if (!vNodes.push(root))
                    return STATUS_NO_MEM;

                // Parse
                return parser.parse_data(this, &sq, WRAP_CLOSE);
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


