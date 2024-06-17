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
        ICatalogClient::ICatalogClient()
        {
            pCatalog        = NULL;
            nRequest        = 0;
            nResponse       = 0;
        }

        ICatalogClient::~ICatalogClient()
        {
            do_close();
        }

        status_t ICatalogClient::do_close()
        {
            status_t res = (pCatalog != NULL) ? pCatalog->remove_client(this) : STATUS_NOT_BOUND;
            pCatalog = NULL;
            return res;
        }

        status_t ICatalogClient::connect(Catalog *catalog)
        {
            if (catalog == NULL)
                return STATUS_BAD_ARGUMENTS;
            if (pCatalog != NULL)
                return STATUS_ALREADY_BOUND;

            // Mark for update
            const uint32_t response = nResponse;
            nResponse               = nRequest - 1;

            // Connect and verify result
            status_t res = catalog->add_client(this);
            if (res != STATUS_OK)
                nResponse               = response;

            return res;
        }

        status_t ICatalogClient::close()
        {
            return do_close();
        }

        void ICatalogClient::request()
        {
            atomic_add(&nRequest, 1);
        }

        void ICatalogClient::update()
        {
        }

        void ICatalogClient::apply()
        {
        }

        bool ICatalogClient::connected() const
        {
            return pCatalog     != NULL;
        }

    } /* namespace core */
} /* namespace lsp */






