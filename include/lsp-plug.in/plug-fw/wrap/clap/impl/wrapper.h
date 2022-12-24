/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 дек. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_WRAPPER_H_

#include <clap/clap.h>
#include <lsp-plug.in/plug-fw/wrap/clap/wrapper.h>

namespace lsp
{
    namespace clap
    {
        Wrapper::Wrapper(
            plug::Module *module,
            const meta::package_t *package,
            resource::ILoader *loader,
            const clap_host_t *host):
            IWrapper(module, loader)
        {
            pHost           = host;
            pPackage        = package;
        }

        Wrapper::~Wrapper()
        {
        }

        status_t Wrapper::init()
        {
            // TODO
            return STATUS_OK;
        }

        void Wrapper::destroy()
        {
            // Destroy plugin
            if (pPlugin != NULL)
            {
                pPlugin->destroy();
                delete pPlugin;
                pPlugin         = NULL;
            }

            // Destroy the loader
            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader         = NULL;
            }

            // Release pointers
            pHost           = NULL;
            pPackage        = NULL;
        }

        status_t Wrapper::activate(double sample_rate, uint32_t min_frames_count, uint32_t max_frames_count)
        {
            return STATUS_OK;
        }

        void Wrapper::deactivate()
        {
        }

        status_t Wrapper::start_processing()
        {
            return STATUS_OK;
        }

        void Wrapper::stop_processing()
        {
        }

        void Wrapper::reset()
        {
        }

        clap_process_status Wrapper::process(const clap_process_t *process)
        {
            return CLAP_PROCESS_CONTINUE;
        }

        const void *Wrapper::get_extension(const char *id)
        {
            return NULL;
        }

        void Wrapper::on_main_thread()
        {
        }
    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_WRAPPER_H_ */
