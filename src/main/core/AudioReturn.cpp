/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-plugin-template
 * Created on: 26 июн. 2024 г.
 *
 * lsp-plugins-plugin-template is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-plugin-template is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-plugin-template. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/plug-fw/core/AudioReturn.h>

namespace lsp
{
    namespace core
    {
        static constexpr uint32_t STALLED_THRESHOLD = 0x10000;

        //---------------------------------------------------------------------
        AudioReturn::Client::Client(AudioReturn *rtrn)
        {
            pReturn         = rtrn;
        }

        AudioReturn::Client::~Client()
        {
            pReturn         = NULL;
        }

        bool AudioReturn::Client::update(dspu::Catalog * catalog)
        {
            return (pReturn != NULL) ? pReturn->update(catalog) : ICatalogClient::update(catalog);
        }

        bool AudioReturn::Client::apply(dspu::Catalog * catalog)
        {
            return (pReturn != NULL) ? pReturn->apply(catalog) : ICatalogClient::apply(catalog);
        }

        void AudioReturn::Client::keep_alive(dspu::Catalog *catalog)
        {
            ICatalogClient::keep_alive(catalog);
            if (pReturn != NULL)
                pReturn->keep_alive(catalog);
        }

        void AudioReturn::Client::apply_settings()
        {
            ICatalogClient::request_apply();
        }

        //---------------------------------------------------------------------
        AudioReturn::AudioReturn():
            sClient(this),
            sStream(free_stream),
            sParams(free_params)
        {
            enStatus            = ST_INACTIVE;
            sRecord.index       = -1;
            sRecord.magic       = 0;
            sRecord.version     = 0;

            sSetup.sName[0]     = '\0';
            sSetup.bFree        = false;

            for (size_t i=0; i<4; ++i)
            {
                params_t *p         = &vState[i];
                p->sName[0]         = '\0';
                p->bFree            = true;
            }

            pStream             = NULL;
            enStatus            = ST_INACTIVE;
            bProcessing         = false;
        }

        AudioReturn::~AudioReturn()
        {
            sClient.detach();
        }

        void AudioReturn::free_params(params_t *ptr)
        {
            ptr->bFree          = true;
        }

        status_t AudioReturn::attach(Catalog *catalog)
        {
            return sClient.attach(catalog);
        }

        status_t AudioReturn::detach()
        {
            return sClient.detach();
        }

        bool AudioReturn::attached() const
        {
            return sClient.attached();
        }

        AudioReturn::stream_t *AudioReturn::create_stream(const Record *record, dspu::Catalog *catalog, const params_t * params)
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

            strcpy(st->sParams.sName, params->sName);
            st->nStreamCounter      = 0;
            st->nStallCounter       = STALLED_THRESHOLD;
            st->sParams.bFree       = false;
            params                  = &st->sParams;

            // Read the catalog entry, return stalled state on failure
            if (record->id.is_empty())
                return release_ptr(st);
            if (record->magic != CATALOG_ID_STREAM)
                return release_ptr(st);

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

            // Open audio stream
            status_t res = stream->open(&record->id);
            if (res != STATUS_OK)
                return release_ptr(st);

            // Commit record and return
            lsp_trace("Connected audio stream name=%s, as index=%d, version=%d, shm_id=%s",
                params->sName,
                int(record->index), int(record->version), record->id.get_utf8());

            st->pStream         = release_ptr(stream);
            st->nStreamCounter  = 0;
            st->nStallCounter   = 0;
            return release_ptr(st);
        }

        void AudioReturn::free_stream(stream_t *ptr)
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

        bool AudioReturn::connect(const char *name)
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

                    p->bFree            = false;

                    // Store connection parameters
                    strcpy(sSetup.sName, p->sName);
                    sSetup.bFree        = false;

                    // Update state, transfer request to non-RT thread
                    atomic_store(&enStatus, ST_UPDATING);
                    sParams.push(p);
                    sClient.apply_settings();

                    return true;
                }
            }

            return false;
        }

        bool AudioReturn::disconnect()
        {
            return connect(NULL);
        }

        bool AudioReturn::active() const
        {
            return atomic_load(&enStatus) == ST_ACTIVE;
        }

        bool AudioReturn::stalled() const
        {
            return atomic_load(&enStatus) == ST_STALLED;
        }

        const char *AudioReturn::name() const
        {
            stream_t *st = sStream.current();
            if (st == NULL)
                return NULL;
            return (st->pStream != NULL) ? st->sParams.sName : NULL;
        }

        ssize_t AudioReturn::channels() const
        {
            stream_t *st = sStream.current();
            if (st == NULL)
                return -1;
            return (st->pStream != NULL) ? st->pStream->channels() : -1;
        }

        ssize_t AudioReturn::length() const
        {
            stream_t *st = sStream.current();
            if (st == NULL)
                return -1;
            return (st->pStream != NULL) ? st->pStream->length() : -1;
        }

        status_t AudioReturn::begin(ssize_t block_size)
        {
            if (bProcessing)
                return STATUS_BAD_STATE;

            pStream         = sStream.get();
            bProcessing     = true;

            // Check for stalled state
            if ((pStream != NULL) && (pStream->pStream != NULL))
            {
                const uint32_t counter  = pStream->pStream->counter();
                if (counter != pStream->nStreamCounter)
                {
                    pStream->nStallCounter = 0;
                    atomic_store(&enStatus, ST_ACTIVE);
                }
                else
                {
                    pStream->nStallCounter = lsp_min(pStream->nStallCounter + lsp_min(block_size, 512), STALLED_THRESHOLD);
                    if (pStream->nStallCounter >= STALLED_THRESHOLD)
                        atomic_store(&enStatus, ST_STALLED);
                }

                return pStream->pStream->begin(block_size);
            }

            return STATUS_OK;;
        }

        status_t AudioReturn::read(size_t channel, float *dst, size_t samples)
        {
            if (!bProcessing)
                return STATUS_BAD_STATE;
            if ((pStream == NULL) || (pStream->pStream == NULL))
            {
                dsp::fill_zero(dst, samples);
                return STATUS_OK;
            }

            return pStream->pStream->read(channel, dst, samples);
        }

        status_t AudioReturn::read_sanitized(size_t channel, float *dst, size_t samples)
        {
            if (!bProcessing)
                return STATUS_BAD_STATE;
            if ((pStream == NULL) || (pStream->pStream == NULL))
            {
                dsp::fill_zero(dst, samples);
                return STATUS_OK;
            }

            return pStream->pStream->read_sanitized(channel, dst, samples);
        }

        status_t AudioReturn::end()
        {
            if (!bProcessing)
                return STATUS_BAD_STATE;

            bProcessing = false;

            if (pStream == NULL)
                return STATUS_OK;

            status_t res = (pStream->pStream != NULL) ? pStream->pStream->end() : STATUS_OK;
            pStream     = NULL;

            return res;
        }

        bool AudioReturn::apply(dspu::Catalog *catalog)
        {
            const params_t *params = sParams.get();
            if (params == NULL)
                return true;

            // Get record
            status_t res = catalog->get_or_reserve(&sRecord, params->sName, CATALOG_ID_STREAM);

            // Create audio stream
            stream_t *st;
            if (res != STATUS_OK)
            {
                st                  = new stream_t;
                if (st == NULL)
                    return false;

                st->pStream         = NULL;
                st->nStreamCounter  = 0;
                st->nStallCounter   = (strlen(params->sName)) > 0 ? STALLED_THRESHOLD : 0;
                strcpy(st->sParams.sName, params->sName);
            }
            else
                st = create_stream(&sRecord, catalog, params);

            // Transfer state
            const uatomic_t status =
                (st->pStream != NULL) ? ST_ACTIVE :
                (st->nStallCounter > 0) ? ST_STALLED : ST_INACTIVE;
            atomic_store(&enStatus, status);
            sStream.push(st);

            return true;
        }

        bool AudioReturn::update(dspu::Catalog *catalog)
        {
            // We don't have active stream, nothing to validate
            if ((sRecord.magic == 0) || (strlen(sSetup.sName) <= 0))
                return true;

            // Get record and ensure that it's stat didn't change
            Record tmp;
            status_t res = catalog->get(&tmp, sSetup.sName);
            if ((res == STATUS_OK) &&
                (tmp.magic == sRecord.magic) &&
                (tmp.version == sRecord.version) &&
                (tmp.index == sRecord.index) &&
                (tmp.id.equals(&sRecord.id)))
                return true;

            // Create audio stream
            stream_t *st;
            if (res != STATUS_OK)
            {
                st                  = new stream_t;
                if (st == NULL)
                    return false;

                st->pStream         = NULL;
                st->nStreamCounter  = 0;
                st->nStallCounter   = (strlen(sSetup.sName)) > 0 ? STALLED_THRESHOLD : 0;
            }
            else
            {
                st = create_stream(&tmp, catalog, &sSetup);
                if (st != NULL)
                {
                    sRecord.index       = tmp.index;
                    sRecord.magic       = tmp.magic;
                    sRecord.version     = tmp.version;
                    sRecord.name.swap(&tmp.name);
                    sRecord.id.swap(&tmp.id);
                }
            }

            // Transfer state
            const uatomic_t status =
                (st->pStream != NULL) ? ST_ACTIVE :
                (st->nStallCounter > 0) ? ST_STALLED : ST_INACTIVE;
            atomic_store(&enStatus, status);
            sStream.push(st);

            return true;
        }

        void AudioReturn::keep_alive(dspu::Catalog *catalog)
        {
            if (!sRecord.id.is_empty())
                catalog->keep_alive(&sRecord.name);
            else if (strlen(sSetup.sName) > 0)
                catalog->keep_alive(sSetup.sName);
        }

    } /* namespace core */
} /* namespace lsp */



