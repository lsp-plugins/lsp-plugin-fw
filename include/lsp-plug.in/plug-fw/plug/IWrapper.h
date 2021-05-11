/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/ipc/IExecutor.h>
#include <lsp-plug.in/resource/ILoader.h>

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
            private:
                IWrapper & operator = (const IWrapper &);

            protected:
                Module             *pPlugin;
                resource::ILoader  *pLoader;

            public:
                explicit IWrapper(Module *plugin, resource::ILoader *loader);
                virtual ~IWrapper();

            public:
                /**
                 * Get builtin resource loader
                 * @return builtin resource loader
                 */
                inline resource::ILoader   *resources()         { return pLoader;       }

                /** Get executor service
                 *
                 * @return executor service
                 */
                virtual ipc::IExecutor     *executor();

                /** Query for inline display drawing
                 *
                 */
                virtual void                query_display_draw();

                /** Get current time position
                 *
                 * @return current time position
                 */
                virtual const position_t   *position();

                /**
                 * Create/resize canvas
                 * @param cv valid pointer to ICanvas object or NULL if there's none
                 * @param width the requested width of canvas
                 * @param height the requested height of canvas
                 * @return the pointer to valid canvas object or NULL on error
                 */
                virtual ICanvas            *create_canvas(ICanvas *&cv, size_t width, size_t height);

                /**
                 * Lock KVT storage and return pointer to the storage,
                 * this is non-RT-safe operation
                 * @return pointer to KVT storage or NULL if KVT is not supported
                 */
                virtual core::KVTStorage   *kvt_lock();

                /**
                 * Try to lock KVT storage and return pointer to the storage on success
                 * @return pointer to KVT storage or NULL
                 */
                virtual core::KVTStorage   *kvt_trylock();

                /**
                 * Release the KVT storage
                 * @return true on success
                 */
                virtual bool                kvt_release();

                /**
                 * Notify the host about internal state change
                 */
                virtual void                state_changed();

                /**
                 * Dump the state of plugin
                 */
                virtual void                dump_plugin_state();

                /**
                 * Get package version
                 * @return package version
                 */
                virtual const meta::package_t  *package() const;
        };
    }
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_PLUG_IWRAPPER_H_ */
