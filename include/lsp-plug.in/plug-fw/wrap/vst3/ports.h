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
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/data.h>
#include <lsp-plug.in/stdlib/math.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        enum sync_flags_t
        {
            SYNC_NONE,
            SYNC_CHANGED,
            SYNC_STATE
        };

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
                virtual sync_flags_t sync() { return SYNC_NONE; }
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

            public:
                /**
                 * Setup the port.
                 * Allocates enough space for data sanitize.
                 *
                 * @param max_frames_count maximum possible frames per one process() call
                 * @return false if we have troubles with memory
                 */
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

                /**
                 * Bind the audio port and perform sanitize for input ports
                 * @param ptr buffer to bind
                 * @param samples size of buffer
                 */
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

                /**
                 * Unbind the audio port and perform sanitize for output ports
                 */
                void unbind()
                {
                    // Sanitize plugin's output if possible
                    if (meta::is_out_port(pMetadata))
                        dsp::sanitize1(pBind, nBufSize);

                    pBind       = NULL;
                    nBufSize    = 0;
                    nOffset     = 0;
                }

                /**
                 * Advance read position by the specified amount of samples
                 * @param samples number of samples to advance
                 */
                inline void advance(size_t samples)
                {
                    nOffset    += samples;
                }
        };

        class MidiPort: public Port
        {
            protected:
                plug::midi_t    sQueue;             // MIDI event buffer
                plug::midi_t    sSlice;             // MIDI event buffer (slice)

            public:
                explicit MidiPort(const meta::port_t *meta): Port(meta)
                {
                    sQueue.clear();
                    sSlice.clear();
                }

            public:
                void prepare(size_t offset, size_t count)
                {
                    if (!meta::is_in_port(pMetadata))
                        return;

                    sSlice.clear();
                    sSlice.push_slice(&sQueue, offset, offset + count);
                }

                void commit(size_t offset)
                {
                    if (!meta::is_out_port(pMetadata))
                        return;

                    sQueue.push_all_shifted(&sSlice, offset);
                    sSlice.clear();
                }

                inline plug::midi_t *queue()      { return &sQueue; }

            public:
                using IPort::buffer;

                virtual void *buffer() override
                {
                    return &sSlice;                     // Return time-sliced data instead of the whole buffer
                }
        };

        class ParameterPort: public Port
        {
            protected:
                float                   fValue;         // The actual value of the port
                float                   fPending;       // The pending value
                uint32_t                nChangeIndex;   // The current index of a change in a queue
                Steinberg::Vst::ParamID nID;            // Unique identifier of the port (parameter tag)
                bool                    bVirtual;       // Indicates that port is virtual

            public:
                explicit ParameterPort(const meta::port_t *meta, Steinberg::Vst::ParamID id, bool virt) : Port(meta)
                {
                    fValue              = meta->start;
                    fPending            = fValue;
                    nChangeIndex        = 0;
                    nID                 = id;
                    bVirtual            = virt;
                }

                ParameterPort(const ParameterPort &) = delete;
                ParameterPort(ParameterPort &&) = delete;

                ParameterPort & operator = (const ParameterPort &) = delete;
                ParameterPort & operator = (ParameterPort &&) = delete;

            public:
                inline Steinberg::Vst::ParamID parameter_id() const { return nID; }
                inline uint32_t change_index() const    { return nChangeIndex;          }
                inline void     set_change_index(uint32_t index)    { nChangeIndex = index; }
                inline bool     is_virtual() const      { return bVirtual;              }

                bool commit_value(float value)
                {
                    bool changed    = fValue != value;
                    fValue          = value;
                    fPending        = value;
                    return changed;
                }

                void submit(double value)
                {
                    fPending        = value;
                }

            public:
                virtual sync_flags_t sync() override
                {
                    float pending = fPending;
                    if (fValue == pending)
                        return SYNC_NONE;
                    fValue          = pending;
                    return (bVirtual) ? SYNC_CHANGED : SYNC_STATE;
                }

                virtual float value() override { return fValue; }
                virtual void *buffer() override { return NULL; }
        };

        class MeterPort: public Port
        {
            protected:
                float                   fValue;         // The actual value of the port
                float                   fDisplay;       // Display value
                bool                    bEmpty;         // Parameter does not contaiin any data since last process() call

            public:
                explicit MeterPort(const meta::port_t *meta) : Port(meta)
                {
                    fValue              = meta->start;
                    fDisplay            = meta->start;
                    bEmpty              = true;
                }

                MeterPort(const MeterPort &) = delete;
                MeterPort(MeterPort &&) = delete;

                MeterPort & operator = (const MeterPort &) = delete;
                MeterPort & operator = (MeterPort &&) = delete;

            public:
                inline void     clear()             { bEmpty    = true;             }
                inline float    display() const     { return fDisplay;              }

            public:
                virtual float   value() override
                {
                    return fValue;
                }

                virtual void    set_value(float value) override
                {
                    value       = meta::limit_value(pMetadata, value);

                    if (pMetadata->flags & meta::F_PEAK)
                    {
                        if (bEmpty)
                        {
                            fValue      = value;
                            bEmpty      = false;
                        }
                        else if (fabsf(fValue) < fabsf(value))
                            fValue  = value;
                    }
                    else
                        fValue = value;

                    fDisplay    = fValue;
                }
        };

        class PortGroup: public ParameterPort
        {
            private:
                size_t                  nCols;
                size_t                  nRows;

            public:
                explicit PortGroup(const meta::port_t *meta, Steinberg::Vst::ParamID id, bool virt) : ParameterPort(meta, id, virt)
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
                virtual float value() override
                {
                    return fValue;
                }

            public:
                inline size_t rows() const      { return nRows;     }
                inline size_t cols() const      { return nCols;     }
                inline size_t curr_row() const  { return fValue;    }
        };

        class MeshPort: public Port
        {
            private:
                plug::mesh_t       *pMesh;

            public:
                explicit MeshPort(const meta::port_t *meta) :
                    Port(meta)
                {
                    pMesh   = vst3::create_mesh(meta);
                }

                virtual ~MeshPort() override
                {
                    vst3::destroy_mesh(pMesh);
                    pMesh = NULL;
                }

                MeshPort(const MeshPort &) = delete;
                MeshPort(MeshPort &&) = delete;

                MeshPort & operator = (const MeshPort &) = delete;
                MeshPort & operator = (MeshPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pMesh;
                }
        };

        class StreamPort: public Port
        {
            private:
                plug::stream_t     *pStream;
                float              *pData;
                uint32_t            nFrameID;

            public:
                explicit StreamPort(const meta::port_t *meta):
                    Port(meta)
                {
                    pStream     = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                    pData       = reinterpret_cast<float *>(::malloc(sizeof(float) * STREAM_MAX_FRAME_SIZE));
                    nFrameID    = 0;
                }

                virtual ~StreamPort() override
                {
                    plug::stream_t::destroy(pStream);
                    pStream     = NULL;

                    if (pData != NULL)
                    {
                        free(pData);
                        pData       = NULL;
                    }
                }

                StreamPort(const StreamPort &) = delete;
                StreamPort(StreamPort &&) = delete;

                StreamPort & operator = (const StreamPort &) = delete;
                StreamPort & operator = (StreamPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pStream;
                }

            public:
                inline uint32_t frame_id() const                { return nFrameID;      }
                inline void set_frame_id(uint32_t frame_id)     { nFrameID = frame_id;  }

                inline float *read_frame(uint32_t frame_id, size_t channel, size_t off, size_t count)
                {
                    pStream->read_frame(frame_id, channel, pData, off, count);
                    return pData;
                }
        };

        class FrameBufferPort: public Port
        {
            private:
                plug::frame_buffer_t    sFB;
                uint32_t                nRowID;

            public:
                explicit FrameBufferPort(const meta::port_t *meta):
                    Port(meta)
                {
                    sFB.init(pMetadata->start, pMetadata->step);
                    nRowID              = 0;
                }

                virtual ~FrameBufferPort() override
                {
                    sFB.destroy();
                }

                FrameBufferPort(const FrameBufferPort &) = delete;
                FrameBufferPort(FrameBufferPort &&) = delete;

                FrameBufferPort & operator = (const FrameBufferPort &) = delete;
                FrameBufferPort & operator = (FrameBufferPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return &sFB;
                }

            public:
                inline uint32_t row_id() const          { return nRowID;    }
                void set_row_id(uint32_t row_id)        { nRowID = row_id;  }
        };

        class PathPort: public Port
        {
            private:
                vst3::path_t    sPath;

            public:
                explicit PathPort(const meta::port_t *meta):
                    Port(meta)
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

                virtual sync_flags_t sync() override
                {
                    return (sPath.sync()) ? SYNC_STATE : SYNC_NONE;
                }
        };

        class StringPort: public Port
        {
            private:
                plug::string_t     *pValue;

            public:
                explicit StringPort(const meta::port_t *meta):
                    Port(meta)
                {
                    pValue          = plug::string_t::allocate(size_t(meta->max));
                }

                StringPort(const StringPort &) = delete;
                StringPort(StringPort &&) = delete;

                ~StringPort() override
                {
                    if (pValue != NULL)
                    {
                        plug::string_t::destroy(pValue);
                        pValue          = NULL;
                    }
                }

                StringPort & operator = (const StringPort &) = delete;
                StringPort & operator = (StringPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pValue->sData;
                }

                virtual sync_flags_t sync() override
                {
                    if (pValue == NULL)
                        return SYNC_NONE;
                    if (!pValue->sync())
                        return SYNC_NONE;

                    return (pValue->is_state()) ? SYNC_CHANGED : SYNC_STATE;
                }

            public:
                plug::string_t *data()
                {
                    return pValue;
                }
        };

        class OscPort: public Port
        {
            private:
                core::osc_buffer_t     *pFB;

            public:
                explicit OscPort(const meta::port_t *meta):
                    Port(meta)
                {
                    pFB     = core::osc_buffer_t::create(OSC_BUFFER_MAX);
                }

                virtual ~OscPort() override
                {
                    if (pFB != NULL)
                    {
                        core::osc_buffer_t::destroy(pFB);
                        pFB     = NULL;
                    }
                }

                OscPort(const OscPort &) = delete;
                OscPort(OscPort &&) = delete;

                PathPort & operator = (const PathPort &) = delete;
                PathPort & operator = (PathPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pFB;
                }
        };

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_PORTS_H_ */
