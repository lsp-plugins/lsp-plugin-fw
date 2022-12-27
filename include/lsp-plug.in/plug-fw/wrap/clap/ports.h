/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 26 дек. 2022 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace clap
    {
        /**
         * Audio port: input or output
         */
        class AudioPort: public plug::IPort
        {
            protected:
                float      *pBuffer;            // The original buffer passed by the host OR sanitized buffer
                size_t      nBufSize;           // The actual current buffer size
                size_t      nBufCap;            // The quantized capacity of the buffer

            public:
                explicit AudioPort(const meta::port_t *meta) : IPort(meta)
                {
                    pBuffer     = NULL;
                    nBufSize    = 0;
                    nBufCap     = 0;
                }

                virtual ~AudioPort() override
                {
                    if (pBuffer != NULL)
                    {
                        ::free(pBuffer);
                        pBuffer = NULL;
                    }
                };

            public:
                virtual void *buffer() override
                {
                    return pBuffer;
                }

            public:
                // Activate the port, issued by the plugin activate() method
                // Allocates enough space for data sanitize.
                bool activate(size_t min_frames_count, size_t max_frames_count)
                {
                    // For output ports, we can use the output buffer directly
                    if (meta::is_out_port(pMetadata))
                        return true;

                    // Check that capacity matches
                    size_t capacity = align_size(max_frames_count, 16);
                    if ((pBuffer != NULL) && (capacity == nBufCap))
                        return true;

                    // Re-allocate the buffer
                    if (pBuffer != NULL)
                        free(pBuffer);
                    pBuffer = static_cast<float *>(malloc(capacity * sizeof(float)));
                    if (pBuffer == NULL)
                        return false;

                    nBufCap    = capacity;
                    return true;
                }

                // Bind the audio port and perform sanitize for input ports
                void bind(float *ptr, size_t samples)
                {
                    if (meta::is_out_port(pMetadata))
                        pBuffer     = ptr;
                    else // if (meta::is_in_port(pMetadata))
                        dsp::sanitize2(pBuffer, ptr, samples);

                    nBufSize    = samples;
                }

                // Unbind the audio port and perform sanitize for output ports
                void unbind()
                {
                    // Sanitize plugin's output if possible
                    if (meta::is_out_port(pMetadata))
                    {
                        dsp::sanitize1(pBuffer, nBufSize);
                        pBuffer     = NULL;
                    }
                    nBufSize    = 0;
                }
        };

        class ParameterPort: public plug::IPort
        {
            protected:
                float   fValue;
                clap_id nID;

            public:
                explicit ParameterPort(const meta::port_t *meta) : IPort(meta)
                {
                    fValue  = meta->start;
                    nID     = clap_hash_string(meta->id);
                }

            public:
                inline clap_id uid() const  { return nID; }

            public:
                virtual float value() override { return fValue; }
        };

        class PortGroup: public ParameterPort
        {
            private:
                size_t                  nCols;
                size_t                  nRows;

            public:
                explicit PortGroup(const meta::port_t *meta) : ParameterPort(meta)
                {
                    nCols               = meta::port_list_size(meta->members);
                    nRows               = meta::list_size(meta->items);
                }

                virtual ~PortGroup()
                {
                    nCols               = 0;
                    nRows               = 0;
                }

            public:
                virtual void set_value(float value)
                {
                    int32_t v = value;
                    if ((v >= 0) && (v < ssize_t(nRows)))
                        fValue              = v;
                }

                virtual float value()
                {
                    return fValue;
                }

            public:
                inline size_t rows() const      { return nRows;     }
                inline size_t cols() const      { return nCols;     }
                inline size_t curr_row() const  { return fValue;    }
        };

    } /* namespace clap */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_PORTS_H_ */
