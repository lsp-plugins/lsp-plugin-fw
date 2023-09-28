/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/resource/ILoader.h>

#include <ladspa/ladspa.h>

namespace lsp
{
    namespace ladspa
    {
        class Port;
        class AudioPort;

        /**
         * Wrapper for the plugin module
         */
        class Wrapper: public plug::IWrapper
        {
            private:
                Wrapper(const Wrapper &);
                Wrapper & operator = (const Wrapper &);

            protected:
                lltl::parray<ladspa::Port>          vAllPorts;          // All created ports
                lltl::parray<ladspa::AudioPort>     vAudioPorts;        // All available audio ports
                lltl::parray<ladspa::Port>          vExtPorts;          // All ports visible to host

                ipc::IExecutor                     *pExecutor;          // Executor service
                size_t                              nLatencyID;         // ID of Latency port
                LADSPA_Data                        *pLatency;           // Latency output
                bool                                bUpdateSettings;    // Settings update flag
                plug::position_t                    sNewPosition;       // New position
                meta::package_t                    *pPackage;           // Package descriptor

            protected:
                ladspa::Port                       *create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port);
                void                                do_destroy();

            public:
                explicit Wrapper(plug::Module *plugin, resource::ILoader *loader);
                virtual ~Wrapper() override;

                status_t                            init(unsigned long sr);
                void                                destroy();

            public:
                inline void                         activate();

                inline void                         connect(size_t id, void *data);

                inline void                         run(size_t samples);

                inline void                         deactivate();

            public:
                virtual ipc::IExecutor             *executor() override;

                virtual const meta::package_t      *package() const override;

                virtual void                        request_settings_update() override;
        };

    } /* namespace ladspa */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LADSPA_WRAPPER_H_ */
