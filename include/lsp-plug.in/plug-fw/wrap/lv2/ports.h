/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                 virtual ~Port()
                 {
                     pExt            =   NULL;
                     urid            =   -1;
                     nID             =   -1;
                 }

             public:
                 /** Bind generic port to generic data pointer
                  *
                  * @param data data pointer
                  */
                 virtual void bind(void *data)               { };

                 /** Save state of the port to LV2 state
                  *
                  */
                 virtual void save()                         { };

                 /** Restore state of the port from LV2 state
                  *
                  */
                 virtual void restore()                      { };

                 /** Serialize state of the port to LV2 Atom
                  *
                  */
                 virtual void serialize()                    { };

                 /** Deserialize state of the port from LV2 Atom
                  * @param flags additional flags
                  * @return true if internal state of the port has changed
                  */
                 virtual bool deserialize(const void *data, size_t flags)  { return false; };

                 /** Get type of the LV2 port in terms of Atom
                  *
                  * @return type of the LV2 port in terms of Atom
                  */
                 virtual LV2_URID get_type_urid()            { return 0;         };

                 /** Check that the port is pending for transmission
                  *
                  * @return true if the port is pending for transmission
                  */
                 virtual bool tx_pending()                   { return false;     };

                 /**
                  * Callback: UI has connected to backend
                  */
                 virtual void ui_connected()                 { };

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
                 inline bool            is_virtual() const  { return bVirtual; }
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

                 virtual ~PortGroup()
                 {
                     nCurrRow            = 0;
                     nCols               = 0;
                     nRows               = 0;
                 }

             public:
                 virtual float value()
                 {
                     return nCurrRow;
                 }

                 virtual void set_value(float value)
                 {
                     nCurrRow            = value;
                 }

                 virtual void serialize()
                 {
                     // Serialize and reset pending flag
                     pExt->forge_int(nCurrRow);
                 }

                 virtual bool deserialize(const void *data, size_t flags)
                 {
                     const LV2_Atom_Int *atom = reinterpret_cast<const LV2_Atom_Int *>(data);
                     if ((atom->body >= 0) && (atom->body < int32_t(nRows)) && (nCurrRow != atom->body))
                     {
                         nCurrRow        = atom->body;
                         return true;
                     }

                     return false;
                 }

                 virtual void save()
                 {
                     if (nID >= 0)
                         return;
                     int32_t value   = nCurrRow;
                     lsp_trace("save port id=%s, urid=%d (%s), value=%d", pMetadata->id, urid, get_uri(), int(value));
                     pExt->store_value(urid, pExt->forge.Int, &value, sizeof(float));
                 }

                 virtual void restore()
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

                 virtual LV2_URID get_type_urid()
                 {
                     return pExt->forge.Int;
                 }

             public:
                 inline size_t rows() const  { return nRows; }
                 inline size_t cols() const  { return nCols; }
         };

         class AudioPort: public Port
         {
             protected:
                 float      *pBuffer;
                 float      *pData;
                 float      *pSanitized;

             public:
                 explicit AudioPort(const meta::port_t *meta, lv2::Extensions *ext) : Port(meta, ext, false)
                 {
                     pBuffer        = NULL;
                     pData          = NULL;
                     pSanitized     = NULL;

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

                 virtual ~AudioPort()
                 {
                     pBuffer    = NULL;
                     pData      = NULL;
                     if (pSanitized != NULL)
                     {
                         ::free(pSanitized);
                         pSanitized = NULL;
                     }
                 };

                 virtual void bind(void *data)
                 {
                     pData      = static_cast<float *>(data);
                 };

                 virtual void *buffer() { return pBuffer; };

                 // Should be always called at least once after bind() and before process() call
                 void sanitize_before(size_t off, size_t samples)
                 {
                     pBuffer  = &pData[off];

                     // Sanitize plugin's input if possible
                     if (pSanitized != NULL)
                     {
                         dsp::sanitize2(pSanitized, pBuffer, samples);
                         pBuffer      = pSanitized;
                     }
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

                 virtual ~InputPort()
                 {
                     pData       = NULL;
                     fValue      = pMetadata->start;
                     fPrev       = pMetadata->start;
                 }

             public:
                 virtual float value()
                 {
                     return fValue;
                 }

                 virtual void set_value(float value)
                 {
                     fValue      = value;
                 }

                 virtual void bind(void *data)
                 {
                     pData = static_cast<const float *>(data);
                 };

                 virtual bool pre_process(size_t samples)
                 {
                     if ((nID >= 0) && (pData != NULL))
                         fValue      = meta::limit_value(pMetadata, *pData);
//                     lsp_trace("nID=%d, pData=%x, fPrev=%f, fValue=%f", int(nID), pData, fPrev, fValue);

                     const float old = fPrev;
                     fPrev           = fValue;
                     return (old != fPrev); // Value has changed?
                 }

                 virtual void save()
                 {
                     if (nID >= 0)
                         return;
                     lsp_trace("save port id=%s, urid=%d (%s), value=%f", pMetadata->id, urid, get_uri(), fValue);
                     pExt->store_value(urid, pExt->forge.Float, &fValue, sizeof(float));
                 }

                 virtual void restore()
                 {
                     if (nID >= 0)
                         return;
                     lsp_trace("restore port id=%s, urid=%d (%s)", pMetadata->id, urid, get_uri());
                     size_t count            = 0;
                     const void *data        = pExt->restore_value(urid, pExt->forge.Float, &count);
                     if ((count == sizeof(float)) && (data != NULL))
                         fValue      = meta::limit_value(pMetadata, *(reinterpret_cast<const float *>(data)));
                 }

                 virtual bool deserialize(const void *data, size_t flags)
                 {
                     const LV2_Atom_Float *atom = reinterpret_cast<const LV2_Atom_Float *>(data);
                     if (fValue == atom->body)
                         return false;

                     fValue      = atom->body;
                     return true;
                 }

                 virtual void serialize()
                 {
                     // Serialize and reset pending flag
                     pExt->forge_float(fValue);
                     fPrev       = fValue;
                 }

                 virtual LV2_URID get_type_urid()
                 {
                     return pExt->forge.Float;
                 }
         };

         class BypassPort: public InputPort
         {
             public:
                 explicit BypassPort(const meta::port_t *meta, lv2::Extensions *ext) : InputPort(meta, ext, false) { }

                 virtual ~BypassPort() {}

             public:
                 virtual float value()
                 {
                     return pMetadata->max - fValue;
                 }

                 virtual void set_value(float value)
                 {
                     fValue      = pMetadata->max - value;
                 }

                 virtual void save()
                 {
                     if (nID >= 0)
                         return;
                     float value = pMetadata->max - fValue;
                     lsp_trace("save port id=%s, urid=%d (%s), value=%f", pMetadata->id, urid, get_uri(), value);
                     pExt->store_value(urid, pExt->forge.Float, &value, sizeof(float));
                 }

                 virtual void restore()
                 {
                     if (nID >= 0)
                         return;
                     lsp_trace("restore port id=%s, urid=%d (%s)", pMetadata->id, urid, get_uri());
                     size_t count            = 0;
                     const void *data        = pExt->restore_value(urid, pExt->forge.Float, &count);
                     if ((count == sizeof(float)) && (data != NULL))
                         fValue      = meta::limit_value(pMetadata, pMetadata->max - *(reinterpret_cast<const float *>(data)));
                 }

                 virtual bool deserialize(const void *data, size_t flags)
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

                 virtual ~OutputPort()
                 {
                     pData = NULL;
                 };

             public:
                 virtual float value()
                 {
                     return fValue;
                 }

                 virtual void set_value(float value)
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

                 virtual void bind(void *data)
                 {
                     pData       = static_cast<float *>(data);
                 };

                 virtual bool pre_process(size_t samples)
                 {
                     if (pMetadata->flags & meta::F_PEAK)
                         fValue      = 0.0f;
                     return false;
                 }

                 virtual void post_process(size_t samples)
                 {
                     // Store data i the port
                     if (pData != NULL)
                         *pData      = fValue;

                     // Serialize data and reset tx_pending flag
                     fPrev       = fValue;
                 }

                 virtual bool tx_pending()
                 {
                     if (fValue != fPrev)
                         lsp_trace("pending_value id=%s, prev=%f, value=%f", pMetadata->id, fPrev, fValue);
                     return fValue != fPrev;
                 }

                 virtual void serialize()
                 {
                     // Serialize and reset pending flag
                     pExt->forge_float(fValue);

                     // Update data according to peak protocol, only for Atom transport ports
                     if ((nID < 0) && (pMetadata->flags & meta::F_PEAK))
                         fValue      = 0.0f;
                 }

                 virtual LV2_URID get_type_urid()
                 {
                     return pExt->forge.Float;
                 }
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

                 virtual ~MeshPort()
                 {
                 };

             public:
                 virtual LV2_URID get_type_urid()        { return pExt->uridMeshType; };

                 virtual void *buffer()
                 {
                     return sMesh.pMesh;
                 }

                 virtual bool tx_pending()
                 {
                     plug::mesh_t *mesh = sMesh.pMesh;
                     if (mesh == NULL)
                         return false;

                     // Return true only if mesh contains data
                     return mesh->containsData();
                 };

                 virtual void serialize()
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

                 virtual ~StreamPort()
                 {
                     plug::stream_t::destroy(pStream);
                     pStream     = NULL;

                     if (pData != NULL)
                     {
                         ::free(pData);
                         pData       = NULL;
                     }
                 };

             public:
                 virtual LV2_URID get_type_urid()        { return pExt->uridFrameBufferType; };

                 virtual void *buffer()
                 {
                     return pStream;
                 }

                 virtual bool tx_pending()
                 {
                     return nFrameID != pStream->frame_id();
                 }

                 virtual void ui_connected()
                 {
                     // We need to replay buffer contents for the connected client
                     lsp_trace("UI connected event");
                     nFrameID    = pStream->frame_id() - pStream->frames();
                 }

                 virtual void serialize()
                 {
                     // Serialize not more than number of predefined frames
                     uint32_t frame_id  = nFrameID;
                     uint32_t src_id    = pStream->frame_id();
                     uint32_t delta     = src_id - nFrameID;
                     if (delta == 0)
                         return;
                     size_t num_frames  = pStream->frames();
                     uint32_t last_id   = src_id + 1;
                     if (delta > num_frames)
                     {
                         delta              = num_frames;
                         frame_id           = last_id - num_frames;
                     }
                     if (delta > STREAM_BULK_MAX)
                         last_id            = frame_id + STREAM_BULK_MAX;

//                     lsp_trace("id = %s, nFrameID=0x%x num_frames=%d, src=0x%x, first=0x%x, last=0x%x",
//                         pMetadata->id, int(nFrameID), int(num_frames), int(src_id), int(frame_id), int(last_id));

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

//                         lsp_trace("frame id=0x%x, size=%d", int(frame_id), int(size));

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
//                                 lsp_trace("forge_vector i=%d, nbuffers=%d, size=%d, pData=%p",
//                                     int(i), int(nbuffers), int(size), pData);
                                 pExt->forge_vector(sizeof(float), pExt->forge.Float, size, pData);
                             }
                         }
                         pExt->forge_pop(&frame);
                     }

                     // Update current RowID
                     nFrameID    = frame_id;
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

                 virtual ~FrameBufferPort()
                 {
                     sFB.destroy();
                 };

             public:
                 virtual LV2_URID get_type_urid()        { return pExt->uridFrameBufferType; };

                 virtual void *buffer()
                 {
                     return &sFB;
                 }

                 virtual bool tx_pending()
                 {
                     return sFB.next_rowid() != nRowID;
                 }

                 virtual void ui_connected()
                 {
                     // We need to replay buffer contents for the connected client
                     lsp_trace("UI connected event");
                     nRowID      = sFB.next_rowid() - sFB.rows();
                 }

                 virtual void serialize()
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

             public:
                 virtual void *buffer()
                 {
                     return static_cast<plug::path_t *>(&sPath);
                 }

                 virtual void save()
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

                 void tx_request()
                 {
                     lsp_trace("tx_request");
                     atomic_add(&sPath.nChanges, 1);
                 }

                 virtual void restore()
                 {
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
                         char tmp_path[PATH_MAX];
                         ::strncpy(tmp_path, path, count);
                         tmp_path[count] = '\0';
                         path        = tmp_path;

                         // We need to translate relative path to absolute path?
                         if ((pExt->mapPath != NULL) && (::strstr(path, LSP_BUILTIN_PREFIX) != path))
                         {
                             mapped = pExt->mapPath->absolute_path(pExt->mapPath->handle, path);
                             if (mapped != NULL)
                             {
                                 lsp_trace("unmapped path: %s -> %s", path, mapped);
                                 path  = mapped;
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

                 virtual bool tx_pending()
                 {
                     return sPath.nChanges != nLastChange;
                 }

                 void reset_tx_pending()
                 {
                     lsp_trace("reset_tx_pending");
                     nLastChange     = sPath.nChanges;
                 }

                 virtual void serialize()
                 {
                     pExt->forge_path(sPath.path());
                     reset_tx_pending();
                 }

                 virtual bool deserialize(const void *data, size_t flags)
                 {
                     const LV2_Atom *atom = static_cast<const LV2_Atom *>(data);
                     if (atom->type != pExt->uridPathType)
                         return false;

                     set_string(reinterpret_cast<const char *>(atom + 1), atom->size, flags);
                     return true;
                 }

                 virtual LV2_URID get_type_urid()    { return pExt->uridPathType; }

                 virtual bool pre_process(size_t samples)
                 {
                     return sPath.pending();
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

             public:
                 virtual void *buffer()
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
                     pFB = core::osc_buffer_t::create(OSC_BUFFER_MAX);
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

    } /* namespace lv2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_PORTS_H_ */
