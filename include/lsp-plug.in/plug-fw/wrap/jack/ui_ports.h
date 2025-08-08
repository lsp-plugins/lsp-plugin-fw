/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
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

                virtual void resync()       { };
        };

        class UIControlPort: public UIPort
        {
            protected:
                float                   fValue;
                ui::IPresetManager     *pManager;

            public:
                explicit UIControlPort(jack::Port *port, ui::IPresetManager *manager): UIPort(port)
                {
                    fValue      = port->value();
                    pManager    = manager;
                }

                UIControlPort(const UIControlPort &) = delete;
                UIControlPort(UIControlPort &&) = delete;

                virtual ~UIControlPort() override
                {
                    fValue      = pMetadata->start;
                }

                UIControlPort & operator = (const UIControlPort &) = delete;
                UIControlPort & operator = (UIControlPort &&) = delete;

            public:
                virtual float value() override
                {
                    return fValue;
                }

                virtual void set_value(float value) override
                {
                    value = meta::limit_value(pMetadata, value);
                    if (value == fValue)
                        return;

                    fValue  = value;
                    pPort->commit_value(fValue);
                    if (pManager != NULL)
                        pManager->mark_active_preset_dirty();
                }

                virtual void write(const void *buffer, size_t size) override
                {
                    if (size != sizeof(float))
                        return;

                    set_value(*static_cast<const float *>(buffer));
                }
        };

        class UIPortGroup: public UIControlPort
        {
            private:
                jack::PortGroup        *pPG;

            public:
                explicit UIPortGroup(jack::PortGroup *port, ui::IPresetManager *manager) : UIControlPort(port, manager)
                {
                    pPG                 = port;
                }

                UIPortGroup(const UIPortGroup &) = delete;
                UIPortGroup(UIPortGroup &&) = delete;
                UIPortGroup & operator = (const UIPortGroup &) = delete;
                UIPortGroup & operator = (UIPortGroup &&) = delete;

            public:
                inline size_t rows() const  { return pPG->rows(); }
                inline size_t cols() const  { return pPG->cols(); }
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

                UIMeterPort(const UIMeterPort &) = delete;
                UIMeterPort(UIMeterPort &&) = delete;

                virtual ~UIMeterPort() override
                {
                    fValue      = pMetadata->start;
                }

                UIMeterPort & operator = (const UIMeterPort &) = delete;
                UIMeterPort & operator = (UIMeterPort &&) = delete;

            public:
                virtual float value() override
                {
                    return fValue;
                }

                virtual bool sync() override
                {
                    float value = fValue;
                    jack::MeterPort *mp = static_cast<jack::MeterPort *>(pPort);
                    fValue      = mp->sync_value();

                    return fValue != value;
                }
        };

        class UIMeshPort: public UIPort
        {
            private:
                plug::mesh_t   *pMesh;

            public:
                explicit UIMeshPort(jack::Port *port): UIPort(port)
                {
                    pMesh       = jack::create_mesh(port->metadata());
                }

                UIMeshPort(const UIMeshPort &) = delete;
                UIMeshPort(UIMeshPort &&) = delete;

                virtual ~UIMeshPort() override
                {
                    jack::destroy_mesh(pMesh);
                    pMesh = NULL;
                }

                UIMeshPort & operator = (const UIMeshPort &) = delete;
                UIMeshPort & operator = (UIMeshPort &&) = delete;

            public:
                virtual bool sync() override
                {
                    plug::mesh_t *mesh = pPort->buffer<plug::mesh_t>();
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
                explicit UIStreamPort(jack::Port *port): UIPort(port)
                {
                    pStream     = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                }

                UIStreamPort(const UIStreamPort &) = delete;
                UIStreamPort(UIStreamPort &&) = delete;

                virtual ~UIStreamPort() override
                {
                    plug::stream_t::destroy(pStream);
                    pStream     = NULL;
                }

                UIStreamPort & operator = (const UIStreamPort &) = delete;
                UIStreamPort & operator = (UIStreamPort &&) = delete;

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
                explicit UIFrameBufferPort(jack::Port *port): UIPort(port)
                {
                    sFB.init(pMetadata->start, pMetadata->step);
                }

                UIFrameBufferPort(const UIFrameBufferPort &) = delete;
                UIFrameBufferPort(UIFrameBufferPort &&) = delete;

                virtual ~UIFrameBufferPort() override
                {
                    sFB.destroy();
                }

                UIFrameBufferPort & operator = (const UIFrameBufferPort &) = delete;
                UIFrameBufferPort & operator = (UIFrameBufferPort &&) = delete;

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

                UIOscPortIn(const UIOscPortIn &) = delete;
                UIOscPortIn(UIOscPortIn &&) = delete;

                virtual ~UIOscPortIn() override
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
                virtual bool sync() override
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

                virtual bool sync_again() override
                {
                    return bSyncAgain;
                }

                virtual void *buffer() override
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

                UIOscPortOut(const UIOscPortOut &) = delete;
                UIOscPortOut(UIOscPortOut &&) = delete;
                UIOscPortOut & operator = (const UIOscPortOut &) = delete;
                UIOscPortOut & operator = (UIOscPortOut &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return NULL;
                }

                virtual void write(const void *buffer, size_t size) override
                {
                    core::osc_buffer_t *fb  = pPort->buffer<core::osc_buffer_t>();
                    if (fb != NULL)
                        fb->submit(buffer, size);
                }
        };

        class UIPathPort: public UIPort
        {
            private:
                ui::IPresetManager *pManager;
                jack::path_t       *pPath;
                char                sPath[PATH_MAX];

            public:
                explicit UIPathPort(jack::Port *port, ui::IPresetManager *manager): UIPort(port)
                {
                    pManager                = manager;

                    plug::path_t *path      = pPort->buffer<plug::path_t>();
                    pPath                   = (path != NULL) ? static_cast<jack::path_t *>(path) : NULL;
                    sPath[0]                = '\0';
                }

                UIPathPort(const UIPathPort &) = delete;
                UIPathPort(UIPathPort &&) = delete;
                UIPathPort & operator = (const UIPathPort &) = delete;
                UIPathPort & operator = (UIPathPort &&) = delete;

                virtual ~UIPathPort() override
                {
                    pPath       = NULL;
                }

            public:
                virtual void *buffer() override
                {
                    return sPath;
                }

                virtual void write(const void *buffer, size_t size) override
                {
                    write(buffer, size, 0);
                }

                virtual void write(const void *buffer, size_t size, size_t flags) override
                {
                    // Store path string
                    if (size >= PATH_MAX)
                        size = PATH_MAX - 1;
                    ::memcpy(sPath, buffer, size);
                    sPath[size] = '\0';

                    if (pManager != NULL)
                        pManager->mark_active_preset_dirty();

                    // Submit path string to DSP
                    if (pPath != NULL)
                        pPath->submit(sPath, flags);
                }

                virtual void set_default() override
                {
                    write("", 0, plug::PF_PRESET_IMPORT);
                }
        };

        class UIStringPort: public UIPort
        {
            private:
                ui::IPresetManager *pManager;
                plug::string_t     *pValue;
                char               *pData;
                uint32_t            nSerial;

            public:
                explicit UIStringPort(jack::Port *port, ui::IPresetManager *manager): UIPort(port)
                {
                    pManager                = manager;

                    jack::StringPort *sp    = static_cast<jack::StringPort *>(port);
                    pValue                  = sp->data();
                    pData                   = (pValue != NULL) ? reinterpret_cast<char *>(malloc(pValue->max_bytes() + 1)) : NULL;
                    nSerial                 = (pValue != NULL) ?  pValue->serial() - 1 : 0;

                    if (pData != NULL)
                        pData[0]                = '\0';
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
                virtual void *buffer() override
                {
                    return pData;
                }

                virtual void write(const void *buffer, size_t size) override
                {
                    return write(buffer, size, 0);
                }

                virtual void write(const void *buffer, size_t size, size_t flags) override
                {
                    if ((pData == NULL) || (pValue == NULL))
                        return;

                    const size_t count = lsp_min(size, pValue->nCapacity);
                    plug::utf8_strncpy(pData, count, buffer, size);
                    nSerial = pValue->submit(buffer, size, flags & plug::PF_STATE_RESTORE);

                    if (pManager != NULL)
                        pManager->mark_active_preset_dirty();
                }

                virtual void set_default() override
                {
                    if ((pData == NULL) || (pValue == NULL))
                        return;

                    const meta::port_t *meta = metadata();
                    const char *text = (meta != NULL) ? meta->value : "";

                    plug::utf8_strncpy(pData, pValue->nCapacity, text);
                    write(pData, strlen(pData), plug::PF_PRESET_IMPORT);

                    if (pManager != NULL)
                        pManager->mark_active_preset_dirty();
                }

                virtual bool sync() override
                {
                    jack::StringPort *sp    = static_cast<jack::StringPort *>(pPort);
                    if ((sp != NULL) && (sp->check_reset_pending()))
                    {
                        write("", 0, 0);
                        return true;
                    }
                    return false;
                }
        };

    } /* namespace jack */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_PORTS_H_ */
