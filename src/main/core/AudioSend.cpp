/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 19 июн. 2024 г.
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

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/plug-fw/core/AudioSend.h>

namespace lsp
{
    namespace core
    {
        //---------------------------------------------------------------------
        AudioSend::Client::Client(AudioSend *send)
        {
            pSend           = send;
        }

        AudioSend::Client::~Client()
        {
            pSend           = NULL;
        }

        bool AudioSend::Client::update(dspu::Catalog * catalog)
        {
            return (pSend != NULL) ? pSend->update(catalog) : ICatalogClient::update(catalog);
        }

        bool AudioSend::Client::apply(dspu::Catalog * catalog)
        {
            return (pSend != NULL) ? pSend->apply(catalog) : ICatalogClient::apply(catalog);
        }

        void AudioSend::Client::keep_alive(dspu::Catalog *catalog)
        {
            ICatalogClient::keep_alive(catalog);
            if (pSend != NULL)
                pSend->keep_alive(catalog);
        }

        void AudioSend::Client::apply_settings()
        {
            ICatalogClient::request_apply();
        }

        //---------------------------------------------------------------------
        AudioSend::AudioSend():
            sClient(this),
            sStream(free_stream),
            sParams(free_params)
        {
            enStatus            = ST_INACTIVE;
            sRecord.index       = -1;
            sRecord.magic       = 0;
            sRecord.version     = 0;

            for (size_t i=0; i<4; ++i)
            {
                params_t *p         = &vState[i];
                p->nChannels        = 0;
                p->nLength          = 0;
                p->sName[0]         = '\0';
                p->bFree            = true;
            }

            pStream             = NULL;
            enStatus            = ST_INACTIVE;
            bProcessing         = false;
        }

        AudioSend::~AudioSend()
        {
            sClient.detach();
        }

        void AudioSend::free_params(params_t *ptr)
        {
            ptr->bFree          = true;
        }

        status_t AudioSend::attach(Catalog *catalog)
        {
            return sClient.attach(catalog);
        }

        status_t AudioSend::detach()
        {
            return sClient.detach();
        }

        bool AudioSend::attached() const
        {
            return sClient.attached();
        }

        AudioSend::stream_t *AudioSend::create_stream(Record *record, dspu::Catalog *catalog, const params_t * params)
        {
            // Allocate record
            stream_t *st = new stream_t;
            if (st == NULL)
                return NULL;
            lsp_finally {
                if (st != NULL)
                    delete st;
            };

            // Copy parameters for RT access
            st->pStream             = NULL;

            if ((params == NULL) || (strlen(params->sName) <= 0))
            {
                // Do not need to publish stream?
                record->index           = -1;
                record->magic           = 0;
                record->version         = 0;
                record->name.truncate();
                record->id.truncate();

                return release_ptr(st);
            }

            st->sParams.nChannels   = params->nChannels;
            st->sParams.nLength     = params->nLength;
            strcpy(st->sParams.sName, params->sName);
            st->sParams.bFree       = false;
            params                  = &st->sParams;

            // Allocate audio stream
            dspu::AudioStream *stream = new dspu::AudioStream();
            if (stream == NULL)
                return NULL;
            lsp_finally {
                if (stream != NULL)
                {
                    stream->close();
                    delete stream;
                }
            };

            LSPString stream_id;
            status_t res = stream->allocate(&stream_id, ".shm", params->nChannels, params->nLength);
            if (res != STATUS_OK)
                return NULL;

            // Register audio stream
            const char *id = stream_id.get_utf8();
            if (id == NULL)
                return NULL;

            ssize_t index = catalog->publish(record, CATALOG_ID_STREAM, params->sName, id);
            if (index < 0)
                return NULL;

            // Commit record and return
            lsp_trace("Published audio stream '%s' channels=%d, length=%d at index=%d, version=%d, shm_id=%s",
                params->sName, int(params->nChannels), int(params->nLength),
                int(record->index), int(record->version), record->id.get_utf8());

            st->pStream         = release_ptr(stream);
            return release_ptr(st);
        }

        void AudioSend::free_stream(stream_t *ptr)
        {
            if (ptr == NULL)
                return;

            if (ptr->pStream != NULL)
            {
                ptr->pStream->close();
                delete ptr->pStream;
                ptr->pStream = NULL;
            }

            delete ptr;
        }

        bool AudioSend::publish(const char *name, size_t channels, size_t length)
        {
            for (size_t i=0; i<4; ++i)
            {
                params_t *p         = &vState[i];
                if (p->bFree)
                {
                    if (name != NULL)
                    {
                        strncpy(p->sName, name, sizeof(p->sName));
                        p->sName[sizeof(p->sName) - 1] = '\0';
                    }
                    else
                        p->sName[0]         = '\0';

                    p->nChannels        = channels;
                    p->nLength          = length;
                    p->bFree            = false;

                    // Update state, transfer request to non-RT thread
                    atomic_store(&enStatus, ST_UPDATING);
                    sParams.push(p);
                    sClient.apply_settings();

                    return true;
                }
            }

            return false;
        }

        bool AudioSend::revoke()
        {
            return publish(NULL, 0, 0);
        }

        bool AudioSend::active() const
        {
            return atomic_load(&enStatus) == ST_ACTIVE;
        }

        bool AudioSend::overridden() const
        {
            return atomic_load(&enStatus) == ST_OVERRIDDEN;
        }

        const char *AudioSend::name() const
        {
            stream_t *st = sStream.current();
            if (st == NULL)
                return NULL;
            return (st->pStream != NULL) ? st->sParams.sName : NULL;
        }

        ssize_t AudioSend::channels() const
        {
            stream_t *st = sStream.current();
            if (st == NULL)
                return -1;
            return (st->pStream != NULL) ? st->sParams.nChannels : -1;
        }

        ssize_t AudioSend::length() const
        {
            stream_t *st = sStream.current();
            if (st == NULL)
                return -1;
            return (st->pStream != NULL) ? st->sParams.nLength : -1;
        }

        status_t AudioSend::begin(ssize_t block_size)
        {
            if (bProcessing)
                return STATUS_BAD_STATE;

            pStream         = sStream.get();
            bProcessing     = true;

            if ((pStream == NULL) || (pStream->pStream == NULL))
                return STATUS_OK;

            return pStream->pStream->begin(block_size);
        }

        status_t AudioSend::write(size_t channel, const float *src, size_t samples)
        {
            if (!bProcessing)
                return STATUS_BAD_STATE;
            if ((pStream == NULL) || (pStream->pStream == NULL))
                return STATUS_OK;

            return pStream->pStream->write(channel, src, samples);
        }

        status_t AudioSend::write_sanitized(size_t channel, const float *src, size_t samples)
        {
            if (!bProcessing)
                return STATUS_BAD_STATE;
            if ((pStream == NULL) || (pStream->pStream == NULL))
                return STATUS_OK;

            return pStream->pStream->write_sanitized(channel, src, samples);
        }

        status_t AudioSend::end()
        {
            if (!bProcessing)
                return STATUS_BAD_STATE;

            if (pStream == NULL)
                return STATUS_OK;

            status_t res = (pStream->pStream != NULL) ? pStream->pStream->end() : STATUS_OK;
            bProcessing = false;
            pStream     = NULL;

            return res;
        }

        bool AudioSend::apply(dspu::Catalog *catalog)
        {
            const params_t *params = sParams.get();
            if (params == NULL)
                return true;

            // Create new audio stream
            stream_t *st = create_stream(&sRecord, catalog, params);
            if (st == NULL)
                return false;

            // Transfer state
            if (st == NULL)
                lsp_trace("Disabled publishing of stream");

            atomic_store(&enStatus, (st->pStream != NULL) ? ST_ACTIVE : ST_INACTIVE);
            sStream.push(st);

            return true;
        }

        bool AudioSend::update(dspu::Catalog *catalog)
        {
            // We don't have active stream, nothing to validate
            if (sRecord.magic == 0)
                return true;

            // Ensure that record was not invalidated
            if (catalog->validate(&sRecord))
                return true;

            // Create empty stream record that will reset the transfer
            stream_t *st = create_stream(&sRecord, catalog, NULL);
            if (st == NULL)
                return false;

            // Transfer new state
            lsp_trace("Stream has been overridden by another publisher");
            atomic_store(&enStatus, ST_OVERRIDDEN);
            sStream.push(st);
            return true;
        }

        void AudioSend::keep_alive(dspu::Catalog *catalog)
        {
            if (!sRecord.id.is_empty())
                catalog->keep_alive(&sRecord.name);
        }

    } /* namespace core */
} /* namespace lsp */


