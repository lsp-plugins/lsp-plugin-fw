/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/types.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace gst
    {
        static constexpr size_t MAX_BLOCK_LENGTH       = 8192;

        class AudioPort: public plug::IPort
        {
            protected:
                float      *pBuffer;

            public:
                explicit AudioPort(const meta::port_t *meta) : plug::IPort(meta)
                {
                    pBuffer  = reinterpret_cast<float *>(::malloc(sizeof(float) * MAX_BLOCK_LENGTH));
                    if (pBuffer != NULL)
                        dsp::fill_zero(pBuffer, MAX_BLOCK_LENGTH);
                }

                virtual ~AudioPort() override
                {
                    if (pBuffer != NULL)
                    {
                        ::free(pBuffer);
                        pBuffer = NULL;
                    }
                };

                AudioPort(const AudioPort &) = delete;
                AudioPort(AudioPort &&) = delete;

                AudioPort & operator = (const AudioPort &) = delete;
                AudioPort & operator = (AudioPort &&) = delete;

            public:
                virtual void *buffer() override     { return pBuffer; };

            public:
                // Sanitize non-interleaved input samples
                inline void sanitize_input(const float *src, size_t samples)
                {
                    dsp::sanitize2(pBuffer, src, samples);
                }

                // Sanitize non-interleaved output samples
                inline void sanitize_output(float *dst, size_t samples)
                {
                    dsp::sanitize2(dst, pBuffer, samples);
                }

                // Deinterleave and sanitize samples from buffer
                inline void deinterleave(const float *src, size_t stride, size_t samples)
                {
                    for (size_t i=0; i<samples; ++i)
                    {
                        pBuffer[i]  = *src;
                        src        += stride;
                    }

                    // Sanitize plugin's input
                    dsp::sanitize1(pBuffer, samples);
                }

                // Should be always called at least once after bind() and after process() call
                inline void interleave(float *dst, size_t stride, size_t samples)
                {
                    // Sanitize plugin's output
                    dsp::sanitize1(pBuffer, samples); // Sanitize output of plugin

                    // Clear the buffer pointer
                    pBuffer    = NULL;
                }
        };

        class ParameterPort: public plug::IPort
        {
            protected:
                float   fValue;

            public:
                explicit ParameterPort(const meta::port_t *meta) : plug::IPort(meta)
                {
                    fValue      = meta->start;
                }

                virtual ~ParameterPort() override
                {
                    fValue      = 0.0f;
                }

                ParameterPort(const ParameterPort &) = delete;
                ParameterPort(ParameterPort &&) = delete;

                ParameterPort & operator = (const ParameterPort &) = delete;
                ParameterPort & operator = (ParameterPort &&) = delete;

            public:
                virtual float value() override  { return fValue; }

            public:
                inline bool submit_value(float value)
                {
                    if (value == fValue)
                        return false;

                    fValue      = value;
                    return true;
                }
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

                virtual ~PortGroup() override
                {
                    nCols               = 0;
                    nRows               = 0;
                }

                PortGroup(const PortGroup &) = delete;
                PortGroup(PortGroup &&) = delete;

                PortGroup & operator = (const PortGroup &) = delete;
                PortGroup & operator = (PortGroup &&) = delete;

            public:
                inline size_t rows() const      { return nRows;     }
                inline size_t cols() const      { return nCols;     }
                inline size_t curr_row() const  { return fValue;    }
        };

        class MeterPort: public plug::IPort
        {
            protected:
                float fValue;

            public:
                explicit MeterPort(const meta::port_t *meta) : plug::IPort(meta)
                {
                    fValue      = meta->start;
                }

                virtual ~MeterPort()
                {
                    fValue      = 0.0f;
                };

                MeterPort(const MeterPort &) = delete;
                MeterPort(MeterPort &&) = delete;

                MeterPort & operator = (const MeterPort &) = delete;
                MeterPort & operator = (MeterPort &&) = delete;

            public:
                virtual float value() override
                {
                    return      fValue;
                }

                virtual void set_value(float value) override
                {
                    value       = limit_value(pMetadata, value);
                    if (pMetadata->flags & meta::F_PEAK)
                    {
                        if (fabsf(fValue) < fabsf(value))
                            fValue = value;
                    }
                    else
                        fValue = value;
                };

            public:
                inline void reset()
                {
                    if (pMetadata->flags & meta::F_PEAK)
                        fValue      = 0.0f;
                }
        };

        class PathPort: public plug::IPort
        {
            protected:
                gst::Path           sPath;

            public:
                explicit PathPort(const meta::port_t *meta): plug::IPort(meta)
                {
                    sPath.init();
                }

                PathPort(const PathPort &) = delete;
                PathPort(PathPort &&) = delete;

                PathPort & operator = (const PathPort &) = delete;
                PathPort & operator = (PathPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return static_cast<plug::path_t *>(&sPath);
                }

            public:
                inline void submit(const char *string, size_t flags)
                {
                    sPath.submit(string, strlen(string), flags);
                }

                const char *get() const
                {
                    return sPath.sValue;
                }
        };


    } /* namespace gst */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_PORTS_H_ */
