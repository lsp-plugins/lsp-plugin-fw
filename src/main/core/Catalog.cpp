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
        }

        Catalog::~Catalog()
        {
        }

        status_t Catalog::run()
        {
            while (!cancelled())
            {
                // Ensure that catalog is opened
                if (!sCatalog.opened())
                {
                    status_t res    = sCatalog.open("lsp-catalog.shm", 8192);
                    if (res != STATUS_OK)
                    {
                        Thread::sleep(100);
                        continue;
                    }
                }

                // Process change requests
                while (process_events())
                {
                    // Nothing
                }

                // Perform small sleep
                Thread::sleep(50);
            }

            // Disconnect from catalog
            if (sCatalog.opened())
                sCatalog.close();

            return STATUS_OK;
        }

        bool Catalog::process_events()
        {
            size_t processed = process_requests();
            processed += process_changes();
            return  processed > 0;
        }

        size_t Catalog::process_requests()
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

                // Call the client to apply changes
                const uint32_t response = c->nRequest;
                if (response != c->nResponse)
                {
                    c->apply();
                    c->nResponse = response;
                    ++count;
                }
            }

            return count;
        }

        size_t Catalog::process_changes()
        {
            size_t count = 0;

            if (!sCatalog.sync())
                return count;

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

                // Call the client to update
                const uint32_t response = c->nRequest;
                if (response != c->nResponse)
                {
                    c->update();
                    c->nResponse = response;
                    ++count;
                }
            }

            return count;
        }

        status_t Catalog::add_client(ICatalogClient *client)
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
            return (vClients.add(client)) ? STATUS_OK : STATUS_NO_MEM;
        }

        status_t Catalog::remove_client(ICatalogClient *client)
        {
            // Lock data structures
            if (!sMutex.lock())
                return STATUS_UNKNOWN_ERR;
            lsp_finally {
                sMutex.unlock();
            };

            // Just remove client
            return (vClients.qpremove(client)) ? STATUS_OK : STATUS_NOT_BOUND;
        }
    } /* namespace core */
} /* namespace lsp */


