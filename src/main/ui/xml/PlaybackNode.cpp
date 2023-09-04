/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <private/ui/xml/PlaybackNode.h>
#include <private/ui/xml/Handler.h>

namespace lsp
{
    namespace ui
    {
        namespace xml
        {
            //-----------------------------------------------------------------
            PlaybackNode::xml_event_t::xml_event_t(event_t type)
            {
                nEvent      = type;
            }

            PlaybackNode::xml_event_t::~xml_event_t()
            {
                for (size_t i=0, n=vData.size(); i<n; ++i)
                {
                    LSPString *s = vData.uget(i);
                    if (s != NULL)
                        delete s;
                }
                vData.flush();
            }

            status_t PlaybackNode::xml_event_t::add_param(const LSPString *name)
            {
                LSPString *tmp;
                if ((tmp = name->clone()) == NULL)
                    return STATUS_NO_MEM;
                else if (!vData.add(tmp))
                {
                    delete tmp;
                    return STATUS_NO_MEM;
                }
                return STATUS_OK;
            }

            PlaybackNode::xml_event_t *PlaybackNode::add_event(event_t ev)
            {
                xml_event_t *evt        = new xml_event_t(ev);
                if (evt == NULL)
                    return NULL;
                else if (!vEvents.add(evt))
                {
                    delete evt;
                    evt = NULL;
                }
                return evt;
            }

            //-----------------------------------------------------------------
            PlaybackNode::PlaybackNode(UIContext *ctx, Node *parent): Node(ctx, parent)
            {
            }

            PlaybackNode::~PlaybackNode()
            {
                for (size_t i=0, n=vEvents.size(); i<n; ++i)
                {
                    xml_event_t *ev = vEvents.uget(i);
                    if (ev != NULL)
                        delete ev;
                }
                vEvents.flush();
            }

            status_t PlaybackNode::lookup(Node **child, const LSPString *name)
            {
                // Playback nodes should not handle nested nodes until playback() is executed
                *child      = NULL;
                return STATUS_OK;
            }

            status_t PlaybackNode::playback_start_element(lsp::xml::IXMLHandler *handler, const LSPString *name, const LSPString * const *atts)
            {
                return handler->start_element(name, atts);
            }

            status_t PlaybackNode::playback_end_element(lsp::xml::IXMLHandler *handler, const LSPString *name)
            {
                return handler->end_element(name);
            }

            status_t PlaybackNode::playback()
            {
                status_t res = STATUS_OK;
                Handler h(pContext->wrapper()->resources(), pParent);

                for (size_t i=0, n=vEvents.size(); i<n; ++i)
                {
                    // Fetch event
                    xml_event_t *ev = vEvents.uget(i);
                    if (ev == NULL)
                    {
                        res = STATUS_CORRUPTED;
                        break;
                    }

                    // Parse event
                    LSPString **atts = ev->vData.array();
                    switch (ev->nEvent)
                    {
                        case EVT_START_ELEMENT:
                            res = playback_start_element(&h, atts[0], &atts[1]);
                            break;
                        case EVT_END_ELEMENT:
                            res = playback_end_element(&h, atts[0]);
                            break;
                        default:
                            res = STATUS_CORRUPTED;
                            break;
                    }

                    // Check result
                    if (res != STATUS_OK)
                        break;
                }

                return res;
            }

            status_t PlaybackNode::leave()
            {
                return playback();
            }

            status_t PlaybackNode::start_element(const LSPString *name, const LSPString * const *atts)
            {
                // Allocate event
                status_t res;
                xml_event_t *evt        = add_event(EVT_START_ELEMENT);
                if (evt == NULL)
                    return STATUS_NO_MEM;

                // Clone element name
                if ((res = evt->add_param(name)) != STATUS_OK)
                    return res;

                // Clone tag attributes
                for ( ; *atts != NULL; ++atts)
                {
                    // Clone attribute
                    if ((res = evt->add_param(*atts)) != STATUS_OK)
                        return res;
                }

                // Add terminator
                if (!evt->vData.add(static_cast<LSPString *>(NULL)))
                    return STATUS_NO_MEM;

                return STATUS_OK;
            }

            status_t PlaybackNode::end_element(const LSPString *name)
            {
                // Allocate event and add parameter
                xml_event_t *evt        = add_event(EVT_END_ELEMENT);
                return (evt != NULL) ? evt->add_param(name) : STATUS_NO_MEM;
            }
        } /* namespac xml */
    } /* namespace ui */
} /* namespace lsp */


