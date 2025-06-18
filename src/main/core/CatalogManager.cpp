/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 7 авг. 2024 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/core/CatalogManager.h>


namespace lsp
{
    namespace core
    {

        CatalogManager::CatalogManager()
        {
            pCatalog        = NULL;
            nCatalogRefs    = 0;
        }

        CatalogManager::~CatalogManager()
        {
            destroy_catalog();
        }

        void CatalogManager::destroy_catalog()
        {
            if (pCatalog == NULL)
                return;

            lsp_trace("Destroying shared memory catalog");

            delete pCatalog;
            pCatalog = NULL;
        }

        core::Catalog *CatalogManager::acquire()
        {
            sCatalogMutex.lock();
            lsp_finally { sCatalogMutex.unlock(); };

            if (pCatalog != NULL)
            {
                ++nCatalogRefs;
                return pCatalog;
            }

            core::Catalog *cat = new core::Catalog();
            if (cat == NULL)
                return NULL;
            lsp_finally {
                if (cat != NULL)
                    delete cat;
            };

            // Commit result
            lsp_trace("Created shared memory catalog");
            pCatalog            = release_ptr(cat);
            ++nCatalogRefs;

            return pCatalog;
        }

        void CatalogManager::release(core::Catalog *catalog)
        {
            sCatalogMutex.lock();
            lsp_finally { sCatalogMutex.unlock(); };

            if (catalog != pCatalog)
                return;

            if ((--nCatalogRefs) > 0)
                return;

            destroy_catalog();
        }

    } /* namespace core */
} /* namespace lsp */




