/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 7 февр. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_PLUGLIST_GEN_PLUGLIST_GEN_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_PLUGLIST_GEN_PLUGLIST_GEN_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

namespace lsp
{
    namespace pluglist_gen
    {
        typedef struct cmdline_t
        {
            const char     *php_out;    // PHP file to generate
            const char     *json_out;   // JSON file to generate
        } cmdline_t;

        typedef struct php_plugin_group_t
        {
            int            id;
            const char    *name;
        } php_plugin_group_t;

        extern const php_plugin_group_t php_plugin_groups[];

        /**
         * Parse command-line arguments
         * @param cfg command line configuration
         * @param argc number of command line arguments
         * @param argv list of command line arguments
         * @return status of operation
         */
        status_t parse_cmdline(cmdline_t *cfg, int argc, const char **argv);

        /**
         * Generate PHP file
         * @param file name of output PHP file
         * @param package package descriptor
         * @param plugins list of plugin metadata
         * @param count number of plugins in list
         * @return status of operation
         */
        status_t generate_php(
            const char *file,
            const meta::package_t *package,
            const meta::plugin_t * const *plugins,
            size_t count);

        /**
         * Generate JSON file
         * @param file name of output JSON file
         * @param package package descriptor
         * @param plugins list of plugin metadata
         * @param count number of plugins in list
         * @return status of operation
         */
        status_t generate_json(
            const char *file,
            const meta::package_t *package,
            const meta::plugin_t * const *plugins,
            size_t count);

        /**
         * Generate data
         * @param cmd parsed command line data
         * @return status of operation
         */
        status_t generate(const cmdline_t *cmd);

        /**
         * Launch the tool
         * @param argc number of command-line arguments
         * @param argv list of command-line arguments
         * @return status of operation
         */
        status_t main(int argc, const char **argv);

    } /* namespace pluglist_gen */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_PLUGLIST_GEN_PLUGLIST_GEN_H_ */
