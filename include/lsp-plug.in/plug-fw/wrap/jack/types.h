/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_TYPES_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_TYPES_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/alloc.h>

#define JACK_INLINE_DISPLAY_SIZE        128

namespace lsp
{
    namespace jack
    {
        typedef struct path_t: public plug::path_t
        {
            enum flags_t
            {
                F_PENDING       = 1 << 0,
                F_ACCEPTED      = 1 << 1
            };

            atomic_t    nRequest;
            atomic_t    nSerial;
            atomic_t    nCommit;

            size_t      nFlags;
            size_t      nXFlags;
            size_t      nXFlagsReq;

            char        sPath[PATH_MAX];
            char        sRequest[PATH_MAX];

            virtual void init()
            {
                atomic_init(nRequest);
                nSerial         = 0;
                nCommit         = 0;

                nFlags          = 0;
                nXFlags         = 0;
                nXFlagsReq      = 0;

                sPath[0]        = '\0';
                sRequest[0]     = '\0';
            }

            virtual const char *path() const
            {
                return sPath;
            }

            virtual size_t flags() const
            {
                return nXFlags;
            }

            virtual void accept()
            {
                if (nFlags & F_PENDING)
                    nFlags     |= F_ACCEPTED;
            }

            virtual void commit()
            {
                if (nFlags & (F_PENDING | F_ACCEPTED))
                    nFlags      = 0;
            }

            virtual bool pending()
            {
                // Check accepted flags
                if (nFlags & F_PENDING)
                    return !(nFlags & F_ACCEPTED);

                // Check for pending change
                if (atomic_trylock(nRequest))
                {
                    // Update state of the DSP
                    if (nSerial != nCommit)
                    {
                        // Copy the data
                        ::strncpy(sPath, sRequest, PATH_MAX);
                        sPath[PATH_MAX-1]   = '\0';
                        nFlags              = F_PENDING;
                        nXFlags             = nXFlagsReq;
                        nXFlagsReq          = 0;

                        // Update commit
                        nCommit             ++;
                    }

                    atomic_unlock(nRequest);
                }

                return (nFlags & F_PENDING);
            }

            virtual bool accepted()
            {
                return nFlags & F_ACCEPTED;
            }

            void submit(const char *path, size_t flags)
            {
                // Wait until the queue is empty
                while (true)
                {
                    // Try to acquire critical section
                    if (atomic_trylock(nRequest))
                    {
                        // Write request to DSP
                        ::strcpy(sRequest, path);
                        nXFlagsReq      = flags;
                        nSerial         ++;

                        // Release critical section and leave the cycle
                        atomic_unlock(nRequest);
                        break;
                    }

                    // Wait for a while (10 milliseconds)
                    ipc::Thread::sleep(10);
                }
            }

        } path_t;

        inline plug::mesh_t *create_mesh(const meta::port_t *meta)
        {
            size_t buffers      = meta->step;
            size_t buf_size     = meta->start * sizeof(float);
            size_t mesh_size    = sizeof(plug::mesh_t) + sizeof(float *) * buffers;

            // Align values to 64-byte boundaries
            buf_size            = align_size(buf_size, OPTIMAL_ALIGN);
            mesh_size           = align_size(mesh_size, OPTIMAL_ALIGN);

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
                mesh->pvData[i]    = reinterpret_cast<float *>(ptr);
                ptr                += buf_size;
            }

            return mesh;
        }

        inline void destroy_mesh(plug::mesh_t *mesh)
        {
            if (mesh != NULL)
                free(mesh);
        }
    }
}


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_TYPES_H_ */
