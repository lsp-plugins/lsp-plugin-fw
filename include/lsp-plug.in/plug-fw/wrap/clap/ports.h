/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 дек. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/clap/data.h>
#include <lsp-plug.in/plug-fw/wrap/clap/helpers.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace clap
    {
        // Specify port classes
        class Port: public plug::IPort
        {
            public:
                explicit Port(const meta::port_t *meta): plug::IPort(meta)
                {
                }

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
                virtual status_t serialize(const clap_ostream_t *os) { return STATUS_OK; }

                /** Serialize the state of the port to the chunk
                 *
                 * @param is input stream to perform deserialization
                 */
                virtual status_t deserialize(const clap_istream_t *is) { return STATUS_OK; }
        };

        /**
         * Audio port: input or output
         */
        class AudioPort: public Port
        {
            protected:
                float      *pBind;              // Bound buffer
                float      *pBuffer;            // The original buffer passed by the host OR sanitized buffer
                uint32_t    nOffset;            // The relative offset from the beginning of the buffer
                uint32_t    nBufSize;           // The actual current buffer size
                uint32_t    nBufCap;            // The quantized capacity of the buffer
                bool        bZero;              // Buffer contains zero data

            public:
                explicit AudioPort(const meta::port_t *meta) : Port(meta)
                {
                    pBind       = NULL;
                    pBuffer     = NULL;
                    nOffset     = 0;
                    nBufSize    = 0;
                    nBufCap     = 0;
                    bZero       = false;
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
                    return &pBind[nOffset];
                }

                virtual void post_process(size_t samples) override
                {
                    nOffset    += samples;
                }

            public:
                // Activate the port, issued by the plugin activate() method
                // Allocates enough space for data sanitize.
                bool activate(size_t min_frames_count, size_t max_frames_count)
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

//                    lsp_trace("id=%s, pBind=%p, pBuffer=%p, max_frames_count=%d",
//                        pMetadata->id, pBind, pBuffer, int(max_frames_count));

                    nBufCap    = capacity;
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
                        if (ptr != NULL)
                        {
                            dsp::sanitize2(pBuffer, ptr, samples);
                            bZero       = false;
                        }
                        else if (!bZero)
                        {
                            dsp::fill_zero(pBuffer, nBufCap);
                            bZero       = true;
                        }
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

        class MidiInputPort: public Port
        {
            protected:
                plug::midi_t    sQueue;             // MIDI event buffer

            public:
                explicit MidiInputPort(const meta::port_t *meta): Port(meta)
                {
                    sQueue.clear();
                }

            public:
                virtual void *buffer()
                {
                    return &sQueue;
                }

            public:
                inline void clear()
                {
                    sQueue.clear();
                }

                inline bool push(const midi::event_t *me)
                {
                    return sQueue.push(me);
                }
        };

        class MidiOutputPort: public Port
        {
            protected:
                plug::midi_t    sQueue;             // MIDI event buffer
                size_t          nOffset;            // Read-out offset

            public:
                explicit MidiOutputPort(const meta::port_t *meta): Port(meta)
                {
                    nOffset     = 0;
                    sQueue.clear();
                }

            public:
                virtual void *buffer()
                {
                    return &sQueue;
                }

            public:
                inline void clear()
                {
                    nOffset     = 0;
                    sQueue.clear();
                }

                inline const midi::event_t *get(size_t index) const
                {
                    return (index < sQueue.nEvents) ? &sQueue.vEvents[index] : NULL;
                }

                inline const midi::event_t *front() const
                {
                    return get(nOffset);
                }

                inline const midi::event_t *peek()
                {
                    return (nOffset < sQueue.nEvents) ? &sQueue.vEvents[nOffset++] : NULL;
                }

                inline size_t size() const
                {
                    return sQueue.nEvents;
                }
        };

        class ParameterPort: public Port
        {
            protected:
                float           fValue;         // The actual value of the port
                float           fClapValue;     // The actual value in CLAP units scale
                clap_id         nID;            // Unique CLAP identifier of the port
                uatomic_t       nSID;           // Serial ID of the parameter
                float           fPending;       // Pending value from the UI
                uatomic_t       nPendingSID;    // Serial ID of parameter for the UI request
                volatile bool   bPending;       // There is pending for change request from the UI

            public:
                explicit ParameterPort(const meta::port_t *meta) : Port(meta)
                {
                    fValue              = meta->start;
                    fClapValue          = to_clap_value(meta, fValue, NULL, NULL);
                    nID                 = clap_hash_string(meta->id);
                    nSID                = 0;
                    fPending            = 0.0f;
                    nPendingSID         = nSID;
                    bPending            = false;
                }

            public:
                inline clap_id uid() const      { return nID;       }
                inline uatomic_t sid() const    { return nSID;      }

                bool clap_set_value(float value)
                {
                    lsp_trace("value = %f", value);
                    if (fClapValue == value)
                        return false;

                    fClapValue  = value;
                    value       = from_clap_value(pMetadata, value);
                    lsp_trace("from_clap_value = %f", value);

                    float old   = fValue;
                    float res   = meta::limit_value(pMetadata, value);
                    if (old == res)
                        return false;

                    fValue      = res;
                    atomic_add(&nSID, 1);

                    return true;
                }

                bool clap_mod_value(float value)
                {
                    lsp_trace("value = %f", value);
                    if (value == 0.0f)
                        return false;

                    fClapValue += value;
                    value       = from_clap_value(pMetadata, fClapValue);
                    lsp_trace("from_clap_value = %f", value);

                    float old   = fValue;
                    float res   = meta::limit_value(pMetadata, value);
                    if (old == res)
                        return false;

                    fValue      = res;
                    atomic_add(&nSID, 1);

                    return true;
                }

                void write_value(float value)
                {
                    fPending            = value;
                    nPendingSID         = nSID;
                    bPending            = true;
                }

                bool sync(const clap_output_events_t *out)
                {
                    if (!bPending)
                        return false;

                    // Cleanup pending flags and read the requested value
                    bPending            = false;
                    uatomic_t sid       = nPendingSID;
                    float pending       = fPending;

                    // Do not perform anything if SID does not match
                    if (sid != nSID)
                        return false;

                    // Ensure that value has changed
                    float old   = fValue;
                    float res   = meta::limit_value(pMetadata, pending);
                    if (old == res)
                        return false;

                    // Try to submit the value to the output event queue
                    float clap_value    = to_clap_value(pMetadata, pending, NULL, NULL);
                    clap_event_param_value_t ev;
                    ev.header.size      = sizeof(ev);
                    ev.header.time      = 0;
                    ev.header.space_id  = CLAP_CORE_EVENT_SPACE_ID;
                    ev.header.type      = CLAP_EVENT_PARAM_VALUE;
                    ev.header.flags     = CLAP_EVENT_IS_LIVE;
                    ev.param_id         = nID;
                    ev.cookie           = this;
                    ev.note_id          = -1;
                    ev.port_index       = -1;
                    ev.channel          = -1;
                    ev.key              = -1;
                    ev.value            = clap_value;

                    // Submit the value to the output queue
                    if (out->try_push(out, &ev.header))
                    {
                        fValue              = res;
                        fClapValue          = clap_value;
                        atomic_add(&nSID, 1);
                        return true;
                    }

                    // Try to apply changes next time
                    bPending            = true;
                    return false;
                }

            public:
                virtual float value() override  { return fValue; }

                virtual bool serializable() const  override { return true; }

                virtual status_t serialize(const clap_ostream_t *os) override
                {
                    float v = fValue;
                    status_t res = write_fully(os, uint8_t(clap::TYPE_FLOAT32));
                    if (res == STATUS_OK)
                        res = write_fully(os, v);
                    return res;
                }

                virtual status_t deserialize(const clap_istream_t *is) override
                {
                    uint8_t type = 0;
                    float value = 0.0f;

                    // Read the type
                    status_t res = read_fully(is, &type);
                    if (res != STATUS_OK)
                        return res;
                    else if (type != clap::TYPE_FLOAT32)
                        return STATUS_BAD_FORMAT;

                    // Read the value
                    res = read_fully(is, &value);
                    if (res != STATUS_OK)
                        return res;

                    lsp_trace("  value = %f", value);

                    // Commit value
                    float old           = fValue;
                    fValue              = value;
                    fClapValue          = to_clap_value(pMetadata, fValue, NULL, NULL);
                    if (old != fValue)
                        atomic_add(&nSID, 1);

                    return STATUS_OK;
                }
        };

        class MeterPort: public Port
        {
            public:
                float   fValue;
                bool    bForce;

            public:
                explicit MeterPort(const meta::port_t *meta):
                    Port(meta)
                {
                    fValue      = meta->start;
                    bForce      = true;
                }

                virtual ~MeterPort()
                {
                    fValue      = pMetadata->start;
                }

            public:
                // Native Interface
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
                    pMesh   = clap::create_mesh(meta);
                }

                virtual ~MeshPort() override
                {
                    clap::destroy_mesh(pMesh);
                    pMesh = NULL;
                }

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
                explicit StreamPort(const meta::port_t *meta):
                    Port(meta)
                {
                    pStream     = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                }

                virtual ~StreamPort() override
                {
                    if (pStream != NULL)
                    {
                        plug::stream_t::destroy(pStream);
                        pStream     = NULL;
                    }
                }

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
                explicit FrameBufferPort(const meta::port_t *meta):
                    Port(meta)
                {
                    sFB.init(pMetadata->start, pMetadata->step);
                }

                virtual ~FrameBufferPort() override
                {
                    sFB.destroy();
                }

            public:
                virtual void *buffer() override
                {
                    return &sFB;
                }
        };

        class PathPort: public Port
        {
            private:
                clap::path_t    sPath;

            public:
                explicit PathPort(const meta::port_t *meta):
                    Port(meta)
                {
                    sPath.init();
                }

                virtual ~PathPort()
                {
                }

            public:
                virtual void *buffer() override
                {
                    return static_cast<plug::path_t *>(&sPath);
                }

                virtual bool pre_process(size_t samples) override
                {
                    return sPath.pending();
                }

                virtual status_t serialize(const clap_ostream_t *os) override
                {
                    status_t res = write_fully(os, uint8_t(clap::TYPE_STRING));
                    if (res != STATUS_OK)
                        return res;

                    return sPath.serialize(os);
                }

                virtual status_t deserialize(const clap_istream_t *is) override
                {
                    uint8_t type = 0;

                    // Read the type
                    status_t res = read_fully(is, &type);
                    if (res != STATUS_OK)
                        return res;
                    else if (type != clap::TYPE_STRING)
                        return STATUS_BAD_FORMAT;

                    res = sPath.deserialize(is);
                    if (res == STATUS_OK)
                        lsp_trace("  value = %s", sPath.sDspRequest);

                    return res;
                }

                virtual bool serializable() const override { return true; }
        };

        class OscPort: public Port
        {
            private:
                core::osc_buffer_t     *pFB;

            public:
                explicit OscPort(const meta::port_t *meta):
                    Port(meta)
                {
                    pFB     = NULL;
                }

                virtual ~OscPort()
                {
                }

            public:
                virtual void *buffer()
                {
                    return pFB;
                }

                virtual int init()
                {
                    pFB     = core::osc_buffer_t::create(OSC_BUFFER_MAX);
                    return (pFB == NULL) ? STATUS_NO_MEM : STATUS_OK;
                }

                virtual void destroy()
                {
                    if (pFB != NULL)
                    {
                        core::osc_buffer_t::destroy(pFB);
                        pFB     = NULL;
                    }
                }
        };
    } /* namespace clap */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_PORTS_H_ */
