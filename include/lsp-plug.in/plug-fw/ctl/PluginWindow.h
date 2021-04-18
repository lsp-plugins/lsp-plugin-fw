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
        class PluginWindow: public ctl::Widget
        {
            public:
                static const ctl_class_t metadata;

//            protected:
//                typedef struct backend_sel_t
//                {
//                    CtlPluginWindow    *ctl;
//                    LSPWidget          *item;
//                    size_t              id;
//                } backend_sel_t;

                typedef struct lang_sel_t
                {
                    ctl::PluginWindow  *ctl;
                    LSPString           lang;
                    tk::MenuItem       *item;
                } lang_sel_t;

            protected:
                bool                        bResizable;

                lltl::parray<tk::Widget>    vWidgets;   // List of created widgets
                tk::Box                    *wBox;       // The main box containing all widgets
                tk::Window                 *wMessage;   // Greeting message window
                tk::Widget                 *wRack[3];   // Rack ears
                tk::Menu                   *wMenu;      // Menu

                ui::IPort                  *pPMStud;
                ui::IPort                  *pPVersion;
                ui::IPort                  *pPBypass;
                ui::IPort                  *pPath;
                ui::IPort                  *pR3DBackend;
                ui::IPort                  *pLanguage;
                ui::IPort                  *pRelPaths;

//                LSPFileDialog      *pImport;
//                LSPFileDialog      *pExport;

//
//                cstorage<backend_sel_t>     vBackendSel;
                lltl::parray<lang_sel_t>    vLangSel;

            protected:
                static status_t slot_window_close(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_window_show(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_message_close(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_menu(tk::Widget *sender, void *ptr, void *data);

                static status_t slot_show_plugin_manual(tk::Widget *sender, void *ptr, void *data);
                static status_t slot_show_ui_manual(tk::Widget *sender, void *ptr, void *data);
//                static status_t slot_export_settings_to_file(LSPWidget *sender, void *ptr, void *data);
//                static status_t slot_import_settings_from_file(LSPWidget *sender, void *ptr, void *data);
//                static status_t slot_export_settings_to_clipboard(LSPWidget *sender, void *ptr, void *data);
//                static status_t slot_import_settings_from_clipboard(LSPWidget *sender, void *ptr, void *data);

                static status_t slot_toggle_rack_mount(tk::Widget *sender, void *ptr, void *data);


                static status_t slot_debug_dump(tk::Widget *sender, void *ptr, void *data);


//                static status_t slot_call_export_settings_to_file(LSPWidget *sender, void *ptr, void *data);
//                static status_t slot_call_import_settings_to_file(LSPWidget *sender, void *ptr, void *data);
//
//
//                static status_t slot_fetch_path(LSPWidget *sender, void *ptr, void *data);
//                static status_t slot_commit_path(LSPWidget *sender, void *ptr, void *data);
//
//                static status_t slot_select_backend(LSPWidget *sender, void *ptr, void *data);

                static status_t slot_select_language(tk::Widget *sender, void *ptr, void *data);

            protected:
                static void                inject_style(tk::Widget *widget, const char *style_name);
                static i18n::IDictionary  *get_default_dict(tk::Widget *src);

            protected:
                void                do_destroy();
                status_t            show_notification();
                status_t            show_menu(tk::Widget *actor, void *data);
                tk::Label          *create_label(tk::WidgetContainer *dst, const char *key, const char *style_name);
                tk::Label          *create_plabel(tk::WidgetContainer *dst, const char *key, const expr::Parameters *params, const char *style_name);
                tk::Hyperlink      *create_hlink(tk::WidgetContainer *dst, const char *text, const char *style_name);

//                status_t        init_r3d_support(LSPMenu *menu);
                status_t            init_i18n_support(tk::Menu *menu);
                status_t            init_window_layout();
                status_t            create_main_menu();
                bool                has_path_ports();
                void                sync_language_selection();

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
                virtual void        set(const char *name, const char *value);

                virtual status_t    add(ctl::Widget *child);

                virtual void        end();

                virtual void        notify(ui::IPort *port);
        };
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_PLUGINWINDOW_H_ */
