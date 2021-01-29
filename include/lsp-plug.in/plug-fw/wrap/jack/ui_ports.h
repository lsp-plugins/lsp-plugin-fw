/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 янв. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/jack/ports.h>
#include <lsp-plug.in/plug-fw/ui.h>

#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/protocol/midi.h>
#include <lsp-plug.in/protocol/osc.h>

namespace lsp
{
    namespace jack
    {
        class UIPort: public ui::IPort
        {
            protected:
                jack::Port     *pPort;

            public:
                explicit UIPort(jack::Port *port) : ui::IPort(port->metadata())
                {
                    pPort       = port;
                }

                virtual ~UIPort()
                {
                    pPort       = NULL;
                }

            public:
                virtual bool sync()         { return false; };
                virtual bool sync_again()   { return false; };

                virtual void resync()       { };
        };

        class UIPortGroup: public UIPort
        {
            private:
                jack::PortGroup        *pPG;

            public:
                explicit UIPortGroup(jack::PortGroup *port) : UIPort(port)
                {
                    pPG                 = port;
                }

                virtual ~UIPortGroup()
                {
                }

            public:
                virtual float value()
                {
                    return pPort->value();
                }

                virtual void set_value(float value)
                {
                    pPort->set_value(value);
                }

            public:
                inline size_t rows() const  { return pPG->rows(); }
                inline size_t cols() const  { return pPG->cols(); }
        };

        class UIControlPort: public UIPort
        {
            protected:
                float           fValue;

            public:
                explicit UIControlPort(jack::Port *port): UIPort(port)
                {
                    fValue      = port->value();
                }

                virtual ~UIControlPort()
                {
                    fValue      = pMetadata->start;
                }

            public:
                virtual float value()
                {
                    return fValue;
                }

                virtual void set_value(float value)
                {
                    fValue  = limit_value(pMetadata, value);
                    static_cast<ControlPort *>(pPort)->update_value(fValue);
                }

                virtual void write(const void *buffer, size_t size)
                {
                    if (size == sizeof(float))
                    {
                        fValue  = *static_cast<const float *>(buffer);
                        static_cast<ControlPort *>(pPort)->update_value(fValue);
                    }
                }
        };

        class UIMeterPort: public UIPort
        {
            private:
                float   fValue;

            public:
                explicit UIMeterPort(jack::Port *port): UIPort(port)
                {
                    fValue      = port->value();
                }

                virtual ~UIMeterPort()
                {
                    fValue      = pMetadata->start;
                }

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
                        jack::MeterPort *mp = static_cast<jack::MeterPort *>(pPort);
                        fValue      = mp->sync_value();
                    }
                    else
                        fValue      = pPort->value();

                    return fValue != value;
                }
        };

        class UIInPort: public UIPort
        {
            private:
                float   fValue;

            public:
                explicit UIInPort(jack::Port *port): UIPort(port)
                {
                    fValue      = port->value();
                }

                virtual ~UIInPort()
                {
                    fValue      = pMetadata->start;
                }

            public:
                virtual float value()
                {
                    return fValue;
                }

                virtual bool sync()
                {
                    float value = fValue;
                    fValue      = pPort->value();
                    return fValue != value;
                }
        };

        class UIMeshPort: public UIPort
        {
            private:
                jack::mesh_t   *pMesh;

            public:
                explicit UIMeshPort(jack::Port *port): UIPort(port)
                {
                    pMesh       = jack::mesh_t::create(port->metadata());
                }

                virtual ~UIMeshPort()
                {
                    jack::mesh_t::destroy(pMesh);
                    pMesh = NULL;
                }

            public:
                virtual bool sync()
                {
                    mesh_t *mesh = pPort->buffer<mesh_t>();
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

        class UIFrameBufferPort: public UIPort
        {
            private:
                plug::frame_buffer_t    sFB;

            public:
                explicit UIFrameBufferPort(jack::Port *port): UIPort(port)
                {
                    sFB.init(pMetadata->start, pMetadata->step);
                }

                virtual ~UIFrameBufferPort()
                {
                    sFB.destroy();
                }

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

        class UIOscPortIn: public UIPort
        {
            private:
                osc::packet_t   sPacket;
                size_t          nCapacity;
                bool            bSyncAgain;

            public:
                explicit UIOscPortIn(jack::Port *port): UIPort(port)
                {
                    bSyncAgain      = false;
                    nCapacity       = 0x100;
                    sPacket.data    = reinterpret_cast<uint8_t *>(::malloc(nCapacity));
                    sPacket.size    = 0;
                }

                virtual ~UIOscPortIn()
                {
                    if (sPacket.data != NULL)
                    {
                        ::free(sPacket.data);
                        sPacket.data    = NULL;
                    }
                }

            public:
                virtual bool sync()
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

                virtual bool sync_again() { return bSyncAgain; }

                virtual void *buffer()
                {
                    return &sPacket;
                }
        };

        class UIOscPortOut: public UIPort
        {
            public:
                explicit UIOscPortOut(jack::Port *port): UIPort(port)
                {
                }

                virtual ~UIOscPortOut()
                {
                }

            public:
                virtual void *buffer() { return NULL; }

                virtual void write(const void *buffer, size_t size)
                {
                    plug::osc_buffer_t *fb  = pPort->buffer<plug::osc_buffer_t>();
                    if (fb != NULL)
                        fb->submit(buffer, size);
                }
        };

        class UIPathPort: public UIPort
        {
            private:
                jack::path_t   *pPath;
                char            sPath[PATH_MAX];

            public:
                explicit UIPathPort(jack::Port *port): UIPort(port)
                {
                    plug::path_t *path      = pPort->buffer<plug::path_t>();
                    pPath = (path != NULL) ? static_cast<jack::path_t *>(path) : NULL;
                    sPath[0]                = '\0';
                }

                virtual ~UIPathPort()
                {
                    pPath       = NULL;
                }

            public:
                virtual void *buffer()
                {
                    return sPath;
                }

                virtual void write(const void *buffer, size_t size)
                {
                    write(buffer, size, 0);
                }

                virtual void write(const void *buffer, size_t size, size_t flags)
                {
                    // Store path string
                    if (size >= PATH_MAX)
                        size = PATH_MAX - 1;
                    ::memcpy(sPath, buffer, size);
                    sPath[size] = '\0';

                    // Submit path string to DSP
                    if (pPath != NULL)
                        pPath->submit(sPath, flags);
                }
        };
    }
}




#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_PORTS_H_ */
