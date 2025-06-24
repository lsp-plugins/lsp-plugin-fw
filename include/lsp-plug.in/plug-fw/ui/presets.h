/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 июн. 2025 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_PRESETS_H_
#define LSP_PLUG_IN_PLUG_FW_UI_PRESETS_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace ui
    {
        enum preset_flags_t
        {
            PRESET_FLAG_NONE        = 0,
            PRESET_FLAG_USER        = 1 << 0,   // User-defined preset
            PRESET_FLAG_DIRTY       = 1 << 1,   // Preset is modified by user
            PRESET_FLAG_PATCH       = 1 << 2,   // Preset is modified by user
            PRESET_FLAG_SELECTED    = 1 << 3,   // Preset is currently selected
            PRESET_FLAG_FAVOURITE   = 1 << 4,   // Preset marked as a favourite
        };

        typedef struct preset_t
        {
            LSPString       name;       // Name of the preset
            LSPString       path;       // Location of the preset
            uint32_t        flags;      // Preset flags
            uint32_t        preset_id;  // Unique identifier of the preset
        } preset_t;

        class IPresetListener
        {
            public:
                IPresetListener();
                IPresetListener(const IPresetListener &) = delete;
                IPresetListener(IPresetListener &&) = delete;
                virtual ~IPresetListener();

                IPresetListener & operator = (const IPresetListener &) = delete;
                IPresetListener & operator = (IPresetListener &&) = delete;

            public:
                /**
                 * Handle event when preset has been selected
                 * @param preset selected preset
                 */
                virtual void    preset_selected(const preset_t *preset);

                /**
                 * Handle event when the list of presets has been updated
                 * @param presets
                 * @param count
                 */
                virtual void    presets_updated(const preset_t *presets, size_t count);
        };

        ssize_t preset_compare_function(const preset_t *a, const preset_t *b);

    } /* namespace ui */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UI_PRESETS_H_ */
