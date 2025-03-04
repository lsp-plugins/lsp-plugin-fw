/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 мар. 2023 г.
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

#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/stdlib/stdlib.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace core
    {
        constexpr size_t DEFAULT_TEMP_BUFFER_SIZE   = 0x1000;

        //-------------------------------------------------------------------------
        // osc_buffer_t methods
        osc_buffer_t *osc_buffer_t::create(size_t capacity)
        {
            if (capacity % sizeof(uint32_t))
                return NULL;

            uint8_t *tmp        = reinterpret_cast<uint8_t *>(malloc(DEFAULT_TEMP_BUFFER_SIZE));
            if (tmp == NULL)
                return NULL;

            size_t to_alloc     = sizeof(osc_buffer_t) + capacity + DEFAULT_ALIGN;
            void *data          = NULL;
            uint8_t *ptr        = alloc_aligned<uint8_t>(data, to_alloc, DEFAULT_ALIGN);
            if (ptr == NULL)
            {
                free(tmp);
                return NULL;
            }

            osc_buffer_t *res   = reinterpret_cast<osc_buffer_t *>(ptr);
            ptr                += align_size(sizeof(osc_buffer_t), DEFAULT_ALIGN);

            atomic_store(&res->nSize, size_t(0));
            res->nCapacity      = capacity;
            res->nHead          = 0;
            res->nTail          = 0;
            res->pBuffer        = ptr;
            res->pTempBuf       = tmp;
            res->nTempSize      = DEFAULT_TEMP_BUFFER_SIZE;
            res->pData          = data;

            return res;
        }

        void osc_buffer_t::destroy(osc_buffer_t *buf)
        {
            if (buf == NULL)
                return;

            if (buf->pTempBuf != NULL)
            {
                free(buf->pTempBuf);
                buf->pTempBuf   = NULL;
            }
            if (buf->pData != NULL)
                free_aligned(buf->pData);
        }

        status_t osc_buffer_t::submit(const void *data, size_t size)
        {
            if ((!size) || (size % sizeof(uint32_t)))
                return STATUS_BAD_ARGUMENTS;

            // Ensure that there is enough space in buffer
            size_t oldsize  = atomic_load(&nSize);
            size_t newsize  = oldsize + size + sizeof(uint32_t);
            if (newsize > nCapacity)
                return (oldsize == 0) ? STATUS_TOO_BIG : STATUS_OVERFLOW;

            // Store packet size to the buffer and move the tail
            *(reinterpret_cast<uint32_t *>(&pBuffer[nTail])) = CPU_TO_BE(uint32_t(size));
            nTail          += sizeof(uint32_t);
            if (nTail > nCapacity)
                nTail          -= nCapacity;

            // Store packet data and move the tail
            size_t head     = nCapacity - nTail;
            if (size > head)
            {
                const uint8_t *src  = reinterpret_cast<const uint8_t *>(data);
                ::memcpy(&pBuffer[nTail], src, head);
                ::memcpy(pBuffer, &src[head], size - head);
            }
            else
                ::memcpy(&pBuffer[nTail], data, size);

            nTail          += size;
            if (nTail > nCapacity)
                nTail          -= nCapacity;

            // Update the size
            atomic_store(&nSize, newsize);
            return STATUS_OK;
        }

        status_t osc_buffer_t::reserve(size_t size)
        {
            if (nTempSize >= size)
                return STATUS_OK;
            else if (size > nCapacity)
                return STATUS_OVERFLOW;

            uint8_t *tmp    = reinterpret_cast<uint8_t *>(realloc(pTempBuf, size));
            if (tmp == NULL)
                return STATUS_NO_MEM;

            pTempBuf        = tmp;
            nTempSize       = size;

            return STATUS_OK;
        }

        status_t osc_buffer_t::submit(const osc::packet_t *packet)
        {
            return (packet != NULL) ? submit(packet->data, packet->size) : STATUS_BAD_ARGUMENTS;
        }

        void osc_buffer_t::clear()
        {
            atomic_store(&nSize, size_t(0));
            nHead   = 0;
            nTail   = 0;
        }

    #define SUBMIT_SIMPLE_IMPL(address, func, ...) \
            osc::packet_t packet; \
            osc::forge_t forge; \
            osc::forge_frame_t sframe, message; \
            \
            status_t res = osc::forge_begin_fixed(&sframe, &forge, pTempBuf, nTempSize); \
            status_t res2; \
            if (res == STATUS_OK) {\
                res     = osc::forge_begin_message(&message, &sframe, address); \
                if (res == STATUS_OK) \
                    res = osc::func(&message, ## __VA_ARGS__); \
                osc::forge_end(&message); \
            } \
            res2 = osc::forge_end(&sframe); \
            if (res == STATUS_OK) res = res2; \
            res2   = osc::forge_close(&packet, &forge); \
            if (res == STATUS_OK) res = res2; \
            res2   = osc::forge_destroy(&forge); \
            if (res == STATUS_OK) res = res2; \
            return (res == STATUS_OK) ? submit(&packet) : res;

        status_t osc_buffer_t::submit_int32(const char *address, int32_t value)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_int32, value);
        }

        status_t osc_buffer_t::submit_float32(const char *address, float value)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_float32, value);
        }

        status_t osc_buffer_t::submit_string(const char *address, const char *s)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_string, s);
        }

        status_t osc_buffer_t::submit_blob(const char *address, const void *data, size_t bytes)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_blob, data, bytes);
        }

        status_t osc_buffer_t::submit_int64(const char *address, int64_t value)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_int64, value);
        }

        status_t osc_buffer_t::submit_double64(const char *address, double value)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_double64, value);
        }

        status_t osc_buffer_t::submit_time_tag(const char *address, uint64_t value)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_time_tag, value);
        }

        status_t osc_buffer_t::submit_type(const char *address, const char *s)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_type, s);
        }

        status_t osc_buffer_t::submit_symbol(const char *address, const char *s)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_symbol, s);
        }

        status_t osc_buffer_t::submit_ascii(const char *address, char c)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_ascii, c);
        }

        status_t osc_buffer_t::submit_rgba(const char *address, const uint32_t rgba)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_rgba, rgba);
        }

        status_t osc_buffer_t::submit_midi(const char *address, const midi::event_t *event)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_midi, event);
        }

        status_t osc_buffer_t::submit_midi_raw(const char *address, const void *event, size_t bytes)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_midi_raw, event, bytes);
        }

        status_t osc_buffer_t::submit_bool(const char *address, bool value)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_bool, value);
        }

        status_t osc_buffer_t::submit_null(const char *address)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_null);
        }

        status_t osc_buffer_t::submit_inf(const char *address)
        {
            SUBMIT_SIMPLE_IMPL(address, forge_inf);
        }

        #undef SUBMIT_SIMPLE_IMPL

        status_t osc_buffer_t::submit_message(const char *address, const char *params...)
        {
            va_list args;
            va_start(args, params);
            status_t res = submit_messagev(address, params, args);
            va_end(args);
            return res;
        }

        status_t osc_buffer_t::submit_messagev(const char *address, const char *params, va_list args)
        {
            osc::packet_t packet;
            osc::forge_t forge;
            osc::forge_frame_t sframe;

            status_t res = osc::forge_begin_fixed(&sframe, &forge, pTempBuf, nTempSize);
            if (res == STATUS_OK)
                res     = osc::forge_message(&sframe, address, params, args);

            res     = update_status(res, osc::forge_end(&sframe));
            if (res == STATUS_OK)
                res         = osc::forge_close(&packet, &forge);

            res     = update_status(res, osc::forge_destroy(&forge));
            return (res == STATUS_OK) ? submit(&packet) : res;
        }

        status_t osc_buffer_t::fetch(void *data, size_t *size, size_t limit)
        {
            if ((data == NULL) || (size == NULL) || (!limit))
                return STATUS_BAD_ARGUMENTS;

            // There is enough space in the buffer?
            size_t bufsz    = atomic_load(&nSize);
            if (bufsz < sizeof(uint32_t))
                return STATUS_NO_DATA;

            // Read size, analyze state of the record and update head
            size_t psize    = BE_TO_CPU(*(reinterpret_cast<uint32_t *>(&pBuffer[nHead])));
            if (psize > limit) // We have enough space to store the data?
                return STATUS_OVERFLOW;
            if ((psize + sizeof(uint32_t)) > bufsz) // Record is valid?
                return STATUS_CORRUPTED;
            *size           = psize;
            nHead          += sizeof(uint32_t);
            if (nHead > nCapacity)
                nHead          -= nCapacity;

            // Copy the buffer contents
            size_t head     = nCapacity - nHead;
            if (head < psize)
            {
                uint8_t *dst    = reinterpret_cast<uint8_t *>(data);
                ::memcpy(dst, &pBuffer[nHead], head);
                ::memcpy(&dst[head], pBuffer, psize - head);
            }
            else
                ::memcpy(data, &pBuffer[nHead], psize);

            nHead          += psize;
            if (nHead > nCapacity)
                nHead          -= nCapacity;

            // Decrement size
            atomic_add(&nSize, -(psize + sizeof(uint32_t)));

            return STATUS_OK;
        }

        status_t osc_buffer_t::fetch(osc::packet_t *packet, size_t limit)
        {
            return (packet != NULL) ? fetch(packet->data, &packet->size, limit) : STATUS_BAD_ARGUMENTS;
        }

        size_t osc_buffer_t::skip()
        {
            size_t bufsz    = atomic_load(&nSize);
            if (bufsz < sizeof(uint32_t))
                return 0;

            size_t ihead    = nHead;
            uint32_t *head  = reinterpret_cast<uint32_t *>(&pBuffer[ihead]);
            size_t psize    = BE_TO_CPU(*head);

            if ((psize + sizeof(uint32_t)) > bufsz) // Record is valid?
                return 0;

            // Decrement the size and update the head
            nHead           = (ihead + psize + sizeof(uint32_t)) % nCapacity;
            atomic_add(&nSize, -(psize + sizeof(uint32_t)));

            return psize;
        }

    } /* namespace core */
} /* namespace lsp */



