/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 25 дек. 2022 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_EXTENSIONS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_EXTENSIONS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace clap
    {
        /**
         * Structure that holds pointers to different host extensions
         */
        struct HostExtensions
        {
            public:
                const clap_host_latency_t *latency;
                const clap_host_state_t   *state;
                const clap_host_params_t  *params;

            protected:
                template <class T>
                inline const T * get_extension(const clap_host_t *host, const char *name)
                {
                    return static_cast<const T *>(host->get_extension(host, name));
                }

            public:
                explicit HostExtensions(const clap_host_t *host)
                {
                    latency     = get_extension<clap_host_latency_t>(host, CLAP_EXT_LATENCY);
                    state       = get_extension<clap_host_state_t>(host, CLAP_EXT_STATE);
                    params      = get_extension<clap_host_params_t>(host, CLAP_EXT_PARAMS);
                }
        };
    } /* namespace clap */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_EXTENSIONS_H_ */
