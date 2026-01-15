/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 янв. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_CONFIG_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_CONFIG_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/resource/ILoader.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace launcher
    {
        /**
         * UI configuration
         */
        typedef struct ui_config_t
        {
            uint32_t            nWidth;             // Width of the UI window
            uint32_t            nHeight;            // Height of the UI window
            float               fUIScaling;         // UI scaling
            float               fFontScaling;       // Font scaling
            LSPString           sSchema;            // Path to the schema
        } ui_config_t;

        /**
         * Initialize config with default values
         * @param config config to initialize
         * @return status of operation
         */
        status_t init_config(ui_config_t & config);

        /**
         * Load UI configuration
         * @param config UI configuration
         * @return status of operation
         */
        status_t load_config(ui_config_t & config);

        /**
         * Save UI configuration
         * @param config UI configuration
         */
        status_t save_config(const ui_config_t & config);

        /**
         * Destroy UI configuration
         * @param config UI configuration to destroy
         */
        void destroy_config(ui_config_t & config);
    } /* namespace launcher */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_CONFIG_H_ */
