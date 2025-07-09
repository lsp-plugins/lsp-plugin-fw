/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        class PluginWindow;
        class SavePresetDialog;

        /**
         * The plugin's window controller
         */
        class PresetsWindow: public ctl::Window, public ui::IPresetListener
        {
            public:
                static const ctl_class_t metadata;

            protected:
                enum button_t
                {
                    BTN_SAVE,
                    BTN_FAVOURITE,
                    BTN_DELETE,

                    BTN_TOTAL
                };

                typedef struct preset_list_t
                {
                    tk::ListBox                    *wList;
                    lltl::parray<ui::preset_t>      vPresets;
                    lltl::parray<tk::ListBoxItem>   vItems;
                } preset_list_t;

                class ConfigSink: public tk::TextDataSink
                {
                    private:
                        ui::IWrapper       *pWrapper;

                    public:
                        explicit ConfigSink(ui::IWrapper *wrapper);

                    public:
                        void             unbind();

                    public:
                        virtual status_t receive(const LSPString *text, const char *mime);
                };

            protected:
                PluginWindow           *pPluginWindow;              // Plugin window
                SavePresetDialog       *pSavePresetDlg;             // Preset saving dialog
                tk::Widget             *wLastActor;                 // Last actor
                tk::FileDialog         *wExport;                    // Export settings dialog
                tk::FileDialog         *wImport;                    // Import settings dialog
                tk::CheckBox           *wRelPaths;                  // Relative path checkbox
                tk::Edit               *wPresetPattern;             // Preset pattern
                tk::Button             *vButtons[BTN_TOTAL];        // Preset management buttons
                tk::MessageBox         *wWConfirm;                  // Confirmation meessage box
                tk::TabControl         *wPresetTabs;                // Preset category tabs
                bool                    bWasVisible;                // Visibility flag

                preset_list_t           vPresetsLists[ui::PRESET_TAB_TOTAL];
                ConfigSink             *pConfigSink;                // Configuration sink

                const ui::preset_t     *pNewPreset;                 // New preset pointer

                ui::IPort              *pPath;                      // Location of user's export/import directory
                ui::IPort              *pFileType;                  // Import/export configuration file type selection
                ui::IPort              *pRelPaths;                  // Relative paths configuration option


            protected:
                // Slots
                static status_t     slot_window_close(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_save_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_favourite_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_remove_click(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_select(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_dbl_click(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_tab_selected(tk::Widget *sender, void *ptr, void *data);

                static status_t     slot_refresh_preset_list(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_reset_settings(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_import_settings_from_clipboard(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_export_settings_to_clipboard(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_submit_import_settings(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_submit_export_settings(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_create_preset_window_closed(tk::Widget *sender, void *ptr, void *data);

                static status_t     slot_fetch_path(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_commit_path(tk::Widget *sender, void *ptr, void *data);

                static status_t     slot_exec_export_settings_to_file(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_exec_import_settings_from_file(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_relative_path_changed(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_preset_filter_changed(tk::Widget *sender, void *ptr, void *data);

                static status_t     slot_accept_preset_selection(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_reject_preset_selection(tk::Widget *sender, void *ptr, void *data);

            protected:
                static void         destroy_preset_list(preset_list_t *list);
                static inline bool  need_indication(size_t i);

            protected:
                tk::FileFilters    *create_config_filters(tk::FileDialog *dlg);
                void                make_preset_list(preset_list_t *list, const ui::preset_t *presets, size_t count, ui::preset_filter_t filter, bool indicate);
                void                sync_preset_button_state();
                void                sync_preset_button_state(const ui::preset_t *preset);
                void                sync_preset_lists();
                void                sync_preset_tab();
                void                do_destroy();
                void                select_active_preset(const ui::preset_t *preset);
                bool                has_path_ports();
                bool                request_change_preset_conrifmation(const ui::preset_t *preset);
                const ui::preset_t *current_preset();
                status_t            create_save_preset_dialog();
                void                sync_preset_name(tk::ListBoxItem *item, const ui::preset_t *preset, bool indicate);

            public:
                explicit PresetsWindow(ui::IWrapper *src, tk::Window *widget, PluginWindow *pluginWindow);
                PresetsWindow(const PresetsWindow &) = delete;
                PresetsWindow(PresetsWindow &&) = delete;
                virtual ~PresetsWindow() override;

                PresetsWindow & operator = (const PresetsWindow &) = delete;
                PresetsWindow & operator = (PresetsWindow &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public: // ui::IPresetListener
                virtual void        preset_activated(const ui::preset_t *preset) override;
                virtual void        preset_deactivated(const ui::preset_t *preset) override;
                virtual void        presets_updated() override;

            public:
                status_t            post_init();
                status_t            show(tk::Widget *actor);
                status_t            hide();
                bool                visible() const;
                status_t            toggle_visibility(tk::Widget *actor);
                status_t            show_export_settings_dialog();
                status_t            show_import_settings_dialog();
                status_t            import_settings_from_clipboard();
                status_t            export_settings_to_clipboard();
                status_t            reset_settings();
                void                select_next_preset(bool forward);
        };


    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_PRESETSWINDOW_H_ */
