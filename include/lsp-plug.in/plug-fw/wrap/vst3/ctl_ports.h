/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 6 февр. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_CTL_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_CTL_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
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
        /**
         * Abstract controller port
         */
        class CtlPort
        {
            protected:
                const meta::port_t             *pMetadata;
                volatile uatomic_t              nSerial;

            public:
                explicit CtlPort(const meta::port_t *meta)
                {
                    pMetadata           = meta;
                    nSerial             = 0;
                }

                virtual ~CtlPort()
                {
                    pMetadata           = NULL;
                    nSerial             = 0;
                }

                CtlPort(const CtlPort &) = delete;
                CtlPort(CtlPort &&) = delete;

                CtlPort & operator = (const CtlPort &) = delete;
                CtlPort & operator = (CtlPort &&) = delete;

            public:
                /** Write some data to port
                 *
                 * @param buffer data to write to port
                 * @param size size of data
                 */
                virtual void        write(const void *buffer, size_t size) {}

                /** Write some data to port
                 *
                 * @param buffer data to write to port
                 * @param size size of data
                 * @param flags additional control flags
                 */
                virtual void        write(const void *buffer, size_t size, size_t flags)
                {
                    write(buffer, size);
                }

                /** Get data from port
                 *
                 * @return associated buffer (may be NULL)
                 */
                virtual void       *buffer() { return NULL; }

                /** Get single float value
                 *
                 * @return single float value
                 */
                virtual float       value() { return 0; }

                /** Get single default float value
                 *
                 * @return default float value
                 */
                virtual float       default_value() { return (pMetadata != NULL) ? pMetadata->start : 0.0f; }

                /**
                 * Set the value to the default
                 */
                virtual void        set_default() { set_value(default_value()); }

                /** Set single float value
                 *
                 * @param value value to set
                 */
                virtual void        set_value(float value) {}

                /** Set single float value
                 *
                 * @param flags additional control flags @see port_flags_t
                 */
                virtual void        set_value(float value, size_t flags) { set_value(value); }

            public:
                /**
                 * Get serial version of the port
                 * @return serial version of the port
                 */
                inline uatomic_t                serial() const { return nSerial; }

                /**
                 * Mark port as been changed
                 * @return the new serial version of the port
                 */
                inline uatomic_t                mark_changed()  { return atomic_add(&nSerial, 1) + 1; }

                /** Get port metadata
                 *
                 * @return port metadata
                 */
                inline const meta::port_t      *metadata() const { return pMetadata; };

                /**
                 * Get unique port identifier
                 * @return unique port identifier
                 */
                virtual const char             *id() const { return (pMetadata != NULL) ? pMetadata->id : NULL; }

                /** Get buffer casted to specified type
                 *
                 * @return buffer casted to specified type
                 */
                template <class T>
                inline T *buffer()
                {
                    return static_cast<T *>(buffer());
                }
        };

        class CtlPortChangeHandler
        {
            public:
                virtual ~CtlPortChangeHandler() {}

            public:
                virtual void port_write(CtlPort *port, size_t flags) = 0;
        };

        class CtlParamPort: public CtlPort
        {
            protected:
                CtlPortChangeHandler   *pHandler;   // Port change handler
                Steinberg::Vst::ParamID nID;        // Unique identifier of the port
                float                   fValue;     // The actual value of the port
                bool                    bVirtual;   // Virtual flag

            public:
                explicit CtlParamPort(const meta::port_t *meta, CtlPortChangeHandler *handler, Steinberg::Vst::ParamID id, bool virt) : CtlPort(meta)
                {
                    pHandler            = handler;
                    nID                 = id;
                    fValue              = meta->start;
                    bVirtual            = virt;
                }

                virtual ~CtlParamPort() override
                {
                    pHandler            = NULL;
                }

                CtlParamPort(const CtlParamPort &) = delete;
                CtlParamPort(CtlParamPort &&) = delete;

                CtlParamPort & operator = (const CtlParamPort &) = delete;
                CtlParamPort & operator = (CtlParamPort &&) = delete;

            public:
                inline Steinberg::Vst::ParamID parameter_id() const { return nID; }

            public:
                virtual float value() override  { return fValue; }

                virtual void set_value(float value, size_t flags) override
                {
                    lsp_trace("id=%s, value=%f, handler=%p", pMetadata->id, value, pHandler);
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

        class CtlMeterPort: public CtlPort
        {
            protected:
                float                   fValue;     // The actual value of the port

            public:
                explicit CtlMeterPort(const meta::port_t *meta) : CtlPort(meta)
                {
                    fValue              = meta->start;
                }

                CtlMeterPort(const CtlMeterPort &) = delete;
                CtlMeterPort(CtlMeterPort &&) = delete;

                CtlMeterPort & operator = (const CtlMeterPort &) = delete;
                CtlMeterPort & operator = (CtlMeterPort &&) = delete;

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

        class CtlPortGroup: public CtlParamPort
        {
            protected:
                size_t          nRows;
                size_t          nCols;

            public:
                CtlPortGroup(const meta::port_t *meta, CtlPortChangeHandler *handler, Steinberg::Vst::ParamID id, bool virt):
                    CtlParamPort(meta, handler, id, virt)
                {
                    nRows               = list_size(meta->items);
                    nCols               = port_list_size(meta->members);
                }

                CtlPortGroup(const CtlPortGroup &) = delete;
                CtlPortGroup(CtlPortGroup &&) = delete;

                CtlPortGroup & operator = (const CtlPortGroup &) = delete;
                CtlPortGroup & operator = (CtlPortGroup &&) = delete;

            public:
                inline size_t rows() const      { return nRows; }
                inline size_t cols() const      { return nCols; }
        };

        class CtlPathPort: public CtlPort
        {
            protected:
                CtlPortChangeHandler   *pHandler;
                char                    sPath[MAX_PATH_LEN];

            public:
                explicit CtlPathPort(const meta::port_t *meta, CtlPortChangeHandler *handler):
                    CtlPort(meta)
                {
                    pHandler        = handler;
                }

                virtual ~CtlPathPort()
                {
                    pHandler        = NULL;
                    sPath[0]        = '\0';
                }

                CtlPathPort(const CtlPathPort &) = delete;
                CtlPathPort(CtlPathPort &&) = delete;

                CtlPathPort & operator = (const CtlPathPort &) = delete;
                CtlPathPort & operator = (CtlPathPort &&) = delete;

            public:
                virtual void write(const void* buffer, size_t size, size_t flags) override
                {
                    size    = lsp_min(size, size_t(MAX_PATH_LEN-1));
                    strncpy(sPath, reinterpret_cast<const char *>(buffer), size);
                    sPath[size] = '\0';
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

        class CtlMeshPort: public CtlPort
        {
            protected:
                plug::mesh_t           *pMesh;

            public:
                explicit CtlMeshPort(const meta::port_t *meta) : CtlPort(meta)
                {
                    pMesh               = vst3::create_mesh(meta);
                }

                virtual ~CtlMeshPort() override
                {
                    vst3::destroy_mesh(pMesh);
                    pMesh               = NULL;
                }

                CtlMeshPort(const CtlMeshPort &) = delete;
                CtlMeshPort(CtlMeshPort &&) = delete;

                CtlMeshPort & operator = (const CtlMeshPort &) = delete;
                CtlMeshPort & operator = (CtlMeshPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pMesh;
                }
        };

        class CtlFrameBufferPort: public CtlPort
        {
            protected:
                plug::frame_buffer_t   *pFBuffer;

            public:
                explicit CtlFrameBufferPort(const meta::port_t *meta) : CtlPort(meta)
                {
                    pFBuffer            = plug::frame_buffer_t::create(pMetadata->start, pMetadata->step);
                }

                virtual ~CtlFrameBufferPort() override
                {
                    plug::frame_buffer_t::destroy(pFBuffer);
                    pFBuffer            = NULL;
                }

                CtlFrameBufferPort(const CtlFrameBufferPort &) = delete;
                CtlFrameBufferPort(CtlFrameBufferPort &&) = delete;

                CtlFrameBufferPort & operator = (const CtlFrameBufferPort &) = delete;
                CtlFrameBufferPort & operator = (CtlFrameBufferPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pFBuffer;
                }
        };

        class CtlStreamPort: public CtlPort
        {
            protected:
                plug::stream_t      *pStream;

            public:
                explicit CtlStreamPort(const meta::port_t *meta) : CtlPort(meta)
                {
                    pStream             = plug::stream_t::create(pMetadata->min, pMetadata->max, pMetadata->start);
                }

                virtual ~CtlStreamPort() override
                {
                    plug::stream_t::destroy(pStream);
                    pStream             = NULL;
                }

                CtlStreamPort(const CtlStreamPort &) = delete;
                CtlStreamPort(CtlStreamPort &&) = delete;

                CtlStreamPort & operator = (const CtlStreamPort &) = delete;
                CtlStreamPort & operator = (CtlStreamPort &&) = delete;

            public:
                virtual void *buffer() override
                {
                    return pStream;
                }
        };

    } /* namespace vst3 */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_CTL_PORTS_H_ */
