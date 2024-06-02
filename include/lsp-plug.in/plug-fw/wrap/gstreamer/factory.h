/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/wrapper.h>
#include <lsp-plug.in/resource/ILoader.h>

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

namespace lsp
{
    namespace gst
    {
        /**
         * GStreamer plugin factory
         */
        class Factory
        {
            private:
                typedef struct enumeration_t
                {
                    lltl::parray<meta::port_t>  inputs;     // Inputs
                    lltl::parray<meta::port_t>  outputs;    // Outputs
                    lltl::parray<meta::port_t>  params;     // Parameters
                    lltl::parray<meta::port_t>  generated;  // Generated metadata
                } enumeration_t;

            private:
                resource::ILoader      *pLoader;        // Resource loader
                meta::package_t        *pPackage;       // Package manifest
                uatomic_t               nReferences;    // Number of references
                ipc::Mutex              sMutex;         // Mutex for managing factory state
                size_t                  nRefExecutor;   // Number of executor references
                ipc::IExecutor         *pExecutor;      // Executor service

            private:
                static const meta::plugin_t *find_plugin(const char *id);
                static plug::Module        *create_plugin(const char *id);
                static void                 destroy_enumeration(enumeration_t *en);
                static bool                 enumerate_port(enumeration_t *en, const meta::port_t *port, const char *postfix);
                static const meta::port_group_t *find_main_group(const meta::plugin_t *plug, bool in);

            public:
                Factory();
                Factory(const Factory &) = delete;
                Factory(Factory &&) = delete;
                ~Factory();

                Factory & operator = (const Factory &) = delete;
                Factory & operator = (Factory &&) = delete;

                status_t                init();
                void                    destroy();

            public: // Reference counting
                atomic_t                acquire();
                atomic_t                release();

            public: // Resource management
                ipc::IExecutor         *acquire_executor();
                void                    release_executor();
                const meta::package_t  *package() const;

            public: // Interface for GStreamer
                void                    init_class(GstAudioFilterClass *klass, const char *plugin_id);
                Wrapper                *instantiate(const char *plugin_id);

        };
    } /* namespace gst */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_FACTORY_H_ */

