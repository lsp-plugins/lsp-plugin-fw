/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 13 янв. 2026 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_PLUGIN_METADATA_H_
#define LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_PLUGIN_METADATA_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/resource/ILoader.h>
#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace launcher
    {
        typedef struct package_t
        {
            LSPString       artifact;
            LSPString       name;
            LSPString       version;
        } package_t;

        typedef struct category_t
        {
            size_t          priority;
            LSPString       id;
        } category_t;

        typedef struct bundle_t
        {
            LSPString       id;
            LSPString       name;
            LSPString       description;
            const category_t *category;
        } bundle_t;

        typedef struct plugin_t
        {
            LSPString       id;
            LSPString       name;
            LSPString       description;
            LSPString       version;
            LSPString       acronym;
            const bundle_t *bundle;
        } plugin_t;

        typedef struct plugin_registry_t
        {
            package_t       package;
            lltl::parray<category_t> categories;
            lltl::parray<bundle_t> bundles;
            lltl::parray<plugin_t> plugins;
        } plugin_registry_t;

        /**
         * Read plugin metadata from JSON file OR from availabel compiled-in plugin metadata
         * @param metadata plugin metadata to store result
         * @param loader resource loader (may be NULL)
         * @param path path to the resource (may be NULL)
         * @return status of operation
         */
        status_t read_plugin_metadata(plugin_registry_t & metadata, resource::ILoader * loader, const char *path);

        /**
         * Destroy plugin metadata
         * @param metadata plugin metadata to destroy
         */
        void destroy_plugin_metadata(plugin_registry_t & metadata);
    } /* namespace launcher */
} /* namespace lsp */



#endif /* LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_PLUGIN_METADATA_H_ */
