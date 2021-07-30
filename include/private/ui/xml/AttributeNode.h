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

#ifndef UI_XML_ATTRIBUTENODE_H_
#define UI_XML_ATTRIBUTENODE_H_

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/ui.h>

#include <private/ui/xml/Node.h>
#include <private/ui/xml/PlaybackNode.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * The ui:attribute node that allows to assign additional attributes to nested tags
             */
            class AttributeNode: public PlaybackNode
            {
                private:
                    AttributeNode &operator = (const AttributeNode &);
                    AttributeNode(const AttributeNode &);

                private:
                    lltl::parray<LSPString> vAtts;
                    size_t                  nLevel;
                    ssize_t                 nRecursion;

                public:
                    explicit AttributeNode(UIContext *ctx, Node *handler);

                    virtual ~AttributeNode();

                public:
                    virtual status_t init(const LSPString * const *atts);

                    virtual status_t playback_start_element(lsp::xml::IXMLHandler *handler, const LSPString *name, const LSPString * const *atts);

                    virtual status_t playback_end_element(lsp::xml::IXMLHandler *handler, const LSPString *name);
            };
        }
    }
}



#endif /* UI_XML_ATTRIBUTENODE_H_ */
