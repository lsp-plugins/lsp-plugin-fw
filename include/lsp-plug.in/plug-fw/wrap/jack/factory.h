/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 6 авг. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/plug-fw/core/Catalog.h>
#include <lsp-plug.in/plug-fw/core/CatalogManager.h>
#include <lsp-plug.in/plug-fw/plug.h>

#ifdef WITH_UI_FEATURE
    #include <lsp-plug.in/plug-fw/ui.h>
#endif /* WITH_UI_FEATURE */

namespace lsp
{
    namespace jack
    {
        class Factory
        {
            private:
                uatomic_t               nReferences;        // Number of references
                core::CatalogManager    sCatalogManager;    // Catalog management

            public:
                Factory();
                Factory(const Factory &) = delete;
                Factory(Factory &&) = delete;
                Factory & operator = (const Factory &) = delete;
                Factory & operator = (Factory &&) = delete;
                ~Factory();

            public:
                size_t              acquire();
                size_t              release();

            public:
                status_t create_plugin(plug::Module **module, const char *id);
            #ifdef WITH_UI_FEATURE
                status_t create_ui(ui::Module **ui, const char *id);
            #endif /* WITH_UI_FEATURE */

                core::Catalog      *acquire_catalog();
                void                release_catalog(core::Catalog *catalog);
        };

    } /* namespace jack */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_FACTORY_H_ */
