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

#ifndef PRIVATE_UI_XML_FORNODE_H_
#define PRIVATE_UI_XML_FORNODE_H_

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
            class ForNode: public PlaybackNode
            {
                private:
                    LSPString      *pID;
                    ssize_t         nFirst;
                    ssize_t         nLast;
                    ssize_t         nStep;

                protected:
                    status_t            iterate(ssize_t value);

                public:
                    explicit ForNode(UIContext *ctx, Node *handler);
                    virtual ~ForNode();

                    virtual status_t    init(const LSPString * const *atts);

                public:
                    virtual status_t    execute();
            };
        }
    }
}

#endif /* PRIVATE_UI_XML_FORNODE_H_ */

