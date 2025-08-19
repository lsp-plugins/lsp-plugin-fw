/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 7 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_HELPERS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_HELPERS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/plug.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/string.h>


namespace lsp
{
    namespace vst2
    {
        inline plug::mesh_t *create_mesh(const meta::port_t *meta)
        {
            const size_t buffers    = meta->step;
            const size_t buf_size   = align_size(meta->start * sizeof(float), 0x40);
            const size_t mesh_size  = align_size(sizeof(plug::mesh_t) + sizeof(float *) * buffers, 0x40);

            // Allocate pointer
            uint8_t *ptr            = static_cast<uint8_t *>(malloc(mesh_size + buf_size * buffers));
            if (ptr == NULL)
                return NULL;

            // Initialize mesh
            plug::mesh_t *mesh      = advance_ptr_bytes<plug::mesh_t>(ptr, mesh_size);
            mesh->init(reinterpret_cast<float *>(ptr), buffers, buf_size / sizeof(float));

            return mesh;
        }

        inline void destroy_mesh(plug::mesh_t *mesh)
        {
            if (mesh != NULL)
                free(mesh);
        }

        inline ssize_t serialize_string(const char *str, uint8_t *buf, size_t len)
        {
            lsp_trace("str=%s, buf=%p, len=%d", str, buf, int(len));
            size_t slen     = strlen(str);
            if (slen > 0xff)
                slen            = 0xff;
            if ((slen + 1) > len)
                return -1;
            *(buf++)        = slen;
            memcpy(buf, str, slen);
            return slen + 1;
        }

        inline ssize_t deserialize_string(char *str, size_t maxlen, const uint8_t *buf, size_t len)
        {
            lsp_trace("str=%p, maxlen=%d, buf=%p, len=%d", str, int(maxlen), buf, int(len));
            if ((len--) <= 0)
                return -1;
            size_t slen     = *(buf++);
            if (slen > len)
                return -1;
            if ((slen + 1) > maxlen)
                return -2;
            memcpy(str, buf, slen);
            str[slen]       = '\0';

            lsp_trace("str=%s", str);
            return slen + 1;
        }
    } /* namespace lsp */
} /* namespace vst2 */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_HELPERS_H_ */
