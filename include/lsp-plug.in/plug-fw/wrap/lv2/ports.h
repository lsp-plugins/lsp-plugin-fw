/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 20 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/plug-fw/core/AudioBuffer.h>
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/types.h>
#include <lsp-plug.in/stdlib/math.h>


namespace lsp
{
    namespace lv2
    {
        // Specify port classes
        class Port: public plug::IPort
        {
            protected:
                lv2::Extensions        *pExt;
                LV2_URID                urid;
                ssize_t                 nID;
                bool                    bVirtual;

            public:
                explicit Port(const meta::port_t *meta, lv2::Extensions *ext, bool virt): IPort(meta)
                {
                    pExt            =   ext;
                    urid            =   (meta != NULL) ? pExt->map_port(meta->id) : -1;
                    nID             =   -1;
                    bVirtual        =   virt;
                }
                Port(const Port &) = delete;
                Port(Port &&) = delete;

                virtual ~Port() override
                {
                    pExt            =   NULL;
                    urid            =   -1;
                    nID             =   -1;
                }

                Port & operator = (const Port &) = delete;
                Port & operator = (Port &&) = delete;

            public:
                /** Pre-process port state before processor execution
                 * @return true if port value has been externally modified
                 */
                virtual bool pre_process()                  { return false; }

                /** Post-process port state after processor execution
                 */
                virtual void post_process()                 {}

                /** Bind generic port to generic data pointer
                 *
                 * @param data data pointer
                 */
                virtual void bind(void *data)               {}

                /** Save state of the port to LV2 state
                 *
                 */
                virtual void save()                         {}

                /** Restore state of the port from LV2 state
                 *
                 */
                virtual void restore()                      {}

                /** Serialize state of the port to LV2 Atom
                 *
                 */
                virtual void serialize()                    {}

                /** Deserialize state of the port from LV2 Atom
                 * @param flags additional flags
                 * @return true if internal state of the port has changed
                 */
                virtual bool deserialize(const void *data, size_t flags)  { return false; }

                /** Get type of the LV2 port in terms of Atom
                 *
                 * @return type of the LV2 port in terms of Atom
                 */
                virtual LV2_URID get_type_urid()            { return 0;         }

                /** Check that the port is pending for transmission
                 *
                 * @return true if the port is pending for transmission
                 */
                virtual bool tx_pending()                   { return false;     }

                /**
                 * Reset transfer pending
                 */
                virtual void reset_tx_pending()             { }

                /**
                 * Callback: UI has connected to backend
                 */
                virtual void ui_connected()                 { }

            public:
                /** Get the URID of the port in terms of Atom
                 *
                 * @return UIRD of the port
                 */
                inline LV2_URID         get_urid() const    { return urid; }

                /** Get the URI of the port
                 *
                 * @return URI of the port
                 */
                inline const char      *get_uri() const     { return (pExt->unmap_urid(urid)); }

                /** Get port ID
                 *
                 * @return poirt ID
                 */
                inline ssize_t          get_id() const      { return nID;   }

                /** Set port ID
                 *
                 * @param id port ID
                 */
                inline void             set_id(size_t id)   { nID = id;     }

                /**
                 * Check that port is virtual
                 * @return true if port is virtual (non controlled by DAW and stored in PLUGIN STATE)
                 */
                inline bool             is_virtual() const  { return bVirtual; }
        };

        class AudioPort: public Port
        {
            protected:
                float     *pBuffer;
                float     *pData;
                float     *pSanitized;
                bool       bZero;

            public:
                explicit AudioPort(const meta::port_t *meta, lv2::Extensions *ext) : Port(meta, ext, false)
                {
                    pBuffer        = NULL;
                    pData          = NULL;
                    pSanitized     = NULL;
                    bZero          = false;

                    if (meta::is_in_port(pMetadata))
                    {
                        size_t length  = pExt->nMaxBlockLength;
                        pSanitized     = static_cast<float *>(::malloc(sizeof(float) * length));
                        if (pSanitized != NULL)
                            dsp::fill_zero(pSanitized, length);
                        else
                            lsp_warn("Failed to allocate sanitize buffer for port %s", pMetadata->id);
                    }
                }

                AudioPort(const AudioPort &) = delete;
                AudioPort(AudioPort &&) = delete;

                virtual ~AudioPort() override
                {
                    pBuffer    = NULL;
                    pData      = NULL;
                    if (pSanitized != NULL)
                    {
                        ::free(pSanitized);
                        pSanitized = NULL;
                    }
                }

                AudioPort & operator = (const AudioPort &) = delete;
                AudioPort & operator = (AudioPort &&) = delete;

            public:
                virtual void bind(void *data) override
                {
                    pData      = static_cast<float *>(data);
                };

                virtual void *buffer() override { return pBuffer; };

            public:
                // Should be always called at least once after bind() and before process() call
                void sanitize_before(size_t off, size_t samples)
                {
                    pBuffer  = &pData[off];
                    if (pSanitized == NULL)
                        return;

                    // Sanitize plugin's input if possible
                    if (pData != NULL)
                    {
                        dsp::sanitize2(pSanitized, pBuffer, samples);
                        bZero      = false;
                    }
                    else if (!bZero)
                    {
                        // This is optional sidechain port that is connectionOptional?
                        dsp::fill_zero(pSanitized, pExt->nMaxBlockLength);
                        bZero      = true;
                    }
                    pBuffer      = pSanitized;
                }

                // Should be always called at least once after bind() and after process() call
                void sanitize_after(size_t off, size_t samples)
                {
                    // Sanitize plugin's output
                    if ((pBuffer != NULL) && (meta::is_out_port(pMetadata)))
                        dsp::sanitize1(pBuffer, samples);

                    // Clear the buffer pointer
                    pBuffer    = NULL;
                }
        };

        class AudioBufferPort: public Port
        {
            private:
                core::AudioBuffer   sBuffer;

            public:
                explicit AudioBufferPort(const meta::port_t *meta, lv2::Extensions *ext) : Port(meta, ext, false)
                {
                    sBuffer.set_size(ext->nMaxBlockLength);
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
        };

        class InputPort: public Port
        {
            protected:
                const float    *pData;
                float           fValue;
                float           fPrev;

            public:
                explicit InputPort(const meta::port_t *meta, lv2::Extensions *ext, bool virt) : Port(meta, ext, virt)
                {
                    pData       = NULL;
                    fValue      = meta->start;
                    fPrev       = meta->start;
                }

                InputPort(const InputPort &) = delete;
                InputPort(InputPort &&) = delete;

                virtual ~InputPort() override
                {
                    pData       = NULL;
                    fValue      = pMetadata->start;
                    fPrev       = pMetadata->start;
                }

                InputPort & operator = (const InputPort &) = delete;
                InputPort & operator = (InputPort &&) = delete;

            public:
                virtual float value() override
                {
                    return fValue;
                }

                virtual void set_value(float value) override
                {
                    fValue      = value;
                }

                virtual void bind(void *data) override
                {
                    pData = static_cast<const float *>(data);
                };

                virtual bool pre_process() override
                {
                    if ((nID >= 0) && (pData != NULL))
                        fValue      = meta::limit_value(pMetadata, *pData);
//                    lsp_trace("nID=%d, pData=%x, fPrev=%f, fValue=%f", int(nID), pData, fPrev, fValue);

                    const float old = fPrev;
                    fPrev           = fValue;
                    return (old != fPrev); // Value has changed?
                }

                virtual void save() override
                {
                    if (nID >= 0)
                        return;
                    lsp_trace("save port id=%s, urid=%d (%s), value=%f", pMetadata->id, urid, get_uri(), fValue);
                    pExt->store_value(urid, pExt->forge.Float, &fValue, sizeof(float));
                }

                virtual void restore() override
                {
                    if (nID >= 0)
                        return;
                    lsp_trace("restore port id=%s, urid=%d (%s)", pMetadata->id, urid, get_uri());
                    size_t count            = 0;
                    const void *data        = pExt->restore_value(urid, pExt->forge.Float, &count);
                    if ((count == sizeof(float)) && (data != NULL))
                        fValue      = meta::limit_value(pMetadata, *(reinterpret_cast<const float *>(data)));
                }

                virtual bool deserialize(const void *data, size_t flags) override
                {
                    const LV2_Atom_Float *atom = reinterpret_cast<const LV2_Atom_Float *>(data);
                    if (fValue == atom->body)
                        return false;

                    fValue      = atom->body;
                    return true;
                }

                virtual void serialize() override
                {
                    // Serialize and reset pending flag
                    pExt->forge_float(fValue);
                    fPrev       = fValue;
                }

                virtual LV2_URID get_type_urid() override
                {
                    return pExt->forge.Float;
                }
        };

        class BypassPort: public InputPort
        {
            public:
                explicit BypassPort(const meta::port_t *meta, lv2::Extensions *ext) : InputPort(meta, ext, false) { }

                BypassPort(const BypassPort &) = delete;
                BypassPort(BypassPort &&) = delete;
                BypassPort & operator = (const BypassPort &) = delete;
                BypassPort & operator = (BypassPort &&) = delete;

            public:
                virtual float value() override
                {
                    return pMetadata->max - fValue;
                }

                virtual void set_value(float value) override
                {
                    fValue      = pMetadata->max - value;
                }

                virtual void save() override
                {
                    if (nID >= 0)
                        return;
                    float value = pMetadata->max - fValue;
                    lsp_trace("save port id=%s, urid=%d (%s), value=%f", pMetadata->id, urid, get_uri(), value);
                    pExt->store_value(urid, pExt->forge.Float, &value, sizeof(float));
                }

                virtual void restore() override
                {
                    if (nID >= 0)
                        return;
                    lsp_trace("restore port id=%s, urid=%d (%s)", pMetadata->id, urid, get_uri());
                    size_t count            = 0;
                    const void *data        = pExt->restore_value(urid, pExt->forge.Float, &count);
                    if ((count == sizeof(float)) && (data != NULL))
                        fValue      = meta::limit_value(pMetadata, pMetadata->max - *(reinterpret_cast<const float *>(data)));
                }

                virtual bool deserialize(const void *data, size_t flags) override
                {
                    const LV2_Atom_Float *atom = reinterpret_cast<const LV2_Atom_Float *>(data);
                    float v = pMetadata->max - atom->body;
                    if (fValue == v)
                        return false;

                    fValue      = v;
                    return true;
                }
        };

        class OutputPort: public Port
        {
            protected:
                float  *pData;
                float   fPrev;
                float   fValue;

            public:
                explicit OutputPort(const meta::port_t *meta, lv2::Extensions *ext) : Port(meta, ext, false)
                {
                    pData       = NULL;
                    fPrev       = meta->start;
                    fValue      = meta->start;
                }

                OutputPort(const OutputPort &) = delete;
                OutputPort(OutputPort &&) = delete;
                OutputPort & operator = (const OutputPort &) = delete;
                OutputPort & operator = (OutputPort &&) = delete;

                virtual ~OutputPort() override
                {
                    pData = NULL;
                };

            public:
                virtual float value() override
                {
                    return fValue;
                }

                virtual void set_value(float value) override
                {
                    value       = meta::limit_value(pMetadata, value);
                    if (pMetadata->flags & meta::F_PEAK)
                    {
                        if (fabsf(fValue) < fabsf(value))
                            fValue = value;
                    }
                    else
                        fValue = value;
                }

                virtual void bind(void *data) override
                {
                    pData       = static_cast<float *>(data);
                };

                virtual bool pre_process() override
                {
                    if (pMetadata->flags & meta::F_PEAK)
                        fValue      = 0.0f;
                    return false;
                }

                virtual void post_process() override
                {
                    // Store data i the port
                    if (pData != NULL)
                        *pData      = fValue;

                    // Serialize data and reset tx_pending flag
                    fPrev       = fValue;
                }

                virtual bool tx_pending() override
                {
                    if (fValue != fPrev)
                        lsp_trace("pending_value id=%s, prev=%f, value=%f", pMetadata->id, fPrev, fValue);
                    return fValue != fPrev;
                }

                virtual void serialize() override
                {
                    // Serialize and reset pending flag
                    pExt->forge_float(fValue);

                    // Update data according to peak protocol, only for Atom transport ports
                    if ((nID < 0) && (pMetadata->flags & meta::F_PEAK))
                        fValue      = 0.0f;
                }

                virtual LV2_URID get_type_urid() override
                {
                    return pExt->forge.Float;
                }
        };

        class PortGroup: public Port
        {
            protected:
                float                   nCurrRow;
                size_t                  nCols;
                size_t                  nRows;

            public:
                explicit PortGroup(const meta::port_t *meta, lv2::Extensions *ext, bool virt) : Port(meta, ext, virt)
                {
                    nCurrRow            = meta->start;
                    nCols               = port_list_size(meta->members);
                    nRows               = list_size(meta->items);
                }

                PortGroup(const PortGroup &) = delete;
                PortGroup(PortGroup &&) = delete;
                PortGroup & operator = (const PortGroup &) = delete;
                PortGroup & operator = (PortGroup &&) = delete;

                virtual ~PortGroup() override
                {
                    nCurrRow            = 0;
                    nCols               = 0;
                    nRows               = 0;
                }

            public:
                virtual float value() override
                {
                    return nCurrRow;
                }

                virtual void set_value(float value) override
                {
                    nCurrRow            = value;
                }

                virtual void serialize() override
                {
                    // Serialize and reset pending flag
                    pExt->forge_int(nCurrRow);
                }

                virtual bool deserialize(const void *data, size_t flags) override
                {
                    const LV2_Atom_Int *atom = reinterpret_cast<const LV2_Atom_Int *>(data);
                    if ((atom->body >= 0) && (atom->body < int32_t(nRows)) && (nCurrRow != atom->body))
                    {
                        nCurrRow        = atom->body;
                        return true;
                    }

                    return false;
                }

                virtual void save() override
                {
                    if (nID >= 0)
                        return;
                    int32_t value   = nCurrRow;
                    lsp_trace("save port id=%s, urid=%d (%s), value=%d", pMetadata->id, urid, get_uri(), int(value));
                    pExt->store_value(urid, pExt->forge.Int, &value, sizeof(float));
                }

                virtual void restore() override
                {
                    if (nID >= 0)
                        return;
                    lsp_trace("restore port id=%s, urid=%d (%s)", pMetadata->id, urid, get_uri());
                    size_t count            = 0;
                    const int32_t *data     = reinterpret_cast<const int32_t *>(pExt->restore_value(urid, pExt->forge.Int, &count));
                    if ((count != sizeof(int32_t)) || (data == NULL))
                        return;

                    if (((*data) >= 0) && ((*data) < int32_t(nRows)))
                        nCurrRow        = *data;
                }

                virtual LV2_URID get_type_urid() override
                {
                    return pExt->forge.Int;
                }

            public:
                inline size_t rows() const  { return nRows; }
                inline size_t cols() const  { return nCols; }
        };

        class MeshPort: public Port
        {
            protected:
                lv2_mesh_t                 sMesh;

            public:
                explicit MeshPort(const meta::port_t *meta, lv2::Extensions *ext): Port(meta, ext, false)
                {
                    sMesh.init(meta);
                }

                MeshPort(const MeshPort &) = delete;
                MeshPort(MeshPort &&) = delete;
                MeshPort & operator = (const MeshPort &) = delete;
                MeshPort & operator = (MeshPort &&) = delete;

            public:
                virtual LV2_URID get_type_urid() override
                {
                    return pExt->uridMeshType;
                }

                virtual void *buffer() override
                {
                    return sMesh.pMesh;
                }

                virtual bool tx_pending() override
                {
                    plug::mesh_t *mesh = sMesh.pMesh;
                    if (mesh == NULL)
                        return false;

                    // Return true only if mesh contains data
                    return mesh->containsData();
                };

                virtual void serialize() override
                {
                    plug::mesh_t *mesh = sMesh.pMesh;

                    // Forge number of vectors (dimensions)
                    pExt->forge_key(pExt->uridMeshDimensions);
                    pExt->forge_int(mesh->nBuffers);

                    // Forge number of items per vector
                    pExt->forge_key(pExt->uridMeshItems);
                    pExt->forge_int(mesh->nItems);

                    // Forge vectors
                    for (size_t i=0; i < mesh->nBuffers; ++i)
                    {
                        pExt->forge_key(pExt->uridMeshData);
                        pExt->forge_vector(sizeof(float), pExt->forge.Float, mesh->nItems, mesh->pvData[i]);
                    }

                    // Set mesh waiting until next frame is allowed
                    mesh->setWaiting();
                }
        };

        class StreamPort: public Port
        {
            protected:
                plug::stream_t     *pStream;
                uint32_t            nFrameID;
                float              *pData;

            public:
                explicit StreamPort(const meta::port_t *meta, lv2::Extensions *ext): Port(meta, ext, false)
                {
                    pStream     = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                    pData       = reinterpret_cast<float *>(::malloc(sizeof(float) * STREAM_MAX_FRAME_SIZE));
                    nFrameID    = 0;
                }

                StreamPort(const StreamPort &) = delete;
                StreamPort(StreamPort &&) = delete;
                StreamPort & operator = (const StreamPort &) = delete;
                StreamPort & operator = (StreamPort &&) = delete;

                virtual ~StreamPort() override
                {
                    plug::stream_t::destroy(pStream);
                    pStream     = NULL;

                    if (pData != NULL)
                    {
                        ::free(pData);
                        pData       = NULL;
                    }
                }

            public:
                virtual LV2_URID get_type_urid() override
                {
                    return pExt->uridFrameBufferType;
                }

                virtual void *buffer() override
                {
                    return pStream;
                }

                virtual bool tx_pending() override
                {
                    return nFrameID != pStream->frame_id();
                }

                virtual void ui_connected() override
                {
                    // We need to replay buffer contents for the connected client
                    lsp_trace("UI connected event");
                    nFrameID    = pStream->frame_id() - pStream->frames();
                }

                virtual void serialize() override
                {
                    // Serialize not more than number of predefined frames
                    uint32_t frame_id   = nFrameID;
                    uint32_t src_id     = pStream->frame_id();
                    size_t num_frames   = pStream->frames();
                    uint32_t delta      = lsp_min(src_id - frame_id, num_frames);
                    if (delta == 0)
                        return;

                    frame_id            = src_id - delta + 1;
                    delta               = lsp_min(delta, uint32_t(STREAM_BULK_MAX)); // Limit number of frames per message
                    uint32_t last_id    = frame_id + delta;

//                    lsp_trace("id = %s, nFrameID=0x%x num_frames=%d, src=0x%x, first=0x%x, last=0x%x",
//                        pMetadata->id, int(nFrameID), int(num_frames), int(src_id), int(frame_id), int(last_id));

                    // Forge frame buffer parameters
                    size_t nbuffers = pStream->channels();

                    pExt->forge_key(pExt->uridStreamDimensions);
                    pExt->forge_int(nbuffers);

                    // Forge vectors
                    for ( ; frame_id != last_id; ++frame_id)
                    {
                        LV2_Atom_Forge_Frame frame;
                        ssize_t size = pStream->get_frame_size(frame_id);
                        if (size < 0)
                            continue;

//                        lsp_trace("frame id=0x%x, size=%d", int(frame_id), int(size));

                        pExt->forge_key(pExt->uridStreamFrame);
                        pExt->forge_object(&frame, pExt->uridBlank, pExt->uridStreamFrameType);
                        {
                            pExt->forge_key(pExt->uridStreamFrameId);
                            pExt->forge_int(frame_id);

                            pExt->forge_key(pExt->uridStreamFrameSize);
                            pExt->forge_int(size);

                            // Forge vectors
                            for (size_t i=0; i < nbuffers; ++i)
                            {
                                pStream->read_frame(frame_id, i, pData, 0, size);

                                pExt->forge_key(pExt->uridStreamFrameData);
//                                lsp_trace("forge_vector i=%d, nbuffers=%d, size=%d, pData=%p",
//                                    int(i), int(nbuffers), int(size), pData);
                                pExt->forge_vector(sizeof(float), pExt->forge.Float, size, pData);
                            }
                        }
                        pExt->forge_pop(&frame);
                    }

                    // Update current RowID
                    nFrameID    = frame_id - 1;
                }
        };


        class FrameBufferPort: public Port
        {
            protected:
                plug::frame_buffer_t   sFB;
                size_t                 nRowID;

            public:
                explicit FrameBufferPort(const meta::port_t *meta, lv2::Extensions *ext): Port(meta, ext, false)
                {
                    sFB.init(meta->start, meta->step);
                    nRowID = 0;
                }

                FrameBufferPort(const FrameBufferPort &) = delete;
                FrameBufferPort(FrameBufferPort &&) = delete;
                FrameBufferPort & operator = (const FrameBufferPort &) = delete;
                FrameBufferPort & operator = (FrameBufferPort &&) = delete;

                virtual ~FrameBufferPort() override
                {
                    sFB.destroy();
                };

            public:
                virtual LV2_URID get_type_urid() override
                {
                    return pExt->uridFrameBufferType;
                }

                virtual void *buffer() override
                {
                    return &sFB;
                }

                virtual bool tx_pending() override
                {
                    return sFB.next_rowid() != nRowID;
                }

                virtual void ui_connected() override
                {
                    // We need to replay buffer contents for the connected client
                    lsp_trace("UI connected event");
                    nRowID      = sFB.next_rowid() - sFB.rows();
                }

                virtual void serialize() override
                {
                    // Serialize not more than 4 rows
                    size_t delta = sFB.next_rowid() - nRowID;
                    uint32_t first_row = (delta > sFB.rows()) ? sFB.next_rowid() - sFB.rows() : nRowID;
                    if (delta > FRAMEBUFFER_BULK_MAX)
                        delta = FRAMEBUFFER_BULK_MAX;
                    uint32_t last_row = first_row + delta;

                    lsp_trace("id = %s, first=%d, last=%d", pMetadata->id, int(first_row), int(last_row));

                    // Forge frame buffer parameters
                    pExt->forge_key(pExt->uridFrameBufferRows);
                    pExt->forge_int(sFB.rows());
                    pExt->forge_key(pExt->uridFrameBufferCols);
                    pExt->forge_int(sFB.cols());
                    pExt->forge_key(pExt->uridFrameBufferFirstRowID);
                    pExt->forge_int(first_row);
                    pExt->forge_key(pExt->uridFrameBufferLastRowID);
                    pExt->forge_int(last_row);

                    // Forge vectors
                    while (first_row != last_row)
                    {
                        pExt->forge_key(pExt->uridFrameBufferData);
                        pExt->forge_vector(sizeof(float), pExt->forge.Float, sFB.cols(), sFB.get_row(first_row++));
                    }

                    // Update current RowID
                    nRowID = first_row;
                }
        };

        class PathPort: public Port
        {
            protected:
                lv2_path_t          sPath;
                atomic_t            nLastChange;

                inline void set_string(const char *string, size_t len, size_t flags)
                {
                    lsp_trace("submitting path to '%s' (length = %d), flags=0x%x", string, int(len), int(flags));
                    sPath.submit(string, len, flags);
                }

            public:
                explicit PathPort(const meta::port_t *meta, lv2::Extensions *ext): Port(meta, ext, true)
                {
                    sPath.init();
                    nLastChange = sPath.nChanges;
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

                virtual void save() override
                {
                    const char *path = sPath.sPath;

                    lsp_trace("save port id=%s, urid=%d (%s), value=%s", pMetadata->id, urid, get_uri(), path);

                    if (::strlen(path) > 0)
                    {
                        char *mapped = NULL;

                        // We need to translate absolute path to relative path?
                        if ((pExt->mapPath != NULL) && (::strstr(path, LSP_BUILTIN_PREFIX) != path))
                        {
                            mapped = pExt->mapPath->abstract_path(pExt->mapPath->handle, path);
                            if (mapped != NULL)
                            {
                                lsp_trace("mapped path: %s -> %s", path, mapped);
                                path = mapped;
                            }
                        }

                        // Store the actual value of the path
                        pExt->store_value(urid, pExt->uridPathType, path, ::strlen(path) + sizeof(char));

                        if (mapped != NULL)
                            ::free(mapped);
                    }
                }

                virtual void restore() override
                {
                    char tmp_path[PATH_MAX];

                    lsp_trace("restore port id=%s, urid=%d (%s)", pMetadata->id, urid, get_uri());

                    size_t count            = 0;
                    uint32_t type           = -1;
                    const char *path        = reinterpret_cast<const char *>(pExt->retrieve_value(urid, &type, &count));
                    char *mapped            = NULL;

                    if (path != NULL)
                    {
                        if (type == pExt->forge.URID)
                        {
                            const LV2_URID *urid    = reinterpret_cast<const LV2_URID *>(path);
                            path                = pExt->unmap_urid(*urid);
                            count               = ::strnlen(path, PATH_MAX-1);
                        }
                        else if ((type != pExt->uridPathType) && (type != pExt->forge.String))
                        {
                            lsp_trace("Invalid type: %d = %s", int(type), pExt->unmap_urid(type));
                            path                    = NULL;
                        }
                    }

                    if ((path != NULL) && (count > 0))
                    {
                        // Save path as temporary variable
                        ::strncpy(tmp_path, path, count);
                        tmp_path[count] = '\0';
                        path        = tmp_path;

                        // We need to translate relative path to absolute path?
                        io::Path io_path;
                        if ((pExt->mapPath != NULL) && (::strstr(path, LSP_BUILTIN_PREFIX) != path))
                        {
                            mapped = pExt->mapPath->absolute_path(pExt->mapPath->handle, path);
                            if (mapped != NULL)
                            {
                                lsp_trace("unmapped path: %s -> %s", path, mapped);
                                path  = mapped;

                                // Path may be a symlink within a DAW. Make it pointing to the real path
                                if (io_path.set_native(path) == STATUS_OK)
                                {
                                    if (io_path.to_final_path() == STATUS_OK)
                                    {
                                        lsp_trace("final path: %s -> %s", path, io_path.as_native());
                                        path = io_path.as_native();
                                    }
                                }

                                count = ::strnlen(path, PATH_MAX-1);
                            }
                        }

                        // Restore the actual value of the path
                        set_string(path, count, plug::PF_STATE_IMPORT);
                    }
                    else
                        set_string("", 0, plug::PF_STATE_IMPORT);
                    tx_request();

                    if (mapped != NULL)
                        ::free(mapped);
                }

                virtual bool tx_pending() override
                {
                    return sPath.nChanges != nLastChange;
                }

                virtual void serialize() override
                {
                    pExt->forge_path(sPath.path());
                }

                virtual bool deserialize(const void *data, size_t flags) override
                {
                    const LV2_Atom *atom = static_cast<const LV2_Atom *>(data);
                    if (atom->type != pExt->uridPathType)
                        return false;

                    set_string(reinterpret_cast<const char *>(atom + 1), atom->size, flags);
                    return true;
                }

                virtual LV2_URID get_type_urid() override
                {
                    return pExt->uridPathType;
                }

                virtual bool pre_process() override
                {
                    return sPath.pending();
                }

                virtual void reset_tx_pending() override
                {
                    lsp_trace("reset_tx_pending");
                    nLastChange     = sPath.nChanges;
                }

            public:
                void tx_request()
                {
                    lsp_trace("tx_request");
                    atomic_add(&sPath.nChanges, 1);
                }
        };

        class StringPort: public Port
        {
            protected:
                plug::string_t     *pValue;
                uint32_t            nSerial;
                uint32_t            nUIPending;
                uint32_t            nUIActual;

                inline void set_string(const char *string, size_t len, size_t flags)
                {
                    lsp_trace("submitting string to '%s' (length = %d), flags=0x%x", string, int(len), int(flags));
                    pValue->submit(string, len, flags);
                }

            public:
                explicit StringPort(const meta::port_t *meta, lv2::Extensions *ext): Port(meta, ext, true)
                {
                    pValue          = plug::string_t::allocate(size_t(meta->max));
                    nSerial         = pValue->serial();
                    atomic_store(&nUIPending, 0);
                    nUIActual       = 0;
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
                virtual float value() override
                {
                    return (pValue != NULL) ? (pValue->nSerial & 0x3fffff) : 0.0f;
                }

                virtual void *buffer() override
                {
                    return (pValue != NULL) ? pValue->sData : NULL;
                }

                virtual bool tx_pending() override
                {
                    return atomic_load(&nUIPending) != nUIActual;
                }

                virtual void save() override
                {
                    const char *path = (pValue != NULL) ? pValue->sData : pMetadata->value;
                    lsp_trace("save port id=%s, urid=%d (%s), value=%s", pMetadata->id, urid, get_uri(), path);

                    pExt->store_value(urid, pExt->forge.String, path, ::strlen(path) + sizeof(char));
                }

                virtual void restore() override
                {
                    lsp_trace("restore port id=%s, urid=%d (%s)", pMetadata->id, urid, get_uri());

                    size_t count            = 0;
                    uint32_t type           = -1;
                    const char *str         = reinterpret_cast<const char *>(pExt->retrieve_value(urid, &type, &count));
                    if (str == NULL)
                    {
                        str                     = pMetadata->value;
                        count                   = strlen(str);
                    }

                    if (pValue != NULL)
                        pValue->submit(str, count, true);
                }

                virtual void serialize() override
                {
                    const uint32_t serial   = (pValue != NULL) ? pValue->serial() : 0;
                    const char *str         = (pValue != NULL) ? pValue->sData : pMetadata->value;
                    pExt->forge_string(str);
                    nSerial                 = serial;
                }

                virtual bool deserialize(const void *data, size_t flags) override
                {
                    const LV2_Atom *atom = static_cast<const LV2_Atom *>(data);
                    if (atom->type != pExt->forge.String)
                        return false;

                    if (pValue != NULL)
                        pValue->set(reinterpret_cast<const char *>(atom + 1), atom->size, flags & plug::PF_STATE_RESTORE);
                    return true;
                }

                virtual LV2_URID get_type_urid() override
                {
                    return pExt->forge.String;
                }

                virtual bool pre_process() override
                {
                    if (pValue == NULL)
                        return false;
                    if (!pValue->sync())
                        return false;

                    lsp_trace("port id=%s synchronized to value %s", id(), pValue->sData);
                    return true;
                }

                virtual void set_default() override
                {
                    strcpy(pValue->sData, pMetadata->value);
                    atomic_add(&nUIPending, 1);
                }

                virtual void reset_tx_pending() override
                {
                    nUIActual   = atomic_load(&nUIPending);
                }

            public:
                plug::string_t *data()
                {
                    return pValue;
                }
        };

        class MidiPort: public Port
        {
            protected:
                plug::midi_t           sQueue;

            public:
                explicit MidiPort(const meta::port_t *meta, lv2::Extensions *ext): Port(meta, ext, false)
                {
                    sQueue.clear();
                }

                MidiPort(const MidiPort &) = delete;
                MidiPort(MidiPort &&) = delete;
                MidiPort & operator = (const MidiPort &) = delete;
                MidiPort & operator = (MidiPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return &sQueue;
                }
        };

        class OscPort: public Port
        {
            protected:
                core::osc_buffer_t    *pFB;

            public:
                explicit OscPort(const meta::port_t *meta, lv2::Extensions *ext) : Port(meta, ext, false)
                {
                    pFB = core::osc_buffer_t::create(OSC_BUFFER_MAX);
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

    } /* namespace lv2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_PORTS_H_ */
