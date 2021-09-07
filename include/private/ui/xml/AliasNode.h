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

#ifndef PRIVATE_UI_XML_ALIASNODE_H_
#define PRIVATE_UI_XML_ALIASNODE_H_

#include <private/ui/xml/Node.h>
#include <private/ui/xml/PlaybackNode.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * The ui:for node which adds possibility to implement loops in the UI
             */
            class AliasNode: public Node
            {
                private:
                    AliasNode &operator = (const AliasNode &);
                    AliasNode(const AliasNode &);

                public:
                    explicit AliasNode(UIContext *ctx, Node *parent);

                public:
                    virtual status_t init(const LSPString * const *atts);
            };
        }
    }
}



#endif /* PRIVATE_UI_XML_ALIASNODE_H_ */
