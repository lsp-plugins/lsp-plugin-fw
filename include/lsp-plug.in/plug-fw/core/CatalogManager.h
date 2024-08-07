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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_CATALOGMANAGER_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_CATALOGMANAGER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/plug-fw/core/Catalog.h>

namespace lsp
{
    namespace core
    {

        class CatalogManager
        {
            private:
                core::Catalog      *pCatalog;           // Catalog
                size_t              nCatalogRefs;       // Number of catalog references
                ipc::Mutex          sCatalogMutex;      // Catalog mutex

            protected:
                void                destroy_catalog();

            public:
                CatalogManager();
                CatalogManager(const CatalogManager &) = delete;
                CatalogManager(CatalogManager &&) = delete;
                ~CatalogManager();

                CatalogManager & operator = (const CatalogManager &) = delete;
                CatalogManager & operator = (CatalogManager &&) = delete;

            public:
                core::Catalog      *acquire();
                void                release(core::Catalog *catalog);
        };

    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_CATALOGMANAGER_H_ */
