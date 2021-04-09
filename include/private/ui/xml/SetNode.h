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

#ifndef PRIVATE_UI_XML_SETNODE_H_
#define PRIVATE_UI_XML_SETNODE_H_

#include <private/ui/UIContext.h>
#include <private/ui/xml/Node.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * The ui:for node which adds possibility to declare scoped variables in the UI
             */
            class SetNode: public Node
            {
                private:
                    SetNode & operator = (const SetNode &src);

                public:
                    explicit SetNode(UIContext *ctx);

                public:
                    virtual status_t init(const LSPString * const *atts);
            };
        }
    }
}



#endif /* PRIVATE_UI_XML_SETNODE_H_ */
