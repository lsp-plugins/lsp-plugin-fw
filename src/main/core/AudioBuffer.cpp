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

#include <lsp-plug.in/plug-fw/core/AudioBuffer.h>

namespace lsp
{
    namespace core
    {
        void AudioBuffer::set_clean_state(bool clean)
        {
            if (clean)
            {
                // Apply cleanup if needed
                if ((bClean) || (pBuffer == NULL))
                    return;

                dsp::fill_zero(pBuffer, nBufSize);
                bClean      = true;
            }
            else
                bClean      = clean;
        }

        void AudioBuffer::set_size(size_t size)
        {
            // Buffer size has changed?
            if (nBufSize == size)
                return;

            float *buf  = reinterpret_cast<float *>(::realloc(pBuffer, sizeof(float) * size));
            if (buf == NULL)
            {
                if (pBuffer != NULL)
                {
                    ::free(pBuffer);
                    pBuffer = NULL;
                }
                return;
            }

            nBufSize    = size;
            pBuffer     = buf;
            dsp::fill_zero(pBuffer, nBufSize);
            bClean      = true;
        }
    } /* namespace core */
} /* namespace lsp */

