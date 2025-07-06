/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 июн. 2025 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IPRESETLISTENER_H_
#define LSP_PLUG_IN_PLUG_FW_UI_IPRESETLISTENER_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/ui/presets.h>

namespace lsp
{
    namespace ui
    {
        /**
         * Callback interface for handling changes of presets
         */
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
                 * Handle event when preset has been deactivated
                 * @param preset deactivated preset
                 */
                virtual void    preset_deactivated(const preset_t *preset);

                /**
                 * Handle event when preset has been activated
                 * @param preset activated preset
                 */
                virtual void    preset_activated(const preset_t *preset);

                /**
                 * Handle event when the whole list of presets has been updated
                 */
                virtual void    presets_updated();
        };

    } /* namespace ui */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UI_IPRESETLISTENER_H_ */
