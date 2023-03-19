/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp/dsp.h>

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

                virtual ~Port()
                {
                    pWrapper        = NULL;
                }

                virtual int init()
                {
                    return STATUS_OK;
                }

                virtual void destroy()
                {
                }

                virtual void update_value(float value)
                {
                    set_value(value);
                }
        };

        class PortGroup: public Port
        {
            private:
                float                   nCurrRow;
                size_t                  nCols;
                size_t                  nRows;

            public:
                explicit PortGroup(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    nCurrRow            = meta->start;
                    nCols               = meta::port_list_size(meta->members);
                    nRows               = meta::list_size(meta->items);
                }

                virtual ~PortGroup()
                {
                    nCurrRow            = 0.0f;
                    nCols               = 0;
                    nRows               = 0;
                }

            public:
                virtual void set_value(float value)
                {
                    int32_t v = value;
                    if ((v >= 0) && (v < ssize_t(nRows)))
                        nCurrRow        = v;
                }

                virtual float value()
                {
                    return nCurrRow;
                }

            public:
                inline size_t rows() const      { return nRows; }
                inline size_t cols() const      { return nCols; }
                inline size_t curr_row() const  { return nCurrRow; }
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

                virtual ~DataPort()
                {
                    pPort       = NULL;
                    pDataBuffer = NULL;
                    pBuffer     = NULL;
                    pMidi       = NULL;
                    pSanitized  = NULL;
                    nBufSize    = 0;
                };

                virtual int init()
                {
                    return connect();
                }

                virtual void destroy()
                {
                    disconnect();
                }

            public:
                virtual void *buffer()
                {
                    return pBuffer;
                };

                int disconnect()
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

                int connect()
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
                        ::free(pSanitized);
                        pSanitized = NULL;
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

                virtual bool pre_process(size_t samples)
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

                virtual void post_process(size_t samples)
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

        class ControlPort: public Port
        {
            private:
                float       fNewValue;
                float       fCurrValue;

            public:
                explicit ControlPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    fNewValue   = meta->start;
                    fCurrValue  = meta->start;
                }

                virtual ~ControlPort()
                {
                    fNewValue   = pMetadata->start;
                    fCurrValue  = pMetadata->start;
                };

            public:
                virtual bool pre_process(size_t samples)
                {
                    if (fNewValue == fCurrValue)
                        return false;

                    fCurrValue   = fNewValue;
                    return true;
                }

                virtual float value()
                {
                    return fCurrValue;
                }

                virtual void update_value(float value)
                {
                    fNewValue   = meta::limit_value(pMetadata, value);
                }
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

                virtual ~MeterPort()
                {
                    fValue      = pMetadata->start;
                };

            public:
                virtual float value()
                {
                    return fValue;
                }

                virtual void set_value(float value)
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

                virtual ~MeshPort()
                {
                    pMesh   = NULL;
                }

                virtual int init()
                {
                    pMesh   = jack::create_mesh(pMetadata);
                    return (pMesh == NULL) ? STATUS_NO_MEM : STATUS_OK;
                }

                virtual void destroy()
                {
                    if (pMesh == NULL)
                        return;

                    jack::destroy_mesh(pMesh);
                    pMesh = NULL;
                }

            public:
                virtual void *buffer()
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

                virtual ~StreamPort()
                {
                    pStream     = NULL;
                }

            public:
                virtual void *buffer()
                {
                    return pStream;
                }

                virtual int init()
                {
                    pStream = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                    return (pStream == NULL) ? STATUS_NO_MEM : STATUS_OK;
                }

                virtual void destroy()
                {
                    plug::stream_t::destroy(pStream);
                    pStream     = NULL;
                }
        };

        class FrameBufferPort: public Port
        {
            private:
                plug::frame_buffer_t        sFB;

            public:
                explicit FrameBufferPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                }

                virtual ~FrameBufferPort()
                {
                }

                virtual int init()
                {
                    return sFB.init(pMetadata->start, pMetadata->step);
                }

                virtual void destroy()
                {
                    sFB.destroy();
                }

            public:
                virtual void *buffer()
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

                virtual ~OscPort()
                {
                }

                virtual int init()
                {
                    pFB = core::osc_buffer_t::create(OSC_BUFFER_MAX);
                    return (pFB == NULL) ? STATUS_NO_MEM : STATUS_OK;
                }

                virtual void destroy()
                {
                    if (pFB == NULL)
                        return;

                    core::osc_buffer_t::destroy(pFB);
                    pFB     = NULL;
                }

            public:
                virtual void *buffer()
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

                virtual ~PathPort()
                {
                }

            public:
                virtual void *buffer()
                {
                    return static_cast<plug::path_t *>(&sPath);
                }

                virtual bool pre_process(size_t samples)
                {
                    return sPath.pending();
                }
        };
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_PORTS_H_ */
