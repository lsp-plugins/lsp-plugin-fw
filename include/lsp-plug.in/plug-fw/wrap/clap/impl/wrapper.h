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
            pHost               = host;
            pPackage            = package;
            pExt                = NULL;
            nLatency            = 0;

            bRestartRequested   = false;
            bUpdateSettings     = true;
        }

        Wrapper::~Wrapper()
        {
            if (pExt != NULL)
            {
                delete pExt;
                pExt            = NULL;
            }
        }

        status_t Wrapper::init()
        {
            // Create extensions
            pExt    = new HostExtensions(pHost);
            if (pExt == NULL)
                return STATUS_NO_MEM;

            // TODO: fill audio port info -- vInAudioPortInfo and vOutAudioPortInfo

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
            // Clear the flag that the restart has been requested
            bRestartRequested   = false;
            bUpdateSettings     = true;

            if (sample_rate > MAX_SAMPLE_RATE)
            {
                lsp_warn(
                    "Unsupported sample rate: %f, maximum supported sample rate is %ld",
                    sample_rate,
                    long(MAX_SAMPLE_RATE));
                sample_rate  = MAX_SAMPLE_RATE;
            }
            pPlugin->set_sample_rate(sample_rate);

            return STATUS_OK;
        }

        void Wrapper::deactivate()
        {
            // Set the actual latency
            nLatency            = pPlugin->latency();
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
            // Report the latency
            // From CLAP documentation:
            // The latency is only allowed to change if the plugin is deactivated.
            // If the plugin is activated, call host->request_restart()
            ssize_t latency = pPlugin->latency();
            if ((latency != nLatency) && (!bRestartRequested))
            {
                pHost->request_restart(pHost);
                bRestartRequested       = true;
            }

            return CLAP_PROCESS_CONTINUE;
        }

        const void *Wrapper::get_extension(const char *id)
        {
            return NULL;
        }

        void Wrapper::on_main_thread()
        {
        }

        size_t Wrapper::latency() const
        {
            return nLatency;
        }

        size_t Wrapper::audio_ports_count(bool is_input) const
        {
            return (is_input) ? vInAudioPortInfo.size() : vOutAudioPortInfo.size();
        }

        const clap_audio_port_info_t *Wrapper::audio_port_info(size_t index, bool is_input) const
        {
            return (is_input) ? vInAudioPortInfo.get(index) : vOutAudioPortInfo.get(index);
        }

        status_t Wrapper::save_state(const clap_ostream_t *stream)
        {
            return STATUS_OK;
        }

        status_t Wrapper::load_state(const clap_istream_t *stream)
        {
            return STATUS_OK;
        }

        size_t Wrapper::params_count() const
        {
            return vParamPorts.size();
        }

        status_t Wrapper::param_info(clap_param_info *info, size_t index) const
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t Wrapper::get_param_value(double *value, size_t index) const
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t Wrapper::format_param_value(char *buffer, size_t buf_size, size_t param_id, double value) const
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t Wrapper::parse_param_value(double *value, size_t param_id, const char *text) const
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        void Wrapper::flush_param_events(const clap_input_events_t *in, const clap_output_events_t *out)
        {
        }

    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_WRAPPER_H_ */
