/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_TYPES_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_TYPES_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/plug-fw/plug.h>

#include <lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

namespace lsp
{
    namespace lv2
    {
        #define LSP_LV2_BASE_URI            "http://lsp-plug.in/"
        #define LSP_LV2_TYPES_URI           LSP_LV2_BASE_URI "types/lv2"
        #define LSP_LV2_UI_URI              LSP_LV2_BASE_URI "ui/lv2"
        #define LSP_LV2_KVT_URI             LSP_LV2_BASE_URI "ui/kvt"

        #define LSP_LV2_ATOM_KEY_SIZE       (sizeof(uint32_t) * 2)
        #define LSP_LV2_SIZE_PAD(size)      ::lsp::align_size((size + 0x200), 0x200)


        struct Extensions;

        enum
        {
            LSP_LV2_PRIVATE     = 1 << 0
        };

        /**
         * Mesh wrapper for LV2
         */
        typedef struct lv2_mesh_t
        {
            size_t                  nMaxItems;
            size_t                  nBuffers;
            plug::mesh_t           *pMesh;
            uint8_t                *pData;

            lv2_mesh_t()
            {
                nMaxItems       = 0;
                nBuffers        = 0;
                pMesh           = NULL;
                pData           = NULL;
            }

            ~lv2_mesh_t()
            {
                // Simply delete root structure
                if (pData != NULL)
                {
                    delete [] (pData);
                    pData       = NULL;
                }
                pMesh       = NULL;
            }

            void init(const meta::port_t *meta)
            {
                // Calculate sizes
                nBuffers            = meta->step;
                nMaxItems           = meta->start;

                size_t hdr_size     = align_size(sizeof(plug::mesh_t) + sizeof(float *) * nBuffers, DEFAULT_ALIGN);
                size_t urid_size    = align_size(sizeof(LV2_URID) * nBuffers, DEFAULT_ALIGN);
                size_t buf_size     = align_size(sizeof(float) * nMaxItems, DEFAULT_ALIGN);
                size_t to_alloc     = hdr_size + urid_size + buf_size * nBuffers;

                lsp_trace("buffers = %d, max_items=%d, hdr_size=%d, urid_size=%d, buf_size=%d, to_alloc=%d",
                        int(nBuffers), int(nMaxItems), int(hdr_size), int(urid_size), int(buf_size), int(to_alloc));
                pData               = new uint8_t[to_alloc + DEFAULT_ALIGN];
                if (pData == NULL)
                    return;
                uint8_t *ptr        = align_ptr(pData, DEFAULT_ALIGN);
                pMesh               = reinterpret_cast<plug::mesh_t *>(ptr);
                ptr                += hdr_size;

                lsp_trace("ptr = %p, pMesh = %p", ptr, pMesh);

                for (size_t i=0; i<nBuffers; ++i)
                {
                    lsp_trace("bufs[%d] = %p", int(i), ptr);
                    pMesh->pvData[i]    = reinterpret_cast<float *>(ptr);
                    ptr                += buf_size;
                }

                lsp_assert(ptr <= &pData[to_alloc + DEFAULT_ALIGN]);

                pMesh->nState       = plug::M_WAIT;
                pMesh->nBuffers     = 0;
                pMesh->nItems       = 0;

                lsp_trace("Initialized");
            }

            static size_t size_of_port(const meta::port_t *meta)
            {
                size_t hdr_size     = sizeof(LV2_Atom_Int) + sizeof(LV2_Atom_Int) + 0x100; // Some extra bytes
                size_t prop_size    = sizeof(uint32_t) * 2;
                size_t vector_size  = prop_size + sizeof(LV2_Atom_Vector) + meta->start * sizeof(float);

                return LSP_LV2_SIZE_PAD(size_t(hdr_size + vector_size * meta->step));
            }
        } lv2_mesh_t;

        /**
         * Path wrapper for LV2
         */
        typedef struct lv2_path_t: public plug::path_t
        {
            enum flags_t
            {
                S_EMPTY,
                S_PENDING,
                S_ACCEPTED
            };

            atomic_t    nRequest;
            atomic_t    nChanges;
            size_t      nState;
            size_t      nFlags;
            bool        bRequest;
            size_t      sFlags;
            char        sPath[PATH_MAX];
            char        sRequest[PATH_MAX];

            virtual void init()
            {
                atomic_init(nRequest);
                nState      = S_EMPTY;
                nFlags      = 0;
                bRequest    = false;
                sFlags      = 0;
                sPath[0]    = '\0';
                sRequest[0] = '\0';
            }

            virtual const char *get_path()
            {
                return sPath;
            }

            virtual size_t get_flags()
            {
                return nFlags;
            }

            virtual void accept()
            {
                if (nState != S_PENDING)
                    return;
                atomic_add(&nChanges, 1);
                nState  = S_ACCEPTED;
            }

            virtual void commit()
            {
                if (nState != S_ACCEPTED)
                    return;
                nState  = S_EMPTY;
            }

            virtual bool pending()
            {
                // Check accepted state
                if (nState == S_PENDING)
                    return true;
                else if ((nState != S_EMPTY) || (!bRequest))
                    return false;

                // Move pending request to path if present,
                // do it in spin-lock synchronized mode
                if (atomic_trylock(nRequest))
                {
                    // Copy the data
                    ::strncpy(sPath, sRequest, PATH_MAX);
                    sPath[PATH_MAX-1]   = '\0';
                    sRequest[0]         = '\0';
                    nFlags              = sFlags;
                    sFlags              = 0;
                    bRequest            = false;
                    nState              = S_PENDING;

                    atomic_unlock(nRequest);
                }

                return nState == S_PENDING;
            }

            virtual bool accepted()
            {
                return (nState == S_ACCEPTED);
            }

            /**
             * This is non-RT-safe method to submit new path value to the RT thread
             * @param path path string to submit
             * @param len length of the path string
             * @param flags additional flags
             */
            void submit(const char *path, size_t len, size_t flags = 0)
            {
                // Determine size of path
                size_t count = (len >= PATH_MAX) ? PATH_MAX - 1 : len;

                // Wait until the queue is empty
                while (true)
                {
                    // Try to acquire critical section, this will always be true when using LV2 atom transport
                    if (atomic_trylock(nRequest))
                    {
                        // Copy data to request
                        ::memcpy(sRequest, path, count);
                        sRequest[count]     = '\0';
                        sFlags              = flags;
                        bRequest            = true; // Mark request pending

                        // Release critical section and leave the cycle
                        atomic_unlock(nRequest);
                        break;
                    }

                    // Wait for a while, this won't happen when lv2
                    ipc::Thread::sleep(10);
                }
            }

        } lv2_path_t;
    } /* namespace lv2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_TYPES_H_ */
