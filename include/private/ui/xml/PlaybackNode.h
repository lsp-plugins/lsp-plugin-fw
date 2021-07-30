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

#ifndef PRIVATE_UI_XML_PLAYBACKNODE_H_
#define PRIVATE_UI_XML_PLAYBACKNODE_H_

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/fmt/xml/IXMLHandler.h>
#include <lsp-plug.in/lltl/parray.h>

#include <private/ui/xml/Node.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            /**
             * Node that records all XML events and plays them back
             */
            class PlaybackNode: public Node
            {
                private:
                    PlaybackNode & operator = (const PlaybackNode &);
                    PlaybackNode(const PlaybackNode &);

                protected:
                    enum event_t
                    {
                        EVT_START_ELEMENT,
                        EVT_END_ELEMENT
                    };

                    typedef struct xml_event_t
                    {
                        event_t                     nEvent;
                        lltl::parray<LSPString>     vData;

                        inline xml_event_t(event_t type);
                        ~xml_event_t();
                        status_t                    add_param(const LSPString *name);
                    } xml_event_t;

                private:
                    Node                           *pHandler;
                    lltl::parray<xml_event_t>       vEvents;

                protected:
                    status_t                playback();
                    xml_event_t            *add_event(event_t ev);

                public:
                    explicit PlaybackNode(UIContext *ctx, Node *handler);
                    virtual ~PlaybackNode();

                public:
                    /**
                     * Playback start element
                     * @param handler XML handler
                     * @param name element name
                     * @param atts element attributes
                     * @return status of operation
                     */
                    virtual status_t        playback_start_element(lsp::xml::IXMLHandler *handler, const LSPString *name, const LSPString * const *atts);

                    /**
                     * Playback end element
                     * @param handler XML handler
                     * @param name element name
                     * @param atts element attributes
                     * @return status of operation
                     */
                    virtual status_t        playback_end_element(lsp::xml::IXMLHandler *handler, const LSPString *name);

                    virtual status_t        execute();

                    virtual status_t        start_element(Node **child, const LSPString *name, const LSPString * const *atts);

                    virtual status_t        end_element(const LSPString *name);

            };
        }
    }
}

#endif /* PRIVATE_UI_XML_PLAYBACKNODE_H_ */
