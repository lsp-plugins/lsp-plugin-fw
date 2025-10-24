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
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>

namespace lsp
{
    namespace core
    {
        static constexpr size_t PRESET_NAME_BYTES   = 0x200;

        enum preset_flags_t
        {
            PRESET_FLAG_NONE        = 0,
            PRESET_FLAG_USER        = 1 << 0,   // User-defined preset
            PRESET_FLAG_DIRTY       = 1 << 1
        };

        typedef struct preset_state_t
        {
            uint32_t flags;                     // Current preset flags
            uint32_t tab;                       // Current preset tab
            char name[PRESET_NAME_BYTES];       // UTF-8 name of current preset
        } preset_state_t;

        typedef struct preset_param_t
        {
            kvt_param_t     value;              // Value of parameter
            char            name[];             // Name of parameter
        } preset_param_t;

        typedef struct preset_data_t
        {
            lltl::parray<preset_param_t>    values;     // Values
            bool                            empty;      // Empty flag for lazy initialization
            bool                            dirty;      // Dirty flag for synchronization
        } preset_data_t;

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

        /**
         * Initialize preset state with default values
         * @param state preset state
         */
        void init_preset_state(preset_state_t *state);

        /**
         * Copy preset state
         * @param dst destination state to store value
         * @param src source state to read value
         */
        void copy_preset_state(preset_state_t *dst, const preset_state_t *src);

        /**
         * Set preset parameter
         * @param dst destination preset list to add
         * @param name name of parameter
         * @param value value of parameter
         */
        status_t add_preset_data_param(preset_data_t *dst, const char *name, const kvt_param_t * value);

        /**
         * Copy preset data
         * @param data preset data
         */
        status_t copy_preset_data(preset_data_t *dst, const preset_data_t *src);

        /**
         * Destroy preset data
         * @param data preset data
         */
        void destroy_preset_data(preset_data_t *data);

        /**
         * Initialize preset data
         * @param data preset data to initialize
         */
        void init_preset_data(preset_data_t *data);

    } /* namespace core */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CORE_PRESETS_H_ */
