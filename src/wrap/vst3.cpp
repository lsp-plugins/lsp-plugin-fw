/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
  * This file is part of lsp-plugin-fw
 * Created on: 26 дек. 2023 г.
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

#include <steinberg/vst3.h>
#include <steinberg/vst3_static.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/singletone.h>
#include <lsp-plug.in/common/static.h>
#include <lsp-plug.in/common/types.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/impl/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/impl/wrapper.h>

#define VST3_LOG_FILE   "lsp-vst3.log"

namespace lsp
{
    namespace vst3
    {
        static volatile atomic_t entry_counter = 0;
        static lsp::singletone_t library;
        static PluginFactory *plugin_factory = NULL;

        bool initialize_library()
        {
            return true;
        }

        void finalize_library()
        {
        }

        Steinberg::IPluginFactory *get_plugin_factory()
        {
            if (!library.initialized())
            {
                // Create new factory and set trigger for disposal
                PluginFactory *factory      = new PluginFactory();
                if (factory == NULL)
                    return NULL;
                lsp_finally {
                    if (factory != NULL)
                    {
                        factory->destroy();
                        delete factory;
                    }
                };
                lsp_trace("created factory %p", factory);

                // Initialize factory
                status_t res    = factory->init();
                if (res != STATUS_OK)
                    return NULL;
                lsp_trace("initialized factory %p", factory);

                // Commit the factory
                lsp_singletone_init(library) {
                    lsp::swap(plugin_factory, factory);
                };
            }

            // Return the obtained factory
            Steinberg::IPluginFactory *result = safe_acquire<Steinberg::IPluginFactory>(plugin_factory);
            lsp_trace("returning factory %p", result);

            return result;
        }

        void drop_factory()
        {
            lsp_trace("releasing plugin factory %p", plugin_factory);
            safe_release(plugin_factory);
        };

        //---------------------------------------------------------------------
        // Static finalizer for the list of descriptors at library finalization
        static StaticFinalizer finalizer(drop_factory);
    } /* namespace vst3 */
} /* namespace lsp */

extern "C"
{
    STMG_INIT_FUNCTION
    {
        if (lsp::atomic_add(&lsp::vst3::entry_counter, 1) == 0)
            return lsp::vst3::initialize_library();
        return true;
    }

    STMG_FINI_FUNCTION
    {
        lsp::atomic_t res = lsp::atomic_add(&lsp::vst3::entry_counter, -1);
        if (res <= 0)
            return false;
        else if (res == 1)
            lsp::vst3::finalize_library();
        return true;
    }

    STMG_GET_PLUGIN_FACTORY_FUNCTION
    {
        return lsp::vst3::get_plugin_factory();
    }
} /* extern "C" */
