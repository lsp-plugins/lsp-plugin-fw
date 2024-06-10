/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp/dsp.h>

#define LADSPA_MAX_BLOCK_LENGTH             8192

namespace lsp
{
    namespace ladspa
    {
        // Specify port classes
        class Port: public plug::IPort
        {
            protected:
                float      *pData;

            public:
                explicit Port(const meta::port_t *meta) : IPort(meta), pData(NULL) {};
                virtual ~Port() override
                {
                    pData   = NULL;
                }

            public:
                void bind(void *data)
                {
                    pData   = reinterpret_cast<float *>(data);
                }
        };

        class AudioPort: public Port
        {
            protected:
                float      *pSanitized;
                float      *pBuffer;

            public:
                explicit AudioPort(const meta::port_t *meta) : Port(meta)
                {
                    pBuffer     = NULL;
                    pSanitized  = NULL;
                    if (meta::is_in_port(meta))
                    {
                        pSanitized = reinterpret_cast<float *>(::malloc(sizeof(float) * LADSPA_MAX_BLOCK_LENGTH));
                        if (pSanitized != NULL)
                            dsp::fill_zero(pSanitized, LADSPA_MAX_BLOCK_LENGTH);
                        else
                            lsp_warn("Failed to allocate sanitize buffer for port %s", pMetadata->id);
                    }
                }

                virtual ~AudioPort() override
                {
                    if (pSanitized != NULL)
                    {
                        ::free(pSanitized);
                        pSanitized = NULL;
                    }
                };

            public:
                virtual void *buffer() override
                {
                    return pBuffer;
                };

                // Should be always called at least once after bind() and before process() call
                void sanitize_before(size_t off, size_t samples)
                {
                    pBuffer     = &pData[off];

                    // Sanitize plugin's input if possible
                    if (pSanitized != NULL)
                    {
                        dsp::sanitize2(pSanitized, pBuffer, samples);
                        pBuffer     = pSanitized;
                    }
                }

                // Should be always called at least once after bind() and after process() call
                void sanitize_after(size_t off, size_t samples)
                {
                    // Sanitize plugin's output
                    if ((pBuffer != NULL) && (meta::is_out_port(pMetadata)))
                        dsp::sanitize1(pBuffer, samples); // Sanitize output of plugin

                    // Clear the buffer pointer
                    pBuffer    = NULL;
                }
        };

        class InputPort: public Port
        {
            private:
                float   fValue;

            public:
                explicit InputPort(const meta::port_t *meta) : Port(meta)
                {
                    fValue      = meta->start;
                }

                virtual ~InputPort() override
                {
                    fValue      = 0.0f;
                }

            public:
                virtual float value() override
                {
                    return fValue;
                }

            public:
                bool changed()
                {
                    if (pData == NULL)
                        return false;

                    const float value   = limit_value(pMetadata, *pData);
                    bool changed        = value != fValue;
                    fValue              = value;
                    return changed;
                }
        };

        class OutputPort: public Port
        {
            protected:
                float fValue;

            public:
                explicit OutputPort(const meta::port_t *meta) : Port(meta)
                {
                    fValue      = meta->start;
                }

                virtual ~OutputPort() override
                {
                    fValue      = 0.0f;
                };

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
                        if (fabs(fValue) < fabs(value))
                            fValue = value;
                    }
                    else
                        fValue = value;
                };

            public:
                void clear()
                {
                    if (pMetadata->flags & meta::F_PEAK)
                        fValue      = 0.0f;
                }

                void sync()
                {
                    if (pData != NULL)
                        *pData      = fValue;
                }
        };

    } /* namespace ladspa */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_PORTS_H_ */
