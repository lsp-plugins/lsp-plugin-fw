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
#include <lsp-plug.in/plug-fw/wrap/vst3/ctl_ports.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        class UIPort: public ui::IPort
        {
            protected:
                CtlPort        *pPort;
                uatomic_t       nSerial;

            public:
                explicit UIPort(CtlPort *port): ui::IPort(port->metadata())
                {
                    pPort           = port;
                    nSerial         = port->serial() - 1;
                }

                UIPort(const UIPort &) = delete;
                UIPort(UIPort &&) = delete;

                UIPort & operator = (const UIPort &) = delete;
                UIPort & operator = (UIPort &&) = delete;

            public:
                void                sync()
                {
                    uatomic_t serial    = pPort->serial();
                    if (serial == nSerial)
                        return;

                    nSerial             = serial;
                    ui::IPort::notify_all(ui::PORT_NONE);
                }

            public: // ui::IPort
                virtual void        write(const void *buffer, size_t size) override
                {
                    pPort->write(buffer, size);
                }

                virtual void        write(const void *buffer, size_t size, size_t flags) override
                {
                    pPort->write(buffer, size, flags);
                }

                virtual void       *buffer() override
                {
                    return pPort->buffer();
                }

                virtual float       value() override
                {
                    return pPort->value();
                }

                virtual float       default_value() override
                {
                    return pPort->default_value();
                }

                virtual void        set_default() override
                {
                    return pPort->set_default();
                }

                virtual void        set_value(float value) override
                {
                    lsp_trace("id=%s, value=%f", pMetadata->id, value);
                    pPort->set_value(value);
                }

                virtual void        set_value(float value, size_t flags) override
                {
                    lsp_trace("id=%s, value=%f", pMetadata->id, value);
                    pPort->set_value(value, flags);
                }

                virtual void        notify_all(size_t flags) override
                {
                    nSerial             = pPort->mark_changed();
                    ui::IPort::notify_all(flags);
                }
        };

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_UI_PORTS_H_ */
