/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 5 янв. 2023 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_UI_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_UI_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/wrap/clap/ports.h>

namespace lsp
{
    namespace clap
    {
        class UIPort: public ui::IPort
        {
            protected:
                clap::Port             *pPort;      // The backend port this port is related to

            public:
                explicit UIPort(const meta::port_t *meta, clap::Port *port):
                    ui::IPort(meta)
                {
                    pPort       = port;
                }

                virtual ~UIPort()
                {
                    pPort       = NULL;
                }

            public:
                /**
                 * Perform the sync of value with the backend port
                 * @return true if the value of the front-end port has changed
                 */
                virtual bool sync()         { return false; }

                virtual bool sync_again()   { return false; }
        };

        class UIParameterPort: public UIPort
        {
            protected:
                float           fValue;
                uatomic_t       nSID;
                bool           *bRqFlag;

            public:
                explicit UIParameterPort(clap::ParameterPort *port, bool *rq_flag):
                    UIPort(port->metadata(), port)
                {
                    fValue      = pMetadata->start;
                    nSID        = port->sid() - 1;
                    bRqFlag     = rq_flag;
                }

                virtual ~UIParameterPort() override
                {
                    fValue      = pMetadata->start;
                }

            public:
                virtual float value() override
                {
                    return fValue;
                }

                virtual void set_value(float value) override
                {
                    fValue = value;
                    if (bRqFlag != NULL)
                        *bRqFlag    = true;

                    if (pPort == NULL)
                        return;

                    clap::ParameterPort *port = static_cast<clap::ParameterPort *>(pPort);
                    port->write_value(value);
                }

                virtual bool sync() override
                {
                    if (pPort == NULL)
                        return false;

                    clap::ParameterPort *port = static_cast<clap::ParameterPort *>(pPort);
                    if (port == NULL)
                        return false;

                    uatomic_t sid   = port->sid();
                    if (sid == nSID)
                        return false;

                    nSID        = sid;
                    fValue      = pPort->value();
                    return true;
                }

                virtual void *buffer() override
                {
                    return pPort->buffer();
                }
        };

        class UIPortGroup: public UIParameterPort
        {
            public:
                explicit UIPortGroup(clap::PortGroup *port, bool *rq_flag) : UIParameterPort(port, rq_flag)
                {
                }

                virtual ~UIPortGroup() override
                {
                }

            public:
                inline size_t rows() const
                {
                    if (pPort == NULL)
                        return 0;
                    const clap::PortGroup *pg =  static_cast<const clap::PortGroup *>(pPort);
                    return pg->rows();
                }

                inline size_t cols() const
                {
                    if (pPort == NULL)
                        return 0;
                    const clap::PortGroup *pg =  static_cast<const clap::PortGroup *>(pPort);
                    return pg->cols();
                }
        };

        class UIMeterPort: public UIPort
        {
            private:
                float   fValue;

            public:
                explicit UIMeterPort(clap::Port *port):
                    UIPort(port->metadata(), port)
                {
                    fValue      = port->default_value();
                }

                virtual ~UIMeterPort() override
                {
                    fValue      = pMetadata->start;
                }

            public:
                virtual float value() override
                {
                    return fValue;
                }

                virtual bool sync() override
                {
                    float value = fValue;
                    if (pMetadata->flags & meta::F_PEAK)
                    {
                        clap::MeterPort *mp = static_cast<clap::MeterPort *>(pPort);
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
                explicit UIMeshPort(clap::Port *port):
                    UIPort(port->metadata(), port)
                {
                    pMesh       = clap::create_mesh(pMetadata);
                }

                virtual ~UIMeshPort() override
                {
                    clap::destroy_mesh(pMesh);
                    pMesh = NULL;
                }

            public:
                virtual bool sync() override
                {
                    plug::mesh_t *mesh = reinterpret_cast<plug::mesh_t *>(pPort->buffer());
                    if ((mesh == NULL) || (!mesh->containsData()))
                        return false;

                    // Copy mesh data
                    for (size_t i=0; i < mesh->nBuffers; ++i)
                        dsp::copy_saturated(pMesh->pvData[i], mesh->pvData[i], mesh->nItems);
                    pMesh->data(mesh->nBuffers, mesh->nItems);

                    // Clean the source mesh
                    mesh->cleanup();

                    return true;
                }

                virtual void *buffer() override
                {
                    return pMesh;
                }
        };

        class UIStreamPort: public UIPort
        {
            private:
                plug::stream_t     *pStream;

            public:
                explicit UIStreamPort(clap::Port *port):
                    UIPort(port->metadata(), port)
                {
                    pStream     = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                }

                virtual ~UIStreamPort() override
                {
                    plug::stream_t::destroy(pStream);
                    pStream     = NULL;
                }

            public:
                virtual bool sync() override
                {
                    plug::stream_t *stream = pPort->buffer<plug::stream_t>();
                    return (stream != NULL) ? pStream->sync(stream) : false;
                }

                virtual void *buffer() override
                {
                    return pStream;
                }
        };

        class UIFrameBufferPort: public UIPort
        {
            private:
                plug::frame_buffer_t    sFB;

            public:
                explicit UIFrameBufferPort(clap::Port *port):
                    UIPort(port->metadata(), port)
                {
                    sFB.init(pMetadata->start, pMetadata->step);
                }

                virtual ~UIFrameBufferPort() override
                {
                    sFB.destroy();
                }

            public:
                virtual bool sync() override
                {
                    // Check if there is data for viewing
                    plug::frame_buffer_t *fb = pPort->buffer<plug::frame_buffer_t>();
                    return (fb != NULL) ? sFB.sync(fb) : false;
                }

                virtual void *buffer() override
                {
                    return &sFB;
                }
        };

        class UIPathPort: public UIPort
        {
            private:
                clap::path_t   *pPath;

            public:
                explicit UIPathPort(clap::Port *port): UIPort(port->metadata(), port)
                {
                    plug::path_t *path  = pPort->buffer<plug::path_t>();
                    if (path != NULL)
                        pPath               = static_cast<clap::path_t *>(path);
                    else
                        pPath               = NULL;
                }

                virtual ~UIPathPort() override
                {
                    pPath       = NULL;
                }

            public:
                virtual bool sync() override
                {
                    return pPath->ui_sync();
                }

                virtual void *buffer() override
                {
                    return (pPath != NULL) ? pPath->sUiPath : NULL;
                }

                virtual void write(const void *buffer, size_t size) override
                {
                    write(buffer, size, 0);
                }

                virtual void write(const void *buffer, size_t size, size_t flags) override
                {
                    if (pPath != NULL)
                        pPath->submit(static_cast<const char *>(buffer), size, true, flags);
                }

                virtual void set_default() override
                {
                    write("", 0, plug::PF_PRESET_IMPORT);
                }
        };

        class UIOscPortIn: public UIPort
        {
            private:
                osc::packet_t   sPacket;
                size_t          nCapacity;
                bool            bSyncAgain;

            public:
                explicit UIOscPortIn(const meta::port_t *meta, clap::Port *port): UIPort(meta, port)
                {
                    bSyncAgain      = false;
                    nCapacity       = 0x100;
                    sPacket.data    = reinterpret_cast<uint8_t *>(::malloc(nCapacity));
                    sPacket.size    = 0;
                }

                virtual ~UIOscPortIn() override
                {
                    if (sPacket.data != NULL)
                    {
                        ::free(sPacket.data);
                        sPacket.data    = NULL;
                    }
                }

            public:
                virtual bool sync() override
                {
                    // Check if there is data for viewing
                    bSyncAgain              = false;
                    plug::osc_buffer_t *fb  = pPort->buffer<plug::osc_buffer_t>();

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

                virtual bool sync_again() override { return bSyncAgain; }

                virtual void *buffer() override
                {
                    return &sPacket;
                }
        };

        class UIOscPortOut: public UIPort
        {
            public:
                UIOscPortOut(const meta::port_t *meta, clap::Port *port):
                    UIPort(meta, port)
                {
                }

                virtual ~UIOscPortOut() override
                {
                }

            public:
                virtual void *buffer() override { return NULL; }

                virtual void write(const void *buffer, size_t size) override
                {
                    plug::osc_buffer_t *fb = pPort->buffer<plug::osc_buffer_t>();
                    if (fb != NULL)
                        fb->submit(buffer, size);
                }
        };

    } /* namespace clap */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_UI_PORTS_H_ */
