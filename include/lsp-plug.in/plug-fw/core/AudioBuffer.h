/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 авг. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_AUDIOBUFFER_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_AUDIOBUFFER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace core
    {
        /**
         * Audio buffer, simple data structure that contains some audio data.
         */
        class AudioBuffer
        {
            private:
                uint32_t        nBufSize;           // Size of sanitized buffer in samples
                uint32_t        nOffset;            // Offset from the beginning of the buffer
                bool            bActive;            // Activity flag
                bool            bClean;             // Port is clean
                float          *pBuffer;            // Buffer data

            private:
                void set_clean_state(bool clean);

            public:
                explicit AudioBuffer()
                {
                    nBufSize    = 0;
                    nOffset     = 0;
                    bActive     = false;
                    bClean      = false;
                    pBuffer     = NULL;
                }

                AudioBuffer(const AudioBuffer &) = delete;
                AudioBuffer(AudioBuffer &&) = delete;

                ~AudioBuffer()
                {
                    if (pBuffer != NULL)
                    {
                        nBufSize    = 0;
                        free(pBuffer);
                    }
                    bActive     = false;
                };

                AudioBuffer & operator = (const AudioBuffer &) = delete;
                AudioBuffer & operator = (AudioBuffer &&) = delete;

            public:
                /**
                 * Get pointer to the beginning of the buffer
                 * @return pointer to the beginning of the buffer
                 */
                inline float *data()
                {
                    return pBuffer;
                }

                /**
                 * Get pointer to the buffer data respective to the offset
                 * @return pointer to the buffer data
                 */
                inline float *buffer()
                {
                    return (pBuffer != NULL) ? &pBuffer[nOffset] : NULL;
                }

                /**
                 * Check that buffer is marked as clean and contains zeros
                 * @return true if buffer is marked as clean and contains zeros
                 */
                inline bool is_clean() const
                {
                    return bClean;
                }

                /**
                 * Mark buffer as being clean
                 */
                inline void set_clean()
                {
                    set_clean_state(true);
                }

                /**
                 * Mark buffer as being not clean
                 */
                inline void set_dirty()
                {
                    set_clean_state(false);
                }

                /**
                 * Check if buffer is active
                 * @return true if buffer is active
                 */
                inline bool active() const
                {
                    return bActive;
                }

                /**
                 * Set activity of the buffer
                 * @param active activity of the buffer
                 */
                inline void set_active(bool active)
                {
                    bActive     = active;
                }

                /**
                 * Get actual buffer size
                 * @return actual buffer size
                 */
                inline size_t size() const
                {
                    return nBufSize;
                }

                /**
                 * Set buffer size
                 * @param size buffer size
                 */
                void set_size(size_t size);

                /**
                 * Get current offset
                 * @return current offset
                 */
                inline size_t offset() const
                {
                    return nOffset;
                }

                /**
                 * Set current offset
                 * @param offset current offset
                 */
                inline void set_offset(size_t offset)
                {
                    nOffset = offset;
                }
        };
    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_AUDIOBUFFER_H_ */
