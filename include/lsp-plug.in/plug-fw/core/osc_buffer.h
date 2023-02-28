/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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


#ifndef LSP_PLUG_IN_PLUG_FW_CORE_OSC_BUFFER_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_OSC_BUFFER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/protocol/osc.h>

namespace lsp
{
    namespace core
    {
        /**
         * Buffer to transfer OSC packets between two threads.
         * It is safe to use if one thread is reading data and one thread is
         * submitting data. Otherwise, additional synchronization mechanism
         * should be used
         */
        typedef struct osc_buffer_t
        {
            volatile size_t     nSize;
            size_t              nCapacity;
            size_t              nHead;
            size_t              nTail;
            uint8_t            *pBuffer;
            uint8_t            *pTempBuf;
            size_t              nTempSize;
            void               *pData;

            /**
             * Clear the buffer
             */
            void                clear();

            /**
             * Get buffer size
             * @return buffer size
             */
            inline size_t       size() const { return nSize; }

            /**
             * Initialize buffer
             * @param capacity the buffer capacity
             * @return status of operation
             */
            static osc_buffer_t *create(size_t capacity);

            /**
             * Destroy the buffer
             */
            static void destroy(osc_buffer_t *buf);

            /**
             * Reserve space for temporary buffer, by default 0x1000 bytes
             * @return status of operation
             */
            status_t    reserve(size_t size);

            /**
             * Submit OSC packet to the queue
             * @param data packet data
             * @param size size of the data
             * @return status of operation
             */
            status_t    submit(const void *data, size_t size);

            /**
             * Submit OSC packet to the queue
             * @param data packet data
             * @param size size of the data
             * @return status of operation
             */
            status_t    submit(const osc::packet_t *packet);

            status_t submit_int32(const char *address, int32_t value);
            status_t submit_float32(const char *address, float value);
            status_t submit_string(const char *address, const char *s);
            status_t submit_blob(const char *address, const void *data, size_t bytes);
            status_t submit_int64(const char *address, int64_t value);
            status_t submit_double64(const char *address, double value);
            status_t submit_time_tag(const char *address, uint64_t value);
            status_t submit_type(const char *address, const char *s);
            status_t submit_symbol(const char *address, const char *s);
            status_t submit_ascii(const char *address, char c);
            status_t submit_rgba(const char *address, const uint32_t rgba);
            status_t submit_midi(const char *address, const midi::event_t *event);
            status_t submit_midi_raw(const char *address, const void *event, size_t bytes);
            status_t submit_bool(const char *address, bool value);
            status_t submit_null(const char *address);
            status_t submit_inf(const char *address);

            /**
             * Try to send message
             * @param address message address
             * @param params message parameters
             * @param args list of arguments
             * @return status of operation
             */
            status_t    submit_message(const char *address, const char *params...);

            /**
             * Try to send message
             * @param ref forge reference
             * @param address message address
             * @param params message parameters
             * @param args list of arguments
             * @return status of operation
             */
            status_t    submit_messagev(const char *address, const char *params, va_list args);

            /**
             * Fetch OSC packet to the already allocated memory
             * @param data pointer to store the packet data
             * @param size pointer to store size of fetched data
             * @param limit
             * @return status of operation
             */
            status_t    fetch(void *data, size_t *size, size_t limit);

            /**
             * Fetch OSC packet to the already allocated memory
             * @param packet pointer to packet structure
             * @param limit maximum available size of data for the packet
             * @return status of operation
             */
            status_t    fetch(osc::packet_t *packet, size_t limit);

            /**
             * Skip current message in the buffer
             * @return number of bytes skipped
             */
            size_t      skip();
        } osc_buffer_t;

    } /* namespace core */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CORE_OSC_BUFFER_H_ */
