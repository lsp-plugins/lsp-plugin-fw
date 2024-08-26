/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp/dsp.h>

#include <lsp-plug.in/plug-fw/core/AudioBuffer.h>
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/plug-fw/wrap/jack/types.h>
#include <lsp-plug.in/plug-fw/wrap/jack/wrapper.h>

#include <jack/jack.h>
#include <jack/midiport.h>

namespace lsp
{
    namespace jack
    {
        class Wrapper;

        class Port: public plug::IPort
        {
            protected:
                Wrapper         *pWrapper;

            public:
                explicit Port(const meta::port_t *meta, Wrapper *w): IPort(meta)
                {
                    pWrapper        = w;
                }

                Port(const Port &) = delete;
                Port(Port &&) = delete;

                virtual ~Port() override
                {
                    pWrapper        = NULL;
                }

                Port & operator = (const Port &) = delete;
                Port & operator = (Port &&) = delete;

            public:
                virtual status_t init()
                {
                    return STATUS_OK;
                }

                virtual void destroy()
                {
                }

                /**
                 * Synchronize port state and return changed status
                 * @return changed status
                 */
                virtual bool sync()
                {
                    return false;
                }

                virtual void commit_value(float value)
                {
                }
        };

        class DataPort: public Port
        {
            private:
                jack_port_t    *pPort;              // JACK port descriptor
                void           *pDataBuffer;        // Real data buffer passed from JACK
                void           *pBuffer;            // Data buffer
                plug::midi_t   *pMidi;              // Midi buffer for operating MIDI messages
                float          *pSanitized;         // Input float data for sanitized buffers
                size_t          nBufSize;           // Size of sanitized buffer in samples

            public:
                explicit DataPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    pPort       = NULL;
                    pDataBuffer = NULL;
                    pBuffer     = NULL;
                    pMidi       = NULL;
                    pSanitized  = NULL;
                    nBufSize    = 0;
                }

                DataPort(const DataPort &) = delete;
                DataPort(DataPort &&) = delete;

                virtual ~DataPort() override
                {
                    pPort       = NULL;
                    pDataBuffer = NULL;
                    pBuffer     = NULL;
                    pMidi       = NULL;
                    pSanitized  = NULL;
                    nBufSize    = 0;
                };

                DataPort & operator = (const DataPort &) = delete;
                DataPort & operator = (DataPort &&) = delete;

            public:
                virtual int init() override
                {
                    return connect();
                }

                virtual void destroy() override
                {
                    disconnect();
                }

            public:
                virtual void *buffer() override
                {
                    return pBuffer;
                };

            public:
                status_t disconnect()
                {
                    if (pPort == NULL)
                        return STATUS_OK;

                    jack_client_t *cl   = pWrapper->client();
                    if (cl != NULL)
                        jack_port_unregister(cl, pPort);

                    if (pSanitized != NULL)
                    {
                        ::free(pSanitized);
                        pSanitized = NULL;
                    }

                    if (pMidi != NULL)
                    {
                        ::free(pMidi);
                        pMidi       = NULL;
                    }

                    pPort       = NULL;
                    nBufSize    = 0;

                    return STATUS_OK;
                }

                status_t connect()
                {
                    // Determine port type
                    const char *port_type = NULL;
                    if (meta::is_audio_port(pMetadata))
                        port_type = JACK_DEFAULT_AUDIO_TYPE;
                    else if (meta::is_midi_port(pMetadata))
                    {
                        port_type   = JACK_DEFAULT_MIDI_TYPE;
                        pMidi       = static_cast<plug::midi_t *>(::malloc(sizeof(plug::midi_t)));
                        if (pMidi == NULL)
                            return STATUS_NO_MEM;
                        pMidi->clear();
                    }
                    else
                        return STATUS_BAD_FORMAT;

                    // Determine flags
                    size_t flags = (meta::is_out_port(pMetadata)) ? JackPortIsOutput : JackPortIsInput;

                    // Get client
                    jack_client_t *cl   = pWrapper->client();
                    if (cl == NULL)
                    {
                        if (pMidi != NULL)
                        {
                            ::free(pMidi);
                            pMidi   = NULL;
                        }
                        return STATUS_DISCONNECTED;
                    }

                    // Register port
                    pPort       = jack_port_register(cl, pMetadata->id, port_type, flags, 0);

                    return (pPort != NULL) ? STATUS_OK : STATUS_UNKNOWN_ERR;
                }

                const char *jack_name() const
                {
                    return jack_port_name(pPort);
                }

                void set_buffer_size(size_t size)
                {
                    // set_buffer_size should affect only input audio ports at this moment
                    if ((!meta::is_in_port(pMetadata)) || (pMidi != NULL))
                        return;

                    // Buffer size has changed?
                    if (nBufSize == size)
                        return;

                    float *buf  = reinterpret_cast<float *>(::realloc(pSanitized, sizeof(float) * size));
                    if (buf == NULL)
                    {
                        if (pSanitized != NULL)
                        {
                            ::free(pSanitized);
                            pSanitized = NULL;
                        }
                        return;
                    }

                    nBufSize    = size;
                    pSanitized  = buf;
                    dsp::fill_zero(pSanitized, nBufSize);
                }

                void report_latency(ssize_t latency)
                {
                    // Only output ports should report latency
                    if ((pMetadata == NULL) || (!meta::is_out_port(pMetadata)))
                        return;

                    // Report latency
                    jack_latency_range_t range;
                    jack_port_get_latency_range(pPort, JackCaptureLatency, &range);
                    range.min += latency;
                    range.max += latency;
                    jack_port_set_latency_range (pPort, JackCaptureLatency, &range);
                }

                bool before_process(size_t samples)
                {
                    if (pPort == NULL)
                    {
                        pBuffer     = NULL;
                        return false;
                    }

                    pDataBuffer = jack_port_get_buffer(pPort, samples);
                    pBuffer     = pDataBuffer;

                    if (pMidi != NULL)
                    {
                        if ((pBuffer != NULL) && (meta::is_in_port(pMetadata)))
                        {
                            // Clear our buffer
                            pMidi->clear();

                            // Read MIDI events
                            jack_midi_event_t   midi_event;
                            midi::event_t       ev;

                            jack_nframes_t event_count = jack_midi_get_event_count(pBuffer);
                            for (jack_nframes_t i=0; i<event_count; i++)
                            {
                                // Read MIDI event
                                if (jack_midi_event_get(&midi_event, pBuffer, i) != 0)
                                {
                                    lsp_warn("Could not fetch MIDI event #%d from JACK port", int(i));
                                    continue;
                                }

                                // Convert MIDI event
                                if (midi::decode(&ev, midi_event.buffer) <= 0)
                                {
                                    lsp_warn("Could not decode MIDI event #%d at timestamp %d from JACK port", int(i), int(midi_event.time));
                                    continue;
                                }

                                // Update timestamp and store event
                                ev.timestamp    = midi_event.time;
                                if (!pMidi->push(ev))
                                    lsp_warn("Could not append MIDI event #%d at timestamp %d due to buffer overflow", int(i), int(midi_event.time));
                            }

                            // All MIDI events ARE ordered chronologically, we do not need to perform sort
                        }

                        // Replace pBuffer with pMidi
                        pBuffer     = pMidi;
                    }
                    else if (pSanitized != NULL) // Need to sanitize?
                    {
                        // Perform sanitize() if possible
                        if (samples <= nBufSize)
                        {
                            dsp::sanitize2(pSanitized, reinterpret_cast<float *>(pDataBuffer), samples);
                            pBuffer = pSanitized;
                        }
                        else
                        {
                            lsp_warn("Could not sanitize buffer data for port %s, not enough buffer size (required: %d, actual: %d)",
                                    pMetadata->id, int(samples), int(nBufSize));
                        }
                    }

                    return false;
                }

                void after_process(size_t samples)
                {
                    if ((pMidi != NULL) && (pDataBuffer != NULL) && (meta::is_out_port(pMetadata)))
                    {
                        // Reset buffer
                        jack_midi_clear_buffer(pDataBuffer);

                        // Transfer MIDI events
                        pMidi->sort();  // All events SHOULD be ordered chonologically

                        // Transmit all events
                        for (size_t i=0, events=pMidi->nEvents; i<events; ++i)
                        {
                            // Determine size of the message
                            midi::event_t *ev   = &pMidi->vEvents[i];
                            ssize_t size        = midi::size_of(ev);
                            if (size <= 0)
                            {
                                lsp_warn("Could not encode output MIDI message of type 0x%02x, timestamp=%d", int(ev->type), int(ev->timestamp));
                                continue;
                            }

                            // Allocate MIDI event
                            jack_midi_data_t *midi_data     = jack_midi_event_reserve(pDataBuffer, ev->timestamp, size);
                            if (midi_data == NULL)
                            {
                                lsp_warn("Could not write MIDI message of type 0x%02x, size=%d, timestamp=%d to JACK output port buffer=%p",
                                        int(ev->type), int(size), int(ev->timestamp), pBuffer);
                                continue;
                            }

                            // Encode MIDI event
                            midi::encode(midi_data, ev);
                        }

                        // Cleanup the output buffer
                        pMidi->clear();
                    }
                    else if (meta::is_audio_out_port(pMetadata))
                        // Sanitize output data
                        dsp::sanitize1(reinterpret_cast<float *>(pDataBuffer), samples);

                    pBuffer     = NULL;
                }
        };

        class AudioBufferPort: public Port
        {
            private:
                core::AudioBuffer   sBuffer;

            public:
                explicit AudioBufferPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                }

                AudioBufferPort(const DataPort &) = delete;
                AudioBufferPort(DataPort &&) = delete;

                AudioBufferPort & operator = (const AudioBufferPort &) = delete;
                AudioBufferPort & operator = (AudioBufferPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return &sBuffer;
                }

            public:
                void set_buffer_size(size_t size)
                {
                    sBuffer.set_size(size);
                }
        };

        class ControlPort: public Port
        {
            protected:
                float       fNewValue;
                float       fCurrValue;

            public:
                explicit ControlPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    fNewValue   = meta->start;
                    fCurrValue  = meta->start;
                }

                ControlPort(const ControlPort &) = delete;
                ControlPort(ControlPort &&) = delete;

                virtual ~ControlPort() override
                {
                    fNewValue   = pMetadata->start;
                    fCurrValue  = pMetadata->start;
                };

                ControlPort & operator = (const ControlPort &) = delete;
                ControlPort & operator = (ControlPort &&) = delete;

            public:
                virtual bool sync() override
                {
                    if (fNewValue == fCurrValue)
                        return false;

                    fCurrValue   = fNewValue;
                    return true;
                }

                virtual float value() override
                {
                    return fCurrValue;
                }

                virtual void commit_value(float value) override
                {
                    fNewValue   = meta::limit_value(pMetadata, value);
                }
        };

        class PortGroup: public ControlPort
        {
            private:
                size_t                  nCols;
                size_t                  nRows;

            public:
                explicit PortGroup(const meta::port_t *meta, Wrapper *w) : ControlPort(meta, w)
                {
                    nCols               = meta::port_list_size(meta->members);
                    nRows               = meta::list_size(meta->items);
                }

                PortGroup(const Port &) = delete;
                PortGroup(Port &&) = delete;

                virtual ~PortGroup() override
                {
                    nCols               = 0;
                    nRows               = 0;
                }

                PortGroup & operator = (const PortGroup &) = delete;
                PortGroup & operator = (PortGroup &&) = delete;

            public:
                virtual void commit_value(float value) override
                {
                    fNewValue   = lsp_limit(ssize_t(value), 0, ssize_t(nRows));
                }

            public:
                inline size_t rows() const      { return nRows; }
                inline size_t cols() const      { return nCols; }
        };

        class MeterPort: public Port
        {
            private:
                float       fValue;
                bool        bForce;

            public:
                explicit MeterPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    fValue      = meta->start;
                    bForce      = true;
                }

                MeterPort(const Port &) = delete;
                MeterPort(Port &&) = delete;

                virtual ~MeterPort() override
                {
                    fValue      = pMetadata->start;
                };

                MeterPort & operator = (const MeterPort &) = delete;
                MeterPort & operator = (MeterPort &&) = delete;

            public:
                virtual float value() override
                {
                    return fValue;
                }

                virtual void set_value(float value) override
                {
                    value   = meta::limit_value(pMetadata, value);

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
                    bForce  = true;
                    return value;
                }
        };

        class MeshPort: public Port
        {
            private:
                plug::mesh_t       *pMesh;

            public:
                explicit MeshPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    pMesh   = NULL;
                }

                MeshPort(const MeshPort &) = delete;
                MeshPort(MeshPort &&) = delete;

                virtual ~MeshPort() override
                {
                    pMesh   = NULL;
                }

                MeshPort & operator = (const MeshPort &) = delete;
                MeshPort & operator = (MeshPort &&) = delete;

            public:
                virtual int init() override
                {
                    pMesh   = jack::create_mesh(pMetadata);
                    return (pMesh == NULL) ? STATUS_NO_MEM : STATUS_OK;
                }

                virtual void destroy() override
                {
                    if (pMesh == NULL)
                        return;

                    jack::destroy_mesh(pMesh);
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
                explicit StreamPort(const meta::port_t *meta, Wrapper *w): Port(meta, w)
                {
                    pStream     = NULL;
                }

                StreamPort(const Port &) = delete;
                StreamPort(Port &&) = delete;

                virtual ~StreamPort() override
                {
                    pStream     = NULL;
                }

                StreamPort & operator = (const StreamPort &) = delete;
                StreamPort & operator = (StreamPort &&) = delete;

            public:
                virtual int init() override
                {
                    pStream = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                    return (pStream == NULL) ? STATUS_NO_MEM : STATUS_OK;
                }

                virtual void destroy() override
                {
                    plug::stream_t::destroy(pStream);
                    pStream     = NULL;
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
                plug::frame_buffer_t        sFB;

            public:
                explicit FrameBufferPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    bzero(&sFB, sizeof(sFB));
                }

                FrameBufferPort(const FrameBufferPort &) = delete;
                FrameBufferPort(FrameBufferPort &&) = delete;
                FrameBufferPort & operator = (const FrameBufferPort &) = delete;
                FrameBufferPort & operator = (FrameBufferPort &&) = delete;

            public:
                virtual int init() override
                {
                    return sFB.init(pMetadata->start, pMetadata->step);
                }

                virtual void destroy() override
                {
                    sFB.destroy();
                }

            public:
                virtual void *buffer() override
                {
                    return &sFB;
                }
        };

        class OscPort: public Port
        {
            private:
                core::osc_buffer_t     *pFB;

            public:
                explicit OscPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    pFB     = NULL;
                }

                OscPort(const OscPort &) = delete;
                OscPort(OscPort &&) = delete;
                OscPort & operator = (const OscPort &) = delete;
                OscPort & operator = (OscPort &&) = delete;

            public:
                virtual int init() override
                {
                    pFB = core::osc_buffer_t::create(OSC_BUFFER_MAX);
                    return (pFB == NULL) ? STATUS_NO_MEM : STATUS_OK;
                }

                virtual void destroy() override
                {
                    if (pFB == NULL)
                        return;

                    core::osc_buffer_t::destroy(pFB);
                    pFB     = NULL;
                }

            public:
                virtual void *buffer() override
                {
                    return pFB;
                }
        };

        class PathPort: public Port
        {
            private:
                jack::path_t    sPath;

            public:
                explicit PathPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
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

                virtual bool sync() override
                {
                    return sPath.pending();
                }
        };

        class StringPort: public Port
        {
            private:
                plug::string_t     *pValue;
                uint32_t            nUISerial;      // Actual serial number for UI
                uint32_t            nUIPending;     // Pending serial number for UI

            public:
                explicit StringPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    pValue          = plug::string_t::allocate(size_t(meta->max));
                    nUISerial       = 0;
                    atomic_store(&nUIPending, 0);
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

                virtual bool sync() override
                {
                    return (pValue != NULL) ? pValue->sync() : false;
                }

                virtual float value() override
                {
                    return (pValue != NULL) ? (pValue->nSerial & 0x3fffff) : 0.0f;
                }

                virtual void set_default() override
                {
                    pValue->sData[0] = '\0';
                    atomic_add(&nUIPending, 1);
                }

            public:
                plug::string_t *data()
                {
                    return pValue;
                }

                bool check_reset_pending()
                {
                    const uint32_t request = atomic_load(&nUIPending);
                    if (request == nUISerial)
                        return false;

                    nUISerial = request;
                    return true;
                }
        };

    } /* namespace jack */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_PORTS_H_ */
