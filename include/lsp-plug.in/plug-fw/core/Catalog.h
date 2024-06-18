/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 17 июн. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_CATALOG_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_CATALOG_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/dsp-units/shared/Catalog.h>

namespace lsp
{
    namespace core
    {
        class ICatalogClient;

        /**
         * Catalog manager
         */
        class Catalog: public ipc::IRunnable
        {
            private:
                friend class ICatalogClient;

            private:
                dspu::Catalog           sCatalog;
                ipc::Mutex              sThread;
                ipc::Mutex              sMutex;
                ipc::Thread            *pThread;
                lltl::parray<ICatalogClient>  vClients;

            protected:
                status_t            add_client(ICatalogClient *client);
                status_t            remove_client(ICatalogClient *client);
                bool                process_events();
                void                sync_catalog();
                size_t              process_apply();
                size_t              process_update();

            public:
                explicit Catalog();
                Catalog(const Catalog &) = delete;
                Catalog(Catalog &&) = delete;
                virtual ~Catalog() override;

                Catalog & operator = (const Catalog &) = delete;
                Catalog & operator = (Catalog &) = delete;

            public: // ipc::IRunnable
                virtual status_t    run() override;
        };

    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_CATALOG_H_ */
