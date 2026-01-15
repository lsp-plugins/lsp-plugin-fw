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

#ifndef PLUG_IN_PLUG_FW_UTIL_LAUNCHER_VISUAL_SCHEMA_H_
#define PLUG_IN_PLUG_FW_UTIL_LAUNCHER_VISUAL_SCHEMA_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/resource/ILoader.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace launcher
    {
        typedef struct visual_schema_t
        {
            LSPString       name;
            LSPString       path;
            tk::StyleSheet  sheet;
        } visual_schema_t;

        typedef lltl::parray<visual_schema_t> visual_schemas_t;

        /**
         * Load all available visual schemas
         * @param list list to store visual schemas
         * @param loader resource loader
         * @return status of operation
         */
        status_t load_visual_schemas(visual_schemas_t & list, resource::ILoader * loader);

        /**
         * Destroy visual schemas
         * @param list list of visual schemas
         */
        void destroy_visual_schemas(visual_schemas_t & list);
    } /* namespace launcher */
} /* namespace lsp */



#endif /* PLUG_IN_PLUG_FW_UTIL_LAUNCHER_VISUAL_SCHEMA_H_ */
