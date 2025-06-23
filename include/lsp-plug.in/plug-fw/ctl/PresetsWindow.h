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
        class PresetsWindow;

        /**
         * Preset descriptor
         */
        typedef struct preset_item_t
        {
            PluginWindow       *plugin_window;
            PresetsWindow      *presets_window;
            LSPString           name;
            LSPString           location;
            // LSPString           author;
            //                     date;
        } preset_item_t;

        /**
         * The plugin's window controller
         */
        class PresetsWindow: public ctl::Window
        {
            public:
                static const ctl_class_t metadata;

            protected:
                enum preset_list_type_t
                {
                    PLT_FACTORY,
                    PLT_USER,
                    PLT_FAVOURITES,
                    PLT_ALL,

                    PLT_TOTAL
                };

                typedef struct preset_list_t
                {
                    tk::ListBox        *wList;
                } preset_list_t;

                typedef struct preset_t
                {
                    LSPString           sPath;          // Location of the preset
                    bool                bUser;          // Indicator that it is a user preset
                    bool                bFavourite;     // Indicator that preset is marked as favourite
                    bool                bPatch;         // Indicator that preset is a patch
                } preset_t;

            protected:
                PluginWindow           *pPluginWindow;              // Plugin window
                tk::FileDialog         *wExport;                    // Export settings dialog
                tk::FileDialog         *wImport;                    // Import settings dialog
                tk::CheckBox           *wRelPaths;                  // Relative path checkbox
                preset_list_t           vPresetsLists[PLT_TOTAL];

                ui::IPort              *pPath;                      // Location of user's export/import directory
                ui::IPort              *pFileType;                  // Import/export configuration file type selection
                ui::IPort              *pRelPaths;                  // Relative paths configuration option

                int16_t                 iSelectedPreset = -1;

            protected:
                // Slots
                static status_t     slot_window_close(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_new_click(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_delete_click(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_save_click(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_state_copy_click(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_state_paste_click(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_select(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_dblclick(tk::Widget *sender, void *ptr, void *data);

                static status_t     slot_submit_import_settings(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_submit_export_settings(tk::Widget *sender, void *ptr, void *data);

                static status_t     slot_fetch_path(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_commit_path(tk::Widget *sender, void *ptr, void *data);

                static status_t     slot_exec_export_settings_to_file(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_exec_import_settings_from_file(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_relative_path_changed(tk::Widget *sender, void *ptr, void *data);

            protected:
                tk::FileFilters    *create_config_filters(tk::FileDialog *dlg);

            protected:
                void                bind_slot(const char *widget_id, tk::slot_t id, tk::event_handler_t handler);
                status_t            refresh_presets();
                status_t            refresh_user_presets();
                void                put_presets_to_list(lltl::darray<resource::resource_t> *presets);
                void                sync_preset_selection(preset_list_t *list);
                bool                has_path_ports();

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
                status_t            show(tk::Widget *actor);
                status_t            show_export_settings_dialog();
                status_t            show_import_settings_dialog();

        };


    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_PRESETSWINDOW_H_ */
