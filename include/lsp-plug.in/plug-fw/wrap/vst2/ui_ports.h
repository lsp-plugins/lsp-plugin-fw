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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_UI_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_UI_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ports.h>
#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace vst2
    {
        class UIPort: public ui::IPort
        {
            protected:
                vst2::Port             *pPort;

            public:
                explicit UIPort(const meta::port_t *meta, vst2::Port *port):
                    ui::IPort(meta)
                {
                    pPort       = port;
                }

                UIPort(const UIPort &) = delete;
                UIPort(UIPort &&) = delete;

                virtual ~UIPort() override
                {
                    pPort       = NULL;
                }

                UIPort & operator = (const UIPort &) = delete;
                UIPort & operator = (UIPort &&) = delete;

            public:
                virtual bool sync()         { return false; };

                virtual bool sync_again()   { return false; };
        };

        class UIPortGroup: public UIPort
        {
            private:
                vst2::PortGroup        *pPG;
                uatomic_t               nSID;

            public:
                explicit UIPortGroup(vst2::PortGroup *port) : UIPort(port->metadata(), port)
                {
                    pPG                 = port;
                    nSID                = port->sid() - 1;
                }

                UIPortGroup(const UIPortGroup &) = delete;
                UIPortGroup(UIPortGroup &&) = delete;
                UIPortGroup & operator = (const UIPortGroup &) = delete;
                UIPortGroup & operator = (UIPortGroup &&) = delete;

            public:
                virtual float value()
                {
                    return pPort->value();
                }

                virtual void set_value(float value)
                {
                    pPort->set_value(value);
                }

                virtual bool sync()
                {
                    uatomic_t sid = pPG->sid();
                    if (sid == nSID)
                        return false;

                    nSID        = sid;
                    return true;
                }

            public:
                inline size_t rows() const  { return pPG->rows(); }
                inline size_t cols() const  { return pPG->cols(); }
        };

        class UIParameterPort: public UIPort
        {
            protected:
                float           fValue;
                uatomic_t       nSID;

            public:
                explicit UIParameterPort(const meta::port_t *meta, vst2::ParameterPort *port):
                    UIPort(meta, port)
                {
                    fValue      = meta->start;
                    nSID        = port->sid() - 1;
                }

                UIParameterPort(const UIParameterPort &) = delete;
                UIParameterPort(UIParameterPort &&) = delete;

                virtual ~UIParameterPort()
                {
                    fValue      = pMetadata->start;
                }

                UIParameterPort & operator = (const UIParameterPort &) = delete;
                UIParameterPort & operator = (UIParameterPort &&) = delete;

            public:
                virtual float value()
                {
                    return fValue;
                }

                virtual void set_value(float value)
                {
                    fValue = meta::limit_value(pMetadata, value);
                    if (pPort != NULL)
                        pPort->write_value(value);
                }

                virtual bool sync()
                {
                    uatomic_t sid = static_cast<vst2::ParameterPort *>(pPort)->sid();
                    if (sid == nSID)
                        return false;

                    fValue      = pPort->value();
                    nSID        = sid;
                    return true;
                }

                virtual void *buffer()
                {
                    return pPort->buffer();
                }
        };

        class UIMeterPort: public UIPort
        {
            private:
                float   fValue;

            public:
                explicit UIMeterPort(const meta::port_t *meta, vst2::Port *port):
                    UIPort(meta, port)
                {
                    fValue      = meta->start;
                }

                UIMeterPort(const UIMeterPort &) = delete;
                UIMeterPort(UIMeterPort &&) = delete;

                virtual ~UIMeterPort()
                {
                    fValue      = pMetadata->start;
                }

                UIMeterPort & operator = (const UIMeterPort &) = delete;
                UIMeterPort & operator = (UIMeterPort &&) = delete;

            public:
                virtual float value()
                {
                    return fValue;
                }

                virtual bool sync()
                {
                    float value = fValue;
                    if (pMetadata->flags & meta::F_PEAK)
                    {
                        vst2::MeterPort *mp = static_cast<vst2::MeterPort *>(pPort);
                        fValue              = mp->sync_value();
                    }
                    else
                        fValue      = pPort->value();
                    return value != fValue;
                }
        };

        class UIMeshPort: public UIPort
        {
            private:
                plug::mesh_t   *pMesh;

            public:
                explicit UIMeshPort(const meta::port_t *meta, vst2::Port *port):
                    UIPort(meta, port)
                {
                    pMesh       = vst2::create_mesh(meta);
                }

                UIMeshPort(const UIMeshPort &) = delete;
                UIMeshPort(UIMeshPort &&) = delete;

                virtual ~UIMeshPort()
                {
                    vst2::destroy_mesh(pMesh);
                    pMesh = NULL;
                }

                UIMeshPort & operator = (const UIMeshPort &) = delete;
                UIMeshPort & operator = (UIMeshPort &&) = delete;

            public:
                virtual bool sync()
                {
                    plug::mesh_t *mesh = reinterpret_cast<plug::mesh_t *>(pPort->buffer());
                    if ((mesh == NULL) || (!mesh->containsData()))
                        return false;

                    // Copy mesh data
                    for (size_t i=0; i < mesh->nBuffers; ++i)
                        dsp::copy_saturated(pMesh->pvData[i], mesh->pvData[i], mesh->nItems);
                    pMesh->data(mesh->nBuffers, mesh->nItems);

                    // Clean source mesh
                    mesh->cleanup();

                    return true;
                }

                virtual void *buffer()
                {
                    return pMesh;
                }
        };

        class UIStreamPort: public UIPort
        {
            private:
                plug::stream_t     *pStream;

            public:
                explicit UIStreamPort(const meta::port_t *meta, vst2::Port *port):
                    UIPort(meta, port)
                {
                    pStream     = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                }

                UIStreamPort(const UIStreamPort &) = delete;
                UIStreamPort(UIStreamPort &&) = delete;

                virtual ~UIStreamPort()
                {
                    plug::stream_t::destroy(pStream);
                    pStream     = NULL;
                }

                UIStreamPort & operator = (const UIStreamPort &) = delete;
                UIStreamPort & operator = (UIStreamPort &&) = delete;

            public:
                virtual bool sync()
                {
                    plug::stream_t *stream = pPort->buffer<plug::stream_t>();
                    return (stream != NULL) ? pStream->sync(stream) : false;
                }

                virtual void *buffer()
                {
                    return pStream;
                }
        };

        class UIFrameBufferPort: public UIPort
        {
            private:
                plug::frame_buffer_t    sFB;

            public:
                explicit UIFrameBufferPort(const meta::port_t *meta, vst2::Port *port):
                    UIPort(meta, port)
                {
                    sFB.init(pMetadata->start, pMetadata->step);
                }

                UIFrameBufferPort(const UIFrameBufferPort &) = delete;
                UIFrameBufferPort(UIFrameBufferPort &&) = delete;

                virtual ~UIFrameBufferPort()
                {
                    sFB.destroy();
                }

                UIFrameBufferPort & operator = (const UIFrameBufferPort &) = delete;
                UIFrameBufferPort & operator = (UIFrameBufferPort &&) = delete;

            public:
                virtual bool sync()
                {
                    // Check if there is data for viewing
                    plug::frame_buffer_t *fb = pPort->buffer<plug::frame_buffer_t>();
                    return (fb != NULL) ? sFB.sync(fb) : false;
                }

                virtual void *buffer()
                {
                    return &sFB;
                }
        };

        class UIPathPort: public UIPort
        {
            private:
                vst2::path_t   *pPath;

            public:
                explicit UIPathPort(const meta::port_t *meta, vst2::Port *port): UIPort(meta, port)
                {
                    plug::path_t *path  = pPort->buffer<plug::path_t>();
                    if (path != NULL)
                        pPath               = static_cast<vst2::path_t *>(path);
                    else
                        pPath               = NULL;
                }

                UIPathPort(const UIPathPort &) = delete;
                UIPathPort(UIPathPort &&) = delete;

                virtual ~UIPathPort()
                {
                    pPath       = NULL;
                }

                UIPathPort & operator = (const UIPathPort &) = delete;
                UIPathPort & operator = (UIPathPort &&) = delete;

            public:
                virtual bool sync()
                {
                    return pPath->ui_sync();
                }

                virtual void *buffer()
                {
                    return (pPath != NULL) ? pPath->sUiPath : NULL;
                }

                virtual void write(const void *buffer, size_t size)
                {
                    write(buffer, size, 0);
                }

                virtual void write(const void *buffer, size_t size, size_t flags)
                {
                    if (pPath != NULL)
                        pPath->submit(static_cast<const char *>(buffer), size, true, flags);
                }

                virtual void set_default()
                {
                    write("", 0, plug::PF_PRESET_IMPORT);
                }
        };

        class UIStringPort: public UIPort
        {
            private:
                plug::string_t     *pValue;
                char               *pData;
                uint32_t            nSerial;

            public:
                explicit UIStringPort(const meta::port_t *meta, vst2::Port *port): UIPort(meta, port)
                {
                    vst2::StringPort *sp    = static_cast<vst2::StringPort *>(port);
                    pValue                  = sp->data();
                    pData                   = (pValue != NULL) ? reinterpret_cast<char *>(malloc(pValue->max_bytes() + 1)) : NULL;
                    nSerial                 = (pValue != NULL) ?  pValue->serial() - 1 : 0;

                    if (pData != NULL)
                        pData[0]                = '\0';
                }

                UIStringPort(const UIStringPort &) = delete;
                UIStringPort(UIStringPort &&) = delete;

                virtual ~UIStringPort()
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
                virtual void *buffer()
                {
                    return pData;
                }

                virtual bool sync()
                {
                    return pValue->fetch(&nSerial, pData, pValue->max_bytes() + 1);
                }

                virtual void write(const void *buffer, size_t size)
                {
                    write(buffer, size, 0);
                }

                virtual void write(const void *buffer, size_t size, size_t flags) override
                {
                    if ((pData == NULL) || (pValue == NULL))
                        return;

                    const size_t count = lsp_min(size, pValue->nCapacity);
                    plug::utf8_strncpy(pData, count, buffer, size);
                    nSerial = pValue->submit(buffer, size, flags & plug::PF_STATE_RESTORE);
                }

                virtual void set_default() override
                {
                    const meta::port_t *meta = metadata();
                    const char *text = (meta != NULL) ? meta->value : "";

                    write(text, strlen(text), plug::PF_PRESET_IMPORT);
                }
        };

        class UIOscPortIn: public UIPort
        {
            private:
                osc::packet_t   sPacket;
                size_t          nCapacity;
                bool            bSyncAgain;

            public:
                explicit UIOscPortIn(const meta::port_t *meta, vst2::Port *port): UIPort(meta, port)
                {
                    bSyncAgain      = false;
                    nCapacity       = 0x100;
                    sPacket.data    = reinterpret_cast<uint8_t *>(::malloc(nCapacity));
                    sPacket.size    = 0;
                }

                UIOscPortIn(const UIOscPortIn &) = delete;
                UIOscPortIn(UIOscPortIn &&) = delete;

                virtual ~UIOscPortIn()
                {
                    if (sPacket.data != NULL)
                    {
                        ::free(sPacket.data);
                        sPacket.data    = NULL;
                    }
                }

                UIOscPortIn & operator = (const UIOscPortIn &) = delete;
                UIOscPortIn & operator = (UIOscPortIn &&) = delete;

            public:
                virtual bool sync()
                {
                    // Check if there is data for viewing
                    bSyncAgain              = false;
                    core::osc_buffer_t *fb  = pPort->buffer<core::osc_buffer_t>();

                    while (true)
                    {
                        // Try to fetch record from buffer
                        status_t res = fb->fetch(&sPacket, nCapacity);

                        switch (res)
                        {
                            case STATUS_OK:
                            {
                                bSyncAgain    = true;
                                lsp_trace("Received OSC message of %d bytes", int(sPacket.size));
                                osc::dump_packet(&sPacket);
                                return true;
                            }

                            case STATUS_NO_DATA:
                                return false;

                            case STATUS_OVERFLOW:
                            {
                                // Reallocate memory
                                uint8_t *newptr    = reinterpret_cast<uint8_t *>(::realloc(sPacket.data, nCapacity << 1));
                                if (newptr == NULL)
                                    fb->skip();
                                else
                                    sPacket.data    = newptr;
                                break;
                            }

                            default:
                                return false;
                        }
                    }
                }

                virtual bool sync_again() { return bSyncAgain; }

                virtual void *buffer()
                {
                    return &sPacket;
                }
        };

        class UIOscPortOut: public UIPort
        {
            public:
                UIOscPortOut(const meta::port_t *meta, vst2::Port *port):
                    UIPort(meta, port)
                {
                }

                UIOscPortOut(const UIOscPortOut &) = delete;
                UIOscPortOut(UIOscPortOut &&) = delete;
                UIOscPortOut & operator = (const UIOscPortOut &) = delete;
                UIOscPortOut & operator = (UIOscPortOut &&) = delete;

            public:
                virtual void *buffer() { return NULL; }

                virtual void write(const void *buffer, size_t size)
                {
                    core::osc_buffer_t *fb = pPort->buffer<core::osc_buffer_t>();
                    if (fb != NULL)
                        fb->submit(buffer, size);
                }
        };
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_UI_PORTS_H_ */
