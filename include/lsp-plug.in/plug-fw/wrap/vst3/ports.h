/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 дек. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        // Specify port classes
        class Port: public plug::IPort
        {
            public:
                explicit Port(const meta::port_t *meta): plug::IPort(meta)
                {
                }

                Port(const Port &) = delete;
                Port(Port &&) = delete;
                Port & operator = (const Port &) = delete;
                Port & operator = (Port &&) = delete;

            public:
                /**
                 * Ensure that port is serializable
                 * @return true if port is serializable
                 */
                virtual bool serializable() const { return false; }

                /** Serialize the state of the port to the chunk
                 *
                 * @param os output stream to perform serialization
                 */
                virtual status_t serialize(const Steinberg::IBStream *os) { return STATUS_OK; }

                /** Serialize the state of the port to the chunk
                 *
                 * @param is input stream to perform deserialization
                 */
                virtual status_t deserialize(const Steinberg::IBStream *is) { return STATUS_OK; }
        };

        /**
         * Audio port: input or output
         */
        class AudioPort: public Port
        {
            protected:
                float                      *pBind;      // Bound buffer
                float                      *pBuffer;    // The original buffer passed by the host OR sanitized buffer
                uint32_t                    nOffset;    // The relative offset from the beginning of the buffer
                uint32_t                    nBufSize;   // The actual current buffer size
                uint32_t                    nBufCap;    // The quantized capacity of the buffer
                Steinberg::Vst::Speaker     nSpeaker;   // Associated speaker
                bool                        bActive;    // Activity flag
                bool                        bZero;      // Indicator that data in the buffer is zeroed out

            public:
                explicit AudioPort(const meta::port_t *meta) : Port(meta)
                {
                    pBind       = NULL;
                    pBuffer     = NULL;
                    nOffset     = 0;
                    nBufSize    = 0;
                    nBufCap     = 0;
                    nSpeaker    = Steinberg::Vst::kSpeakerM;
                    bActive     = true;
                    bZero       = false;
                }
                AudioPort(const AudioPort &) = delete;
                AudioPort(AudioPort &&) = delete;

                virtual ~AudioPort() override
                {
                    if (pBuffer != NULL)
                    {
                        ::free(pBuffer);
                        pBuffer = NULL;
                    }
                };

                AudioPort & operator = (const AudioPort &) = delete;
                AudioPort & operator = (AudioPort &&) = delete;

            public:
                inline Steinberg::Vst::Speaker      speaker() const         { return nSpeaker;  }
                inline void                         set_speaker(Steinberg::Vst::Speaker id)     { nSpeaker = id; }
                inline bool                         active() const          { return bActive;   }
                inline void                         set_active(bool active) { bActive = active; }

            public:
                virtual void *buffer() override
                {
                    return &pBind[nOffset];
                }

                virtual void post_process(size_t samples) override
                {
                    nOffset    += samples;
                }

            public:
                // Setup the port
                // Allocates enough space for data sanitize.
                bool setup(size_t max_frames_count)
                {
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
                    pBind       = pBuffer;

//                    lsp_trace("id=%s, pBind=%p, pBuffer=%p, max_frames_count=%d",
//                        pMetadata->id, pBind, pBuffer, int(max_frames_count));

                    nBufCap     = capacity;
                    return true;
                }

                // Bind the audio port and perform sanitize for input ports
                void bind(float *ptr, size_t samples)
                {
//                    lsp_trace("id=%s, pBind=%p, pBuffer=%p, ptr=%p, samples=%d",
//                        pMetadata->id, pBind, pBuffer, ptr, samples);

                    if (meta::is_out_port(pMetadata))
                        pBind       = (ptr != NULL) ? ptr : pBuffer;
                    else // if (meta::is_in_port(pMetadata))
                    {
                        if ((ptr != NULL) && (bActive))
                            dsp::sanitize2(pBuffer, ptr, samples);
                        else if (pBind != NULL)
                            dsp::fill_zero(pBuffer, nBufCap);
                        pBind       = pBuffer;
                    }

                    nBufSize    = samples;
                    nOffset     = 0;
                }

                // Unbind the audio port and perform sanitize for output ports
                void unbind()
                {
                    // Sanitize plugin's output if possible
                    if (meta::is_out_port(pMetadata))
                        dsp::sanitize1(pBind, nBufSize);

                    pBind       = NULL;
                    nBufSize    = 0;
                    nOffset     = 0;
                }
        };

        class ParameterPort: public Port
        {
            protected:
                float                   fValue; // The actual value of the port
                Steinberg::Vst::ParamID nID;    // Unique identifier of the port

            public:
                explicit ParameterPort(const meta::port_t *meta) : Port(meta)
                {
                    fValue              = meta->start;
                    nID                 = vst3::gen_parameter_id(meta->id);
                }

                ParameterPort(const ParameterPort &) = delete;
                ParameterPort(ParameterPort &&) = delete;

                ParameterPort & operator = (const ParameterPort &) = delete;
                ParameterPort & operator = (ParameterPort &&) = delete;

            public:
                inline Steinberg::Vst::ParamID parameter_id() const { return nID; }

            public:
                virtual float value() override { return fValue; }
                virtual void *buffer() override { return NULL; }

                /** Pre-process port state before processor execution
                 * @param samples number of estimated samples to process
                 * @return true if port value has been externally modified
                 */
                virtual bool pre_process(size_t samples);

                /** Post-process port state after processor execution
                 * @param samples number of samples processed by plugin
                 */
                virtual void post_process(size_t samples);
        };

        class InParamPort: public ParameterPort
        {
            protected:
                uint32_t                nChangeIndex;   // The current index of a change in a queue

            public:
                explicit InParamPort(const meta::port_t *meta) : ParameterPort(meta)
                {
                    nChangeIndex    = 0;
                }

                InParamPort(const InParamPort &) = delete;
                InParamPort(InParamPort &&) = delete;

                InParamPort & operator = (const InParamPort &) = delete;
                InParamPort & operator = (InParamPort &&) = delete;

            public:
                inline uint32_t change_index() const   { return nChangeIndex; }
                inline void     set_change_index(uint32_t index)    { nChangeIndex = index; }

            public:
                virtual bool pre_process(size_t samples)
                {
                    nChangeIndex    = 0;
                    return false;
                }

            public:
                bool commit_value(float value)
                {
                    bool changed    = fValue != value;
                    fValue          = value;
                    return changed;
                }
        };

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_PORTS_H_ */
