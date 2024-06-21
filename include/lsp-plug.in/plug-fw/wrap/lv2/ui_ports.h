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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_UI_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_UI_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/ports.h>

namespace lsp
{
    namespace lv2
    {
        // Specify port classes
        class UIPort: public ui::IPort, public lv2::Serializable
        {
            protected:
                ssize_t                 nID;

            public:
                explicit UIPort(const meta::port_t *meta, lv2::Extensions *ext) : ui::IPort(meta), lv2::Serializable(ext)
                {
                    nID         = -1;
                    urid        = (meta != NULL) ? pExt->map_port(meta->id) : -1;
                }
                UIPort(const UIPort &) = delete;
                UIPort(UIPort &&) = delete;
                UIPort & operator = (const UIPort &) = delete;
                UIPort & operator = (UIPort &&) = delete;

            public:
                virtual void notify(const void *buffer, size_t protocol, size_t size)
                {
                }

                inline const char      *get_uri() const         { return (pExt->unmap_urid(urid)); };
                virtual LV2_URID        get_type_urid() const   { return 0; };
                inline void             set_id(ssize_t id)      { nID = id; };
                inline ssize_t          get_id() const          { return nID; };
                virtual bool            sync()                  { return false; };
        };

        class UIPortGroup: public UIPort
        {
            protected:
                size_t          nRows;
                size_t          nCols;
                size_t          nCurrRow;
                lv2::Port      *pPort;

            public:
                UIPortGroup(const meta::port_t *meta, lv2::Extensions *ext, lv2::Port *port) : UIPort(meta, ext)
                {
                    nCurrRow            = meta->start;
                    nRows               = list_size(meta->items);
                    nCols               = port_list_size(meta->members);
                    pPort               = port;

                    if (port != NULL)
                    {
                        lsp_trace("Connected direct group port id=%s", port->metadata()->id);
                        nCurrRow            = port->value();
                    }
                }

                UIPortGroup(const UIPortGroup &) = delete;
                UIPortGroup(UIPortGroup &&) = delete;
                UIPortGroup & operator = (const UIPortGroup &) = delete;
                UIPortGroup & operator = (UIPortGroup &&) = delete;

            public:
                virtual float value() override
                {
                    return nCurrRow;
                }

                virtual void set_value(float value) override
                {
                    size_t new_value = meta::limit_value(pMetadata, value);
                    if ((new_value >= 0) && (new_value < nRows) && (new_value != nCurrRow))
                    {
                        nCurrRow        = new_value;
                        lsp_trace("writing patch event id=%s, value=%d", pMetadata->id, int(new_value));
                        pExt->ui_write_patch(this);
                    }
                }

                virtual void serialize() override
                {
                    // Serialize and reset pending flag
                    pExt->forge_int(nCurrRow);
                }

                virtual void deserialize(const void *data) override
                {
                    const LV2_Atom_Int *atom = reinterpret_cast<const LV2_Atom_Int *>(data);
                    if ((atom->body >= 0) && (atom->body < int32_t(nRows)))
                        nCurrRow        = atom->body;
                }

                virtual LV2_URID get_type_urid() const override
                {
                    return pExt->forge.Int;
                }

            public:
                inline size_t rows() const  { return nRows; }
                inline size_t cols() const  { return nCols; }
        };

        class UIFloatPort: public UIPort
        {
            protected:
                float           fValue;
                bool            bForce;
                lv2::Port      *pPort;

            public:
                explicit UIFloatPort(const meta::port_t *meta, lv2::Extensions *ext, lv2::Port *port) :
                    UIPort(meta, ext)
                {
                    fValue      = meta->start;
                    pPort       = port;
                    if (port != NULL)
                    {
                        lsp_trace("Connected direct float port id=%s", port->metadata()->id);
                        fValue      = port->value();
                    }
                    bForce      = port != NULL;
                }

                UIFloatPort(const UIFloatPort &) = delete;
                UIFloatPort(UIFloatPort &&) = delete;

                virtual ~UIFloatPort() override
                {
                    fValue  =   pMetadata->start;
                }

                UIFloatPort & operator = (const UIFloatPort &) = delete;
                UIFloatPort & operator = (UIFloatPort &&) = delete;

            public:
                virtual float value() override
                {
                    return fValue;
                }

                virtual void set_value(float value) override
                {
                    fValue      = meta::limit_value(pMetadata, value);
                    if (nID >= 0)
                    {
                        // Use standard mechanism to access port
                        //lsp_trace("write(%d, %d, %d, %f)", int(nID), int(sizeof(float)), int(0), fValue);
                        pExt->write_data(nID, sizeof(float), 0, &fValue);
                    }
                    else
                    {
                        lsp_trace("writing patch event id=%s, value=%f", pMetadata->id, fValue);
                        pExt->ui_write_patch(this);
                    }
                }

                virtual LV2_URID        get_type_urid() const override
                {
                    return pExt->forge.Float;
                }

                virtual void serialize() override
                {
                    pExt->forge_float(fValue);
                };

                virtual void deserialize(const void *data) override
                {
                    const LV2_Atom_Float *atom = reinterpret_cast<const LV2_Atom_Float *>(data);
                    fValue      = meta::limit_value(pMetadata, atom->body);
                }

                virtual void notify(const void *buffer, size_t protocol, size_t size) override
                {
                    if (size == sizeof(float))
                        fValue = meta::limit_value(pMetadata, *(reinterpret_cast<const float *>(buffer)));
                }

                virtual bool sync() override
                {
                    if ((pPort == NULL) || (nID >= 0))
                        return false;

                    float old   = fValue;
                    fValue      = meta::limit_value(pMetadata, pPort->value());
                    bool synced = (fValue != old) || bForce;
                    bForce      = false;

                #ifdef LSP_TRACE
                    if (synced)
                        lsp_trace("Directly received float port id=%s, value=%f",
                            pPort->metadata()->id, fValue);
                #endif
                    return synced;
                }
        };

        class UIBypassPort: public UIFloatPort
        {
            public:
                explicit UIBypassPort(const meta::port_t *meta, lv2::Extensions *ext, lv2::Port *port) :
                    UIFloatPort(meta, ext, port)
                {
                }

                UIBypassPort(const UIBypassPort &) = delete;
                UIBypassPort(UIBypassPort &&) = delete;
                UIBypassPort & operator = (const UIBypassPort &) = delete;
                UIBypassPort & operator = (UIBypassPort &&) = delete;

            public:
                virtual void set_value(float value) override
                {
                    fValue      = meta::limit_value(pMetadata, value);
                    if (nID >= 0)
                    {
                        // Use standard mechanism to access port
                        float value = pMetadata->max - fValue;
                        lsp_trace("write(%d, %d, %d, %f)", int(nID), int(sizeof(float)), int(0), value);
                        pExt->write_data(nID, sizeof(float), 0, &value);
                    }
                    else
                    {
                        lsp_trace("writing patch event id=%s, value=%f", pMetadata->id, fValue);
                        pExt->ui_write_patch(this);
                    }
                }

                virtual void serialize() override
                {
                    pExt->forge_float(pMetadata->max - fValue);
                };

                virtual void deserialize(const void *data) override
                {
                    const LV2_Atom_Float *atom = reinterpret_cast<const LV2_Atom_Float *>(data);
                    fValue      = meta::limit_value(pMetadata, pMetadata->max - atom->body);
                }

                virtual void notify(const void *buffer, size_t protocol, size_t size) override
                {
                    if (size == sizeof(float))
                        fValue = meta::limit_value(pMetadata, pMetadata->max - *(reinterpret_cast<const float *>(buffer)));
                    lsp_trace("set value of port %s = %f", pMetadata->id, fValue);
                }
        };

        class UIPeakPort: public UIFloatPort
        {
            public:
                explicit UIPeakPort(const meta::port_t *meta, lv2::Extensions *ext, lv2::Port *port) :
                    UIFloatPort(meta, ext, port) {}

                UIPeakPort(const UIPeakPort &) = delete;
                UIPeakPort(UIPeakPort &&) = delete;
                UIPeakPort & operator = (const UIPeakPort &) = delete;
                UIPeakPort & operator = (UIPeakPort &&) = delete;

            public:
                virtual void notify(const void *buffer, size_t protocol, size_t size) override
                {
                    if (size == sizeof(LV2UI_Peak_Data))
                    {
                        fValue = meta::limit_value(pMetadata, (reinterpret_cast<const LV2UI_Peak_Data *>(buffer))->peak);
                        return;
                    }
                    UIFloatPort::notify(buffer, protocol, size);
    //                lsp_trace("id=%s, value=%f", pMetadata->id, fValue);
                }
        };

        class UIMeshPort: public UIPort
        {
            protected:
                lv2_mesh_t              sMesh;
                bool                    bParsed;
                lv2::MeshPort          *pPort;

            public:
                explicit UIMeshPort(const meta::port_t *meta, lv2::Extensions *ext, lv2::Port *xport) : UIPort(meta, ext)
                {
                    sMesh.init(meta);
                    bParsed     = false;
                    pPort       = NULL;

                    lsp_trace("id=%s, ext=%p, xport=%p", meta->id, ext, xport);

                    // Try to perform direct access to the port using LV2:Instance interface
                    const meta::port_t *xmeta = (xport != NULL) ? xport->metadata() : NULL;
                    if (meta::is_mesh_port(xmeta))
                    {
                        pPort                   = static_cast<lv2::MeshPort *>(xport);
                        plug::mesh_t *mesh      = static_cast<plug::mesh_t *>(pPort->buffer());
                        mesh->cleanup();  // Mark mesh as empty to force the DSP to write data to mesh
                        lsp_trace("Connected direct mesh port id=%s", xmeta->id);
                    }
                }

                UIMeshPort(const UIMeshPort &) = delete;
                UIMeshPort(UIMeshPort &&) = delete;
                UIMeshPort & operator = (const UIMeshPort &) = delete;
                UIMeshPort & operator = (UIMeshPort &&) = delete;

            public:
                virtual LV2_URID        get_type_urid() const override
                {
                    return pExt->uridMeshType;
                }

                virtual void deserialize(const void *data) override
                {
                    const LV2_Atom_Object* obj = reinterpret_cast<const LV2_Atom_Object *>(data);

                    // Parse atom body
                    bParsed     = false;
                    LV2_Atom_Property_Body *body    = lv2_atom_object_begin(&obj->body);

                    // Get number of vectors (dimensions)
                    if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                        return;
    //                lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
    //                lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    if ((body->key != pExt->uridMeshDimensions) || (body->value.type != pExt->forge.Int))
                        return;
                    ssize_t dimensions = (reinterpret_cast<const LV2_Atom_Int *>(& body->value))->body;
    //                lsp_trace("dimensions=%d", int(dimensions));
                    if (dimensions > ssize_t(sMesh.nBuffers))
                        return;
                    sMesh.pMesh->nBuffers   = dimensions;

                    // Get size of each vector
                    body = lv2_atom_object_next(body);
                    if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                        return;

    //                lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
    //                lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    if ((body->key != pExt->uridMeshItems) || (body->value.type != pExt->forge.Int))
                        return;
                    ssize_t vector_size = (reinterpret_cast<const LV2_Atom_Int *>(& body->value))->body;
    //                lsp_trace("vector size=%d", int(vector_size));
                    if ((vector_size < 0) || (vector_size > ssize_t(sMesh.nMaxItems)))
                        return;
                    sMesh.pMesh->nItems     = vector_size;

                    // Now parse each vector
                    for (ssize_t i=0; i < dimensions; ++i)
                    {
                        // Read vector as array of floats
                        body = lv2_atom_object_next(body);
                        if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                            return;

    //                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
    //                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));
    //                    lsp_trace("sMesh.pUrids[%d] (%d) = %s", int(i), int(sMesh.pUrids[i]), pExt->unmap_urid(sMesh.pUrids[i]));

                        if ((body->key != pExt->uridMeshData) || (body->value.type != pExt->forge.Vector))
                            return;
                        const LV2_Atom_Vector *v = reinterpret_cast<const LV2_Atom_Vector *>(&body->value);

    //                    lsp_trace("v->body.child_type (%d) = %s", int(v->body.child_type), pExt->unmap_urid(v->body.child_type));
    //                    lsp_trace("v->body.child_size = %d", int(v->body.child_size));
                        if ((v->body.child_size != sizeof(float)) || (v->body.child_type != pExt->forge.Float))
                            return;
                        ssize_t v_items     = (v->atom.size - sizeof(LV2_Atom_Vector_Body)) / sizeof(float);
    //                    lsp_trace("vector items=%d", int(v_items));
                        if (v_items != vector_size)
                            return;

                        // Now we can surely copy data
                        dsp::copy_saturated(sMesh.pMesh->pvData[i], reinterpret_cast<const float *>(v + 1), v_items);
    //                    memcpy(sMesh.pMesh->pvData[i], reinterpret_cast<const float *>(v + 1), v_items * sizeof(float));
                    }

                    // Update mesh parameters
    //                lsp_trace("Mesh was successful parsed dimensions=%d, items=%d", int(sMesh.pMesh->nBuffers), int(sMesh.pMesh->nItems));
                    bParsed                 = true;
                }

                virtual void *buffer() override
                {
                    if (!bParsed)
                        return NULL;

                    return sMesh.pMesh;
                }

                virtual bool sync() override
                {
                    if (pPort == NULL)
                        return false;

                    plug::mesh_t *mesh = reinterpret_cast<plug::mesh_t *>(pPort->buffer());
                    if ((mesh == NULL) || (!mesh->containsData()))
                        return false;

                    // Copy mesh data
                    for (size_t i=0; i < mesh->nBuffers; ++i)
                        dsp::copy_saturated(sMesh.pMesh->pvData[i], mesh->pvData[i], mesh->nItems);
                    sMesh.pMesh->data(mesh->nBuffers, mesh->nItems);
    //                lsp_trace("Directly received mesh port id=%s, buffers=%d, items=%d",
    //                        pPort->metadata()->id, int(sMesh.pMesh->nBuffers), int(sMesh.pMesh->nItems));

                    // Clean source mesh
                    mesh->cleanup();
                    bParsed = true;
                    return sMesh.pMesh->containsData();
                }
        };

        class UIStreamPort: public UIPort
        {
            protected:
                plug::stream_t         *pStream;
                lv2::StreamPort        *pPort;

            public:
                explicit UIStreamPort(const meta::port_t *meta, lv2::Extensions *ext, lv2::Port *xport) : UIPort(meta, ext)
                {
                    pStream         = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                    pPort           = NULL;

                    // Try to perform direct access to the port using LV2:Instance interface
                    const meta::port_t *xmeta = (xport != NULL) ? xport->metadata() : NULL;
                    if (meta::is_stream_port(xmeta))
                    {
                        pPort               = static_cast<lv2::StreamPort *>(xport);
                        lsp_trace("Connected direct stream port id=%s", xmeta->id);
                    }
                }

                UIStreamPort(const UIStreamPort &) = delete;
                UIStreamPort(UIStreamPort &&) = delete;

                virtual ~UIStreamPort() override
                {
                    plug::stream_t::destroy(pStream);
                    pStream         = NULL;
                };

                UIStreamPort & operator = (const UIStreamPort &) = delete;
                UIStreamPort & operator = (UIStreamPort &&) = delete;

            protected:
                void deserialize_frame(LV2_Atom_Object *frame)
                {
                    // Parse atom body
                    LV2_Atom_Property_Body *body    = lv2_atom_object_begin(&frame->body);

                    // Get frame identifier
                    if (lv2_atom_object_is_end(&frame->body, frame->atom.size, body))
                        return;
//                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
//                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));
                    if ((body->key != pExt->uridStreamFrameId) || (body->value.type != pExt->forge.Int))
                        return;
                    uint32_t frame_id = (reinterpret_cast<const LV2_Atom_Int *>(&body->value))->body;
//                    lsp_trace("frame_id = %d", int(frame_id));

                    // Get frame size
                    body = lv2_atom_object_next(body);
                    if (lv2_atom_object_is_end(&frame->body, frame->atom.size, body))
                        return;
//                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
//                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));
                    if ((body->key != pExt->uridStreamFrameSize) || (body->value.type != pExt->forge.Int))
                        return;
                    ssize_t frame_size  = (reinterpret_cast<const LV2_Atom_Int *>(&body->value))->body;
//                    lsp_trace("frame_size = %d", int(frame_size));

                    frame_size          = lsp_min(frame_size, STREAM_MAX_FRAME_SIZE);
//                    lsp_trace("act_frame_size = %d", int(frame_size));

                    // If frame differs to the current one - clear the stream
                    uint32_t prev_id    = frame_id - 1;
//                    lsp_trace("prev_id = %d, frame_id=%d", int(prev_id), int(pStream->frame_id()));
                    if (pStream->frame_id() != prev_id)
                    {
                        pStream->clear(prev_id);
//                        lsp_trace("cleared stream to frame %d", int(pStream->frame_id()));
                    }

                    // Now we are able to commit the frame
                    frame_size          = pStream->add_frame(frame_size);
                    for (size_t i=0, n=pStream->channels(); i<n; ++i)
                    {
                        // Read vector as array of floats
                        body = lv2_atom_object_next(body);
                        if (lv2_atom_object_is_end(&frame->body, frame->atom.size, body))
                            break;
//                        lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
//                        lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                        if ((body->key != pExt->uridStreamFrameData) || (body->value.type != pExt->forge.Vector))
                            return;
                        const LV2_Atom_Vector *v = reinterpret_cast<const LV2_Atom_Vector *>(&body->value);
//                        lsp_trace("body->child_size = %d, body->child_type (%d) = %s", int(v->body.child_size), int(v->body.child_type), pExt->unmap_urid(v->body.child_type));
                        if ((v->body.child_size != sizeof(float)) || (v->body.child_type != pExt->forge.Float))
                            return;

                        ssize_t v_items     = lsp_min(frame_size, ssize_t((v->atom.size - sizeof(LV2_Atom_Vector_Body)) / sizeof(float)));
//                        lsp_trace("channel = %d, floats = %d", int(i), int(v_items));
                        pStream->write_frame(i, reinterpret_cast<const float *>(v + 1), 0, v_items);
                    }

                    // Commit the frame
                    pStream->commit_frame();
                }

            public:
                virtual LV2_URID        get_type_urid() const override
                {
                    return pExt->uridStreamType;
                }

                virtual void deserialize(const void *data) override
                {
                    const LV2_Atom_Object* obj = reinterpret_cast<const LV2_Atom_Object *>(data);
//                    lsp_trace("id = %s", pMetadata->id);

                    // Parse atom body
                    LV2_Atom_Property_Body *body    = lv2_atom_object_begin(&obj->body);

                    // Get number of vectors (dimensions)
                    if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                        return;
//                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
//                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    if ((body->key != pExt->uridStreamDimensions) || (body->value.type != pExt->forge.Int))
                        return;
                    uint32_t dimensions = (reinterpret_cast<const LV2_Atom_Int *>(&body->value))->body;
                    if (dimensions != pStream->channels())
                        return;

                    while (true)
                    {
                        // Get size of each vector
                        body = lv2_atom_object_next(body);
                        if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                            break;

//                        lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
//                        lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                        if ((body->key != pExt->uridStreamFrame) || (body->value.type != pExt->forge.Object))
                            continue;

                        LV2_Atom_Object *fobj       = (reinterpret_cast<LV2_Atom_Object *>(& body->value));
                        if ((fobj->body.id != pExt->uridBlank) || (fobj->body.otype != pExt->uridStreamFrameType))
                            continue;

                        // Read frame data
                        deserialize_frame(fobj);
                    }
                }

                virtual void *buffer() override
                {
                    return pStream;
                }

                virtual bool sync() override
                {
                    // Check if there is data for syncing
                    if (pPort == NULL)
                        return false;

                    plug::stream_t *s = static_cast<plug::stream_t *>(pPort->buffer());
                    return (s != NULL) ? pStream->sync(s) : false;
                }
        };

        class UIFrameBufferPort: public UIPort
        {
            protected:
                plug::frame_buffer_t    sFB;
                lv2::FrameBufferPort   *pPort;

            public:
                explicit UIFrameBufferPort(const meta::port_t *meta, lv2::Extensions *ext, lv2::Port *xport):
                    UIPort(meta, ext)
                {
                    sFB.init(meta->start, meta->step);
                    pPort       = NULL;

                    lsp_trace("id=%s, ext=%p, xport=%p", meta->id, ext, xport);

                    // Try to perform direct access to the port using LV2:Instance interface
                    const meta::port_t *xmeta = (xport != NULL) ? xport->metadata() : NULL;
                    if (meta::is_framebuffer_port(xmeta))
                    {
                        pPort               = static_cast<lv2::FrameBufferPort *>(xport);
                        lsp_trace("Connected direct framebuffer port id=%s", xmeta->id);
                    }
                }

                UIFrameBufferPort(const UIFrameBufferPort &) = delete;
                UIFrameBufferPort(UIFrameBufferPort &&) = delete;
                UIFrameBufferPort & operator = (const UIFrameBufferPort &) = delete;
                UIFrameBufferPort & operator = (UIFrameBufferPort &&) = delete;

            public:
                virtual LV2_URID        get_type_urid() const override
                {
                    return pExt->uridMeshType;
                }

                virtual void deserialize(const void *data) override
                {
                    const LV2_Atom_Object* obj = reinterpret_cast<const LV2_Atom_Object *>(data);

    //                lsp_trace("id = %s", pMetadata->id);

                    // Parse atom body
                    LV2_Atom_Property_Body *body    = lv2_atom_object_begin(&obj->body);

                    // Get number of vectors (dimensions)
                    if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                        return;
    //                lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
    //                lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    if ((body->key != pExt->uridFrameBufferRows) || (body->value.type != pExt->forge.Int))
                        return;
                    uint32_t rows = (reinterpret_cast<const LV2_Atom_Int *>(& body->value))->body;
                    if (rows != sFB.rows())
                    {
    //                    lsp_trace("Row count does not match: %d vs %d", int(rows), int(sFB.rows()));
                        return;
                    }

                    // Get size of each vector
                    body = lv2_atom_object_next(body);
                    if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                    {
    //                    lsp_trace("unexpected end of object");
                        return;
                    }
    //                lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
    //                lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    if ((body->key != pExt->uridFrameBufferCols) || (body->value.type != pExt->forge.Int))
                        return;
                    uint32_t cols = (reinterpret_cast<const LV2_Atom_Int *>(& body->value))->body;
                    if (cols != sFB.cols())
                    {
    //                    lsp_trace("Column count does not match: %d vs %d", int(cols), int(sFB.cols()));
                        return;
                    }

                    // Get first row
                    body = lv2_atom_object_next(body);
                    if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                    {
    //                    lsp_trace("unexpected end of object");
                        return;
                    }

    //                lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
    //                lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));
                    if ((body->key != pExt->uridFrameBufferFirstRowID) || (body->value.type != pExt->forge.Int))
                        return;
                    uint32_t first_row = (reinterpret_cast<const LV2_Atom_Int *>(& body->value))->body;
    //                lsp_trace("first_row = %d", int(first_row));

                    // Get last row
                    body = lv2_atom_object_next(body);
                    if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                    {
    //                    lsp_trace("unexpected end of object");
                        return;
                    }

    //                lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
    //                lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                    if ((body->key != pExt->uridFrameBufferLastRowID) || (body->value.type != pExt->forge.Int))
                        return;
                    uint32_t last_row = (reinterpret_cast<const LV2_Atom_Int *>(& body->value))->body;
    //                lsp_trace("last_row = %d", int(last_row));

                    lsp_trace("first_row = %d, last_row = %d", int(first_row), int(last_row));

                    // Validate
                    uint32_t delta = last_row - first_row;
                    if (delta > FRAMEBUFFER_BULK_MAX)
                    {
    //                    lsp_trace("delta too large: %d", int(delta));
                        return;
                    }

                    // Now parse each vector
                    while (first_row != last_row)
                    {
                        // Read vector as array of floats
                        body = lv2_atom_object_next(body);
                        if (lv2_atom_object_is_end(&obj->body, obj->atom.size, body))
                        {
    //                        lsp_trace("unexpected end of object");
                            return;
                        }

    //                    lsp_trace("body->key (%d) = %s", int(body->key), pExt->unmap_urid(body->key));
    //                    lsp_trace("body->value.type (%d) = %s", int(body->value.type), pExt->unmap_urid(body->value.type));

                        if ((body->key != pExt->uridFrameBufferData) || (body->value.type != pExt->forge.Vector))
                            return;
                        const LV2_Atom_Vector *v = reinterpret_cast<const LV2_Atom_Vector *>(&body->value);

    //                    lsp_trace("body->child_size = %d, body->child_type (%d) = %s", int(v->body.child_size), int(v->body.child_type), pExt->unmap_urid(v->body.child_type));

                        if ((v->body.child_size != sizeof(float)) || (v->body.child_type != pExt->forge.Float))
                            return;
                        ssize_t v_items     = (v->atom.size - sizeof(LV2_Atom_Vector_Body)) / sizeof(float);
                        if (v_items != ssize_t(cols))
                        {
    //                        lsp_trace("vector items does not match columns count: %d vs %d", int(v_items), int(cols));
                            return;
                        }

                        sFB.write_row(first_row++, reinterpret_cast<const float *>(v + 1));
                    }
                    sFB.seek(first_row);
                }

                virtual void *buffer() override
                {
                    return &sFB;
                }

                virtual bool sync() override
                {
                    // Check if there is data for viewing
                    if (pPort == NULL)
                        return false;

                    plug::frame_buffer_t *fb = reinterpret_cast<plug::frame_buffer_t *>(pPort->buffer());
                    return (fb != NULL) ? sFB.sync(fb) : false;
                }
        };

        class UIPathPort: public UIPort
        {
            protected:
                lv2::PathPort      *pPort;
                char                sPath[PATH_MAX];

            public:
                explicit UIPathPort(const meta::port_t *meta, lv2::Extensions *ext, lv2::Port *xport):
                    UIPort(meta, ext)
                {
                    sPath[0]    = '\0';
                    pPort       = NULL;

                    lsp_trace("id=%s, ext=%p, xport=%p", meta->id, ext, xport);

                    // Try to perform direct access to the port using LV2:Instance interface
                    const meta::port_t *xmeta = (xport != NULL) ? xport->metadata() : NULL;
                    if (meta::is_path_port(xmeta))
                    {
                        pPort               = static_cast<lv2::PathPort *>(xport);
                        pPort->tx_request();
                        lsp_trace("Connected direct path port id=%s", xmeta->id);
                    }
                }

                UIPathPort(const UIPathPort &) = delete;
                UIPathPort(UIPathPort &&) = delete;
                UIPathPort & operator = (const UIPathPort &) = delete;
                UIPathPort & operator = (UIPathPort &&) = delete;

            public:
                virtual void deserialize(const void *data) override
                {
                    // Read path value
                    const LV2_Atom *atom = reinterpret_cast<const LV2_Atom *>(data);
                    lv2_set_string(sPath, PATH_MAX, reinterpret_cast<const char *>(atom + 1), atom->size);

                    // The code below works only when restoring plugin state
                    lsp_trace("mapPath = %p, path = %s", pExt->mapPath, sPath);
                    if ((pExt->mapPath != NULL) && (::strstr(sPath, LSP_BUILTIN_PREFIX) != sPath))
                    {
                        char *unmapped_path = pExt->mapPath->absolute_path(pExt->mapPath->handle, sPath);
                        if (unmapped_path != NULL)
                        {
                            lsp_trace("unmapped path: %s -> %s", sPath, unmapped_path);
                            lv2_set_string(sPath, PATH_MAX, unmapped_path, ::strlen(unmapped_path));
                            ::free(unmapped_path);
                        }
                    }
                }

                virtual LV2_URID        get_type_urid() const override
                {
                    return pExt->uridPathType;
                }

                virtual void serialize() override
                {
                    lsp_trace("mapPath = %p, path = %s", pExt->mapPath, sPath);
                    if ((pExt->mapPath != NULL) && (::strstr(sPath, LSP_BUILTIN_PREFIX) != sPath))
                    {
                        char* mapped_path = pExt->mapPath->abstract_path(pExt->mapPath->handle, sPath);
                        if (mapped_path != NULL)
                        {
                            lsp_trace("mapped path: %s -> %s", sPath, mapped_path);
                            pExt->forge_path(mapped_path);
                            ::free(mapped_path);
                        }
                        else
                            pExt->forge_path(sPath);
                    }
                    else
                        pExt->forge_path(sPath);
                }

                virtual void write(const void* buffer, size_t size, size_t flags) override
                {
                    lv2_set_string(sPath, PATH_MAX, reinterpret_cast<const char *>(buffer), size);

                    // Try to perform direct access to the port using LV2:Instance interface
                    lsp_trace(
                        "writing patch event id=%s, path=%s (%d)",
                        pMetadata->id, static_cast<const char *>(buffer), int(size));
                    pExt->ui_write_patch(this);
                }

                virtual void write(const void* buffer, size_t size) override
                {
                    write(buffer, size, 0);
                }

                virtual bool sync() override
                {
                    if (!pPort->tx_pending())
                        return false;
                    pPort->reset_tx_pending();

                    plug::path_t *path  = static_cast<plug::path_t *>(pPort->buffer());
                    ::strncpy(sPath, path->path(), PATH_MAX-1); // Copy current contents
                    sPath[PATH_MAX-1]   = '\0';

                    lsp_trace("Directly received path port id=%s, path=%s",
                            pPort->metadata()->id, sPath);

                    return true;
                }

                virtual void *buffer() override
                {
                    return sPath;
                }

                virtual void set_default() override
                {
                    write("", 0, plug::PF_PRESET_IMPORT);
                }
        };

        class UIStringPort: public UIPort
        {
            protected:
                plug::string_t     *pValue;
                char               *pData;
                uint32_t            nCapacity;
                uint32_t            nSerial;

            public:
                explicit UIStringPort(const meta::port_t *meta, lv2::Extensions *ext, lv2::Port *xport):
                    UIPort(meta, ext)
                {
                    lv2::StringPort *sp     = (xport != NULL) ? static_cast<lv2::StringPort *>(xport) : NULL;
                    if (xport != NULL)
                    {
                        pValue                  = sp->data();
                        nCapacity               = pValue->max_bytes();
                        nSerial                 = pValue->serial() - 1;
                    }
                    else
                    {
                        pValue                  = NULL;
                        nCapacity               = size_t(meta->max) * 4;
                    }

                    // Allocate buffer to store value
                    pData                   = reinterpret_cast<char *>(malloc(nCapacity + 1));
                    if (pData != NULL)
                        pData[0]                = '\0';

                    lsp_trace("id=%s, ext=%p, xport=%p", meta->id, ext, xport);
                    if (sp != NULL)
                        lsp_trace("Connected direct string port id=%s", sp->metadata()->id);
                }

                UIStringPort(const UIStringPort &) = delete;
                UIStringPort(UIStringPort &&) = delete;

                virtual ~UIStringPort() override
                {
                    pValue                  = NULL;
                    if (pData != NULL)
                    {
                        free(pData);
                        pData                   = NULL;
                    }
                }

                UIStringPort & operator = (const UIStringPort &) = delete;
                UIStringPort & operator = (UIStringPort &&) = delete;

            public:
                virtual void deserialize(const void *data) override
                {
                    // Read path value
                    const LV2_Atom *atom = reinterpret_cast<const LV2_Atom *>(data);
                    plug::utf8_strncpy(pData, nCapacity, reinterpret_cast<const char *>(atom + 1), atom->size);

                    lsp_trace("id=%s, value=%s", pMetadata->id, pData);
                }

                virtual LV2_URID        get_type_urid() const override
                {
                    return pExt->forge.String;
                }

                virtual void serialize() override
                {
                    pExt->forge_string(pData);
                }

                virtual void write(const void* buffer, size_t size, size_t flags) override
                {
                    plug::utf8_strncpy(pData, nCapacity, buffer, size);

                    // Try to perform direct access to the port using LV2:Instance interface
                    lsp_trace(
                        "writing patch event id=%s, value=%s (%d)",
                        pMetadata->id, static_cast<const char *>(buffer), int(size));
                    pExt->ui_write_patch(this);
                }

                virtual void write(const void* buffer, size_t size) override
                {
                    write(buffer, size, 0);
                }

                virtual bool sync() override
                {
                    if (pValue == NULL)
                        return false;

                    return pValue->fetch(&nSerial, pData, nCapacity + 1);
                }

                virtual void *buffer() override
                {
                    return pData;
                }

                virtual void set_default() override
                {
                    const meta::port_t *meta = metadata();
                    const char *text = (meta != NULL) ? meta->value : "";

                    write(text, strlen(text), plug::PF_PRESET_IMPORT);
                }
        };
    } /* namespace lv2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_UI_PORTS_H_ */
