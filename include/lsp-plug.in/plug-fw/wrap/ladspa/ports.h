/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                virtual ~Port()
                {
                    pData   = NULL;
                }

            public:
                virtual void bind(void *data)
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

                virtual ~AudioPort()
                {
                    if (pSanitized != NULL)
                    {
                        ::free(pSanitized);
                        pSanitized = NULL;
                    }
                };

            public:
                virtual void *buffer()      { return pBuffer; };

                // Should be always called at least once after bind() and before processing
                void sanitize(size_t off, size_t samples)
                {
                    pBuffer     = &pData[off];
                    if (pSanitized == NULL)
                        return;

                    if (samples <= LADSPA_MAX_BLOCK_LENGTH)
                    {
                        dsp::sanitize2(pSanitized, reinterpret_cast<float *>(pBuffer), samples);
                        pBuffer     = pSanitized;
                    }
                }
        };

        class InputPort: public Port
        {
            private:
                float   fPrev;
                float   fValue;

            public:
                explicit InputPort(const meta::port_t *meta) : Port(meta)
                {
                    fPrev       = meta->start;
                    fValue      = meta->start;
                }

                virtual ~InputPort()
                {
                    fPrev       = 0.0f;
                    fValue      = 0.0f;
                }

            public:
                virtual float value()   { return fValue; }

                virtual bool pre_process(size_t samples)
                {
                    if (pData == NULL)
                        return false;

                    fValue      = limit_value(pMetadata, *pData);
                    return fPrev != fValue;
                }

                virtual void post_process(size_t samples) { fPrev = fValue; };
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

                virtual ~OutputPort()
                {
                    fValue      = 0.0f;
                };

            public:
                virtual float value()
                {
                    return      fValue;
                }

                virtual void setValue(float value)
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

                virtual void bind(void *data)   { pData = reinterpret_cast<float *>(data); };

                virtual bool pre_process(size_t samples)
                {
                    if (pMetadata->flags & meta::F_PEAK)
                        fValue      = 0.0f;
                    return false;
                }

                virtual void post_process(size_t samples)
                {
                    if (pData != NULL)
                        *pData      = fValue;
                    if (pMetadata->flags & meta::F_PEAK)
                        fValue      = 0.0f;
                }
        };

    } /* namespace ladspa */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_PORTS_H_ */
