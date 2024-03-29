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

#ifndef PRIVATE_UI_XML_ROOTNODE_H_
#define PRIVATE_UI_XML_ROOTNODE_H_

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl.h>

#include <private/ui/xml/Node.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * Root node for the XML document, creates the root widget controller and launches
             * the recursive XML parsing
             */
            class RootNode: public Node
            {
                private:
                    RootNode & operator = (const RootNode &);
                    RootNode(const RootNode &);

                private:
                    ctl::Widget    *pWidget;
                    LSPString       sName;

                public:
                    explicit RootNode(UIContext *ctx, const char *name, ctl::Widget *widget);
                    virtual ~RootNode();

                public:
                    virtual status_t        lookup(Node **child, const LSPString *name);
            };

        }
    }
}

#endif /* PRIVATE_UI_XML_ROOTNODE_H_ */
