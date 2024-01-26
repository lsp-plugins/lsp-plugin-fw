/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 21 янв. 2024 г.
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


#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_UI_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_UI_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/data.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        class IPortChangeHandler
        {
            public:
                virtual ~IPortChangeHandler() {}

            public:
                virtual void port_write(ui::IPort *port, size_t flags) = 0;
        };

        class UIPort: public ui::IPort
        {
            public:
                explicit UIPort(const meta::port_t *meta): ui::IPort(meta)
                {
                }

                UIPort(const UIPort &) = delete;
                UIPort(UIPort &&) = delete;

                UIPort & operator = (const UIPort &) = delete;
                UIPort & operator = (UIPort &&) = delete;
        };

        class UIParameterPort: public UIPort
        {
            protected:
                IPortChangeHandler     *pHandler;   // Port change handler
                Steinberg::Vst::ParamID nID;        // Unique identifier of the port
                float                   fValue;     // The actual value of the port
                bool                    bVirtual;   // Virtual flag

            public:
                explicit UIParameterPort(const meta::port_t *meta, IPortChangeHandler *handler, bool virt) : UIPort(meta)
                {
                    pHandler            = handler;
                    nID                 = vst3::gen_parameter_id(meta->id);
                    fValue              = meta->start;
                    bVirtual            = virt;
                }

                virtual ~UIParameterPort() override
                {
                    pHandler            = NULL;
                }

                UIParameterPort(const UIParameterPort &) = delete;
                UIParameterPort(UIParameterPort &&) = delete;

                UIParameterPort & operator = (const UIParameterPort &) = delete;
                UIParameterPort & operator = (UIParameterPort &&) = delete;

            public:
                inline Steinberg::Vst::ParamID parameter_id() const { return nID; }

            public:
                virtual float value() override  { return fValue; }

                virtual void set_value(float value, size_t flags) override
                {
                    fValue      = meta::limit_value(pMetadata, value);
                    if (pHandler != NULL)
                        pHandler->port_write(this, flags);
                }

                virtual void set_value(float value) override
                {
                    set_value(value, ui::IMPORT_FLAG_NONE);
                }

            public:
                inline bool is_virtual() const      { return bVirtual; }

                void commit_value(float value)
                {
                    fValue      = meta::limit_value(pMetadata, value);
                }
        };

        class UIMeterPort: public UIPort
        {
            protected:
                float                   fValue;     // The actual value of the port

            public:
                explicit UIMeterPort(const meta::port_t *meta) : UIPort(meta)
                {
                    fValue              = meta->start;
                }

                UIMeterPort(const UIParameterPort &) = delete;
                UIMeterPort(UIParameterPort &&) = delete;

                UIMeterPort & operator = (const UIMeterPort &) = delete;
                UIMeterPort & operator = (UIMeterPort &&) = delete;

            public:
                virtual float value() override  { return fValue; }

            public:
                bool commit_value(float value)
                {
                    value = meta::limit_value(pMetadata, value);
                    if (value == fValue)
                        return false;

                    fValue      = value;
                    return true;
                }
        };

        class UIPortGroup: public UIParameterPort
        {
            protected:
                size_t          nRows;
                size_t          nCols;

            public:
                UIPortGroup(const meta::port_t *meta, IPortChangeHandler *handler, bool virt):
                    UIParameterPort(meta, handler, virt)
                {
                    nRows               = list_size(meta->items);
                    nCols               = port_list_size(meta->members);
                }

                UIPortGroup(const UIPortGroup &) = delete;
                UIPortGroup(UIPortGroup &&) = delete;

                UIPortGroup & operator = (const UIPortGroup &) = delete;
                UIPortGroup & operator = (UIPortGroup &&) = delete;

            public:
                inline size_t rows() const      { return nRows; }
                inline size_t cols() const      { return nCols; }
        };

        class UIPathPort: public UIPort
        {
            protected:
                IPortChangeHandler *pHandler;
                char                sPath[MAX_PATH_LEN];

            public:
                explicit UIPathPort(const meta::port_t *meta, IPortChangeHandler *handler):
                    UIPort(meta)
                {
                    pHandler        = handler;
                }

                virtual ~UIPathPort()
                {
                    pHandler        = NULL;
                    sPath[0]        = '\0';
                }

                UIPathPort(const UIPathPort &) = delete;
                UIPathPort(UIPathPort &&) = delete;

                UIPathPort & operator = (const UIPathPort &) = delete;
                UIPathPort & operator = (UIPathPort &&) = delete;

            public:
                virtual void write(const void* buffer, size_t size, size_t flags) override
                {
                    size    = lsp_min(size, size_t(MAX_PATH_LEN));
                    strncpy(sPath, reinterpret_cast<const char *>(buffer), size);
                    sPath[MAX_PATH_LEN - 1] = '\0';
                    if (pHandler != NULL)
                        pHandler->port_write(this, flags);
                }

                virtual void write(const void* buffer, size_t size) override
                {
                    write(buffer, size, 0);
                }

                virtual void *buffer() override
                {
                    return sPath;
                }

                virtual void set_default() override
                {
                    write("", 0, plug::PF_PRESET_IMPORT);
                }

            public:
                void commit_value(const char *text)
                {
                    strncpy(sPath, text, MAX_PATH_LEN-1);
                    sPath[MAX_PATH_LEN - 1] = '\0';
                }
        };

        class UIMeshPort: public UIPort
        {
            protected:
                plug::mesh_t           *pMesh;

            public:
                explicit UIMeshPort(const meta::port_t *meta) : UIPort(meta)
                {
                    pMesh               = vst3::create_mesh(meta);
                }

                virtual ~UIMeshPort() override
                {
                    vst3::destroy_mesh(pMesh);
                    pMesh               = NULL;
                }

                UIMeshPort(const UIMeshPort &) = delete;
                UIMeshPort(UIMeshPort &&) = delete;

                UIMeshPort & operator = (const UIMeshPort &) = delete;
                UIMeshPort & operator = (UIMeshPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pMesh;
                }
        };

        class UIFrameBufferPort: public UIPort
        {
            protected:
                plug::frame_buffer_t   *pFBuffer;

            public:
                explicit UIFrameBufferPort(const meta::port_t *meta) : UIPort(meta)
                {
                    pFBuffer            = plug::frame_buffer_t::create(pMetadata->start, pMetadata->step);
                }

                virtual ~UIFrameBufferPort() override
                {
                    plug::frame_buffer_t::destroy(pFBuffer);
                    pFBuffer            = NULL;
                }

                UIFrameBufferPort(const UIFrameBufferPort &) = delete;
                UIFrameBufferPort(UIFrameBufferPort &&) = delete;

                UIFrameBufferPort & operator = (const UIFrameBufferPort &) = delete;
                UIFrameBufferPort & operator = (UIFrameBufferPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pFBuffer;
                }
        };

        class UIStreamPort: public UIPort
        {
            protected:
                plug::stream_t      *pStream;

            public:
                explicit UIStreamPort(const meta::port_t *meta) : UIPort(meta)
                {
                    pStream             = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                }

                virtual ~UIStreamPort() override
                {
                    plug::stream_t::destroy(pStream);
                    pStream             = NULL;
                }

                UIStreamPort(const UIStreamPort &) = delete;
                UIStreamPort(UIStreamPort &&) = delete;

                UIStreamPort & operator = (const UIStreamPort &) = delete;
                UIStreamPort & operator = (UIStreamPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pStream;
                }
        };

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_UI_PORTS_H_ */
