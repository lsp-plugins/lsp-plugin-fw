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

#include <lsp-plug.in/expr/types.h>

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
                    ForNode & operator = (const ForNode &src);
                    ForNode(const ForNode &);

                protected:
                    enum flags_t
                    {
                        F_ID_SET        = 1 << 0,
                        F_FIRST_SET     = 1 << 1,
                        F_LAST_SET      = 1 << 2,
                        F_STEP_SET      = 1 << 3,
                        F_COUNT_SET     = 1 << 4,
                        F_LIST_SET      = 1 << 5,
                        F_COUNTER_SET   = 1 << 6
                    };

                private:
                    LSPString       sID;
                    LSPString       sList;
                    LSPString       sCounter;
                    ssize_t         nFirst;
                    ssize_t         nLast;
                    ssize_t         nStep;
                    size_t          nFlags;

                protected:
                    status_t            iterate(const expr::value_t *value, ssize_t counter);

                public:
                    explicit ForNode(UIContext *ctx, Node *parent);
                    virtual ~ForNode();

                public:
                    virtual status_t    enter(const LSPString * const *atts);
                    virtual status_t    leave();
            };
        }
    }
}

#endif /* PRIVATE_UI_XML_FORNODE_H_ */

