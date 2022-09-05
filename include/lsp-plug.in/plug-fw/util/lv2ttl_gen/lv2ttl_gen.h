/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_LV2TTL_GEN_LV2TTL_GEN_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_LV2TTL_GEN_LV2TTL_GEN_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>


#define LV2TTL_PLUGIN_PREFIX                "plug"
#define LV2TTL_PLUGIN_UI_PREFIX             "plug_ui"
#define LV2TTL_PORT_GROUP_PREFIX            "plug_pg"
#define LV2TTL_PORTS_PREFIX                 "plug_p"
#define LV2TTL_DEVELOPERS_PREFIX            "plug_dev"

// Different LV2 UI classes for different platforms
#if defined(PLATFORM_LINUX) || defined(PLATFORM_BSD)
    #define LV2TTL_UI_CLASS                             "X11UI"
#elif defined(PLATFORM_WINDOWS)
    #define LV2TTL_UI_CLASS                             "WindowsUI"
#elif defined(PLATFORM_MACOSX)
    #define LV2TTL_UI_CLASS                             "CocoaUI"
#elif defined(PLATFORM_UNIX_COMPATIBLE)
    #define LV2TTL_UI_CLASS                             "X11UI"
#else
    #define LV2TTL_UI_CLASS                             "X11UI"
#endif

namespace lsp
{
    namespace lv2ttl_gen
    {
        //-----------------------------------------------------------------------------
        enum requirements_t
        {
            REQ_PATCH       = 1 << 0,
            REQ_STATE       = 1 << 1,
            REQ_LV2UI       = 1 << 2,
            REQ_PORT_GROUPS = 1 << 3,
            REQ_WORKER      = 1 << 4,
            REQ_MIDI_IN     = 1 << 5,
            REQ_MIDI_OUT    = 1 << 6,
            REQ_PATCH_WR    = 1 << 7,
            REQ_INSTANCE    = 1 << 8,
            REQ_TIME        = 1 << 9,
            REQ_IDISPLAY    = 1 << 10,
            REQ_OSC_IN      = 1 << 11,
            REQ_OSC_OUT     = 1 << 12,
            REQ_MAP_PATH    = 1 << 13,

            REQ_PATH_MASK   = REQ_PATCH | REQ_STATE | REQ_MAP_PATH | REQ_WORKER | REQ_PATCH_WR,
            REQ_MIDI        = REQ_MIDI_IN | REQ_MIDI_OUT
        };

        typedef struct plugin_group_t
        {
            int            id;
            const char    *name;
        } plugin_group_t;

        typedef struct plugin_unit_t
        {
            int             id;             // Internal ID from metadata
            const char     *name;           // LV2 name of the unit
            const char     *label;          // Custom label of the unit (if name is not present)
            const char     *render;         // Formatting of the unit (if name is not present)
        } plugin_unit_t;

        typedef struct cmdline_t
        {
            const char     *out_dir;        // Output directory
            const char     *lv2_binary;     // The name of the LV2 binary file
            const char     *lv2ui_binary;   // The name of the LV2 UI binary file
        } cmdline_t;

        //-----------------------------------------------------------------------------
        extern const plugin_group_t plugin_groups[];

        extern const plugin_unit_t plugin_units[];

        /**
         * Parse command-line arguments
         * @param cfg command line configuration
         * @param argc number of command line arguments
         * @param argv list of command line arguments
         * @return status of operation
         */
        status_t parse_cmdline(cmdline_t *cfg, int argc, const char **argv);

        /**
         * Generate TTL file for the plugin's UI
         * @param out output file stream
         * @param requirements plugin requirements (flags of requirements_t enum)
         * @param meta plugin metadata
         * @param manifest package manifest metadata
         * @param cmd command line arguments
         */
        void write_plugin_ui_ttl(
            FILE *out,
            size_t requirements,
            const meta::plugin_t &meta,
            const meta::package_t *manifest,
            const cmdline_t *cmd);

        /**
         * Generate TTL file for the plugin
         * @param out output file stream
         * @param requirements plugin requirements (flags of requirements_t enum)
         * @param meta plugin metadata
         * @param manifest package manifest metadata
         * @param cmd command line arguments
         */
        void write_plugin_ttl(
            FILE *out,
            size_t requirements,
            const meta::plugin_t &m,
            const meta::package_t *manifest,
            const cmdline_t *cmd);

        /**
         * Generate plugin TTL files
         * @param meta plugin metadata
         * @param manifest package manifest metadata
         * @param cmd command line arguments
         */
        void gen_plugin_ttl_files(
            const meta::plugin_t &m,
            const meta::package_t *manifest,
            const cmdline_t *cmd);

        /**
         * Generate the TTL manifest file
         * @param manifest package manifest metadata
         * @param plugins list of plugins
         * @param cmd command line arguments
         */
        void gen_manifest(
            const meta::package_t *manifest,
            const lltl::parray<meta::plugin_t> &plugins,
            const cmdline_t *cmd);

        /**
         * The main function of utility
         * @param argc number of command-line arguments
         * @param argv list of command-line arguments
         * @return status of operation
         */
        status_t main(int argc, const char **argv);

    } /* namespace lv2ttl_gen */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_LV2TTL_GEN_LV2TTL_GEN_H_ */
