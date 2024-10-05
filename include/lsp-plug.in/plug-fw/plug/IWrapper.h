/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_PLUG_IWRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_PLUG_IWRAPPER_H_

#ifndef LSP_PLUG_IN_PLUG_FW_PLUG_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/plug.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_PLUG_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/plug/data.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/core/ShmState.h>
#include <lsp-plug.in/ipc/IExecutor.h>
#include <lsp-plug.in/resource/ILoader.h>
#include <lsp-plug.in/resource/PrefixLoader.h>

namespace lsp
{
    namespace plug
    {
        class Module;

        /**
         * Plugin wrapper interface
         */
        class IWrapper
        {
            protected:
                plug::Module               *pPlugin;
                resource::ILoader          *pLoader;
                plug::ICanvas              *pCanvas;            // Inline display featured canvas
                plug::position_t            sPosition;          // Actual time position

            protected:
                plug::ICanvas              *create_canvas(size_t width, size_t height);

            public:
                explicit IWrapper(Module *plugin, resource::ILoader *loader);
                IWrapper(const IWrapper &) = delete;
                IWrapper(IWrapper &&) = delete;
                virtual ~IWrapper();

                IWrapper & operator = (const IWrapper &) = delete;
                IWrapper & operator = (IWrapper &&) = delete;

            public:
                /**
                 * Get builtin resource loader
                 * @return builtin resource loader
                 */
                inline resource::ILoader       *resources()         { return pLoader;           }

                /**
                 * Get the wrapped plugin module
                 * @return wrapped plugin module
                 */
                inline plug::Module            *module()            { return pPlugin;           }

                /** Get executor service
                 *
                 * @return executor service
                 */
                virtual ipc::IExecutor         *executor();

                /** Query for inline display drawing
                 *
                 */
                virtual void                    query_display_draw();

                /** Get current time position
                 *
                 * @return current time position
                 */
                const position_t               *position();

                /**
                 * Lock KVT storage and return pointer to the storage,
                 * this is non-RT-safe operation
                 * @return pointer to KVT storage or NULL if KVT is not supported
                 */
                virtual core::KVTStorage       *kvt_lock();

                /**
                 * Try to lock KVT storage and return pointer to the storage on success
                 * @return pointer to KVT storage or NULL
                 */
                virtual core::KVTStorage       *kvt_trylock();

                /**
                 * Release the KVT storage
                 * @return true on success
                 */
                virtual bool                    kvt_release();

                /**
                 * Notify the host about internal state change
                 */
                virtual void                    state_changed();

                /**
                 * Request from plugin to call it's update_settings() method when it is possible
                 */
                virtual void                    request_settings_update();

                /**
                 * Dump the state of plugin
                 */
                virtual void                    dump_plugin_state();

                /**
                 * Get package version
                 * @return package version
                 */
                virtual const meta::package_t  *package() const;

                /**
                 * Get metadata of wrapped plugin
                 * @return metadata of wrapped plugin
                 */
                const meta::plugin_t           *metadata() const;

                /**
                 * Get the plugin format
                 * @return plugin format
                 */
                virtual meta::plugin_format_t   plugin_format() const;

                /**
                 * Get the actual shared memory state (list of connections).
                 * Note that multiple calls may change the result pointer and
                 * invalidate the previously returned pointer.
                 *
                 * @return pointer to actual shared memory state or NULL
                 */
                virtual const core::ShmState   *shm_state();
        };
    } /* namespace plug */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_PLUG_IWRAPPER_H_ */
