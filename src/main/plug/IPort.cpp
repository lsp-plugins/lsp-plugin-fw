/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 22 окт. 2015 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace plug
    {
        IPort::IPort(const meta::port_t *meta)
        {
            pMetadata       = meta;
        }

        IPort::~IPort()
        {
        }

        float IPort::value()
        {
            return (pMetadata != NULL) ? pMetadata->start : 0.0f;
        }

        float IPort::default_value() const
        {
            return (pMetadata != NULL) ? pMetadata->start : 0.0f;
        }
    
        void IPort::set_value(float value)
        {
        }

        void IPort::set_default()
        {
            set_value(default_value());
        }

        void *IPort::buffer()
        {
            return NULL;
        }

    } /* namespace plug */
} /* namespace lsp */
