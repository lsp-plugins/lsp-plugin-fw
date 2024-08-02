/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/plug/data.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/const.h>
#include <lsp-plug.in/io/charset.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/stdlib/stdlib.h>


namespace lsp
{
    namespace plug
    {
        //-------------------------------------------------------------------------
        // midi_t methods
        bool midi_t::push_all(const midi_t *src)
        {
            size_t count    = lsp_min(src->nEvents, MIDI_EVENTS_MAX - nEvents);
            if (count > 0)
            {
                ::memcpy(&vEvents[nEvents], src->vEvents, count * sizeof(midi::event_t));
                nEvents        += count;
            }

            return count >= src->nEvents;
        }

        bool midi_t::push_slice(const midi_t *src, uint32_t start, uint32_t end)
        {
            // Find the start position to perform the copy of events, assuming events being sorted
            const midi::event_t *se;
            ssize_t first=0, last=src->nEvents - 1;
            while (first < last)
            {
                ssize_t middle = (first + last) >> 1;
                se = &src->vEvents[middle];
                if (se->timestamp >= start)
                    last    = middle - 1;
                else
                    first   = middle + 1;
            }

            // Copy events
            for (size_t i=first; i<src->nEvents; ++i)
            {
                // Check that event's timestamp is within the specified range
                se                  = &src->vEvents[i];
                if (se->timestamp < start)
                    continue;
                else if (se->timestamp >= end)
                    return true;

                // Check that we are able to add one more event
                if (nEvents >= MIDI_EVENTS_MAX)
                    return false;

                // Copy event and update timestamp
                midi::event_t *ev   = &vEvents[nEvents++];
                *ev                 = src->vEvents[i];
                ev->timestamp      -= start;
            }

            return true;
        }

        bool midi_t::push_all_shifted(const midi_t *src, uint32_t offset)
        {
            for (size_t i=0; i<src->nEvents; ++i)
            {
                // Check that we are able to add one more event
                if (nEvents >= MIDI_EVENTS_MAX)
                    return false;

                // Copy event and update timestamp
                midi::event_t *ev   = &vEvents[nEvents++];
                *ev                 = src->vEvents[i];
                ev->timestamp      += offset;
            }

            return true;
        }

        //-------------------------------------------------------------------------
        // path_t methods
        path_t::~path_t()
        {
        }

        void path_t::init()
        {
        }

        const char *path_t::path() const
        {
            return "";
        }

        size_t path_t::flags() const
        {
            return 0;
        }

        void path_t::accept()
        {
        }

        void path_t::commit()
        {
        }

        bool path_t::pending()
        {
            return false;
        }

        bool path_t::accepted()
        {
            return false;
        }

        //-------------------------------------------------------------------------
        // string_t methods
        uint32_t string_t::submit(const char *str, bool state)
        {
            // Acquire lock
            while (!atomic_trylock(nLock))
                ipc::Thread::yield();
            lsp_finally {
                atomic_unlock(nLock);
            };

            // Update string
            utf8_strncpy(sPending, nCapacity, str);
            const uint32_t serial = ((nRequest + 2) & (~uint32_t(1))) | (state ? 1 : 0);
            nRequest        = serial;
            return serial;
        }

        uint32_t string_t::submit(const void *buffer, size_t size, bool state)
        {
            // Acquire lock
            while (!atomic_trylock(nLock))
                ipc::Thread::yield();
            lsp_finally {
                atomic_unlock(nLock);
            };

            // Update string
            utf8_strncpy(sPending, nCapacity, buffer, size);
            const uint32_t serial = ((nRequest + 2) & (~uint32_t(1))) | (state ? 1 : 0);
            nRequest        = serial;
            return serial;
        }

        uint32_t string_t::submit(const LSPString *str, bool state)
        {
            // Obtain a valid UTF-8 string limite by nCapacity characters
            size_t len = lsp_min(str->length(), nCapacity);
            const char *src = str->get_utf8(0, len);
            if (src == NULL)
                return nRequest;

            // Acquire lock
            while (!atomic_trylock(nLock))
                ipc::Thread::yield();
            lsp_finally { atomic_unlock(nLock); };

            // Update string
            strcpy(sPending, src);

            const uint32_t serial = ((nRequest + 2) & (~uint32_t(1))) | (state ? 1 : 0);
            nRequest        = serial;
            return serial;
        }

        bool string_t::fetch(uint32_t *serial, char *dst, size_t size)
        {
            if (nSerial == *serial)
                return false;

            // Acquire lock
            while (!atomic_trylock(nLock))
                ipc::Thread::yield();
            lsp_finally {
                atomic_unlock(nLock);
            };

            // Copy data
            strncpy(dst, sData, size);
            sData[size-1] = '\0';
            *serial = nSerial;

            return true;
        }

        bool string_t::sync()
        {
            if (!atomic_trylock(nLock))
                return false;
            lsp_finally { atomic_unlock(nLock); };

            if (nSerial == nRequest)
                return false;

            // Copy contents and update state
            strcpy(sData, sPending);
            nSerial     = nRequest;

            return true;
        }

        bool string_t::is_state() const
        {
            return nSerial & 1;
        }

        size_t string_t::max_bytes() const
        {
            return nCapacity * 4;
        }

        uint32_t string_t::serial() const
        {
            return nSerial;
        }

        string_t *string_t::allocate(size_t max_length)
        {
            const size_t szof_type      = align_size(sizeof(string_t), DEFAULT_ALIGN);
            const size_t szof_data      = align_size(max_length * 4 + 1, DEFAULT_ALIGN); // Max 4 bytes per code point + end of line
            const size_t to_alloc       = szof_type + 2 * szof_data;

            uint8_t *ptr                = reinterpret_cast<uint8_t *>(malloc(to_alloc));
            if (ptr == NULL)
                return NULL;

            // Initialize object
            string_t *res               = advance_ptr_bytes<string_t>(ptr, szof_type);
            res->sData                  = advance_ptr_bytes<char>(ptr, szof_data);
            res->sPending               = advance_ptr_bytes<char>(ptr, szof_data);
            res->nCapacity              = max_length;
            atomic_init(res->nLock);
            res->nSerial                = 0;
            res->nRequest               = 0;

            // Cleanup string content
            bzero(res->sData, szof_data * 2);

            return res;
        }

        void string_t::destroy(string_t *str)
        {
            if (str != NULL)
                free(str);
        }

        //-------------------------------------------------------------------------
        // stream_t methods
        stream_t *stream_t::create(size_t channels, size_t frames, size_t capacity)
        {
            // Estimate the number of power-of-two frames
            size_t nframes  = frames * 8;
            size_t fcap         = 1;
            while (fcap < nframes)
                fcap                <<= 1;

            size_t bcap     = ((capacity*2 + STREAM_MAX_FRAME_SIZE - 1) / STREAM_MAX_FRAME_SIZE) * STREAM_MAX_FRAME_SIZE;
            size_t sz_of    = align_size(sizeof(stream_t), STREAM_MESH_ALIGN);
            size_t sz_chan  = align_size(sizeof(float *)*channels, STREAM_MESH_ALIGN);
            size_t sz_frm   = align_size(sizeof(frame_t)*fcap, STREAM_MESH_ALIGN);
            size_t sz_buf   = align_size(bcap * sizeof(float), STREAM_MESH_ALIGN);
            size_t to_alloc = sz_of + sz_frm + sz_chan + sz_buf * channels;

            uint8_t *pdata  = NULL;
            uint8_t *ptr    = alloc_aligned<uint8_t>(pdata, to_alloc, STREAM_MESH_ALIGN);
            if (ptr == NULL)
                return NULL;

            // Allocate and initialize space
            stream_t *mesh          = reinterpret_cast<stream_t *>(ptr);
            ptr                    += sz_of;

            mesh->nFrames           = frames;
            mesh->nChannels         = channels;
            mesh->nBufMax           = capacity;
            mesh->nBufCap           = bcap;
            mesh->nFrameCap         = fcap;

            mesh->nFrameId          = 0;

            mesh->vFrames           = reinterpret_cast<frame_t *>(ptr);
            ptr                    += sz_frm;

            for (size_t i=0; i<fcap; ++i)
            {
                frame_t *f              = &mesh->vFrames[i];
                f->id                   = 0;
                f->head                 = 0;
                f->tail                 = 0;
                f->size                 = 0;
                f->length               = 0;
            }

            mesh->vChannels         = reinterpret_cast<float **>(ptr);
            ptr                    += sz_chan;

            float *buf              = reinterpret_cast<float *>(ptr);
            dsp::fill_zero(buf, bcap * channels);
            for (size_t i=0; i<channels; ++i, buf += bcap)
                mesh->vChannels[i] = buf;

            mesh->pData             = pdata;

            return mesh;
        }

        void stream_t::clear(uint32_t current)
        {
            frame_t *f;
            for (size_t i=0; i<nFrameCap; ++i)
            {
                f                       = &vFrames[i];
                f->id                   = 0;
                f->head                 = 0;
                f->tail                 = 0;
                f->size                 = 0;
                f->length               = 0;
            }

            nFrameId                = current;
        }

        void stream_t::clear()
        {
            clear(nFrameId + 1);
        }

        void stream_t::destroy(stream_t *buf)
        {
            if (buf == NULL)
                return;
            uint8_t *data   = buf->pData;
            if (data == NULL)
                return;

            buf->vChannels      = NULL;
            buf->pData          = NULL;
            free_aligned(data);
        }

        ssize_t stream_t::get_tail(uint32_t frame) const
        {
            const frame_t *f = &vFrames[frame & (nFrameCap - 1)];
            size_t tail = f->tail;
            return (f->id == frame) ? tail : -STATUS_NOT_FOUND;
        }

        ssize_t stream_t::get_head(uint32_t frame) const
        {
            const frame_t *f = &vFrames[frame & (nFrameCap - 1)];
            size_t head = f->head;
            return (f->id == frame) ? head : -STATUS_NOT_FOUND;
        }

        ssize_t stream_t::get_frame_size(uint32_t frame) const
        {
            const frame_t *f = &vFrames[frame & (nFrameCap - 1)];
            ssize_t size = f->size;
            return (f->id == frame) ? size : -STATUS_NOT_FOUND;
        }

        ssize_t stream_t::get_position(uint32_t frame) const
        {
            const frame_t *f = &vFrames[frame & (nFrameCap - 1)];
            ssize_t pos = f->tail - f->length;
            if (pos < 0)
                pos        += nBufCap;
            return (f->id == frame) ? pos : -STATUS_NOT_FOUND;
        }

        ssize_t stream_t::get_length(uint32_t frame) const
        {
            const frame_t *f = &vFrames[frame & (nFrameCap - 1)];
            size_t size = f->length;
            return (f->id == frame) ? size : -STATUS_NOT_FOUND;
        }

        size_t stream_t::add_frame(size_t size)
        {
            size_t prev_id  = nFrameId;
            size_t frame_id = prev_id + 1;
            frame_t *curr   = &vFrames[prev_id & (nFrameCap - 1)];
            frame_t *next   = &vFrames[frame_id & (nFrameCap - 1)];

            size            = lsp_min(size, size_t(STREAM_MAX_FRAME_SIZE));

            // Write data for new frame
            next->id        = frame_id;
            next->head      = curr->tail;
            next->tail      = next->head + size;
            next->size      = size;
            next->length    = size;

            // Clear data for all buffers
            if (next->tail < nBufCap)
            {
                for (size_t i=0; i<nChannels; ++i)
                {
                    float *dst = vChannels[i];
                    dsp::fill_zero(&dst[next->head], size);
                }
            }
            else
            {
                next->tail     -= nBufCap;

                for (size_t i=0; i<nChannels; ++i)
                {
                    float *dst = vChannels[i];
                    dsp::fill_zero(&dst[next->head], nBufCap - next->head);
                    dsp::fill_zero(dst, next->tail);
                }
            }

            return size;
        }

        ssize_t stream_t::write_frame(size_t channel, const float *data, size_t off, size_t count)
        {
            if (channel >= nChannels)
                return -STATUS_INVALID_VALUE;

            size_t frame_id = nFrameId + 1;
            frame_t *next   = &vFrames[frame_id & (nFrameCap - 1)];
            if (next->id != frame_id)
                return -STATUS_BAD_STATE;

            // Estimate number of items to copy
            if (off >= next->size)
                return 0;
            count           = lsp_min(count, next->size - off);

            // Copy data to the frame
            float *dst      = vChannels[channel];
            size_t head     = next->head + off;
            if (head >= nBufCap)
                head           -= nBufCap;

            size_t tail     = head + count;
            if (tail > nBufCap)
            {
                dsp::copy(&dst[head], data, nBufCap - head);
                dsp::copy(dst, &data[nBufCap - head], tail - nBufCap);
            }
            else
                dsp::copy(&dst[head], data, count);

            return count;
        }

        float *stream_t::frame_data(size_t channel, size_t off, size_t *count)
        {
            if (channel >= nChannels)
                return NULL;

            size_t frame_id = nFrameId + 1;
            frame_t *next   = &vFrames[frame_id & (nFrameCap - 1)];
            if (next->id != frame_id)
                return NULL;

            // Estimate number of items to copy
            if (off >= next->size)
                return NULL;

            // Copy data to the frame
            float *dst      = vChannels[channel];
            size_t head     = next->head + off;
            if (head >= nBufCap)
                head           -= nBufCap;
            size_t tail     = next->head + next->size;
            if (tail >= nBufCap)
                head           -= nBufCap;

            // Store the available number of elements
            if (count != NULL)
                *count          = (head < tail) ? tail - head : nBufCap - head;

            return &dst[head];
        }

        ssize_t stream_t::read_frame(uint32_t frame_id, size_t channel, float *data, size_t off, size_t count)
        {
            if (channel >= nChannels)
                return -STATUS_INVALID_VALUE;
            frame_t *frame = &vFrames[frame_id & (nFrameCap - 1)];
            if (frame->id != frame_id)
                return -STATUS_BAD_STATE;

            // Estimate number of items to copy
            if (off >= frame->size)
                return -STATUS_EOF;
            count           = lsp_min(count, frame->size - off);

            // Copy data from the frame
            float *dst      = vChannels[channel];
            size_t head     = frame->head + off;
            if (head >= nBufCap)
                head           -= nBufCap;

            size_t tail     = head + count;
            if (tail > nBufCap)
            {
                dsp::copy(data, &dst[head], nBufCap - head);
                dsp::copy(&data[nBufCap - head], dst, tail - nBufCap);
            }
            else
                dsp::copy(data, &dst[head], count);

            return count;
        }

        ssize_t stream_t::read(size_t channel, float *data, size_t off, size_t count)
        {
            if (channel >= nChannels)
                return -STATUS_INVALID_VALUE;

            // Check that we're reading proper frame
            size_t frame_id     = nFrameId;
            frame_t *frm        = &vFrames[frame_id & (nFrameCap - 1)];
            if (frm->id != frame_id)
                return -STATUS_BAD_STATE;

            // Estimate the offset and number of items to read
            if (off >= frm->length)
                return -STATUS_EOF;
            count               = lsp_min(count, frm->length - off);

            // Determine position of head
            ssize_t head        = frm->tail - frm->length + off;
            if (head < 0)
                head               += nBufCap;

            size_t tail         = head + count;
            const float *s      = vChannels[channel];
            if (tail > nBufCap)
            {
                dsp::copy(data, &s[head], nBufCap - head);
                dsp::copy(&data[nBufCap - head], s, tail - nBufCap);
            }
            else
                dsp::copy(data, &s[head], count);

            return count;
        }

        bool stream_t::commit_frame()
        {
            size_t prev_id  = nFrameId;
            size_t frame_id = prev_id + 1;
            frame_t *curr   = &vFrames[prev_id & (nFrameCap - 1)];
            frame_t *next   = &vFrames[frame_id & (nFrameCap - 1)];
            if (next->id != frame_id)
                return false;

            // Commit new frame size and update frame identifier
            next->length    = lsp_min(curr->length + next->length, nBufMax);
            nFrameId        = frame_id;

            return true;
        }

        bool stream_t::sync(const stream_t *src)
        {
            // Check if there is data to sync
            if ((src == NULL) || (src->nChannels != nChannels))
                return false;

            // Estimate what to do
            uint32_t src_frm = src->nFrameId, dst_frm = nFrameId;
            uint32_t delta = src_frm - dst_frm;
            if (delta == 0)
                return false; // No changes

            if (delta > nFrames)
            {
                // Need to perform full sync
                frame_t *df         = &vFrames[src_frm & (nFrameCap - 1)];
                frame_t sf          = src->vFrames[src_frm & (src->nFrameCap - 1)];

                df->id              = src_frm;
                df->length          = lsp_min(sf.length, nBufMax);
                df->tail            = df->length;

                // Copy data from the source frame
                ssize_t head        = sf.tail - df->length;
                if (head < 0)
                {
                    head += src->nBufMax;
                    for (size_t i=0; i<nChannels; ++i)
                    {
                        const float *s  = src->vChannels[i];
                        float *d        = vChannels[i];

                        dsp::copy(d, &s[head], src->nBufMax - head);
                        dsp::copy(&d[src->nBufMax - head], s, sf.tail);
                    }
                }
                else
                {
                    for (size_t i=0; i<nChannels; ++i)
                    {
                        const float *s  = src->vChannels[i];
                        float *d        = vChannels[i];
                        dsp::copy(d, &s[head], df->length);
                    }
                }

                // Compute destination frame size and compute the head value of the frame
                ssize_t df_sz       = sf.tail - sf.head;
                if (df_sz < 0)
                    df_sz              += src->nBufMax;
                df_sz               = lsp_min(df_sz, ssize_t(df->length));
                df_sz               = lsp_min(df_sz, ssize_t(STREAM_MAX_FRAME_SIZE));
                df->head            = df->tail - df_sz;
            }
            else
            {
                uint32_t last_frm = src_frm + 1;

                // Need to perform incremental sync
                while (dst_frm != last_frm)
                {
                    // Determine the frames to sync
                    frame_t *pf         = &vFrames[(dst_frm - 1) & (nFrameCap - 1)];
                    frame_t *df         = &vFrames[dst_frm & (nFrameCap - 1)];
                    frame_t sf          = src->vFrames[dst_frm & (src->nFrameCap - 1)];

                    ssize_t fsize       = sf.tail - sf.head;
                    if (fsize < 0)
                        fsize              += src->nBufCap;

                    df->id          = dst_frm;
                    df->head        = pf->tail;
                    df->tail        = df->head;
                    df->length      = fsize;

                    // Copy frame data
                    for (ssize_t n=0; n<fsize; )
                    {
                        // Estimate the amount of samples to copy
                        size_t ns   = (sf.tail >= sf.head) ? sf.tail - sf.head : src->nBufCap - sf.head;
                        size_t nd   = nBufCap - df->tail;
                        size_t count= lsp_min(ns, nd);

                        // Synchronously copy samples for each channel
                        for (size_t i=0; i<nChannels; ++i)
                        {
                            const float *s  = src->vChannels[i];
                            float *d        = vChannels[i];
                            dsp::copy(&d[df->tail], &s[sf.head], count);
                        }

                        // Update positions
                        sf.head        += count;
                        df->tail       += count;
                        n              += count;

                        // Fixup positions
                        if (sf.head >= src->nBufCap)
                            sf.head        -= src->nBufCap;
                        if (df->tail >= nBufCap)
                            df->tail       -= nBufCap;
                    }

                    // Update frame size and increment frame number
                    df->length      = lsp_min(df->length + pf->length, nBufMax);
                    ++dst_frm;
                }
            }

            // Update current frame
            nFrameId    = src_frm;

            return true;
        }

        //-------------------------------------------------------------------------
        // frame_buffer_t methods
        void frame_buffer_t::clear()
        {
            dsp::fill_zero(vData, nCapacity * nCols);
            atomic_add(&nRowID, nRows);
        }

        void frame_buffer_t::seek(uint32_t row_id)
        {
            atomic_store(&nRowID, row_id);
        }

        void frame_buffer_t::read_row(float *dst, size_t row_id) const
        {
            uint32_t off    = row_id & (nCapacity - 1);
            dsp::copy(dst, &vData[off * nCols], nCols);
        }

        float *frame_buffer_t::get_row(size_t row_id) const
        {
            uint32_t off    = row_id & (nCapacity - 1);
            return &vData[off * nCols];
        }

        float *frame_buffer_t::next_row() const
        {
            uint32_t off    = atomic_load(&nRowID) & (nCapacity - 1);
            return &vData[off * nCols];
        }

        void frame_buffer_t::write_row(const float *row)
        {
            uint32_t off    = atomic_load(&nRowID) & (nCapacity - 1);
            dsp::copy(&vData[off * nCols], row, nCols);
            atomic_add(&nRowID, 1); // Increment row identifier after bulk write
        }

        void frame_buffer_t::write_row(uint32_t row_id, const float *row)
        {
            uint32_t off    = row_id & (nCapacity - 1);
            dsp::copy(&vData[off * nCols], row, nCols);
        }

        void frame_buffer_t::write_row()
        {
            atomic_add(&nRowID, 1); // Just increment row identifier
        }

        bool frame_buffer_t::sync(const frame_buffer_t *fb)
        {
            // Check if there is data for viewing
            if (fb == NULL)
                return false;

            // Estimate what to do
            uint32_t src_rid = fb->next_rowid(), dst_rid = atomic_load(&nRowID);
            uint32_t delta = src_rid - dst_rid;
            if (delta == 0)
                return false; // No changes
            else if (delta > nRows)
                dst_rid = src_rid - nRows;

            // Synchronize buffer data
            while (dst_rid != src_rid)
            {
                const float *row = fb->get_row(dst_rid);
                size_t off      = (dst_rid) & (nCapacity - 1);
                dsp::copy(&vData[off * nCols], row, nCols);
                dst_rid++;
            }

            atomic_store(&nRowID, dst_rid);
            return true;
        }

        frame_buffer_t  *frame_buffer_t::create(size_t rows, size_t cols)
        {
            // Estimate capacity
            size_t cap          = rows * 4;
            size_t hcap         = 1;
            while (hcap < cap)
                hcap                <<= 1;

            // Estimate amount of data to allocate
            size_t h_size       = align_size(sizeof(frame_buffer_t), 64);
            size_t b_size       = hcap * cols * sizeof(float);

            // Allocate memory
            uint8_t *ptr = NULL, *data = NULL;
            ptr     = alloc_aligned<uint8_t>(data, h_size + b_size);
            if (ptr == NULL)
                return NULL;

            // Create object
            frame_buffer_t *fb  = reinterpret_cast<frame_buffer_t *>(ptr);
            ptr                += h_size;

            fb->nRows           = rows;
            fb->nCols           = cols;
            fb->nCapacity       = hcap;
            atomic_store(&fb->nRowID, rows);
            fb->vData           = reinterpret_cast<float *>(ptr);
            fb->pData           = data;

            dsp::fill_zero(fb->vData, rows * cols);
            return fb;
        }

        status_t frame_buffer_t::init(size_t rows, size_t cols)
        {
            // Estimate capacity
            size_t cap          = rows * 4;
            size_t hcap         = 1;
            while (hcap < cap)
                hcap                <<= 1;

            // Estimate amount of data to allocate
            size_t b_size       = hcap * cols;

            // Allocate memory
            pData               = NULL;
            vData               = alloc_aligned<float>(pData, b_size);
            if (vData == NULL)
                return STATUS_NO_MEM;

            // Create object
            nRows               = rows;
            nCols               = cols;
            nCapacity           = hcap;
            atomic_store(&nRowID, rows);

            dsp::fill_zero(vData, rows * cols);
            return STATUS_OK;
        }

        void frame_buffer_t::destroy()
        {
            void *ptr           = pData;
            vData               = NULL;
            pData               = NULL;
            free_aligned(ptr);
        }

        void frame_buffer_t::destroy(frame_buffer_t *buf)
        {
            void *ptr           = buf->pData;
            buf->vData          = NULL;
            buf->pData          = NULL;
            free_aligned(ptr);
        }

        //-------------------------------------------------------------------------
        // position_t methods
        void position_t::init(position_t *pos)
        {
            pos->sampleRate             = LSP_DSP_UNITS_DEFAULT_SAMPLE_RATE;
            pos->speed                  = 1.0;
            pos->frame                  = 0;
            pos->numerator              = 4.0;
            pos->denominator            = 4.0;
            pos->beatsPerMinute         = BPM_DEFAULT;
            pos->beatsPerMinuteChange   = 0.0f;
            pos->tick                   = 0;
            pos->ticksPerBeat           = DEFAULT_TICKS_PER_BEAT;
        }

        //-------------------------------------------------------------------------
        // midi-related methods
        static int compare_midi_events(const void *p1, const void *p2)
        {
            const midi::event_t *e1 = reinterpret_cast<const midi::event_t *>(p1);
            const midi::event_t *e2 = reinterpret_cast<const midi::event_t *>(p2);
            return (e1->timestamp < e2->timestamp) ? -1 :
                    (e1->timestamp > e2->timestamp) ? 1 : 0;
        }

        void midi_t::sort()
        {
            if (nEvents > 1)
                ::qsort(vEvents, nEvents, sizeof(midi::event_t), compare_midi_events);
        }

        //-------------------------------------------------------------------------
        // misc functions
        void utf8_strncpy(char *dst, size_t dst_max, const char *src)
        {
            for (size_t i=0; i<dst_max; ++i)
            {
                const lsp_utf32_t ch = read_utf8_codepoint(&src);
                if (ch == '\0')
                    break;
                write_utf8_codepoint(&dst, ch);
            }

            *dst    = '\0';
        }

        void utf8_strncpy(char *dst, size_t dst_max, const void *buffer, size_t size)
        {
            const char *src = static_cast<const char *>(buffer);
            for (size_t i=0; i<dst_max; ++i)
            {
                const lsp_utf32_t ch = read_utf8_streaming(&src, &size, true);
                if (ch == LSP_UTF32_EOF)
                    break;
                write_utf8_codepoint(&dst, ch);
            }

            *dst    = '\0';
        }


    } /* namespace plug */
} /* namespace lsp */



