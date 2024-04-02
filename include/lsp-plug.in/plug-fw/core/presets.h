/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-pluginfw
 * Created on: 2 апр. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_PRESETS_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_PRESETS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>

namespace lsp
{
    namespace core
    {
        /**
         * Scan resources directory for presets
         * @param presets target collection to store data
         * @param loader resource loader
         * @param location directory to look for
         * @return status of operation
         */
        status_t scan_presets(lltl::darray<resource::resource_t> *presets, resource::ILoader *loader, const char *location);

        /**
         * Sort presets
         * @param presets presets to sort
         * @param ascending sort order
         */
        void sort_presets(lltl::darray<resource::resource_t> *presets, bool ascending);

    } /* namespace core */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CORE_PRESETS_H_ */
