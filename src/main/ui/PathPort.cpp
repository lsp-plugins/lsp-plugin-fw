/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 апр. 2021 г.
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
        PathPort::PathPort(const meta::port_t *meta, IWrapper *wrapper): IPort(meta)
        {
            sPath[0]    = '\0';
            pWrapper    = wrapper;
        }

        PathPort::~PathPort()
        {
            sPath[0]    = '\0';
            pWrapper    = NULL;
        }

        void PathPort::write(const void* buffer, size_t size)
        {
            // Check that attribute didn't change
            if ((size == strlen(sPath)) && (memcmp(sPath, buffer, size) == 0))
                return;

            if ((buffer != NULL) && (size > 0))
            {
                size_t copy     = (size >= PATH_MAX) ? PATH_MAX-1 : size;
                memcpy(sPath, buffer, size);
                sPath[copy]     = '\0';
            }
            else
                sPath[0]        = '\0';

            // Notify wrapper about configuration change
            if (pWrapper != NULL)
                pWrapper->global_config_changed(this);
        }

        void *PathPort::get_buffer()
        {
            return sPath;
        }

    } /* namespace ctl */
} /* namespace lsp */


