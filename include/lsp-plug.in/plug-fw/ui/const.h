/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_CONST_H_
#define LSP_PLUG_IN_PLUG_FW_UI_CONST_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>

// UI configuration ports
#define UI_CONFIG_PORT_PREFIX               "_ui_"
#define UI_KVT_PORT_PREFIX                  "_kvt_"
#define UI_MOUNT_STUD_PORT_ID               "mount_stud"
#define UI_LAST_VERSION_PORT_ID             "last_version"
#define UI_DLG_DEFAULT_PATH_ID              "dlg_default_path"
#define UI_DLG_DEFAULT_FTYPE_ID             "dlg_default_ftype"
#define UI_DLG_SAMPLE_PATH_ID               "dlg_sample_path"
#define UI_DLG_SAMPLE_FTYPE_ID              "dlg_sample_ftype"
#define UI_DLG_IR_PATH_ID                   "dlg_ir_path"
#define UI_DLG_IR_FTYPE_ID                  "dlg_ir_ftype"
#define UI_DLG_CONFIG_PATH_ID               "dlg_config_path"
#define UI_DLG_CONFIG_FTYPE_ID              "dlg_config_ftype"
#define UI_DLG_MODEL3D_PATH_ID              "dlg_model3d_path"
#define UI_DLG_MODEL3D_FTYPE_ID             "dlg_model3d_ftype"
#define UI_DLG_REW_PATH_ID                  "dlg_rew_path"
#define UI_DLG_REW_FTYPE_ID                 "dlg_rew_ftype"
#define UI_DLG_HYDROGEN_PATH_ID             "dlg_hydrogen_path"
#define UI_DLG_HYDROGEN_FTYPE_ID            "dlg_hydrogen_ftype"
#define UI_DLG_LSPC_BUNDLE_PATH_ID          "dlg_lspc_bundle_path"
#define UI_DLG_LSPC_BUNDLE_FTYPE_ID         "dlg_lspc_bundle_ftype"
#define UI_DLG_SFZ_PATH_ID                  "dlg_sfz_path"
#define UI_DLG_SFZ_FTYPE_ID                 "dlg_sfz_ftype"
#define UI_R3D_BACKEND_PORT_ID              "r3d_backend"
#define UI_LANGUAGE_PORT_ID                 "language"
#define UI_REL_PATHS_PORT_ID                "use_relative_paths"
#define UI_SCALING_PORT_ID                  "ui_scaling"
#define UI_SCALING_HOST_ID                  "ui_scaling_host"
#define UI_FONT_SCALING_PORT_ID             "font_scaling"
#define UI_VISUAL_SCHEMA_FILE_ID            "visual_schema_file"
#define UI_PREVIEW_AUTO_PLAY_ID             "preview_auto_play"
#define UI_ENABLE_KNOB_SCALE_ACTIONS_ID     "enable_knob_scale_actions"
#define UI_USER_HYDROGEN_KIT_PATH_ID        "user_hydrogen_kit_path"
#define UI_OVERRIDE_HYDROGEN_KIT_PATH_ID    "override_hydrogen_kit_path"
#define UI_OVERRIDE_HYDROGEN_KITS_ID        "override_hydrogen_kits"
#define UI_INVERT_VSCROLL_ID                "invert_vscroll"
#define UI_GRAPH_DOT_INVERT_VSCROLL_ID      "invert_graph_dot_vscroll"
#define UI_ZOOMABLE_SPECTRUM_GRAPH_ID       "zoomable_spectrum_graph"
#define UI_FILTER_POINT_THICK_ID            "filter_point_thickness"

#define MSTUD_PORT                          UI_CONFIG_PORT_PREFIX UI_MOUNT_STUD_PORT_ID
#define VERSION_PORT                        UI_CONFIG_PORT_PREFIX UI_LAST_VERSION_PORT_ID
#define DEFAULT_PATH_PORT                   UI_CONFIG_PORT_PREFIX UI_DLG_DEFAULT_PATH_ID
#define DEFAULT_FTYPE_PORT                  UI_CONFIG_PORT_PREFIX UI_DLG_DEFAULT_FTYPE_ID
#define SAMPLE_PATH_PORT                    UI_CONFIG_PORT_PREFIX UI_DLG_SAMPLE_PATH_ID
#define SAMPLE_FTYPE_PORT                   UI_CONFIG_PORT_PREFIX UI_DLG_SAMPLE_FTYPE_ID
#define IR_PATH_PORT                        UI_CONFIG_PORT_PREFIX UI_DLG_IR_PATH_ID
#define IR_FTYPE_PORT                       UI_CONFIG_PORT_PREFIX UI_DLG_IR_FTYPE_ID
#define CONFIG_PATH_PORT                    UI_CONFIG_PORT_PREFIX UI_DLG_CONFIG_PATH_ID
#define CONFIG_FTYPE_PORT                   UI_CONFIG_PORT_PREFIX UI_DLG_CONFIG_FTYPE_ID
#define MODEL3D_PATH_PORT                   UI_CONFIG_PORT_PREFIX UI_DLG_MODEL3D_PATH_ID
#define MODEL3D_FTYPE_PORT                  UI_CONFIG_PORT_PREFIX UI_DLG_MODEL3D_FTYPE_ID
#define REW_PATH_PORT                       UI_CONFIG_PORT_PREFIX UI_DLG_REW_PATH_ID
#define REW_FTYPE_PORT                      UI_CONFIG_PORT_PREFIX UI_DLG_REW_FTYPE_ID
#define HYDROGEN_PATH_PORT                  UI_CONFIG_PORT_PREFIX UI_DLG_HYDROGEN_PATH_ID
#define HYDROGEN_FTYPE_PORT                 UI_CONFIG_PORT_PREFIX UI_DLG_HYDROGEN_FTYPE_ID
#define LSPC_BUNDLE_PATH_PORT               UI_CONFIG_PORT_PREFIX UI_DLG_LSPC_BUNDLE_PATH_ID
#define LSPC_BUNDLE_FTYPE_PORT              UI_CONFIG_PORT_PREFIX UI_DLG_LSPC_BUNDLE_FTYPE_ID
#define SFZ_PATH_PORT                       UI_CONFIG_PORT_PREFIX UI_DLG_SFZ_PATH_ID
#define SFZ_FTYPE_PORT                      UI_CONFIG_PORT_PREFIX UI_DLG_SFZ_FTYPE_ID
#define R3D_BACKEND_PORT                    UI_CONFIG_PORT_PREFIX UI_R3D_BACKEND_PORT_ID
#define LANGUAGE_PORT                       UI_CONFIG_PORT_PREFIX UI_LANGUAGE_PORT_ID
#define REL_PATHS_PORT                      UI_CONFIG_PORT_PREFIX UI_REL_PATHS_PORT_ID
#define UI_SCALING_PORT                     UI_CONFIG_PORT_PREFIX UI_SCALING_PORT_ID
#define UI_SCALING_HOST                     UI_CONFIG_PORT_PREFIX UI_SCALING_HOST_ID
#define UI_FONT_SCALING_PORT                UI_CONFIG_PORT_PREFIX UI_FONT_SCALING_PORT_ID
#define UI_VISUAL_SCHEMA_PORT               UI_CONFIG_PORT_PREFIX UI_VISUAL_SCHEMA_FILE_ID
#define UI_PREVIEW_AUTO_PLAY_PORT           UI_CONFIG_PORT_PREFIX UI_PREVIEW_AUTO_PLAY_ID
#define UI_ENABLE_KNOB_SCALE_ACTIONS_PORT   UI_CONFIG_PORT_PREFIX UI_ENABLE_KNOB_SCALE_ACTIONS_ID
#define UI_USER_HYDROGEN_KIT_PATH_PORT      UI_CONFIG_PORT_PREFIX UI_USER_HYDROGEN_KIT_PATH_ID
#define UI_OVERRIDE_HYDROGEN_KIT_PATH_PORT  UI_CONFIG_PORT_PREFIX UI_OVERRIDE_HYDROGEN_KIT_PATH_ID
#define UI_OVERRIDE_HYDROGEN_KITS_PORT      UI_CONFIG_PORT_PREFIX UI_OVERRIDE_HYDROGEN_KITS_ID
#define UI_INVERT_VSCROLL_PORT              UI_CONFIG_PORT_PREFIX UI_INVERT_VSCROLL_ID
#define UI_GRAPH_DOT_INVERT_VSCROLL_PORT    UI_CONFIG_PORT_PREFIX UI_GRAPH_DOT_INVERT_VSCROLL_ID
#define UI_ZOOMABLE_SPECTRUM_GRAPH_PORT     UI_CONFIG_PORT_PREFIX UI_ZOOMABLE_SPECTRUM_GRAPH_ID
#define UI_FILTER_POINT_THICK_PORT          UI_CONFIG_PORT_PREFIX UI_FILTER_POINT_THICK_ID

// Special widget identifiers
#define WUID_MAIN_MENU                      "main_menu"
#define WUID_PRESETS_MENU                   "presets_menu"
#define WUID_EXPORT_MENU                    "export_menu"
#define WUID_IMPORT_MENU                    "import_menu"
#define WUID_LANGUAGE_MENU                  "language_menu"
#define WUID_RESET_SETTINGS_MENU            "reset_settings_menu"

// Special ports for handling current time
#define TIME_PORT_PREFIX                    "_time_"
#define TIME_SAMPLE_RATE_PORT               "sr"
#define TIME_SPEED_PORT                     "speed"
#define TIME_FRAME_PORT                     "frame"
#define TIME_NUMERATOR_PORT                 "num"
#define TIME_DENOMINATOR_PORT               "denom"
#define TIME_BEATS_PER_MINUTE_PORT          "bpm"
#define TIME_TICK_PORT                      "tick"
#define TIME_TICKS_PER_BEAT_PORT            "tpb"

#define SAMPLE_RATE_PORT                    TIME_PORT_PREFIX TIME_SAMPLE_RATE_PORT
#define SPEED_PORT                          TIME_PORT_PREFIX TIME_SPEED_PORT
#define FRAME_PORT                          TIME_PORT_PREFIX TIME_FRAME_PORT
#define NUMERATOR_PORT                      TIME_PORT_PREFIX TIME_NUMERATOR_PORT
#define DENOMINATOR_PORT                    TIME_PORT_PREFIX TIME_DENOMINATOR_PORT
#define BEATS_PER_MINUTE_PORT               TIME_PORT_PREFIX TIME_BEATS_PER_MINUTE_PORT
#define TICK_PORT                           TIME_PORT_PREFIX TIME_TICK_PORT
#define TICKS_PER_BEAT_PORT                 TIME_PORT_PREFIX TIME_TICKS_PER_BEAT_PORT

#endif /* LSP_PLUG_IN_PLUG_FW_UI_CONST_H_ */
