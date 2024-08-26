/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 авг. 2024 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/plug-fw/core/AudioBuffer.h>
#include <lsp-plug.in/plug-fw/core/ShmClient.h>
#include <lsp-plug.in/plug-fw/core/ShmState.h>

namespace lsp
{
    namespace core
    {
        //---------------------------------------------------------------------
        ShmClient::Listener::Listener(ShmClient *client)
        {
            pClient     = client;
        }

        ShmClient::Listener::~Listener()
        {
            pClient     = NULL;
        }

        bool ShmClient::Listener::update(dspu::Catalog *catalog)
        {
            if (pClient != NULL)
                return pClient->update_catalog(catalog);
            return true;
        }

        //---------------------------------------------------------------------
        ShmClient::ShmClient():
            sState(shm_state_deleter)
        {
            pWrapper        = NULL;
            pFactory        = NULL;
            pCatalog        = NULL;
            pListener       = NULL;
            nSampleRate     = 0;
            nBufferSize     = 0;
        }

        ShmClient::~ShmClient()
        {
            destroy();
        }

        void ShmClient::destroy_send(send_t *item)
        {
            if (item == NULL)
                return;

            if (item->sID != NULL)
                lsp_trace("Destroying send id=%s", item->sID);

            if (item->pSend != NULL)
            {
                item->pSend->detach();
                delete item->pSend;
                item->pSend     = NULL;
            }
            free(item);
        }

        void ShmClient::destroy_return(return_t *item)
        {
            if (item == NULL)
                return;

            if (item->sID != NULL)
                lsp_trace("Destroying return id=%s", item->sID);

            if (item->pReturn != NULL)
            {
                item->pReturn->detach();
                delete item->pReturn;
                item->pReturn   = NULL;
            }
            free(item);
        }

        void ShmClient::destroy()
        {
            // Destroy listener
            if (pListener != NULL)
            {
                pListener->detach();
                delete pListener;
                pListener = NULL;
            }

            // Destroy sends
            for (size_t i=0, n=vSends.size(); i<n; ++i)
                destroy_send(vSends.uget(i));
            vSends.flush();

            // Destroy returns
            for (size_t i=0, n=vReturns.size(); i<n; ++i)
                destroy_return(vReturns.uget(i));
            vReturns.flush();

            // Release catalog
            if (pCatalog != NULL)
            {
                lsp_trace("Releasing shared memory catalog");
                if (pFactory != NULL)
                    pFactory->release_catalog(pCatalog);
                pCatalog = NULL;
            }

            // Forget anything about factory
            pFactory = NULL;
        }

        size_t ShmClient::channels_count(const char *id, lltl::parray<plug::IPort> *ports)
        {
            const size_t count = ports->size();
            size_t max_index = 0;

            for (size_t i=0; i < count; ++i)
            {
                plug::IPort *ap = ports->uget(i);
                const meta::port_t *meta = ap->metadata();
                if ((meta->value == NULL) || (strcmp(meta->value, id) != 0))
                    continue;

                max_index = lsp_max(max_index, size_t(meta->start));
            }

            return max_index + 1;
        }

        void ShmClient::bind_channels(plug::IPort **channels, const char *id, lltl::parray<plug::IPort> *ports)
        {
            // Bind channels to send
            const size_t count = ports->size();
            for (size_t i=0; i < count; ++i)
            {
                plug::IPort *ap         = ports->uget(i);
                const meta::port_t *meta = ap->metadata();
                if ((meta->value == NULL) || (strcmp(meta->value, id) != 0))
                    continue;

                const size_t index      = size_t(meta->start);
                channels[index]         = ap;
            }
        }

        void ShmClient::create_send(plug::IPort *p, lltl::parray<plug::IPort> *sends)
        {
            const meta::port_t *port    = p->metadata();

            // Estimate the number of channels per send
            const size_t channels       = channels_count(port->id, sends);

            // Create item
            const size_t to_alloc   = sizeof(send_t) + sizeof(plug::IPort *) * channels;
            send_t *item            = reinterpret_cast<send_t *>(malloc(to_alloc));
            lsp_finally { destroy_send(item); };

            item->sID               = port->id;
            item->nChannels         = channels;
            item->bActive           = false;
            item->bPublish          = true;
            item->pSend             = new core::AudioSend();
            item->sLastName[0]      = '\0';
            item->fLastSerial       = -1;
            if (item->pSend == NULL)
                return;

            item->pName             = p;
            for (size_t i=0; i < channels; ++i)
                item->vChannels[i]      = NULL;

            // Bind channels to send
            bind_channels(item->vChannels, port->id, sends);

            // Add send
            if (vSends.add(item))
            {
                lsp_trace("Created send id=%s, channels=%d", item->sID, int(channels));
                item                    = NULL;
            }
        }

        void ShmClient::create_return(plug::IPort *sp, lltl::parray<plug::IPort> *returns)
        {
            const meta::port_t *port = sp->metadata();

            // Estimate the number of channels per return
            const size_t channels       = channels_count(port->id, returns);

            // Create item
            const size_t to_alloc   = sizeof(return_t) + sizeof(plug::IPort *) * channels;
            return_t *item          = reinterpret_cast<return_t *>(malloc(to_alloc));
            lsp_finally { destroy_return(item); };

            item->sID               = port->id;
            item->nChannels         = channels;
            item->bActive           = false;
            item->bConnect          = true;
            item->pReturn           = new core::AudioReturn();
            item->sLastName[0]      = '\0';
            item->fLastSerial       = -1;
            if (item->pReturn == NULL)
                return;

            item->pName             = sp;
            for (size_t i=0; i < channels; ++i)
                item->vChannels[i]      = NULL;

            // Bind channels to return
            bind_channels(item->vChannels, port->id, returns);

            // Add return
            if (vReturns.add(item))
            {
                lsp_trace("Created return id=%s, channels=%d", item->sID, int(channels));
                item                    = NULL;
            }
        }

        void ShmClient::scan_ports(lltl::parray<plug::IPort> *dst, meta::role_t role, plug::IPort **ports, size_t count)
        {
            dst->clear();
            for (size_t i=0; i<count; ++i)
            {
                plug::IPort *p = ports[i];
                const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                if ((meta != NULL) && (meta->role == role))
                    dst->add(p);
            }
        }

        void ShmClient::shm_state_deleter(ShmState *state)
        {
            delete state;
        }

        void ShmClient::init(plug::IWrapper *wrapper, core::ICatalogFactory *factory, plug::IPort **ports, size_t count)
        {
            pWrapper = wrapper;
            pFactory = factory;

            lltl::parray<plug::IPort> items;

            // Create sends
            scan_ports(&items, meta::R_AUDIO_SEND, ports, count);
            if (!items.is_empty())
            {
                for (size_t i=0; i<count; ++i)
                {
                    plug::IPort *p = ports[i];
                    const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                    if ((meta != NULL) && (meta->role == meta::R_SEND_NAME))
                        create_send(p, &items);
                }
            }

            // Create returns
            scan_ports(&items, meta::R_AUDIO_RETURN, ports, count);
            if (!items.is_empty())
            {
                for (size_t i=0; i<count; ++i)
                {
                    plug::IPort *p = ports[i];
                    const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                    if ((meta != NULL) && (meta->role == meta::R_RETURN_NAME))
                        create_return(p, &items);
                }
            }

            // Attach sends and returns to catalog
            pCatalog    = pFactory->acquire_catalog();
            if (pCatalog == NULL)
                return;

            for (size_t i=0, n=vSends.size(); i<n; ++i)
            {
                send_t *s   = vSends.uget(i);
                if ((s == NULL) || (s->pSend == NULL))
                    continue;
                if (s->pSend->attach(pCatalog) == STATUS_OK)
                    lsp_trace("Attached send id=%s to shared memory catalog", s->sID);
            }

            for (size_t i=0, n=vReturns.size(); i<n; ++i)
            {
                return_t *r = vReturns.uget(i);
                if ((r == NULL) || (r->pReturn == NULL))
                    continue;
                if (r->pReturn->attach(pCatalog) == STATUS_OK)
                    lsp_trace("Attached return id=%s to shared memory catalog", r->sID);
            }

            // Attach listener to catalog
            const meta::plugin_t *plug = wrapper->metadata();
            if ((vSends.size() > 0) ||
                (vReturns.size() > 0) ||
                ((plug != NULL) && (plug->extensions & meta::E_SHM_TRACKING)))
            {
                pListener = new Listener(this);
                if (pListener != NULL)
                {
                    if (pListener->attach(pCatalog) == STATUS_OK)
                        lsp_trace("Attached catalog listener to shared memory catalog");
                }
            }
        }

        void ShmClient::set_buffer_size(size_t size)
        {
            if (nBufferSize == size)
                return;

            nBufferSize     = size;

            // Force sends to re-publish data
            for (size_t i=0, n=vSends.size(); i<n; ++i)
            {
                send_t *s       = vSends.uget(i);
                if (s != NULL)
                    s->bPublish     = true;
            }
        }

        void ShmClient::set_sample_rate(size_t sample_rate)
        {
            if (nSampleRate == sample_rate)
                return;

            // Force sends to re-publish data
            for (size_t i=0, n=vSends.size(); i<n; ++i)
            {
                send_t *s       = vSends.uget(i);
                if (s != NULL)
                    s->bPublish     = true;
            }
        }

        bool ShmClient::connection_updated(send_t *s)
        {
            const float serial = s->pName->value();
            const char *new_name = s->pName->buffer<char>();
            size_t new_len      = strlen(new_name) + 1;
            if (new_len > MAX_SHM_SEGMENT_NAME_BYTES)
            {
                new_name            = "";
                new_len             = 0;
            }

            if ((s->fLastSerial == serial) && (memcmp(new_name, s->sLastName, new_len) == 0))
                return false;

            s->fLastSerial      = serial;
            memcpy(s->sLastName, new_name, new_len);

            return true;
        }

        bool ShmClient::connection_updated(return_t *r)
        {
            const float serial = r->pName->value();
            const char *new_name = r->pName->buffer<char>();
            size_t new_len      = strlen(new_name) + 1;
            if (new_len > MAX_SHM_SEGMENT_NAME_BYTES)
            {
                new_name            = "";
                new_len             = 0;
            }

            if ((r->fLastSerial == serial) && (memcmp(new_name, r->sLastName, new_len) == 0))
                return false;

            r->fLastSerial      = serial;
            memcpy(r->sLastName, new_name, new_len);

            return true;
        }

        void ShmClient::update_settings()
        {
            // Check that send settings have been updated
            for (size_t i=0, n=vSends.size(); i<n; ++i)
            {
                send_t *s       = vSends.uget(i);
                if ((s == NULL) || (s->pName == NULL))
                    continue;

                // Check that send name has changed
                if (connection_updated(s))
                    s->bPublish     = true;
            }

            // Check that return settings have been updated
            for (size_t i=0, n=vReturns.size(); i<n; ++i)
            {
                return_t *r     = vReturns.uget(i);
                if ((r == NULL) || (r->pName == NULL))
                    continue;

                // Check that send name has changed
                if (connection_updated(r))
                    r->bConnect     = true;
            }
        }

        void ShmClient::begin(size_t samples)
        {
            // Trigger start of processing for sends and returns
            for (size_t i=0, n=vSends.size(); i<n; ++i)
            {
                send_t *s       = vSends.uget(i);
                if ((s != NULL) && (s->pSend != NULL))
                {
                    // Publish if publish is pending
                    if (s->bPublish)
                    {
                        s->pSend->publish(s->sLastName, s->nChannels, nBufferSize * 16);
                        s->bPublish = false;
                    }

                    // Check that send is overridden by another send
                    if (s->pSend->overridden())
                    {
                        s->pName->set_default();        // Tell the UI to reset the state
                        pWrapper->state_changed();
                    }

                    // Check send activity
                    s->bActive = s->pSend->active();
                    for (size_t j=0, m=s->nChannels; j<m; ++j)
                    {
                        core::AudioBuffer *ab = s->vChannels[j]->buffer<core::AudioBuffer>();
                        if (ab != NULL)
                            ab->set_active(s->bActive);
                    }

                    if (s->bActive)
                        s->pSend->begin(samples);
                }
            }

            for (size_t i=0, n=vReturns.size(); i<n; ++i)
            {
                return_t *r     = vReturns.uget(i);
                if ((r != NULL) && (r->pReturn != NULL))
                {
                    // Connect if connection is pending
                    if (r->bConnect)
                    {
                        r->pReturn->connect(r->sLastName);
                        r->bConnect = false;
                    }

                    // Check activity of the return
                    r->bActive = r->pReturn->active();
                    for (size_t j=0, m=r->nChannels; j<m; ++j)
                    {
                        plug::IPort *p = r->vChannels[j];
                        if (p == NULL)
                            continue;

                        core::AudioBuffer *ab = p->buffer<core::AudioBuffer>();
                        ab->set_active(r->bActive);
                    }

                    if (r->bActive)
                        r->pReturn->begin(samples);
                }
            }
        }

        void ShmClient::pre_process(size_t samples)
        {
            // Receive data from returns
            for (size_t i=0, n=vReturns.size(); i<n; ++i)
            {
                return_t *r     = vReturns.uget(i);
                if (r == NULL)
                    continue;

                const bool active = r->bActive;
                for (size_t j=0; j<r->nChannels; ++j)
                {
                    plug::IPort *p = r->vChannels[j];
                    if (p == NULL)
                        continue;

                    core::AudioBuffer *ab = p->buffer<core::AudioBuffer>();
                    if (ab == NULL)
                        continue;

                    if (active)
                    {
                        float *buffer = ab->data();
                        if (buffer != NULL)
                            r->pReturn->read_sanitized(j, buffer, samples);

                        ab->set_dirty();    // Reset clean flag
                    }
                    else
                        ab->set_clean();    // Request cleanup
                }
            }
        }

        void ShmClient::post_process(size_t samples)
        {
            // Transmit data to sends
            for (size_t i=0, n=vSends.size(); i<n; ++i)
            {
                send_t *s       = vSends.uget(i);
                if ((s == NULL) || (!s->bActive))
                    continue;

                for (size_t j=0; j<s->nChannels; ++j)
                {
                    plug::IPort *p = s->vChannels[j];
                    if (p == NULL)
                        continue;

                    core::AudioBuffer *ab = p->buffer<core::AudioBuffer>();
                    if (ab == NULL)
                        continue;

                    const float *buffer = ab->data();
                    if (buffer != NULL)
                        s->pSend->write_sanitized(j, buffer, samples);
                }
            }
        }

        void ShmClient::end()
        {
            // Trigger end of processing for sends and returns
            for (size_t i=0, n=vSends.size(); i<n; ++i)
            {
                send_t *s       = vSends.uget(i);
                if ((s != NULL) && (s->pSend != NULL) && (s->bActive))
                {
                    s->pSend->end();
                    s->bActive  = false;
                }
            }

            for (size_t i=0, n=vReturns.size(); i<n; ++i)
            {
                return_t *r     = vReturns.uget(i);
                if ((r != NULL) && (r->pReturn != NULL) && (r->bActive))
                {
                    r->pReturn->end();
                    r->bActive  = false;
                }
            }
        }

        bool ShmClient::update_catalog(dspu::Catalog *catalog)
        {
            lltl::parray<dspu::Catalog::Record> records;
            status_t res = catalog->enumerate(&records);
            if (res != STATUS_OK)
                return false;

            // Create records
            lltl::darray<ShmRecord> shm_items;
            io::OutMemoryStream os;
            for (size_t i=0, n=records.size(); i<n; ++i)
            {
                const dspu::Catalog::Record *src = records.uget(i);
                if (src == NULL)
                    continue;

                const char *id = src->id.get_utf8();
                if (id == NULL)
                    return false;

                const char *name = src->name.get_utf8();
                if (name == NULL)
                    return false;

                // Emit string records
                const ptrdiff_t id_offset = os.position();
                if (!os.write(id, strlen(id) + 1))
                    return false;

                const ptrdiff_t name_offset = os.position();
                if (!os.write(name, strlen(name) + 1))
                    return false;

                // Create record
                ShmRecord *dst = shm_items.add();
                if (dst == NULL)
                    return false;

                dst->id             = reinterpret_cast<const char *>(id_offset);
                dst->name           = reinterpret_cast<const char *>(name_offset);
                dst->index          = src->index;
                dst->magic          = src->magic;
            }

            // Complete the data structures and patch pointers
            char *strings       = reinterpret_cast<char *>(os.release());
            size_t count        = shm_items.size();
            ShmRecord *items    = shm_items.release();

            for (size_t i=0; i<count; ++i)
            {
                ShmRecord *dst      = &items[i];
                dst->id             = strings + reinterpret_cast<ptrdiff_t>(dst->id);
                dst->name           = strings + reinterpret_cast<ptrdiff_t>(dst->name);
            }

            // Create shared memory state
            ShmState *state     = new ShmState(items, strings, count);
            if (state == NULL)
            {
                free(items);
                free(strings);
                return false;
            }

            // Submit shared state
            sState.push(state);
            return true;
        }

        bool ShmClient::state_updated()
        {
            return sState.pending();
        }

        const ShmState *ShmClient::state()
        {
            return sState.get();
        }

    } /* namespace core */
} /* namespace lsp */



