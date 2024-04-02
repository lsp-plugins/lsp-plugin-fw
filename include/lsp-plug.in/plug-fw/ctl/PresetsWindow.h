/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 31 мар. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_PRESETSWINDOW_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_PRESETSWINDOW_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>
#include <lsp-plug.in/lltl/parray.h>

namespace lsp
{
    namespace ctl
    {

        class PluginWindow;

        /**
         * The plugin's window controller
         */
        class PresetsWindow: public ctl::Window
        {
            public:
                static const ctl_class_t metadata;

            protected:
                PluginWindow *pPluginWindow;

            protected:
                // Slots
                static status_t slot_window_close(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_preset_new_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_preset_delete_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_preset_override_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_import_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_export_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_state_copy_click(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_state_paste_click(tk::Widget *sender, void *ptr, void *data);

            protected:
                void bind_slot(const char *widget_id, tk::slot_t id, tk::event_handler_t handler);

            public:
                explicit PresetsWindow(ui::IWrapper *src, tk::Window *widget, PluginWindow *pluginWindow);
                PresetsWindow(const PresetsWindow &) = delete;
                PresetsWindow(PresetsWindow &&) = delete;
                virtual ~PresetsWindow() override;

                PresetsWindow & operator = (const PresetsWindow &) = delete;
                PresetsWindow & operator = (PresetsWindow &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                status_t            post_init();

        };


    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_PRESETSWINDOW_H_ */
