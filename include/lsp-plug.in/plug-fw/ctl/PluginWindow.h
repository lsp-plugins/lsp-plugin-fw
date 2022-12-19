/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 13 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_PLUGINWINDOW_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_PLUGINWINDOW_H_

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
        /**
         * The plugin's window controller
         */
        class PluginWindow: public ctl::Window
        {
            public:
                static const ctl_class_t metadata;

            private:
                PluginWindow & operator = (const PluginWindow &);
                PluginWindow(const PluginWindow &);

            private:
                static const tk::tether_t top_tether[];
                static const tk::tether_t bottom_tether[];

            protected:
                typedef struct backend_sel_t
                {
                    ctl::PluginWindow  *ctl;
                    tk::MenuItem       *item;
                    size_t              id;
                } backend_sel_t;

                typedef struct lang_sel_t
                {
                    ctl::PluginWindow  *ctl;
                    LSPString           lang;
                    tk::MenuItem       *item;
                } lang_sel_t;

                typedef struct scaling_sel_t
                {
                    ctl::PluginWindow  *ctl;
                    float               scaling;
                    tk::MenuItem       *item;
                } scaling_sel_t;

                typedef struct schema_sel_t
                {
                    ctl::PluginWindow  *ctl;
                    tk::MenuItem       *item;
                    LSPString           location;
                } schema_sel_t;

                typedef struct preset_sel_t
                {
                    ctl::PluginWindow  *ctl;
                    tk::MenuItem       *item;
                    bool                patch;
                    LSPString           location;
                } preset_sel_t;

                typedef struct window_scale_t
                {
                    size_t              nMFlags;
                    ws::rectangle_t     sSize;
                    bool                bActive;
                    ssize_t             nMouseX;
                    ssize_t             nMouseY;
                } window_scale_t;

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
                bool                        bResizable;

                tk::WidgetContainer        *wContent;       // The main box containing all widgets
                tk::Window                 *wGreeting;      // Greeting message window
                tk::Window                 *wAbout;         // About message window
                tk::Menu                   *wMenu;          // Menu
                tk::Menu                   *wUIScaling;     // UI Scaling menu
                tk::Menu                   *wFontScaling;   // UI Scaling menu
                tk::Menu                   *wResetSettings; // Reset settings menu
                tk::FileDialog             *wExport;        // Export settings dialog
                tk::FileDialog             *wImport;        // Import settings dialog
                tk::MenuItem               *wPreferHost;    // Prefer host menu item
                tk::CheckBox               *wRelPaths;       // Relative path checkbox

                ui::IPort                  *pPVersion;
                ui::IPort                  *pPBypass;
                ui::IPort                  *pPath;
                ui::IPort                  *pR3DBackend;
                ui::IPort                  *pLanguage;
                ui::IPort                  *pRelPaths;
                ui::IPort                  *pUIScaling;
                ui::IPort                  *pUIScalingHost;
                ui::IPort                  *pUIFontScaling;
                ui::IPort                  *pVisualSchema;

                ConfigSink                 *pConfigSink;    // Configuration sink

                window_scale_t              sWndScale;

                lltl::parray<backend_sel_t> vBackendSel;
                lltl::parray<lang_sel_t>    vLangSel;
                lltl::parray<scaling_sel_t> vScalingSel;
                lltl::parray<scaling_sel_t> vFontScalingSel;
                lltl::parray<schema_sel_t>  vSchemaSel;
                lltl::parray<preset_sel_t>  vPresetSel;

            protected:
                static status_t slot_window_close(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_window_show(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_greeting_close(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_about_close(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_main_menu(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_ui_scaling_menu(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_font_scaling_menu(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_show_plugin_manual(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_ui_manual(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_about(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_export_settings_to_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_export_settings_to_clipboard(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_import_settings_from_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_import_settings_from_clipboard(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_reset_settings(tk::Widget *sender, void *ptr, void *data);\
                static status_t slot_confirm_reset_settings(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_debug_dump(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_call_export_settings_to_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_call_import_settings_from_file(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_fetch_path(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_commit_path(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_select_backend(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_select_language(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_select_preset(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_scaling_toggle_prefer_host(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scaling_select(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_font_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_font_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_font_scaling_select(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_visual_schema_select(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_window_resize(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_scale_mouse_down(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scale_mouse_move(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scale_mouse_up(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_relative_path_changed(tk::Widget *sender, void *ptr, void *data);

            protected:
                static i18n::IDictionary   *get_default_dict(tk::Widget *src);
                static tk::FileFilters     *create_config_filters(tk::FileDialog *dlg);
                static ssize_t              compare_presets(const resource::resource_t *a, const resource::resource_t *b);

            protected:
                void                do_destroy();
                status_t            show_greeting_window();
                status_t            show_about_window();
                status_t            locate_window();
                status_t            show_menu(tk::Widget *menu, tk::Widget *actor, void *data);
                tk::Label          *create_label(tk::WidgetContainer *dst, const char *key, const char *style_name);
                tk::Label          *create_plabel(tk::WidgetContainer *dst, const char *key, const expr::Parameters *params, const char *style_name);
                tk::Hyperlink      *create_hlink(tk::WidgetContainer *dst, const char *url, const char *text, const expr::Parameters *params, const char *style_name);
                tk::MenuItem       *create_menu_item(tk::Menu *dst);
                tk::Menu           *create_menu();
                status_t            create_dialog_window(ctl::Window **ctl, tk::Window **dst, const char *path);

                status_t            init_r3d_support(tk::Menu *menu);
                status_t            init_i18n_support(tk::Menu *menu);
                status_t            init_scaling_support(tk::Menu *menu);
                status_t            init_font_scaling_support(tk::Menu *menu);
                status_t            init_visual_schema_support(tk::Menu *menu);
                status_t            init_presets(tk::Menu *menu);
                status_t            scan_presets(const char *location, lltl::darray<resource::resource_t> *presets);
                status_t            create_main_menu();
                status_t            create_reset_settings_menu();
                bool                has_path_ports();
                void                sync_language_selection();
                void                sync_ui_scaling();
                void                sync_font_scaling();
                void                sync_visual_schemas();
                void                bind_trigger(const char *uid, tk::slot_t ev, tk::event_handler_t handler);

                status_t            init_context(ui::UIContext *ctx);

            public:
                explicit PluginWindow(ui::IWrapper *src, tk::Window *widget);
                virtual ~PluginWindow();

                /** Init widget
                 *
                 */
                virtual status_t    init();

                /**
                 * Destroy widget
                 */
                virtual void        destroy();

            public:
                virtual void        begin(ui::UIContext *ctx);

                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);

                virtual status_t    add(ui::UIContext *ctx, ctl::Widget *child);

                virtual void        end(ui::UIContext *ctx);

                virtual void        notify(ui::IPort *port);
        };
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_PLUGINWINDOW_H_ */
