/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
        class PresetsWindow;

        /**
         * The plugin's window controller
         */
        class PluginWindow: public ctl::Window
        {
            public:
                static const ctl_class_t metadata;

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

                typedef struct enum_menu_t
                {
                    PluginWindow       *pWnd;
                    tk::Menu           *pMenu;
                    ui::IPort          *pPort;
                    lltl::parray<tk::MenuItem>  vItems;
                } enum_menu_t;

                typedef struct ui_flag_t
                {
                    ui::IPort          *pPort;
                    tk::MenuItem       *wItem;
                } ui_flag_t;

            protected:
                bool                        bResizable;

                ctl::Window                *pUserPaths;                 // User paths controller
                ctl::PresetsWindow         *pPresetsWindow;             // Presets window

                tk::WidgetContainer        *wContent;                   // The main box containing all widgets
                tk::Window                 *wGreeting;                  // Greeting message window
                tk::Window                 *wAbout;                     // About message window
                tk::Window                 *wUserPaths;                 // User paths configuration
                tk::Menu                   *wMenu;                      // Menu
                tk::Menu                   *wPresets;                   // Presets menu
                tk::Menu                   *wUIScaling;                 // UI Scaling menu
                tk::Menu                   *wBundleScaling;             // Bundle Scaling menu
                tk::Menu                   *wFontScaling;               // UI Scaling menu
                tk::MenuItem               *wPreferHost;                // Prefer host menu item
                tk::CheckBox               *wRelPaths;                  // Relative path checkbox
                tk::MenuItem               *wInvertVScroll;             // Global inversion of mouse vertical scroll
                tk::MenuItem               *wInvertGraphDotVScroll;     // Invert mouse vertical scroll for GraphDot widgets
                tk::Menu                   *wFilterPointThickness;      // Filter point thickness submenu
                tk::Timer                   wGreetingTimer;             // Greeting window timer

                ui::IPort                  *pPVersion;
                ui::IPort                  *pPBypass;
                ui::IPort                  *pR3DBackend;
                ui::IPort                  *pLanguage;
                ui::IPort                  *pUIScaling;
                ui::IPort                  *pUIScalingHost;
                ui::IPort                  *pUIBundleScaling;
                ui::IPort                  *pUIFontScaling;
                ui::IPort                  *pVisualSchema;
                ui::IPort                  *pInvertVScroll;
                ui::IPort                  *pInvertGraphDotVScroll;

                window_scale_t              sWndScale;
                enum_menu_t                 sFilterPointThickness;

                lltl::parray<backend_sel_t> vBackendSel;
                lltl::parray<lang_sel_t>    vLangSel;
                lltl::parray<scaling_sel_t> vScalingSel;
                lltl::parray<scaling_sel_t> vBundleScalingSel;
                lltl::parray<scaling_sel_t> vFontScalingSel;
                lltl::parray<schema_sel_t>  vSchemaSel;
                lltl::parray<preset_sel_t>  vPresetSel;
                lltl::darray<ui_flag_t>     vBoolFlags;

            protected:
                static status_t slot_window_close(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_window_show(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_greeting_close(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_about_close(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_main_menu(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_presets_menu(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_ui_scaling_menu(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_bundle_scaling_menu(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_font_scaling_menu(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_show_plugin_manual(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_ui_manual(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_about(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_presets_window(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_export_settings_to_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_export_settings_to_clipboard(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_import_settings_from_file(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_import_settings_from_clipboard(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_reset_settings(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_debug_dump(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_select_backend(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_select_language(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_select_preset(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_scaling_toggle_prefer_host(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scaling_select(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_bundle_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_bundle_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_bundle_scaling_select(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_font_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_font_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_font_scaling_select(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_visual_schema_select(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_window_resize(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_scale_mouse_down(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scale_mouse_move(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_scale_mouse_up(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_show_user_paths_dialog(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_user_paths_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_user_paths_close(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_invert_vscroll_changed(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_invert_graph_dot_vscroll_changed(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_submit_enum_menu_item(tk::Widget *sender, void *ptr, void *data);

                static status_t timer_show_greeting(ws::timestamp_t sched, ws::timestamp_t time, void *arg);

                static status_t slot_ui_behaviour_flag_changed(tk::Widget *sender, void *ptr, void *data);

            protected:
                static i18n::IDictionary   *get_default_dict(tk::Widget *src);
                static ssize_t              compare_presets(const resource::resource_t *a, const resource::resource_t *b);
                void                        init_enum_menu(enum_menu_t *menu);

                status_t                    add_scaling_menu_item(
                    lltl::parray<scaling_sel_t> & list,
                    tk::Menu *menu, const char *key, size_t scale,
                    tk::event_handler_t handler);

            protected:
                void                do_destroy();
                status_t            set_greeting_timer();
                status_t            show_greeting_window();
                status_t            show_presets_window();
                status_t            show_user_paths_window();
                status_t            fmt_package_version(LSPString &pkgver);
                status_t            locate_window();
                status_t            show_menu(tk::Widget *menu, tk::Widget *actor, void *data);
                ssize_t             get_bundle_scaling();
                tk::Label          *create_label(tk::WidgetContainer *dst, const char *key, const char *style_name);
                tk::Label          *create_plabel(tk::WidgetContainer *dst, const char *key, const expr::Parameters *params, const char *style_name);
                tk::Hyperlink      *create_hlink(tk::WidgetContainer *dst, const char *url, const char *text, const expr::Parameters *params, const char *style_name);
                tk::MenuItem       *create_menu_item(tk::Menu *dst);
                tk::Menu           *create_menu();
                tk::Menu           *create_enum_menu(enum_menu_t *em, tk::Menu *parent, const char *label);
                status_t            create_dialog_window(ctl::Window **ctl, tk::Window **dst, const char *path);
                status_t            create_presets_window();

                status_t            init_r3d_support(tk::Menu *menu);
                status_t            init_i18n_support(tk::Menu *menu);
                status_t            init_scaling_support(tk::Menu *menu);
                status_t            init_bundle_scaling_support(tk::Menu *menu);
                status_t            init_font_scaling_support(tk::Menu *menu);
                status_t            init_visual_schema_support(tk::Menu *menu);
                status_t            init_ui_behaviour(tk::Menu *menu);
                status_t            add_ui_flag(tk::Menu *menu, const char *port, const char *key);
                status_t            init_presets(tk::Menu *menu, bool add_submenu);
                status_t            create_main_menu();
                void                sync_ui_scaling();
                bool                has_path_ports();
                void                sync_language_selection();
                void                sync_font_scaling();
                void                sync_visual_schemas();
                void                sync_invert_vscroll(ui::IPort *port);
                void                sync_ui_behaviour_flags(ui::IPort *port);
                void                notify_ui_behaviour_flags(size_t flags);
                void                sync_enum_menu(enum_menu_t *menu, ui::IPort *port);
                void                apply_user_paths_settings();
                void                read_path_param(LSPString *value, const char *port_id);
                void                read_path_param(tk::String *value, const char *port_id);
                void                read_bool_param(tk::Boolean *value, const char *port_id);
                void                commit_path_param(tk::String *value, const char *port_id);
                void                commit_bool_param(tk::Boolean *value, const char *port_id);
                void                bind_trigger(const char *uid, tk::slot_t ev, tk::event_handler_t handler);
                bool                open_manual_file(const char *fmt...);

                void                set_preset_button_text(const char *text);

                status_t            init_context(ui::UIContext *ctx);

            public:
                explicit PluginWindow(ui::IWrapper *src, tk::Window *widget);
                PluginWindow(const PluginWindow &) = delete;
                PluginWindow(PluginWindow &&) = delete;
                virtual ~PluginWindow() override;

                PluginWindow & operator = (const PluginWindow &) = delete;
                PluginWindow & operator = (PluginWindow &&) = delete;

                /** Init widget
                 *
                 */
                virtual status_t    init() override;

                /**
                 * Destroy widget
                 */
                virtual void        destroy() override;

            public:
                status_t            show_about_window();
                status_t            show_plugin_manual();
                status_t            show_ui_manual();
                void                host_scaling_changed();

            public:
                virtual void        begin(ui::UIContext *ctx) override;
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual status_t    add(ui::UIContext *ctx, ctl::Widget *child) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_PLUGINWINDOW_H_ */
