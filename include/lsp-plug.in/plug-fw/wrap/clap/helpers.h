/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 янв. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_HELPERS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_HELPERS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace clap
    {
        /**
         * Perform the string copy with guaranteed string termination at the end
         * @param dst destination buffer
         * @param src source buffer
         * @param len length of the buffer
         * @return pointer to destination buffer
         */
        inline char *clap_strcpy(char *dst, const char *src, size_t len)
        {
            strncpy(dst, src, len);
            dst[len-1] = '\0';
            return dst;
        }

        /**
         * Hash the string value and return the hash value as a clap identifier
         * @param str string to hash
         * @return clap identifier as a result of hashing
         */
        inline clap_id clap_hash_string(const char *str)
        {
            constexpr size_t num_primes = 8;
            static const uint16_t primes[num_primes] = {
                0x80ab, 0x815f, 0x8d41, 0x9161,
                0x9463, 0x9b77, 0xabc1, 0xb567,
            };

            size_t prime_id = 0;
            size_t len      = strlen(str);
            clap_id res     = len * primes[prime_id];

            for (size_t i=0; i<len; ++i)
            {
                prime_id        = (prime_id + 1) % num_primes;
                res             = clap_id(res << 7) | clap_id((res >> (sizeof(clap_id) * 8 - 7)) & 0x7f); // rotate 7 bits left
                res            += str[i] * primes[prime_id];
            }

            return res;
        }

        inline plug::mesh_t *create_mesh(const meta::port_t *meta)
        {
            size_t buffers      = meta->step;
            size_t buf_size     = meta->start * sizeof(float);
            size_t mesh_size    = sizeof(plug::mesh_t) + sizeof(float *) * buffers;

            // Align values to 64-byte boundaries
            buf_size            = align_size(buf_size, 0x40);
            mesh_size           = align_size(mesh_size, 0x40);

            // Allocate pointer
            uint8_t *ptr        = static_cast<uint8_t *>(malloc(mesh_size + buf_size * buffers));
            if (ptr == NULL)
                return NULL;

            // Initialize references
            plug::mesh_t *mesh  = reinterpret_cast<plug::mesh_t *>(ptr);
            mesh->nState        = plug::M_EMPTY;
            mesh->nBuffers      = 0;
            mesh->nItems        = 0;
            ptr                += mesh_size;
            for (size_t i=0; i<buffers; ++i)
            {
                mesh->pvData[i]     = reinterpret_cast<float *>(ptr);
                ptr                += buf_size;
            }

            return mesh;
        }

        inline void destroy_mesh(plug::mesh_t *mesh)
        {
            if (mesh != NULL)
                free(mesh);
        }

    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_HELPERS_H_ */
