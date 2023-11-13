/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

namespace lsp
{
    namespace ui
    {
        IPort::IPort(const meta::port_t *meta)
        {
            pMetadata       = meta;
        }

        IPort::~IPort()
        {
            vListeners.flush();
        }

        void IPort::bind(IPortListener *listener)
        {
            vListeners.put(listener);
        }

        void IPort::unbind(IPortListener *listener)
        {
            vListeners.remove(listener);
        }

        void IPort::unbind_all()
        {
            vListeners.flush();
        }

        void IPort::write(const void *buffer, size_t size)
        {
        }

        void IPort::write(const void *buffer, size_t size, size_t flags)
        {
            write(buffer, size);
        }

        void *IPort::buffer()
        {
            return NULL;
        }

        float IPort::value()
        {
            return 0.0f;
        }

        float IPort::default_value()
        {
            return (pMetadata != NULL) ? pMetadata->start : 0.0f;
        }

        void IPort::set_default()
        {
            set_value(default_value());
        }

        void IPort::set_value(float value)
        {
        }

        void IPort::set_value(float value, size_t flags)
        {
            set_value(value);
        }

        const char *IPort::id() const
        {
            return (pMetadata != NULL) ? pMetadata->id : NULL;
        }

        void IPort::notify_all(size_t flags)
        {
            // Prevent from modifying list of listeners at the sync stage
            lltl::parray<IPortListener> listeners;
            if (!vListeners.values(&listeners))
                return;

            // Call notify() for all listeners in the list
            size_t count = listeners.size();
            for (size_t i=0; i<count; ++i)
                listeners.uget(i)->notify(this, flags);
        }

        void IPort::sync_metadata()
        {
            // Prevent from modifying list of listeners at the sync stage
            lltl::parray<IPortListener> listeners;
            if (!vListeners.values(&listeners))
                return;

            // Call sync_metadata() for all listeners in the list
            size_t count = listeners.size();
            for (size_t i=0; i<count; ++i)
                listeners.uget(i)->sync_metadata(this);
        }
    } /* namespace ctl */
} /* namespace lsp */


