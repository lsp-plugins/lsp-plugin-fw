/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 июн. 2024 г.
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

#include <lsp-plug.in/plug-fw/core/Catalog.h>
#include <lsp-plug.in/plug-fw/core/ICatalogClient.h>

namespace lsp
{
    namespace core
    {

        Catalog::Catalog()
        {
            pThread         = NULL;
        }

        Catalog::~Catalog()
        {
        }

        bool Catalog::open_catalog()
        {
            LSPString catalog_name;
            status_t res    = system::get_user_login(&catalog_name);
            if (res != STATUS_OK)
                return false;

            if (!catalog_name.prepend_ascii("lsp-catalog-"))
                return false;

            return sCatalog.open(&catalog_name, 8192) == STATUS_OK;
        }

        status_t Catalog::run()
        {
            while (!ipc::Thread::is_cancelled())
            {
                // Ensure that catalog is opened
                if (!sCatalog.opened())
                {
                    if (!open_catalog())
                        ipc::Thread::sleep(100);
                }

                // Process change requests
                if (!process_events())
                {
                    // Perform short sleep
                    ipc::Thread::sleep(50);
                }
            }

            // Disconnect from catalog
            if (sCatalog.opened())
                sCatalog.close();

            return STATUS_OK;
        }

        bool Catalog::process_events()
        {
            sync_catalog();

            size_t processed = process_update();
            processed += process_apply();

            return processed > 0;
        }

        void Catalog::sync_catalog()
        {
            if (!sCatalog.sync())
                return;

            // Lock the state
            if (!sMutex.lock())
                return;
            lsp_finally {
                sMutex.unlock();
            };

            // Iterate over the clients and mark them for update
            for (lltl::iterator<ICatalogClient> it = vClients.values(); it; ++it)
            {
                ICatalogClient *c = it.get();
                if (c != NULL)
                    atomic_add(&c->sUpdate.nRequest, 1);
            }
        }

        size_t Catalog::process_update()
        {
            size_t count = 0;

            // Lock the state
            if (!sMutex.lock())
                return count;
            lsp_finally {
                sMutex.unlock();
            };

            // Iterate over the clients and try to update them
            for (lltl::iterator<ICatalogClient> it = vClients.values(); it; ++it)
            {
                ICatalogClient *c = it.get();
                if (c == NULL)
                    continue;

                // Call the client to update
                const uint32_t response     = atomic_load(&c->sUpdate.nRequest);
                if (response == c->sUpdate.nResponse)
                    continue;
                ++count;

                // Commit only if update was successful
                if (c->update(&sCatalog))
                    c->sUpdate.nResponse    = response;
            }

            return count;
        }

        size_t Catalog::process_apply()
        {
            size_t count = 0;

            // Lock the state
            if (!sMutex.lock())
                return count;
            lsp_finally {
                sMutex.unlock();
            };

            // Iterate over the clients
            for (lltl::iterator<ICatalogClient> it = vClients.values(); it; ++it)
            {
                ICatalogClient *c = it.get();
                if (c == NULL)
                    continue;

                // Do not call apply if update is pending
                if (atomic_load(&c->sUpdate.nRequest) != c->sUpdate.nResponse)
                {
                    ++count;
                    continue;
                }

                // Call the client to apply changes
                const uint32_t response = atomic_load(&c->sApply.nRequest);
                if (response == c->sApply.nResponse)
                    continue;
                ++count;

                if (c->apply(&sCatalog))
                    c->sApply.nResponse = response;
            }

            return count;
        }

        status_t Catalog::attach_client(ICatalogClient *client)
        {
            // Lock thread mutex
            if (!sThread.lock())
                return STATUS_UNKNOWN_ERR;
            lsp_finally {
                sThread.unlock();
            };

            // Add client
            {
                // Lock data structures
                if (!sMutex.lock())
                    return STATUS_UNKNOWN_ERR;
                lsp_finally {
                    sMutex.unlock();
                };

                // Check that client is not connected already
                if (vClients.contains(client))
                    return STATUS_ALREADY_BOUND;

                // Add client to the list of clients
                if (!vClients.add(client))
                    return STATUS_NO_MEM;

                // Force the client to update
                client->request_update();
                const uint32_t response     = atomic_load(&client->sUpdate.nRequest);

                // Commit only if update was successful
                if (client->update(&sCatalog))
                    client->sUpdate.nResponse   = response;
            }

            // Check that we have a dispatcher tread running
            if (pThread != NULL)
                return STATUS_OK;

            // Start the dispatcher thread
            pThread         = new ipc::Thread(this);
            status_t res    = (pThread != NULL) ? pThread->start() : STATUS_NO_MEM;
            if (res != STATUS_OK)
            {
                if (pThread != NULL)
                    delete pThread;
                vClients.qpremove(client);
            }

            return res;
        }

        status_t Catalog::detach_client(ICatalogClient *client)
        {
            // Lock thread mutex
            if (!sThread.lock())
                return STATUS_UNKNOWN_ERR;
            lsp_finally {
                sThread.unlock();
            };

            // Remove client
            {
                // Lock data structures
                if (!sMutex.lock())
                    return STATUS_UNKNOWN_ERR;
                lsp_finally {
                    sMutex.unlock();
                };

                // Remove client
                if (!vClients.qpremove(client))
                    return STATUS_NOT_BOUND;

                if (!vClients.is_empty())
                    return STATUS_OK;
            }

            // Check that we don't have a dispatcher tread running
            if (pThread == NULL)
                return STATUS_OK;

            // Stop the dispatcher thread
            if (pThread != NULL)
            {
                pThread->cancel();
                pThread->join();
            }

            return STATUS_OK;
        }
    } /* namespace core */
} /* namespace lsp */


