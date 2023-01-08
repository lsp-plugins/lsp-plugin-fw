/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 янв. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_VALIDATOR_VALIDATOR_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_VALIDATOR_VALIDATOR_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

namespace lsp
{
    namespace validator
    {
        typedef struct context_t
        {
            size_t  plugins;
            size_t  warnings;
            size_t  errors;
            lltl::pphash<char, meta::plugin_t>      lsp_acronyms;
            lltl::pphash<char, meta::plugin_t>      lsp_ids;
            lltl::pphash<char, meta::plugin_t>      vst2_ids;
            lltl::pphash<uint32_t, meta::plugin_t>  ladspa_ids;
            lltl::pphash<char, meta::plugin_t>      ladspa_labels;
            lltl::parray<meta::person_t>            developers;
            lltl::parray<meta::bundle_t>            bundles;

            size_t midi_in;
            size_t midi_out;
            size_t osc_in;
            size_t osc_out;
            size_t bypass;
            lltl::parray<meta::port_t>              gen_ports;
            lltl::pphash<char, meta::port_t>        port_ids;
            lltl::pphash<clap_id, meta::port_t>     clap_ids;
        } context_t;

        namespace ladspa
        {
            void validate_plugin(context_t *ctx, const meta::plugin_t *meta);
            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port);
        } /* namespace ladspa */

        namespace lv2
        {
            void validate_plugin(context_t *ctx, const meta::plugin_t *meta);
            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port);
        } /* namespace lv2 */

        namespace vst2
        {
            void validate_plugin(context_t *ctx, const meta::plugin_t *meta);
            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port);
        } /* namespace vst2 */

        namespace jack
        {
            void validate_plugin(context_t *ctx, const meta::plugin_t *meta);
            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port);
        } /* namespace jack */

        namespace clap
        {
            void validate_plugin(context_t *ctx, const meta::plugin_t *meta);
            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port);
        }

        void validate_plugin(context_t *ctx, const meta::plugin_t *meta);
        void validate_ports(context_t *ctx, const meta::plugin_t *meta);
        void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port);
        void validation_error(context_t *ctx, const char *fmt, ...);
        void allocation_error(context_t *ctx);

        /**
         * Main method
         * @param argc number of arguments
         * @param argv list of arguments
         * @return status of operation
         */
        int main(int argc, const char ** argv);
    } /* namespace validator */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_VALIDATOR_VALIDATOR_H_ */
