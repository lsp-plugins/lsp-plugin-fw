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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_DATA_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_DATA_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>

namespace lsp
{
    namespace lv2
    {
        typedef struct LV2Mesh
        {
            size_t                  nMaxItems;
            size_t                  nBuffers;
            plug::mesh_t           *pMesh;
            uint8_t                *pData;

            LV2Mesh()
            {
                nMaxItems       = 0;
                nBuffers        = 0;
                pMesh           = NULL;
                pData           = NULL;
            }

            ~LV2Mesh()
            {
                // Simply delete root structure
                if (pData != NULL)
                {
                    delete [] (pData);
                    pData       = NULL;
                }
                pMesh       = NULL;
            }

            void init(const meta::port_t *meta, Extensions *ext)
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
        } LV2Mesh;
    } /* namespace lv2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_DATA_H_ */
