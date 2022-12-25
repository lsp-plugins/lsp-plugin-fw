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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_WRAPPER_H_

#include <clap/clap.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/wrap/clap/extensions.h>
#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace clap
    {
        /**
         * CLAP plugin wrapper interface
         */
        class Wrapper: public plug::IWrapper
        {
            protected:
                const clap_host_t              *pHost;
                const meta::package_t          *pPackage;
                HostExtensions                 *pExt;
                ssize_t                         nLatency;

                lltl::darray<clap_audio_port_info_t> vInAudioPortInfo;
                lltl::darray<clap_audio_port_info_t> vOutAudioPortInfo;
                lltl::parray<plug::IPort>       vInAudioPorts;
                lltl::parray<plug::IPort>       vOutAudioPorts;
                lltl::parray<plug::IPort>       vParamPorts;

                bool                            bRestartRequested;  // Flag that indicates that the plugin restart was requested
                bool                            bUpdateSettings;    // Trigger settings update for the nearest run

            public:
                explicit Wrapper(
                    plug::Module *module,
                    const meta::package_t *package,
                    resource::ILoader *loader,
                    const clap_host_t *host);
                virtual ~Wrapper() override;

            public:
                // CLAP public API functions
                status_t    init();
                void        destroy();
                status_t    activate(double sample_rate, uint32_t min_frames_count, uint32_t max_frames_count);
                void        deactivate();
                status_t    start_processing();
                void        stop_processing();
                void        reset();
                clap_process_status     process(const clap_process_t *process);
                const void *get_extension(const char *id);
                void        on_main_thread();

            public:
                // CLAP parameter extension
                size_t      params_count() const;
                status_t    param_info(clap_param_info *info, size_t index) const;
                status_t    get_param_value(double *value, size_t index) const;
                status_t    format_param_value(char *buffer, size_t buf_size, size_t param_id, double value) const;
                status_t    parse_param_value(double *value, size_t param_id, const char *text) const;
                void        flush_param_events(const clap_input_events_t *in, const clap_output_events_t *out);

            public:
                // CLAP latency extension
                size_t      latency() const;

            public:
                // CLAP audio port extension
                size_t      audio_ports_count(bool is_input) const;
                const clap_audio_port_info_t     *audio_port_info(size_t index, bool is_input) const;

            public:
                // CLAP state extension
                status_t    save_state(const clap_ostream_t *stream);
                status_t    load_state(const clap_istream_t *stream);
        };
    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_WRAPPER_H_ */
