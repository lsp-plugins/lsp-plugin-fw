/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 мар. 2023 г.
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

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace ui
    {
        ProxyPort::ProxyPort(): ui::IPort(&sMetadata)
        {
            pPort               = NULL;
            sID                 = NULL;

            sMetadata.id        = NULL;
            sMetadata.name      = NULL;
            sMetadata.unit      = meta::U_NONE;
            sMetadata.role      = meta::R_CONTROL;
            sMetadata.flags     = 0;
            sMetadata.min       = 0;
            sMetadata.max       = 0;
            sMetadata.start     = 0;
            sMetadata.step      = 0;
            sMetadata.items     = NULL;
            sMetadata.members   = NULL;
        }

        ProxyPort::~ProxyPort()
        {
            pPort               = NULL;

            if (sID != NULL)
            {
                free(sID);
                sID     = NULL;
            }
        }

        status_t ProxyPort::init(const char *id, ui::IPort *proxied)
        {
            sID             = strdup(id);
            if (sID == NULL)
                return STATUS_NO_MEM;

            pPort           = proxied;
            pPort->bind(this);

            sMetadata       = *(proxied->metadata());
            sMetadata.id    = sID;

            return STATUS_OK;
        }

        void ProxyPort::set_proxy_port(ui::IPort *proxied)
        {
            if (pPort == proxied)
                return;

            if (pPort != NULL)
                pPort->unbind(this);

            pPort           = proxied;
            pPort->bind(this);

            sMetadata       = *(proxied->metadata());
            sMetadata.id    = sID;

            // Notify about port change
            IPort::notify_all();
        }

        void ProxyPort::write(const void *buffer, size_t size)
        {
            pPort->write(buffer, size);
        }

        void ProxyPort::write(const void *buffer, size_t size, size_t flags)
        {
            pPort->write(buffer, size, flags);
        }

        void *ProxyPort::buffer()
        {
            return pPort->buffer();
        }

        float ProxyPort::value()
        {
            return pPort->value();
        }

        float ProxyPort::default_value()
        {
            return pPort->default_value();
        }

        void ProxyPort::set_default()
        {
            pPort->set_default();
        }

        void ProxyPort::set_value(float value)
        {
            pPort->set_value(value);
        }

        void ProxyPort::set_value(float value, size_t flags)
        {
            pPort->set_value(value, flags);
        }

        void ProxyPort::notify_all()
        {
            pPort->notify_all();
            IPort::notify_all();
        }

        void ProxyPort::sync_metadata()
        {
            pPort->sync_metadata();
        }

        const char *ProxyPort::id() const
        {
            return sID;
        }

        void ProxyPort::notify(IPort *port)
        {
            if (port == pPort)
                IPort::notify_all();
        }

    } /* namespace ui */
} /* namespace lsp */


