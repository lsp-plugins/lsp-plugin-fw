/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 24 июн. 2025 г.
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

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ui
    {
        IPresetListener::IPresetListener()
        {
        }

        IPresetListener::~IPresetListener()
        {
        }

        void IPresetListener::preset_selected(const preset_t *preset)
        {
        }

        void IPresetListener::presets_updated(const preset_t *presets, size_t count)
        {
        }

        ssize_t preset_compare_function(const preset_t *a, const preset_t *b)
        {
            ssize_t result = a->name.compare_to_nocase(&b->name);
            if (result != 0)
                return result;

            return ssize_t(a->flags & PRESET_FLAG_USER) - ssize_t(b->flags & PRESET_FLAG_USER);
        }

        bool is_any_preset(const ui::preset_t *preset)
        {
            return true;
        }

        bool is_factory_preset(const ui::preset_t *preset)
        {
            return !(preset->flags & ui::PRESET_FLAG_USER);
        }

        bool is_user_preset(const ui::preset_t *preset)
        {
            return preset->flags & ui::PRESET_FLAG_USER;
        }

        bool is_favourite_preset(const ui::preset_t *preset)
        {
            return preset->flags & ui::PRESET_FLAG_FAVOURITE;
        }

    } /* namespace ui */
} /* namespace lsp */


