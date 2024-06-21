/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 7 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/chunk.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/defs.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/types.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/stdlib.h>

namespace lsp
{
    namespace vst2
    {
        // Specify port classes
        class Port: public plug::IPort
        {
            protected:
                AEffect                *pEffect;
                audioMasterCallback     hCallback;
                ssize_t                 nID;

            protected:
                float to_vst(float value)
                {
                    float min = 0.0f, max = 1.0f, step = 0.0f;
                    get_port_parameters(pMetadata, &min, &max, &step);

    //                lsp_trace("input = %.3f", value);
                    // Set value as integer or normalized
                    if ((meta::is_gain_unit(pMetadata->unit)) || (meta::is_log_rule(pMetadata)))
                    {
//                        float p_value   = value;

                        float thresh    = (pMetadata->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                        float l_step    = log(step + 1.0f) * 0.1f;
                        float l_thresh  = log(thresh);

                        float l_min     = (fabs(min)   < thresh) ? (l_thresh - l_step) : (log(min));
                        float l_max     = (fabs(max)   < thresh) ? (l_thresh - l_step) : (log(max));
                        float l_value   = (fabs(value) < thresh) ? (l_thresh - l_step) : (log(value));

                        value           = (l_value - l_min) / (l_max - l_min);

//                        lsp_trace("%s = %f (%f, %f, %f) -> %f (%f)",
//                            pMetadata->id,
//                            p_value,
//                            min, max, step,
//                            value,
//                            l_thresh);
                    }
                    else if (pMetadata->unit == meta::U_BOOL)
                    {
                        value = (value >= (min + max) * 0.5f) ? 1.0f : 0.0f;
                    }
                    else
                    {
                        if ((pMetadata->flags & meta::F_INT) ||
                            (pMetadata->unit == meta::U_ENUM) ||
                            (pMetadata->unit == meta::U_SAMPLES))
                            value  = truncf(value);

                        // Normalize value
                        value = (max != min) ? (value - min) / (max - min) : 0.0f;
                    }

    //                lsp_trace("result = %.3f", value);
                    return value;
                }

                float from_vst(float value)
                {
    //                lsp_trace("input = %.3f", value);
                    // Set value as integer or normalized
                    float min = 0.0f, max = 1.0f, step = 0.0f;
                    get_port_parameters(pMetadata, &min, &max, &step);

                    if ((meta::is_gain_unit(pMetadata->unit)) || (meta::is_log_rule(pMetadata)))
                    {
//                        float p_value   = value;
                        float thresh    = (pMetadata->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                        float l_step    = log(step + 1.0f) * 0.1f;
                        float l_thresh  = log(thresh);
                        float l_min     = (fabs(min)   < thresh) ? (l_thresh - l_step) : (log(min));
                        float l_max     = (fabs(max)   < thresh) ? (l_thresh - l_step) : (log(max));

                        value           = value * (l_max - l_min) + l_min;
                        value           = (value < l_thresh) ? 0.0f : expf(value);

//                        lsp_trace("%s = %f (%f, %f, %f) -> %f (%f, %f, %f)",
//                            pMetadata->id,
//                            p_value,
//                            l_thresh, l_min, l_max,
//                            value,
//                            min, max, step);
                    }
                    else if (pMetadata->unit == meta::U_BOOL)
                    {
                        value = (value >= 0.5f) ? max : min;
                    }
                    else
                    {
                        value = min + value * (max - min);
                        if ((pMetadata->flags & meta::F_INT) ||
                            (pMetadata->unit == meta::U_ENUM) ||
                            (pMetadata->unit == meta::U_SAMPLES))
                            value  = truncf(value);
                    }

    //                lsp_trace("result = %.3f", value);
                    return value;
                }

            public:
                explicit Port(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback): plug::IPort(meta)
                {
                    pEffect         = effect;
                    hCallback       = callback;
                    nID             = -1;
                }

                Port(const Port &) = delete;
                Port(Port &&) = delete;

                virtual ~Port() override
                {
                    pEffect         = NULL;
                    hCallback       = NULL;
                    nID             = -1;
                }

                Port & operator = (const Port &) = delete;
                Port & operator = (Port &&) = delete;

            public:
                /**
                 * Commit scalar value to the port
                 * @param value scalar value to commit
                 */
                virtual void                    write_value(float value)    {}

                /**
                 * Ensure that port is serializable
                 * @return true if port is serializable
                 */
                virtual bool serializable() const { return false; }

                /** Serialize the state of the port to the chunk
                 *
                 * @param chunk chunk to perform serialization
                 */
                virtual void serialize(vst2::chunk_t *chunk) {}

                /** Deserialize the state of the port from the chunk (legacy version)
                 *
                 * @param data data buffer
                 * @param length length of the buffer in bytes
                 * @return number of bytes deserialized or error
                 */
                virtual ssize_t deserialize_v1(const void *data, size_t length)
                {
                    return -1;
                }

                /**
                 * Deserialize the state of the port from the chunk data, data pointer should
                 * be updated
                 * @param data chunk data
                 * @param limit the data size
                 * @return true on success
                 */
                virtual bool deserialize_v2(const uint8_t *data, size_t size)
                {
                    return true;
                }

                /**
                 * Return true if port has been changed
                 * @return true if port has been changed
                 */
                virtual bool changed()
                {
                    return false;
                }

            public:
                inline AEffect                 *effect()            { return pEffect;               };
                inline audioMasterCallback      callback()          { return hCallback;             };
                inline ssize_t                  id() const          { return nID;                   };
                inline void                     set_id(ssize_t id)  { nID = id;                     };

                inline VstIntPtr                masterCallback(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
                {
                    return hCallback(pEffect, opcode, index, value, ptr, opt);
                }
        };

        class AudioPort: public Port
        {
            private:
                float          *pBuffer;
                float          *pSanitized;
                size_t          nBufSize;

            public:
                explicit AudioPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
                {
                    pBuffer     = NULL;
                    pSanitized  = NULL;
                    nBufSize    = 0;
                }

                AudioPort(const AudioPort &) = delete;
                AudioPort(AudioPort &&) = delete;

                virtual ~AudioPort() override
                {
                    pBuffer     = NULL;

                    if (pSanitized != NULL)
                    {
                        ::free(pSanitized);
                        pSanitized  = NULL;
                        nBufSize    = 0;
                    }
                }

                AudioPort & operator = (const AudioPort &) = delete;
                AudioPort & operator = (AudioPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pBuffer;
                }

            public:
                void bind(float *data)
                {
                    pBuffer     = data;
                }

                void sanitize_before(size_t samples)
                {
                    if (pSanitized == NULL)
                        return;

                    // Perform sanitize() if possible
                    if (samples > nBufSize)
                    {
                        lsp_warn("Could not sanitize buffer data for port %s, not enough buffer size (required: %d, actual: %d)",
                                pMetadata->id, int(samples), int(nBufSize));
                        return;
                    }

                    // Santize input data and update buffer pointer
                    dsp::sanitize2(pSanitized, pBuffer, samples);
                    pBuffer     = pSanitized;
                };

                void sanitize_after(size_t samples)
                {
                    // Sanitize output data
                    if ((pBuffer != NULL) && (meta::is_out_port(pMetadata)))
                        dsp::sanitize1(pBuffer, samples);
                };

                void set_block_size(size_t size)
                {
                    if (!meta::is_in_port(pMetadata))
                        return;
                    if (nBufSize == size)
                        return;

                    float *buf  = reinterpret_cast<float *>(::realloc(pSanitized, sizeof(float) * size));
                    if (buf == NULL)
                    {
                        ::free(pSanitized);
                        pSanitized = NULL;
                        return;
                    }

                    nBufSize    = size;
                    pSanitized  = buf;
                    dsp::fill_zero(pSanitized, nBufSize);
                }
        };

        class ParameterPort: public Port
        {
            protected:
                float       fValue;         // The internal value
                float       fVstPrev;       // Previous value in VST standard notation
                float       fVstValue;      // Current value in VST standard notation
                uatomic_t   nSID;           // Serial ID of the parameter

            public:
                explicit ParameterPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
                {
                    fValue      = meta->start;
                    fVstPrev    = to_vst(meta->start);
                    fVstValue   = fVstPrev;
                    nSID        = 0;
                }

                ParameterPort(const ParameterPort &) = delete;
                ParameterPort(ParameterPort &&) = delete;

                virtual ~ParameterPort() override
                {
                    fValue      = pMetadata->start;
                    fVstPrev    = 0.0f;
                    fVstValue   = 0.0f;
                    nSID        = 0;
                }

                ParameterPort & operator = (const ParameterPort &) = delete;
                ParameterPort & operator = (ParameterPort &&) = delete;

            public:
                virtual float value() override
                {
                    return fValue;
                }

                virtual void set_value(float value) override
                {
                    fValue      = meta::limit_value(pMetadata, value);
                    fVstValue   = to_vst(fValue);
                }

                virtual void write_value(float value) override
                {
                    lsp_trace("id=%s, value=%f", pMetadata->id, value);

                    set_value(value);
                    if ((nID >= 0) && (pEffect != NULL) && (hCallback != NULL))
                    {
                        lsp_trace("hCallback=%p, pEffect=%p, operation=%d, id=%d, value=%.5f, vst_value=%.5f",
                                hCallback, pEffect, int(audioMasterAutomate), int(nID), value, fVstValue);
                        hCallback(pEffect, audioMasterAutomate, nID, 0, NULL, fVstValue);
                    }
                }

                virtual bool serializable() const override
                {
                    return true;
                }

                virtual void serialize(vst2::chunk_t *chunk) override
                {
                    float v = CPU_TO_BE(fValue);
                    chunk->write(&v, sizeof(v));
                }

                virtual ssize_t deserialize_v1(const void *data, size_t length) override
                {
                    if (length < sizeof(float))
                        return -1;
                    float value     = BE_TO_CPU(*(reinterpret_cast<const float *>(data)));
                    write_value(value);
                    atomic_add(&nSID, 1);
                    return sizeof(float);
                }

                virtual bool deserialize_v2(const uint8_t *data, size_t size) override
                {
                    if (size < sizeof(float))
                        return false;

                    float v         = BE_TO_CPU(*(reinterpret_cast<const float *>(data)));
                    write_value(v);
                    atomic_add(&nSID, 1);
                    return true;
                }

                virtual bool changed() override
                {
                    if (fVstValue == fVstPrev)
                        return false;

                    fVstPrev = fVstValue;
                    if ((nID < 0) && (pEffect != NULL) && (hCallback != NULL))
                        hCallback(pEffect, audioMasterUpdateDisplay, 0, 0, 0, 0);

                    return true;
                }

            public:
                void set_vst_value(float value)
                {
                    if (fVstValue == value)
                        return;
                    fValue          = meta::limit_value(pMetadata, from_vst(value));
                    fVstValue       = value;
                    atomic_add(&nSID, 1);
                }

                inline float vst_value()
                {
                    return fVstValue;
                }

                inline uatomic_t sid()
                {
                    return nSID;
                }
        };

        class MeterPort: public Port
        {
            public:
                float   fValue;
                bool    bForce;

            public:
                explicit MeterPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
                {
                    fValue      = meta->start;
                    bForce      = true;
                }

                MeterPort(const MeterPort &) = delete;
                MeterPort(MeterPort &&) = delete;

                virtual ~MeterPort() override
                {
                    fValue      = pMetadata->start;
                }

                MeterPort & operator = (const MeterPort &) = delete;
                MeterPort & operator = (MeterPort &&) = delete;

            public: // Native Interface
                virtual float value() override
                {
                    return fValue;
                }

                virtual void set_value(float value) override
                {
                    value       = meta::limit_value(pMetadata, value);

                    if (pMetadata->flags & meta::F_PEAK)
                    {
                        if ((bForce) || (fabs(fValue) < fabs(value)))
                        {
                            fValue  = value;
                            bForce  = false;
                        }
                    }
                    else
                        fValue = value;
                }

            public:
                float sync_value()
                {
                    float value = fValue;
                    bForce      = true;
                    return value;
                }
        };

        class PortGroup: public ParameterPort
        {
            private:
                size_t                  nCols;
                size_t                  nRows;

            public:
                explicit PortGroup(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    ParameterPort(meta, effect, callback)
                {
                    nCols               = port_list_size(meta->members);
                    nRows               = list_size(meta->items);
                }

                PortGroup(const PortGroup &) = delete;
                PortGroup(PortGroup &&) = delete;

                virtual ~PortGroup() override
                {
                    nCols               = 0;
                    nRows               = 0;
                }

                PortGroup & operator = (const PortGroup &) = delete;
                PortGroup & operator = (PortGroup &&) = delete;

            public:
                virtual void set_value(float value) override
                {
                    int32_t v   = value;
                    if ((v >= 0) && (v < ssize_t(nRows)))
                        fValue              = v;
                }

                virtual void serialize(vst2::chunk_t *chunk) override
                {
                    chunk->write(CPU_TO_BE(int32_t(fValue)));
                }

                virtual ssize_t deserialize_v1(const void *data, size_t length) override
                {
                    if (length < sizeof(int32_t))
                        return -1;
                    int32_t value   = BE_TO_CPU(*(reinterpret_cast<const int32_t *>(data)));
                    if ((value >= 0) && (value < ssize_t(nRows)))
                    {
                        fValue          = value;
                        atomic_add(&nSID, 1);
                    }
                    return sizeof(int32_t);
                }

                virtual bool deserialize_v2(const uint8_t *data, size_t size) override
                {
                    if (size < sizeof(int32_t))
                        return false;

                    int32_t v;
                    IF_UNALIGNED_MEMORY_SAFE(
                        v = BE_TO_CPU(*(reinterpret_cast<const int32_t *>(data)));
                    )
                    IF_UNALIGNED_MEMORY_UNSAFE(
                        memcpy(&v, data, sizeof(v));
                        v               = BE_TO_CPU(v);
                    )
                    if ((v >= 0) && (v < ssize_t(nRows)))
                    {
                        fValue          = v;
                        atomic_add(&nSID, 1);
                    }

                    return true;
                }

            public:
                inline size_t rows() const      { return nRows; }
                inline size_t cols() const      { return nCols; }
                inline size_t curr_row() const  { return fValue; }
        };

        class MeshPort: public Port
        {
            private:
                plug::mesh_t       *pMesh;

            public:
                explicit MeshPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback) :
                    Port(meta, effect, callback)
                {
                    pMesh   = vst2::create_mesh(meta);
                }

                MeshPort(const MeshPort &) = delete;
                MeshPort(MeshPort &&) = delete;

                virtual ~MeshPort() override
                {
                    vst2::destroy_mesh(pMesh);
                    pMesh = NULL;
                }

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

            public:
                explicit StreamPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
                {
                    pStream     = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                }

                StreamPort(const StreamPort &) = delete;
                StreamPort(StreamPort &&) = delete;

                virtual ~StreamPort() override
                {
                    if (pStream != NULL)
                    {
                        plug::stream_t::destroy(pStream);
                        pStream     = NULL;
                    }
                }

                StreamPort & operator = (const StreamPort &) = delete;
                StreamPort & operator = (StreamPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pStream;
                }
        };

        class FrameBufferPort: public Port
        {
            private:
                plug::frame_buffer_t    sFB;

            public:
                explicit FrameBufferPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
                {
                    sFB.init(pMetadata->start, pMetadata->step);
                }

                FrameBufferPort(const FrameBufferPort &) = delete;
                FrameBufferPort(FrameBufferPort &&) = delete;

                virtual ~FrameBufferPort() override
                {
                    sFB.destroy();
                }

                FrameBufferPort & operator = (const FrameBufferPort &) = delete;
                FrameBufferPort & operator = (FrameBufferPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return &sFB;
                }
        };

        class MidiInputPort: public Port
        {
            private:
                plug::midi_t    sQueue;         // MIDI event buffer

            public:
                explicit MidiInputPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
                {
                    sQueue.clear();
                }

                MidiInputPort(const MidiInputPort &) = delete;
                MidiInputPort(MidiInputPort &&) = delete;
                MidiInputPort & operator = (const MidiInputPort &) = delete;
                MidiInputPort & operator = (MidiInputPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return &sQueue;
                }

            public:
                void deserialize(const VstEvents *e)
                {
                    sQueue.clear();

                    size_t count    = e->numEvents;
                    for (size_t i=0; i<count; ++i)
                    {
                        // Get event and check type
                        const VstEvent *ve      = e->events[i];
                        if (ve->type != kVstMidiType)
                            continue;

                        // Cast to VST MIDI event
                        const VstMidiEvent *vme     = reinterpret_cast<const VstMidiEvent *>(ve);
                        const uint8_t *bytes        = reinterpret_cast<const uint8_t *>(vme->midiData);

                        // Decode MIDI event
                        midi::event_t me;
                        if (midi::decode(&me, bytes) <= 0)
                            return;

                        // Put the event to the queue
                        me.timestamp      = vme->deltaFrames;

                        // Debug
                        #ifdef LSP_TRACE
                            #define TRACE_KEY(x)    case midi::MIDI_MSG_ ## x: evt_type = #x; break;
                            lsp_trace("midi dump: %02x %02x %02x", int(bytes[0]) & 0xff, int(bytes[1]) & 0xff, int(bytes[2]) & 0xff);

                            char tmp_evt_type[32];
                            const char *evt_type = NULL;
                            switch (me.type)
                            {
                                TRACE_KEY(NOTE_OFF)
                                TRACE_KEY(NOTE_ON)
                                TRACE_KEY(NOTE_PRESSURE)
                                TRACE_KEY(NOTE_CONTROLLER)
                                TRACE_KEY(PROGRAM_CHANGE)
                                TRACE_KEY(CHANNEL_PRESSURE)
                                TRACE_KEY(PITCH_BEND)
                                TRACE_KEY(SYSTEM_EXCLUSIVE)
                                TRACE_KEY(MTC_QUARTER)
                                TRACE_KEY(SONG_POS)
                                TRACE_KEY(SONG_SELECT)
                                TRACE_KEY(TUNE_REQUEST)
                                TRACE_KEY(END_EXCLUSIVE)
                                TRACE_KEY(CLOCK)
                                TRACE_KEY(START)
                                TRACE_KEY(CONTINUE)
                                TRACE_KEY(STOP)
                                TRACE_KEY(ACTIVE_SENSING)
                                TRACE_KEY(RESET)
                                default:
                                    snprintf(tmp_evt_type, sizeof(tmp_evt_type), "UNKNOWN(0x%02x)", int(me.type));
                                    evt_type = tmp_evt_type;
                                    break;
                            }

                            lsp_trace("MIDI Event: type=%s, timestamp=%ld", evt_type, (long)(me.timestamp));

                            #undef TRACE_KEY

                        #endif /* LSP_TRACE */

                        // Add event to the queue
                        if (!sQueue.push(me))
                            lsp_error("MIDI event queue overflow");
                    }

                    // We don't know anything about ordering of events, reorder them chronologically
                    sQueue.sort();
                }
        };

        class MidiOutputPort: public Port
        {
            private:
                plug::midi_t    sQueue;                     // MIDI event buffer
                VstEvents      *pEvents;                    // Root pointer to VST MIDI events
                VstMidiEvent    vEvents[MIDI_EVENTS_MAX];   // Buffer for VST MIDI events

            public:
                explicit MidiOutputPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
                {
                    sQueue.clear();
                    bzero(vEvents, sizeof(vEvents));

                    // Allocate buffer for VST MIDI events
                    size_t evt_size = align_size(sizeof(VstEvents) + MIDI_EVENTS_MAX * sizeof(VstMidiEvent *), DEFAULT_ALIGN);
                    pEvents         = reinterpret_cast<VstEvents *>(new uint8_t[evt_size]);
                }

                MidiOutputPort(const MidiOutputPort &) = delete;
                MidiOutputPort(MidiOutputPort &&) = delete;
                MidiOutputPort & operator = (const MidiOutputPort &) = delete;
                MidiOutputPort & operator = (MidiOutputPort &&) = delete;

                virtual ~MidiOutputPort() override
                {
                    if (pEvents != NULL)
                    {
                        delete [] pEvents;
                        pEvents = NULL;
                    }
                }

            public:
                virtual void *buffer() override
                {
                    return &sQueue;
                }

            public:
                void flush()
                {
                    // Check that there are pending MIDI events
                    if (sQueue.nEvents <= 0)
                        return;

                    // We don't know anything about ordering of events, reorder them chronologically
                    sQueue.sort();

                    // Translate events
                    pEvents->numEvents  = 0;

                    for (size_t i=0; i<sQueue.nEvents; ++i)
                    {
                        const midi::event_t    *me  = &sQueue.vEvents[i];
                        VstMidiEvent           *dst = &vEvents[pEvents->numEvents];

                        // Debug
                        #ifdef LSP_TRACE
                            #define TRACE_KEY(x)    case midi::MIDI_MSG_ ## x: evt_type = #x; break;

                            char tmp_evt_type[32];
                            const char *evt_type = NULL;
                            switch (me->type)
                            {
                                TRACE_KEY(NOTE_OFF)
                                TRACE_KEY(NOTE_ON)
                                TRACE_KEY(NOTE_PRESSURE)
                                TRACE_KEY(NOTE_CONTROLLER)
                                TRACE_KEY(PROGRAM_CHANGE)
                                TRACE_KEY(CHANNEL_PRESSURE)
                                TRACE_KEY(PITCH_BEND)
                                TRACE_KEY(SYSTEM_EXCLUSIVE)
                                TRACE_KEY(MTC_QUARTER)
                                TRACE_KEY(SONG_POS)
                                TRACE_KEY(SONG_SELECT)
                                TRACE_KEY(TUNE_REQUEST)
                                TRACE_KEY(END_EXCLUSIVE)
                                TRACE_KEY(CLOCK)
                                TRACE_KEY(START)
                                TRACE_KEY(CONTINUE)
                                TRACE_KEY(STOP)
                                TRACE_KEY(ACTIVE_SENSING)
                                TRACE_KEY(RESET)
                                default:
                                    snprintf(tmp_evt_type, sizeof(tmp_evt_type), "UNKNOWN(0x%02x)", int(me->type));
                                    evt_type = tmp_evt_type;
                                    break;
                            }

                            lsp_trace("MIDI Event: type=%s, timestamp=%ld", evt_type, (long)(me->timestamp));

                            #undef TRACE_KEY

                        #endif /* LSP_TRACE */

                        ssize_t bytes = midi::encode(reinterpret_cast<uint8_t *>(dst->midiData), me);
                        if (bytes <= 0)
                        {
                            lsp_error("Tried to serialize invalid MIDI event");
                            continue;
                        }

                        dst->type           = kVstMidiType;
                        dst->byteSize       = sizeof(VstMidiEvent);
                        dst->deltaFrames    = me->timestamp;
                        dst->flags          = (me->type >= midi::MIDI_MSG_CLOCK) ? kVstMidiEventIsRealtime : 0;
                        dst->noteLength     = 0;
                        dst->noteOffset     = 0;
                        dst->detune         = 0;
                        dst->noteOffVelocity= (me->type == midi::MIDI_MSG_NOTE_OFF) ? me->note.velocity : 0;

                        lsp_trace("midi dump: %02x %02x %02x",
                            int(dst->midiData[0]) & 0xff, int(dst->midiData[1]) & 0xff, int(dst->midiData[2]) & 0xff);

                        // Add pointer to the VST event to the VST eventn list
                        pEvents->events[pEvents->numEvents++]       = reinterpret_cast<VstEvent *>(dst);
                    }

                    // Call host to process MIDI events if they are
                    if (pEvents->numEvents > 0)
                    {
                        masterCallback(audioMasterProcessEvents, 0, 0, pEvents, 0.0f);
                        pEvents->numEvents      = 0;
                    }

                    sQueue.clear();
                }
        };

        class PathPort: public Port
        {
            private:
                vst2::path_t    sPath;

            public:
                explicit PathPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
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

                virtual void serialize(vst2::chunk_t *chunk) override
                {
                    chunk->write_string(sPath.sPath);
                }

                virtual ssize_t deserialize_v1(const void *data, size_t length) override
                {
                    const uint8_t  *ptr     = reinterpret_cast<const uint8_t *>(data);
                    const uint8_t  *tail    = ptr + length;
                    if (ptr >= tail)
                        return -1;

                    // Read length of string
                    size_t bytes        = *(ptr++);
                    if (bytes & 0x80)
                    {
                        if (ptr >= tail)
                            return -1;

                        bytes       = ((bytes << 8) | (*(ptr++))) & 0x7fff;
                    }

                    // Read string
                    tail           -= bytes;
                    if (ptr > tail)
                        return -1;

                    // Submit data
                    sPath.submit(reinterpret_cast<const char *>(ptr), bytes, false, plug::PF_STATE_RESTORE);
                    ptr            += bytes;
                    return ptr - reinterpret_cast<const uint8_t *>(data);
                }

                virtual bool deserialize_v2(const uint8_t *data, size_t size) override
                {
                    const char *str = reinterpret_cast<const char *>(data);
                    size_t len  = ::strnlen(str, size) + 1;
                    if (len > size)
                        return false;

                    sPath.submit(str, len, false, plug::PF_STATE_RESTORE);
                    return true;
                }

                virtual bool serializable() const override
                {
                    return true;
                }

                virtual bool changed() override
                {
                    if (!sPath.update())
                        return false;
                    if ((hCallback != NULL) && (pEffect != NULL))
                        hCallback(pEffect, audioMasterUpdateDisplay, 0, 0, 0, 0);
                    return true;
                }
        };

        class StringPort: public Port
        {
            private:
                plug::string_t     *pValue;

            public:
                explicit StringPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
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
                    return (pValue != NULL) ? pValue->sData : NULL;
                }

                virtual void serialize(vst2::chunk_t *chunk) override
                {
                    chunk->write_string(pValue->sData);
                }

                virtual ssize_t deserialize_v1(const void *data, size_t length) override
                {
                    const uint8_t  *ptr     = reinterpret_cast<const uint8_t *>(data);
                    const uint8_t  *tail    = ptr + length;
                    if (ptr >= tail)
                        return -1;

                    // Read length of string
                    size_t bytes        = *(ptr++);
                    if (bytes & 0x80)
                    {
                        if (ptr >= tail)
                            return -1;

                        bytes       = ((bytes << 8) | (*(ptr++))) & 0x7fff;
                    }

                    // Read string
                    tail           -= bytes;
                    if (ptr > tail)
                        return -1;

                    // Submit data
                    if (pValue != NULL)
                        pValue->submit(reinterpret_cast<const char *>(ptr), bytes, true);
                    ptr            += bytes;
                    return ptr - reinterpret_cast<const uint8_t *>(data);
                }

                virtual bool deserialize_v2(const uint8_t *data, size_t size) override
                {
                    const char *str = reinterpret_cast<const char *>(data);
                    size_t len  = ::strnlen(str, size);
                    if (len > size)
                        return false;

                    if (pValue != NULL)
                        pValue->submit(str, len, true);
                    return true;
                }

                virtual bool serializable() const override
                {
                    return true;
                }

                virtual bool changed() override
                {
                    if (pValue == NULL)
                        return false;
                    if (!pValue->sync())
                        return false;
                    if ((!pValue->is_state()) && (hCallback != NULL) && (pEffect != NULL))
                        hCallback(pEffect, audioMasterUpdateDisplay, 0, 0, 0, 0);
                    return true;
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
                explicit OscPort(const meta::port_t *meta, AEffect *effect, audioMasterCallback callback):
                    Port(meta, effect, callback)
                {
                    pFB     = core::osc_buffer_t::create(OSC_BUFFER_MAX);
                }

                OscPort(const OscPort &) = delete;
                OscPort(OscPort &&) = delete;
                OscPort & operator = (const OscPort &) = delete;
                OscPort & operator = (OscPort &&) = delete;

                virtual ~OscPort() override
                {
                    if (pFB != NULL)
                    {
                        core::osc_buffer_t::destroy(pFB);
                        pFB     = NULL;
                    }
                }

            public:
                virtual void *buffer() override
                {
                    return pFB;
                }

        };
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_PORTS_H_ */
