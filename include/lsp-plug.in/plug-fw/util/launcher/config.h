/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 февр. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_CONFIG_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_CONFIG_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace launcher
    {
        typedef struct config_t
        {
            size_t                  nWidth;             // Window width
            size_t                  nHeight;            // Window height
            bool                    bLaunchMultiple;    // Launch multiple plugins
            lltl::phashset<char>    vFavourites;        // List of favourites
        } config_t;


        void init_config(config_t & config);
        void free_config(config_t & config);

        status_t load_config(config_t & config, const io::Path & path);
        status_t load_config(config_t & config, const LSPString & path);
        status_t load_config(config_t & config, const char *path);
        status_t load_config(config_t & config);

        status_t save_config(const config_t & config, const io::Path & path);
        status_t save_config(const config_t & config, const LSPString & path);
        status_t save_config(const config_t & config, const char *path);
        status_t save_config(const config_t & config);
    } /* namespace launcher */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_CONFIG_H_ */
