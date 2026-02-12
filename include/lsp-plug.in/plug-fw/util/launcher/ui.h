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

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/util/launcher/config.h>
#include <lsp-plug.in/plug-fw/util/launcher/plugin_metadata.h>
#include <lsp-plug.in/plug-fw/util/launcher/visual_schema.h>

#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace launcher
    {
        class UI: public ui::IWrapper, public ui::IPortListener
        {
            protected:
                struct plugin_t;
                struct bundle_t;
                struct category_t;

                typedef struct category_t
                {
                    tk::Widget             *wRoot;              // Root widget
                    tk::Label              *wHeading;           // Heading widget
                    meta::bundle_group_t    enCategory;         // Bundle category
                    const char             *sUID;               // Unique identifier
                    size_t                  nVisibility;        // Visibility counter
                    lltl::parray<bundle_t>  vBundles;           // List of bundles
                    LSPString               sNativeName;        // Native name
                    LSPString               sDefaultName;       // Default name
                } category_t;

                typedef struct bundle_t
                {
                    const meta::bundle_t   *pMeta;              // Bundle metadata
                    category_t             *pCategory;          // Bundle category
                    tk::Widget             *wRoot;              // Root widget
                    tk::Grid               *wBundle;            // Bundle grid
                    tk::Label              *wHeading;           // Bundle heading
                    tk::Label              *wDescription;       // Bundle description
                    tk::Box                *wImages;            // Box with images
                    tk::Grid               *wButtons;           // Grid with launch buttons
                    tk::Button             *wFavouries;         // Favourites mark button
                    tk::Button             *wHelp;              // Help button
                    size_t                  nVisibility;        // Visibility counter
                    size_t                  nActivePlugin;      // Active plugin index
                    lltl::parray<plugin_t>  vPlugins;           // List of plugins
                    LSPString               sNativeName;        // Native name
                    LSPString               sDefaultName;       // Default name
                } bundle_t;

                typedef struct plugin_t
                {
                    const meta::plugin_t   *pMeta;              // Plugin metadata
                    bundle_t               *pBundle;            // Plugin bundle
                    tk::Image              *wImage;             // Plugin image
                    tk::Button             *wButton;            // Launch button
                    tk::prop::String        sName;              // Plugin name
                    LSPString               sNativeName;        // Native name
                    LSPString               sDefaultName;       // Default name
                } plugin_t;

                typedef struct res_sel_t
                {
                    LSPString               sLocation;          // Location/name of resource
                    tk::ListBoxItem        *wItem;              // Associated ListBox item
                } res_sel_t;

            protected:
                const meta::package_t      *pPackage;           // Package descriptor
                const meta::plugin_t      **pLaunch;            // Pointer to store metadata of launched plugin
                config_t                    sConfig;            // Launcher configuration
                ctl::UIScaling              sUIScaling;         // UI scaling controller
                ctl::FontScaling            sFontScaling;       // Font scaling controller
                ctl::Documentation          sDocumentation;     // Documentation
                ctl::AboutWindow           *pAboutWindow;       // About window
                size_t                      nConfigChanges;     // Number of configuration changes

                ui::IPort                  *pWindowWidth;       // Launcher window width
                ui::IPort                  *pWindowHeight;      // Launcher window height
                ui::IPort                  *pLanguage;          // Language selection
                ui::IPort                  *pVisualSchema;      // Visual schema selection
                tk::Edit                   *wFilter;            // Filter edit
                tk::TabControl             *wTabs;              // Tab control for tabs
                tk::Widget                 *wLanguageArea;      // Language selector area
                tk::ComboBox               *wLanguage;          // Language selector
                tk::Widget                 *wSchemaArea;        // Visual schema selector area
                tk::ComboBox               *wVisualSchema;      // Visual schema selector
                tk::WidgetContainer        *wAllBundles;        // Container with full list of bundles
                tk::WidgetContainer        *wFavourites;        // Container with favourites

                lltl::parray<plugin_t>      vPlugins;
                lltl::parray<bundle_t>      vBundles;
                lltl::parray<category_t>    vCategories;
                lltl::parray<res_sel_t>     vLangSel;
                lltl::parray<res_sel_t>     vSchemaSel;

            protected:
                template <typename T>
                T                          *create_widget(const char *style);

            protected:
                static status_t             slot_display_idle(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_window_show(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_window_close(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_window_resize(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_filter_change(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_change_tab(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_plugin_mouse_in(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_plugin_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_toggle_favourites(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_select_language(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_select_visual_schema(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_show_about(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_show_bundle_manual(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_show_ui_manual(tk::Widget *sender, void *ptr, void *data);

            protected:
                static ssize_t              plugin_cmp_function(const plugin_t *a, const plugin_t *b);

            protected:
                void                        do_destroy();
                status_t                    build_ui();
                status_t                    scan_metadata();
                status_t                    create_catalog();
                status_t                    create_launcher_ports();
                void                        on_window_resize();
                void                        sync_widget_visibility();
                void                        sync_language_selection();
                void                        sync_visual_schema_selection();
                void                        select_plugin_image(tk::Widget *sender, bool select);
                status_t                    post_init();
                void                        deploy_config();
                bool                        match_filter(const plugin_t *p, const LSPString *filter, bool favourites);
                void                        bind_trigger(const char *uid, tk::slot_t ev, tk::event_handler_t handler);
                void                        sync_favourites_state(bundle_t *b);
                status_t                    init_i18n_support();
                status_t                    init_visual_schema_support();
                i18n::IDictionary          *get_default_dict();
                status_t                    locate_window();

            public:
                UI(resource::ILoader * loader, const meta::package_t *package, const meta::plugin_t **launch);
                virtual ~UI() override;
                UI(const UI &) = delete;
                UI(UI &&) = delete;

                UI & operator = (const UI &) = delete;
                UI & operator = (UI &&) = delete;

                virtual status_t            init(void *root_widget) override;
                virtual void                destroy() override;

            public: // ui::IWrapper
                virtual const meta::package_t      *package() const override;
                virtual void                host_scaling_changed() override;
                virtual void                main_iteration() override;

            public: // ui::IPortListener
                virtual void notify(ui::IPort *port, size_t flags);

            public:
                status_t                    main_loop();
        };


    } /* namespace launcher */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_LAUNCHER_UI_H_ */
