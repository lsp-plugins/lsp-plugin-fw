/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>

#include <lsp-plug.in/audio/iface/backend.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/dsp/dsp.h>

#include <lsp-plug.in/plug-fw/core/AudioBuffer.h>
#include <lsp-plug.in/plug-fw/core/AudioTracer.h>
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/plug-fw/wrap/standalone/types.h>
#include <lsp-plug.in/plug-fw/wrap/standalone/wrapper.h>

namespace lsp
{
    namespace standalone
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
            protected:
                audio::port_id_t    nPortID;            // Port identifier of the backend

            public:
                explicit DataPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    nPortID     = -1;
                }

                DataPort(const DataPort &) = delete;
                DataPort(DataPort &&) = delete;

                virtual ~DataPort() override
                {
                    nPortID     = -1;
                };

                DataPort & operator = (const DataPort &) = delete;
                DataPort & operator = (DataPort &&) = delete;

            public:
                const char *system_name() const
                {
                    if (nPortID < 0)
                        return NULL;

                    audio::backend_t * const backend = pWrapper->backend();
                    return backend->port_system_name(backend, nPortID);
                }

                status_t disconnect()
                {
                    if (nPortID < 0)
                        return STATUS_OK;

                    audio::backend_t * const backend = pWrapper->backend();
                    if (backend != NULL)
                        backend->unregister_port(backend, nPortID);

                    nPortID     = -1;

                    return STATUS_OK;
                }

            public:
                virtual status_t connect()
                {
                    return STATUS_NOT_IMPLEMENTED;
                }

                virtual void set_buffer_size(size_t size)
                {
                }

                virtual void before_process(size_t samples)
                {
                }

                virtual void after_process(size_t samples)
                {
                }
        };

        class AudioPort: public DataPort
        {
            private:
                float              *pBuffer;            // Real buffer passed from audio backend
                float              *pSanitized;         // Buffer for storing sanitized data
                size_t              nBufSize;           // Size of sanitized buffer in samples
                bool                bZero;

                IF_DEBUG( core::AudioTracer sTracer; )

            public:
                explicit AudioPort(const meta::port_t *meta, Wrapper *w) : DataPort(meta, w)
                {
                    pBuffer     = NULL;
                    pSanitized  = NULL;
                    nBufSize    = 0;
                    bZero       = false;
                }

                AudioPort(const AudioPort &) = delete;
                AudioPort(AudioPort &&) = delete;

                virtual ~AudioPort() override
                {
                    pBuffer     = NULL;
                    pSanitized  = NULL;
                    nBufSize    = 0;
                };

                AudioPort & operator = (const AudioPort &) = delete;
                AudioPort & operator = (AudioPort &&) = delete;

            public:
                virtual int init() override
                {
                    return connect();
                }

                virtual void destroy() override
                {
                    disconnect();

                    if (pSanitized != NULL)
                    {
                        ::free(pSanitized);
                        pSanitized = NULL;
                    }
                }

            public:
                virtual void *buffer() override
                {
                    if (nPortID < 0)
                        return NULL;
                    return (pSanitized != NULL) ? pSanitized : pBuffer;
                };

            public:
                virtual status_t connect() override
                {
                    audio::backend_t * const backend = pWrapper->backend();
                    if (backend == NULL)
                        return STATUS_DISCONNECTED;

                    // Register port
                    nPortID     = backend->register_port(
                        backend, pMetadata->id,
                        (meta::is_out_port(pMetadata)) ? audio::PORT_AUDIO_OUT : audio::PORT_AUDIO_IN);

                    return (nPortID >= 0) ? STATUS_OK : STATUS_UNKNOWN_ERR;
                }

                virtual void set_buffer_size(size_t size) override
                {
                    // set_buffer_size should affect only input audio ports at this moment
                    if (!meta::is_in_port(pMetadata))
                        return;

                    // Buffer size has changed?
                    if ((pSanitized != NULL) && (nBufSize == size))
                        return;

                    float * const buf  = reinterpret_cast<float *>(::realloc(pSanitized, sizeof(float) * size));
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
                }

                virtual void before_process(size_t samples) override
                {
                    if (nPortID < 0)
                        return;

                    audio::backend_t * const backend = pWrapper->backend();

                    // Need to sanitize?
                    pBuffer         = backend->get_audio_buffer(backend, nPortID, 0);
                    if (pSanitized == NULL)
                    {
                        IF_DEBUG( sTracer.submit(reinterpret_cast<float *>(pBuffer), samples) ); // Trace input data
                        return;
                    }

                    // Need to mix multiple buffers?
                    const size_t buf_count = backend->audio_buffers_count(backend, nPortID);
                    if ((buf_count == 0) || (pBuffer == NULL))
                    {
                        // Ensure that pSanitized now contains zeros
                        if (!bZero)
                        {
                            dsp::fill_zero(pSanitized, nBufSize);
                            bZero           = true;
                        }
                    }
                    else if (buf_count > 1)
                    {
                        // Need to mix buffers together
                        float * buf         = backend->get_audio_buffer(backend, nPortID, 1);
                        dsp::add3(pSanitized, pBuffer, buf, samples);

                        for (size_t index = 2; index < buf_count; ++index)
                        {
                            float * buf         = backend->get_audio_buffer(backend, nPortID, 1);
                            dsp::add2(pSanitized, buf, samples);
                        }

                        // Reset cleanup flag
                        bZero           = false;
                    }
                    else
                    {
                        dsp::sanitize2(pSanitized, pBuffer, samples);
                        bZero           = false;    // Reset cleanup flag
                    }

                    IF_DEBUG( sTracer.submit(reinterpret_cast<float *>(pSanitized), samples) ); // Trace input data

                    return;
                }

                virtual void after_process(size_t samples) override
                {
                    // Need to sanitize output data?
                    if (pSanitized == NULL)
                    {
                        dsp::sanitize1(reinterpret_cast<float *>(pBuffer), samples);
                        IF_DEBUG( sTracer.submit(reinterpret_cast<float *>(pBuffer), samples) ); // Trace output data
                    }
                    pBuffer     = NULL;
                }

                virtual void trace(const void *instance) override
                {
                    IF_DEBUG( sTracer.set_trace("standalone", id(), instance) );
                }
        };

        class MidiPort: public DataPort
        {
            private:
                plug::midi_t       *pMidi;              // Midi buffer for operating MIDI messages

            public:
                explicit MidiPort(const meta::port_t *meta, Wrapper *w) : DataPort(meta, w)
                {
                    pMidi       = NULL;
                }

                MidiPort(const MidiPort &) = delete;
                MidiPort(MidiPort &&) = delete;

                virtual ~MidiPort() override
                {
                    pMidi       = NULL;
                };

                MidiPort & operator = (const MidiPort &) = delete;
                MidiPort & operator = (MidiPort &&) = delete;

            public:
                virtual int init() override
                {
                    return connect();
                }

                virtual void destroy() override
                {
                    disconnect();

                    if (pMidi != NULL)
                    {
                        ::free(pMidi);
                        pMidi       = NULL;
                    }
                }

            public:
                virtual void *buffer() override
                {
                    return (nPortID >= 0) ? pMidi : NULL;
                };

            public:
                virtual status_t connect() override
                {
                    audio::backend_t * const backend = pWrapper->backend();
                    if (backend == NULL)
                        return STATUS_DISCONNECTED;

                    if (pMidi == NULL)
                    {
                        pMidi       = static_cast<plug::midi_t *>(::malloc(sizeof(plug::midi_t)));
                        if (pMidi == NULL)
                            return STATUS_NO_MEM;
                    }
                    pMidi->clear();

                    // Register port
                    nPortID     = backend->register_port(
                        backend, pMetadata->id,
                        (meta::is_out_port(pMetadata)) ? audio::PORT_MIDI_OUT : audio::PORT_MIDI_IN);

                    return (nPortID >= 0) ? STATUS_OK : STATUS_UNKNOWN_ERR;
                }

                virtual void before_process(size_t samples) override
                {
                    if (pMidi == NULL)
                        return;

                    // Only input MIDI ports require pre-processing
                    if ((nPortID < 0) || (!meta::is_in_port(pMetadata)))
                        return;

                    status_t res;
                    audio::backend_t * const backend = pWrapper->backend();

                    // Clear input MIDI buffer
                    pMidi->clear();

                    // Read MIDI events
                    audio::midi_event_t midi_event;
                    midi::event_t       ev;

                    const size_t event_count = backend->midi_events_count(backend, nPortID);
                    for (size_t i=0; i<event_count; ++i)
                    {
                        // Read MIDI event
                        if ((res = backend->read_midi_event(backend, nPortID, &midi_event, i)) != STATUS_OK)
                        {
                            lsp_warn("Could not fetch MIDI event #%d from MIDI port", int(i));
                            continue;
                        }

                        // Convert MIDI event
                        if (midi::decode(&ev, midi_event.data) <= 0)
                        {
                            lsp_warn("Could not decode MIDI event #%d at timestamp %d from MIDI port", int(i), int(midi_event.timestamp));
                            continue;
                        }

                        // Update timestamp and store event
                        ev.timestamp    = midi_event.timestamp;
                        if (!pMidi->push(ev))
                            lsp_warn("Could not append MIDI event #%d at timestamp %d due to buffer overflow", int(i), int(midi_event.timestamp));
                    }

                    // All MIDI events ARE ordered chronologically, we do not need to perform sort
                }

                virtual void after_process(size_t samples) override
                {
                    if (pMidi == NULL)
                        return;

                    // Cleanup output MIDI buffer at exit
                    lsp_finally { pMidi->clear(); };

                    // Only output MIDI ports require post-processing
                    if ((nPortID < 0) || (!meta::is_out_port(pMetadata)))
                        return;

                    audio::backend_t * const backend = pWrapper->backend();

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
                            lsp_warn(
                                "Could not encode output MIDI message of type 0x%02x, timestamp=%d",
                                int(ev->type), int(ev->timestamp));
                            continue;
                        }

                        // Allocate MIDI event
                        uint8_t * const midi_data       = backend->write_midi_event(backend, nPortID, ev->timestamp, size);
                        if (midi_data == NULL)
                        {
                            lsp_warn(
                                "Could not write MIDI message of type 0x%02x, size=%d, timestamp=%d",
                                int(ev->type), int(size), int(ev->timestamp));
                            continue;
                        }

                        // Encode MIDI event
                        midi::encode(midi_data, ev);
                    }
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

                AudioBufferPort(const AudioBufferPort &) = delete;
                AudioBufferPort(AudioBufferPort &&) = delete;

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
                float       fUIValue;
                bool        bForce;

            public:
                explicit MeterPort(const meta::port_t *meta, Wrapper *w) : Port(meta, w)
                {
                    fValue      = meta->start;
                    fUIValue    = fValue;
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

                void commit()
                {
                    fUIValue        = fValue;
                    bForce          = pMetadata->flags & meta::F_PEAK;
                }

            public:
                float sync_value()
                {
                    return fUIValue;
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
                    pMesh   = standalone::create_mesh(pMetadata);
                    return (pMesh == NULL) ? STATUS_NO_MEM : STATUS_OK;
                }

                virtual void destroy() override
                {
                    if (pMesh == NULL)
                        return;

                    standalone::destroy_mesh(pMesh);
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
                standalone::path_t  sPath;

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
                    strcpy(pValue->sData, pMetadata->value);
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

    } /* namespace standalone */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_PORTS_H_ */
