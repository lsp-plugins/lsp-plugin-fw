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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/util/launcher/plugin_metadata.h>
#include <lsp-plug.in/plug-fw/util/launcher/ui_config.h>
#include <lsp-plug.in/plug-fw/util/launcher/visual_schema.h>

#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace launcher
    {
        class UI
        {
            protected:
                tk::Display                *pDisplay;
                tk::Window                 *wWindow;

                resource::ILoader          *pLoader;

                plugin_registry_t           sRegistry;
                visual_schemas_t            sSchemas;
                ui_config_t                 sConfig;
                bool                        bConfigDirty;

            protected: // slots
                static status_t             slot_display_idle(tk::Widget *sender, void *ptr, void *data);

                static status_t             slot_window_close(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_window_resize(tk::Widget *sender, void *ptr, void *data);

            protected:
                status_t                    update_visual_schema();
                status_t                    apply_visual_schema(tk::StyleSheet *sheet);

            protected:
                template <typename T>
                void                        destroy(T * & w);
                template <typename T>
                status_t                    create(T * & widget);

            public:
                UI(tk::Display *display, resource::ILoader * loader);
                ~UI();
                UI(const UI &) = delete;
                UI(UI &&) = delete;

                UI & operator = (const UI &) = delete;
                UI & operator = (UI &&) = delete;

                status_t                    init();
                void                        destroy();
        };


    } /* namespace launcher */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_H_ */
