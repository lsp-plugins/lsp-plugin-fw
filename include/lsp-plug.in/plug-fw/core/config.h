/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 21 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_CONFIG_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_CONFIG_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/fmt/config/PullParser.h>
#include <lsp-plug.in/fmt/config/PushParser.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/fmt/config/Serializer.h>

// UI configuration ports
#define UI_LAST_VERSION_PORT_ID                 "last_version"
#define UI_DLG_DEFAULT_PATH_ID                  "dlg_default_path"
#define UI_DLG_DEFAULT_FTYPE_ID                 "dlg_default_ftype"
#define UI_DLG_SAMPLE_PATH_ID                   "dlg_sample_path"
#define UI_DLG_SAMPLE_FTYPE_ID                  "dlg_sample_ftype"
#define UI_DLG_IR_PATH_ID                       "dlg_ir_path"
#define UI_DLG_IR_FTYPE_ID                      "dlg_ir_ftype"
#define UI_DLG_CONFIG_PATH_ID                   "dlg_config_path"
#define UI_DLG_CONFIG_FTYPE_ID                  "dlg_config_ftype"
#define UI_DLG_MODEL3D_PATH_ID                  "dlg_model3d_path"
#define UI_DLG_MODEL3D_FTYPE_ID                 "dlg_model3d_ftype"
#define UI_DLG_REW_PATH_ID                      "dlg_rew_path"
#define UI_DLG_REW_FTYPE_ID                     "dlg_rew_ftype"
#define UI_DLG_HYDROGEN_PATH_ID                 "dlg_hydrogen_path"
#define UI_DLG_HYDROGEN_FTYPE_ID                "dlg_hydrogen_ftype"
#define UI_DLG_LSPC_BUNDLE_PATH_ID              "dlg_lspc_bundle_path"
#define UI_DLG_LSPC_BUNDLE_FTYPE_ID             "dlg_lspc_bundle_ftype"
#define UI_DLG_SFZ_PATH_ID                      "dlg_sfz_path"
#define UI_DLG_SFZ_FTYPE_ID                     "dlg_sfz_ftype"
#define UI_R3D_BACKEND_PORT_ID                  "r3d_backend"
#define UI_LANGUAGE_PORT_ID                     "language"
#define UI_REL_PATHS_PORT_ID                    "use_relative_paths"
#define UI_SCALING_PORT_ID                      "ui_scaling"
#define UI_SCALING_HOST_PORT_ID                 "ui_scaling_host"
#define UI_FONT_SCALING_PORT_ID                 "font_scaling"
#define UI_BUNDLE_SCALING_PORT_ID               "ui_bundle_scaling"
#define UI_VISUAL_SCHEMA_FILE_ID                "visual_schema_file"
#define UI_PREVIEW_AUTO_PLAY_ID                 "preview_auto_play"
#define UI_ENABLE_KNOB_SCALE_ACTIONS_ID         "enable_knob_scale_actions"
#define UI_USER_HYDROGEN_KIT_PATH_ID            "user_hydrogen_kit_path"
#define UI_OVERRIDE_HYDROGEN_KIT_PATH_ID        "override_hydrogen_kit_path"
#define UI_OVERRIDE_HYDROGEN_KITS_ID            "override_hydrogen_kits"
#define UI_INVERT_VSCROLL_ID                    "invert_vscroll"
#define UI_GRAPH_DOT_INVERT_VSCROLL_ID          "invert_graph_dot_vscroll"
#define UI_ZOOMABLE_SPECTRUM_GRAPH_ID           "zoomable_spectrum_graph"
#define UI_FILTER_POINT_THICK_ID                "filter_point_thickness"
#define UI_DOCUMENTATION_PATH_ID                "documentation_path"
#define UI_FILELIST_NAVIGATION_AUTOLOAD_ID      "file_list_navigation_autoload"
#define UI_FILELIST_NAVIGATION_AUTOPLAY_ID      "file_list_navigation_auto_play"
#define UI_TAKE_INST_NAME_FROM_FILE_ID          "take_instrument_name_from_file"
#define UI_SHOW_PIANO_LAYOUT_ON_GRAPH_ID        "graph_piano_layout"
#define UI_CONFIG_USER_FRIENDLY_VALUES_ID       "config_user_friendly_values"
#define AUDIO_BACKEND_ID                        "audio_backend"

namespace lsp
{
    namespace core
    {
        constexpr const char *GLOBAL_CONFIG_FILE_NAME   = "lsp-plugins.cfg";
        constexpr const char *GLOBAL_CONFIG_LOCK_NAME   = "lsp-plugins.lock";

        /**
         * Routine for handling configuration file read/write
         * @param location configuration file location
         * @param data user data
         * @return status of operation
         */
        typedef status_t (* config_processor_t)(const io::Path & location, void *data);

        /**
         * Serialize port value
         *
         * @param s configuration serializer
         * @param meta port metadata
         * @param data port data
         * @param base base path (can be NULL)
         * @param flags serialization flags
         * @return status of operation or STATUS_BAD_TYPE if port can not be serialized
         */
        status_t serialize_port_value(config::Serializer *s,
                const meta::port_t *meta, const void *data, const io::Path *base, size_t flags
        );

        /**
         * Format relative path to the output string
         * @param value destination string to store value
         * @param path path to format
         * @param base base path to use
         * @return true if value has been formatted
         */
        bool format_relative_path(LSPString *value, const char *path, const io::Path *base);

        /**
         * Extract relative path to the output path
         * @param path destination path to store relative value
         * @param base base path to use
         * @param value the original value to parse
         * @param len the length in octetx of the original value
         * @return true if the relative path has been extracted
         */
        bool parse_relative_path(io::Path *path, const io::Path *base, const char *value, size_t len);

        /**
         * Get location of the user's configuration file
         * @param path pointer to store result
         * @return status of operation
         */
        status_t get_user_config_path(io::Path *path);

        /**
         * Process global configuration file with exlusive lock mode. The location of configuration
         * file is passed to the handler after the lock has been acquired.
         *
         * @param handler configuration file handler
         * @param data user data passed to the handler
         * @return status of operation
         */
        status_t process_global_config(config_processor_t handler, void *data);

    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_CONFIG_H_ */
