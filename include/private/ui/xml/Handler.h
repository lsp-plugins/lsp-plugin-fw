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

#ifndef PRIVATE_UI_XML_HANDLER_H_
#define PRIVATE_UI_XML_HANDLER_H_

#include <lsp-plug.in/fmt/xml/IXMLHandler.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/resource/ILoader.h>

#include <private/ui/xml/Node.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * The XML event handler that handles all XML events in the document
             */
            class Handler: public lsp::xml::IXMLHandler
            {
                private:
                    Handler & operator = (const Handler &);
                    Handler(const Handler &);

                protected:
                    resource::ILoader          *pLoader;
                    lltl::parray<Node>          vNodes;
                    lltl::parray<LSPString>     vElement;
                    LSPString                   sPath;

                protected:
                    void            drop_element();
                    LSPString      *fetch_element_string(const void **data);
                    status_t        parse(io::IInStream *is, Node *root, size_t flags);

                public:
                    explicit Handler(resource::ILoader *loader);
                    explicit Handler(resource::ILoader *loader, Node *root);
                    virtual ~Handler();

                public:
                    virtual status_t start_element(const LSPString *name, const LSPString * const *atts);

                    virtual status_t end_element(const LSPString *name);

                public:
                    /**
                     * Parse resource from file
                     * @param path path to the file
                     * @param root root node that will handle XML data
                     * @return status of operation
                     */
                    status_t parse_file(const LSPString *path, Node *root);
                    status_t parse_file(const char *path, Node *root);

                    /**
                     * Parse resource from resource descriptor
                     * @param path path to the resource
                     * @param root root node that will handle XML data
                     * @return status of operation
                     */
                    status_t parse_resource(const LSPString *path, Node *root);
                    status_t parse_resource(const char *path, Node *root);

                    /**
                     * Parse resource at specified URI. Depending on compilation flags,
                     * the URI will point at builtin resource or at local filesystem resource
                     * @param uri URI of the resource
                     * @param root root node that will handle XML data
                     * @return status of operation
                     */
                    status_t parse(const LSPString *uri, Node *root);
                    status_t parse(const char *uri, Node *root);
            };
        }
    }
}



#endif /* PRIVATE_UI_XML_HANDLER_H_ */
