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

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/plug-fw/meta/ports.h>

namespace lsp
{
    namespace ctl
    {
        const ctl_class_t PluginWindow::metadata = { "PluginWindow", &Widget::metadata };

        PluginWindow::PluginWindow(ui::IWrapper *src, tk::Window *widget): Widget(src, widget)
        {
            pClass          = &metadata;

            bResizable      = false;

            wBox            = NULL;
            wMessage        = NULL;

            pPMStud         = NULL;
            pPVersion       = NULL;
            pPBypass        = NULL;
            pPath           = NULL;
            pR3DBackend     = NULL;
            pLanguage       = NULL;
            pRelPaths       = NULL;

//            pMessage        = NULL;
//            bResizable      = false;
//            nVisible        = 1;
//            pUI             = src;
//            pBox            = NULL;
//            pMenu           = NULL;
//            pImport         = NULL;
//            pExport         = NULL;

//            nVisible        = 0;
        }

        PluginWindow::~PluginWindow()
        {
            do_destroy();
        }

        void PluginWindow::destroy()
        {
            do_destroy();
            Widget::destroy();
        }

        void PluginWindow::do_destroy()
        {
            // Unregister all child widgets
            if (pWrapper != NULL)
                pWrapper->ui()->unmap_widgets(vWidgets.array(), vWidgets.size());

            // Destroy all nested widgets
            for (size_t i=0, n=vWidgets.size(); i<n; ++i)
            {
                tk::Widget *w = vWidgets.uget(i);
                if (w == NULL)
                    continue;
                w->destroy();
                delete w;
            }
            vWidgets.flush();

//            for (size_t i=0, n=vLangSel.size(); i<n; ++i)
//            {
//                lang_sel_t *s = vLangSel.at(i);
//                if (s != NULL)
//                    delete s;
//            }
//
//            vBackendSel.flush();
//            vLangSel.flush();
        }

        status_t PluginWindow::slot_window_close(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this->pWrapper != NULL)
                _this->pWrapper->quit_main_loop();
            return STATUS_OK;
        }

//        status_t PluginWindow::slot_window_show(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            __this->show_notification();
//            return STATUS_OK;
//        }

        void PluginWindow::set(const char *name, const char *value)
        {
            if (!strcmp(name, "resizable"))
                PARSE_BOOL(value, bResizable = __)

            tk::Window *wnd = tk::widget_cast<tk::Window>(pWidget);
            if (wnd != NULL)
                set_constraints(wnd->constraints(), name, value);
            Widget::set(name, value);
        }

        status_t PluginWindow::init()
        {
            Widget::init();

            // Get window handle
            tk::Window *wnd = tk::widget_cast<tk::Window>(pWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            // Bind ports
            BIND_PORT(pWrapper, pPMStud, MSTUD_PORT);
            BIND_PORT(pWrapper, pPVersion, VERSION_PORT);
            BIND_PORT(pWrapper, pPath, CONFIG_PATH_PORT);
            BIND_PORT(pWrapper, pPBypass, meta::PORT_NAME_BYPASS);
            BIND_PORT(pWrapper, pR3DBackend, R3D_BACKEND_PORT);
            BIND_PORT(pWrapper, pLanguage, LANGUAGE_PORT);
            BIND_PORT(pWrapper, pRelPaths, REL_PATHS_PORT);

            const meta::plugin_t *meta   = pWrapper->ui()->metadata();

            // Initialize window
//            tk::Display *dpy    = wnd->display();

            wnd->set_class(meta->lv2_uid, "lsp-plugins");
            wnd->role()->set("audio-plugin");
            wnd->title()->set_raw(meta->name);

            if (!wnd->nested())
                wnd->actions()->deny(ws::WA_RESIZE);


//            {
//                // Initialize menu
//                pMenu = new LSPMenu(dpy);
//                pMenu->set_unique_id(WUID_MAIN_MENU);
//                vWidgets.add(pMenu);
//                pMenu->init();
//
//                // Initialize menu items
//                {
//                    // Create 'About Plugin' menu item
//                    LSPMenu *submenu = new LSPMenu(dpy);
//                    vWidgets.add(submenu);
//                    submenu->init();
//                    submenu->set_unique_id(WUID_EXPORT_MENU);
//
//                    // Add 'Plugin manual' menu item
//                    LSPMenuItem *itm     = new LSPMenuItem(dpy);
//                    vWidgets.add(itm);
//                    itm->init();
//                    itm->text()->set("actions.plugin_manual");
//                    itm->slots()->bind(LSPSLOT_SUBMIT, slot_show_plugin_manual, this);
//                    pMenu->add(itm);
//
//                    // Add 'UI manual' menu item
//                    itm                 = new LSPMenuItem(dpy);
//                    vWidgets.add(itm);
//                    itm->init();
//                    itm->text()->set("actions.ui_manual");
//                    itm->slots()->bind(LSPSLOT_SUBMIT, slot_show_ui_manual, this);
//                    pMenu->add(itm);
//
//
//                    // Add separator
//                    itm     = new LSPMenuItem(dpy);
//                    vWidgets.add(itm);
//                    itm->init();
//                    itm->set_separator(true);
//                    pMenu->add(itm);
//
//                    // Create export menu
//                    submenu = new LSPMenu(dpy);
//                    vWidgets.add(submenu);
//                    submenu->init();
//                    submenu->set_unique_id(WUID_EXPORT_MENU);
//
//                    itm = new LSPMenuItem(dpy);
//                    vWidgets.add(itm);
//                    itm->init();
//                    itm->text()->set("actions.export");
//                    itm->set_submenu(submenu);
//                    pMenu->add(itm);
//
//                    // Create export menu items
//                    {
//                        LSPMenuItem *child = new LSPMenuItem(dpy);
//                        vWidgets.add(child);
//                        child->init();
//                        child->text()->set("actions.export_settings_to_file");
//                        child->slots()->bind(LSPSLOT_SUBMIT, slot_export_settings_to_file, this);
//                        submenu->add(child);
//
//                        child = new LSPMenuItem(dpy);
//                        vWidgets.add(child);
//                        child->init();
//                        child->text()->set("actions.export_settings_to_clipboard");
//                        child->slots()->bind(LSPSLOT_SUBMIT, slot_export_settings_to_clipboard, this);
//                        submenu->add(child);
//                    }
//
//                    // Create import menu
//                    submenu = new LSPMenu(dpy);
//                    vWidgets.add(submenu);
//                    submenu->init();
//                    submenu->set_unique_id(WUID_IMPORT_MENU);
//
//                    itm = new LSPMenuItem(dpy);
//                    vWidgets.add(itm);
//                    itm->init();
//                    itm->text()->set("actions.import");
//                    itm->set_submenu(submenu);
//                    pMenu->add(itm);
//
//                    // Create import menu items
//                    {
//                        LSPMenuItem *child = new LSPMenuItem(dpy);
//                        vWidgets.add(child);
//                        child->init();
//                        child->text()->set("actions.import_settings_from_file");
//                        child->slots()->bind(LSPSLOT_SUBMIT, slot_import_settings_from_file, this);
//                        submenu->add(child);
//
//                        child = new LSPMenuItem(dpy);
//                        vWidgets.add(child);
//                        child->init();
//                        child->text()->set("actions.import_settings_from_clipboard");
//                        child->slots()->bind(LSPSLOT_SUBMIT, slot_import_settings_from_clipboard, this);
//                        submenu->add(child);
//                    }
//
//                    // Add separator
//                    itm     = new LSPMenuItem(dpy);
//                    vWidgets.add(itm);
//                    itm->init();
//                    itm->set_separator(true);
//                    pMenu->add(itm);
//
//                    // Create 'Toggle rack mount' menu item
//                    itm     = new LSPMenuItem(dpy);
//                    vWidgets.add(itm);
//                    itm->init();
//                    itm->text()->set("actions.toggle_rack_mount");
//                    itm->slots()->bind(LSPSLOT_SUBMIT, slot_toggle_rack_mount, this);
//                    pMenu->add(itm);
//
//                    // Create 'Dump state' menu item if supported
//                    if (meta->extensions & E_DUMP_STATE)
//                    {
//                        itm     = new LSPMenuItem(dpy);
//                        vWidgets.add(itm);
//                        itm->init();
//                        itm->text()->set("actions.debug_dump");
//                        itm->slots()->bind(LSPSLOT_SUBMIT, slot_debug_dump, this);
//                        pMenu->add(itm);
//                    }
//
//                    // Create language selection menu
//                    init_i18n_support(pMenu);
//
//                    // Add support of 3D rendering backend switch
//                    if (meta->extensions & E_3D_BACKEND)
//                        init_r3d_support(pMenu);
//                }
//
//                // Initialize main grid
//                LSPGrid *grd = new LSPGrid(dpy);
//                vWidgets.add(grd);
//                pWnd->add(grd);
//                grd->init();
//                grd->set_rows(2);
//                grd->set_columns((pPBypass != NULL) ? 4 : 3);
//
//                    // Initialize mount studs
//                    char buf[80];
//                    sprintf(buf, "%s  %s", LSP_ACRONYM, meta->acronym);
//
//                    LSPMountStud *mstud = new LSPMountStud(dpy);
//                    vMStud[0] = mstud;
//                    vWidgets.add(mstud);
//                    mstud->init();
//                    mstud->set_angle(2);
//                    mstud->set_text(buf);
//                    mstud->font()->set_size(12);
//                    mstud->slots()->bind(LSPSLOT_SUBMIT, slot_show_menu_top, this);
//                    grd->add(mstud, 1, (pPBypass != NULL) ? 4 : 3);
//
//                    mstud   = new LSPMountStud(dpy);
//                    vMStud[1] = mstud;
//                    vWidgets.add(mstud);
//                    mstud->init();
//                    mstud->set_angle(1);
//                    mstud->set_text(LSP_ACRONYM);
//                    mstud->font()->set_size(16);
//                    mstud->slots()->bind(LSPSLOT_SUBMIT, slot_show_menu_left, this);
//                    grd->add(mstud);
//
//                    if (pPBypass != NULL)
//                    {
//                        LSPBox     *box = new LSPBox(dpy, false);
//                        vWidgets.add(box);
//                        box->init();
//                        box->set_fill(false);
//                        box->padding()->set_all(4);
//                        grd->add(box);
//
//                        LSPLabel   *lbl = new LSPLabel(dpy);
//                        vWidgets.add(lbl);
//                        lbl->init();
//                        lbl->text()->set("labels.bypass");
//                        box->add(lbl);
//
//                        LSPSwitch  *sw  = new LSPSwitch(dpy);
//                        vWidgets.add(sw);
//                        sw->init();
//                        sw->set_size(24);
//                        sw->set_angle(1);
//                        box->add(sw);
//
//                        LSPLed *led = new LSPLed(dpy);
//                        vWidgets.add(led);
//                        led->init();
//                        init_color(C_RED, led->color());
//                        led->set_size(8);
//                        box->add(led);
//
//                        CtlWidget *ctl = new CtlSwitch(pRegistry, sw);
//                        ctl->init();
//                        ctl->set("id", pPBypass->metadata()->id);
//                        ctl->begin();
//                        ctl->end();
//                        pRegistry->add_widget(ctl);
//
//                        ctl = new CtlLed(pRegistry, led);
//                        ctl->init();
//                        ctl->set("id", pPBypass->metadata()->id);
//                        ctl->begin();
//                        ctl->end();
//                        pRegistry->add_widget(ctl);
//                    }
//
//                    pBox    = new LSPBox(dpy);
//                    vWidgets.add(pBox);
//                    pBox->init();
//                    pBox->set_expand(true);
//                    pBox->set_fill(true);
//                    pBox->padding()->set_all(2);
//                    grd->add(pBox);
//
//                    mstud   = new LSPMountStud(dpy);
//                    vMStud[2] = mstud;
//                    vWidgets.add(mstud);
//                    mstud->init();
//                    mstud->set_angle(0);
//                    mstud->set_text(meta->acronym);
//                    mstud->font()->set_size(16);
//                    mstud->slots()->bind(LSPSLOT_SUBMIT, slot_show_menu_right, this);
//                    grd->add(mstud);
//            }
//
//            // Bind close handler
            wnd->slots()->bind(tk::SLOT_CLOSE, slot_window_close, this);
//            pWnd->slots()->bind(LSPSLOT_SHOW, slot_window_show, this);

            return STATUS_OK;
        }

//        status_t PluginWindow::init_i18n_support(LSPMenu *menu)
//        {
//            if (menu == NULL)
//                return STATUS_OK;
//
//            LSPDisplay *dpy   = menu->display();
//            if (dpy == NULL)
//                return STATUS_OK;
//
//            IDictionary *dict = dpy->dictionary();
//            if (dict == NULL)
//                return STATUS_OK;
//
//            // Perform lookup before loading list of languages
//            status_t res = dict->lookup("default.lang.target", &dict);
//            if (res != STATUS_OK)
//                return res;
//
//            // Create submenu item
//            LSPMenuItem *root       = new LSPMenuItem(menu->display());
//            if (root == NULL)
//                return STATUS_NO_MEM;
//            if ((res = root->init()) != STATUS_OK)
//            {
//                delete root;
//                return res;
//            }
//            if (!vWidgets.add(root))
//            {
//                root->destroy();
//                delete root;
//                return STATUS_NO_MEM;
//            }
//            root->text()->set("actions.select_language");
//            if ((res = menu->add(root)) != STATUS_OK)
//                return res;
//
//            // Create submenu
//            menu                = new LSPMenu(menu->display());
//            if (menu == NULL)
//                return STATUS_NO_MEM;
//            if ((res = menu->init()) != STATUS_OK)
//            {
//                menu->destroy();
//                delete menu;
//                return res;
//            }
//            if (!vWidgets.add(menu))
//            {
//                menu->destroy();
//                delete menu;
//                return STATUS_NO_MEM;
//            }
//            root->set_submenu(menu);
//
//            // Iterate all children and add language keys
//            LSPString key, value;
//            lang_sel_t *lang;
//            size_t added = 0;
//            for (size_t i=0, n=dict->size(); i<n; ++i)
//            {
//                // Fetch placeholder for language selection key
//                if ((res = dict->get_value(i, &key, &value)) != STATUS_OK)
//                {
//                    // Skip nested dictionaries
//                    if (res == STATUS_BAD_TYPE)
//                        continue;
//                    return res;
//                }
//                if ((lang = new lang_sel_t()) == NULL)
//                    return STATUS_NO_MEM;
//                if (!lang->lang.set(&key))
//                {
//                    delete lang;
//                    return STATUS_NO_MEM;
//                }
//                if (!vLangSel.add(lang))
//                {
//                    delete lang;
//                    return STATUS_NO_MEM;
//                }
//                lang->ctl   = this;
//
//                // Create menu item
//                LSPMenuItem *item = new LSPMenuItem(menu->display());
//                if (item == NULL)
//                    continue;
//                if ((res = item->init()) != STATUS_OK)
//                {
//                    item->destroy();
//                    delete item;
//                    continue;
//                }
//                if (!vWidgets.add(item))
//                {
//                    item->destroy();
//                    delete item;
//                    continue;
//                }
//
//                item->text()->set_raw(&value);
//                menu->add(item);
//
//                // Create closure and bind
//                item->slots()->bind(LSPSLOT_SUBMIT, slot_select_language, lang);
//
//                ++added;
//            }
//
//            // Set menu item visible only if there are available languages
//            root->set_visible(added > 0);
//            if (pLanguage != NULL)
//            {
//                const char *lang = pLanguage->get_buffer<char>();
//                ui_atom_t atom = dpy->atom_id("language");
//
//                if ((lang != NULL) && (strlen(lang) > 0) && (atom >= 0))
//                {
//                    LSPTheme *theme = dpy->theme();
//                    LSPStyle *style = (theme != NULL) ? theme->root() : NULL;
//                    if (style != NULL)
//                    {
//                        lsp_trace("System language set to: %s", lang);
//                        style->set_string(atom, lang);
//                    }
//                }
//            }
//
//            return STATUS_OK;
//        }
//
//        status_t PluginWindow::init_r3d_support(LSPMenu *menu)
//        {
//            if (menu == NULL)
//                return STATUS_OK;
//
//            IDisplay *dpy   = menu->display()->display();
//            if (dpy == NULL)
//                return STATUS_OK;
//
//            status_t res;
//
//            // Create submenu item
//            LSPMenuItem *item       = new LSPMenuItem(menu->display());
//            if (item == NULL)
//                return STATUS_NO_MEM;
//            if ((res = item->init()) != STATUS_OK)
//            {
//                delete item;
//                return res;
//            }
//            if (!vWidgets.add(item))
//            {
//                item->destroy();
//                delete item;
//                return STATUS_NO_MEM;
//            }
//
//            // Add item to the main menu
//            item->text()->set("actions.3d_rendering");
//            menu->add(item);
//
//            // Get backend port
//            const char *backend = (pR3DBackend != NULL) ? pR3DBackend->get_buffer<char>() : NULL;
//
//            // Create submenu
//            menu                = new LSPMenu(menu->display());
//            if (menu == NULL)
//                return STATUS_NO_MEM;
//            if ((res = menu->init()) != STATUS_OK)
//            {
//                menu->destroy();
//                delete menu;
//                return res;
//            }
//            if (!vWidgets.add(menu))
//            {
//                menu->destroy();
//                delete menu;
//                return STATUS_NO_MEM;
//            }
//            item->set_submenu(menu);
//
//            for (size_t id=0; ; ++id)
//            {
//                // Enumerate next backend information
//                const R3DBackendInfo *info = dpy->enum_backend(id);
//                if (info == NULL)
//                    break;
//
//                // Create menu item
//                item       = new LSPMenuItem(menu->display());
//                if (item == NULL)
//                    continue;
//                if ((res = item->init()) != STATUS_OK)
//                {
//                    item->destroy();
//                    delete item;
//                    continue;
//                }
//                if (!vWidgets.add(item))
//                {
//                    item->destroy();
//                    delete item;
//                    continue;
//                }
//
//                item->text()->set_raw(&info->display);
//                menu->add(item);
//
//                // Create closure and bind
//                backend_sel_t *sel = vBackendSel.add();
//                if (sel != NULL)
//                {
//                    sel->ctl    = this;
//                    sel->item   = item;
//                    sel->id     = id;
//                    item->slots()->bind(LSPSLOT_SUBMIT, slot_select_backend, sel);
//                }
//
//                // Backend identifier matches?
//                if ((backend == NULL) || (!info->uid.equals_ascii(backend)))
//                {
//                    slot_select_backend(item, sel, NULL);
//                    if (backend == NULL)
//                        backend     = info->uid.get_ascii();
//                }
//            }
//
//            return STATUS_OK;
//        }
//
//        bool PluginWindow::has_path_ports()
//        {
//            for (size_t i = 0, n = pUI->ports_count(); i < n; ++i)
//            {
//                CtlPort *p = pUI->port_by_index(i);
//                const port_t *meta = (p != NULL) ? p->metadata() : NULL;
//                if ((meta != NULL) && (meta->role == R_PATH))
//                    return true;
//            }
//            return false;
//        }
//
//        status_t PluginWindow::slot_select_backend(LSPWidget *sender, void *ptr, void *data)
//        {
//            backend_sel_t *sel = reinterpret_cast<backend_sel_t *>(ptr);
//            if ((sender == NULL) || (sel == NULL) || (sel->ctl == NULL))
//                return STATUS_BAD_ARGUMENTS;
//
//            IDisplay *dpy = sender->display()->display();
//            if (dpy == NULL)
//                return STATUS_BAD_STATE;
//
//            const R3DBackendInfo *info = dpy->enum_backend(sel->id);
//            if (info == NULL)
//                return STATUS_BAD_ARGUMENTS;
//
//            // Mark backend as selected
//            dpy->select_backend_id(sel->id);
//
//            // Need to commit backend identifier to config file?
//            const char *value = info->uid.get_ascii();
//            if (value == NULL)
//                return STATUS_NO_MEM;
//
//            if (sel->ctl->pR3DBackend != NULL)
//            {
//                const char *backend = sel->ctl->pR3DBackend->get_buffer<char>();
//                if ((backend == NULL) || (strcmp(backend, value)))
//                {
//                    sel->ctl->pR3DBackend->write(value, strlen(value));
//                    sel->ctl->pR3DBackend->notify_all();
//                }
//            }
//
//            return STATUS_OK;
//        }
//
//        status_t PluginWindow::slot_select_language(LSPWidget *sender, void *ptr, void *data)
//        {
//            lang_sel_t *sel = reinterpret_cast<lang_sel_t *>(ptr);
//            lsp_trace("sender=%p, sel=%p", sender, sel);
//            if ((sender == NULL) || (sel == NULL) || (sel->ctl == NULL))
//                return STATUS_BAD_ARGUMENTS;
//
//            LSPDisplay *dpy = sender->display();
//            lsp_trace("dpy = %p", dpy);
//            if (dpy == NULL)
//                return STATUS_BAD_STATE;
//            ui_atom_t atom = dpy->atom_id("language");
//            lsp_trace("atom = %d", int(atom));
//            if (atom < 0)
//                return STATUS_BAD_STATE;
//
//            LSPTheme *theme = dpy->theme();
//            lsp_trace("theme = %p", theme);
//            if (theme == NULL)
//                return STATUS_BAD_STATE;
//            LSPStyle *style = theme->root();
//            lsp_trace("style = %p", style);
//            if (style == NULL)
//                return STATUS_BAD_STATE;
//
//            const char *dlang = sel->lang.get_utf8();
//            lsp_trace("Select language: \"%s\"", dlang);
//            status_t res = style->set_string(atom, &sel->lang);
//            lsp_trace("Updated style: %d", int(res));
//            if ((res == STATUS_OK) && (sel->ctl->pLanguage != NULL))
//            {
//                const char *slang = sel->ctl->pLanguage->get_buffer<char>();
//                lsp_trace("Current language: \"%s\"", slang);
//                if ((slang == NULL) || (strcmp(slang, dlang)))
//                {
//                    lsp_trace("Write and notify: \"%s\"", dlang);
//                    sel->ctl->pLanguage->write(dlang, strlen(dlang));
//                    sel->ctl->pLanguage->notify_all();
//                }
//            }
//
//            lsp_trace("Language has been selected");
//
//            return STATUS_OK;
//        }

        void PluginWindow::end()
        {
            // Check widget pointer
            tk::Window *wnd = tk::widget_cast<tk::Window>(pWidget);

            if (wnd != NULL)
            {
                // Update window geometry
                wnd->border_style()->set(bResizable ? ws::BS_SIZEABLE : ws::BS_DIALOG);
                wnd->policy()->set(bResizable ? tk::WP_NORMAL : tk::WP_GREEDY);
                wnd->actions()->set_resizable(bResizable);
                wnd->actions()->set_maximizable(bResizable);
            }

            if (pPMStud != NULL)
                notify(pPMStud);

            // Call for parent class method
            Widget::end();
        }

        void PluginWindow::notify(ui::IPort *port)
        {
            Widget::notify(port);

//            if (port == pPMStud)
//            {
//                bool top    = pPMStud->value() < 0.5f;
//                vMStud[0]->set_visible(top);
//                vMStud[1]->set_visible(!top);
//                vMStud[2]->set_visible(!top);
//            }
        }

        status_t PluginWindow::add(ctl::Widget *child)
        {
            // Check widget pointer
            if (wBox == NULL)
                return STATUS_BAD_STATE;

            return wBox->add(child->widget());
        }

//        status_t PluginWindow::slot_export_settings_to_file(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            LSPFileDialog *dlg = __this->pExport;
//            if (dlg == NULL)
//            {
//                dlg = new LSPFileDialog(__this->pWnd->display());
//                __this->vWidgets.add(dlg);
//                __this->pExport = dlg;
//
//                dlg->init();
//                dlg->set_mode(FDM_SAVE_FILE);
//                dlg->title()->set("titles.export_settings");
//                dlg->action_title()->set("actions.save");
//                dlg->set_use_confirm(true);
//                dlg->confirm()->set("messages.file.confirm_overwrite");
//
//                LSPFileFilter *f = dlg->filter();
//                {
//                    LSPFileFilterItem ffi;
//
//                    ffi.pattern()->set("*.cfg");
//                    ffi.title()->set("files.config.lsp");
//                    ffi.set_extension(".cfg");
//                    f->add(&ffi);
//
//                    ffi.pattern()->set("*");
//                    ffi.title()->set("files.all");
//                    ffi.set_extension("");
//                    f->add(&ffi);
//                }
//
//                // Add 'Relative paths' option
//                if (__this->has_path_ports())
//                {
//                    LSPBox *op_rpath        = new LSPBox(__this->pWnd->display());
//                    __this->vWidgets.add(op_rpath);
//                    op_rpath->init();
//                    op_rpath->set_horizontal(true);
//                    op_rpath->set_spacing(4);
//
//                    // Add switch button
//                    LSPButton *btn_rpath    = new LSPButton(__this->pWnd->display());
//                    __this->vWidgets.add(btn_rpath);
//                    btn_rpath->init();
//
//                    CtlWidget *ctl          = new CtlButton(__this->pRegistry, btn_rpath);
//                    ctl->init();
//                    ctl->set(A_ID, REL_PATHS_PORT);
//                    ctl->set(A_COLOR, "yellow");
//                    ctl->set(A_LED, "true");
//                    ctl->set(A_SIZE, "16");
//                    ctl->begin();
//                    ctl->end();
//                    __this->pRegistry->add_widget(ctl);
//                    op_rpath->add(btn_rpath);
//
//                    // Add label
//                    LSPLabel *lbl_rpath     = new LSPLabel(__this->pWnd->display());
//                    __this->vWidgets.add(lbl_rpath);
//                    lbl_rpath->init();
//
//                    lbl_rpath->set_expand(true);
//                    lbl_rpath->set_halign(0.0f);
//                    lbl_rpath->text()->set("labels.relative_paths");
//                    op_rpath->add(lbl_rpath);
//
//                    // Add option to dialog
//                    dlg->add_option(op_rpath);
//                }
//
//                // Bind actions
//                dlg->bind_action(slot_call_export_settings_to_file, ptr);
//                dlg->slots()->bind(LSPSLOT_SHOW, slot_fetch_path, __this);
//                dlg->slots()->bind(LSPSLOT_HIDE, slot_commit_path, __this);
//            }
//
//            return dlg->show(__this->pWnd);
//        }
//
//        status_t PluginWindow::slot_import_settings_from_file(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            LSPFileDialog *dlg = __this->pImport;
//            if (dlg == NULL)
//            {
//                dlg = new LSPFileDialog(__this->pWnd->display());
//                __this->vWidgets.add(dlg);
//                __this->pImport = dlg;
//
//                dlg->init();
//                dlg->set_mode(FDM_OPEN_FILE);
//                dlg->title()->set("titles.import_settings");
//                dlg->action_title()->set("actions.open");
//
//                LSPFileFilter *f = dlg->filter();
//                {
//                    LSPFileFilterItem ffi;
//
//                    ffi.pattern()->set("*.cfg");
//                    ffi.title()->set("files.config.lsp");
//                    ffi.set_extension(".cfg");
//                    f->add(&ffi);
//
//                    ffi.pattern()->set("*");
//                    ffi.title()->set("files.all");
//                    ffi.set_extension("");
//                    f->add(&ffi);
//                }
//                dlg->bind_action(slot_call_import_settings_to_file, ptr);
//                dlg->slots()->bind(LSPSLOT_SHOW, slot_fetch_path, __this);
//                dlg->slots()->bind(LSPSLOT_HIDE, slot_commit_path, __this);
//            }
//
//            return dlg->show(__this->pWnd);
//        }
//
//        status_t PluginWindow::slot_toggle_rack_mount(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            CtlPort *mstud = __this->pPMStud;
//            if (mstud != NULL)
//            {
//                bool x = mstud->get_value() >= 0.5f;
//                mstud->set_value((x) ? 0.0f : 1.0f);
//                mstud->notify_all();
//            }
//
//            return STATUS_OK;
//        }

//        static const char * manual_prefixes[] =
//        {
//            LSP_LIB_PREFIX("/share"),
//            LSP_LIB_PREFIX("/local/share"),
//            "/usr/share",
//            "/usr/local/share",
//            "/share",
//            NULL
//        };
//
//        status_t PluginWindow::slot_show_plugin_manual(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            const plugin_metadata_t *meta = __this->pUI->metadata();
//
//            io::Path path;
//            LSPString spath;
//            status_t res;
//
//            // Try to open local documentation
//            for (const char **prefix = manual_prefixes; *prefix != NULL; ++prefix)
//            {
//                path.fmt("%s/doc/%s/html/plugins/%s.html",
//                        *prefix, LSP_ARTIFACT_ID, meta->lv2_uid
//                    );
//
//                lsp_trace("Checking path: %s", path.as_utf8());
//
//                if (path.exists())
//                {
//                    if (spath.fmt_utf8("file://%s", path.as_utf8()))
//                    {
//                        if ((res = follow_url(&spath)) == STATUS_OK)
//                            return res;
//                    }
//                }
//            }
//
//            // Follow the online documentation
//            if (spath.fmt_utf8("%s?page=manuals&section=%s", LSP_BASE_URI, meta->lv2_uid))
//            {
//                if ((res = follow_url(&spath)) == STATUS_OK)
//                    return res;
//            }
//
//            return STATUS_NOT_FOUND;
//        }
//
//        status_t PluginWindow::slot_show_ui_manual(LSPWidget *sender, void *ptr, void *data)
//        {
//            io::Path path;
//            LSPString spath;
//            status_t res;
//
//            // Try to open local documentation
//            for (const char **prefix = manual_prefixes; *prefix != NULL; ++prefix)
//            {
//                path.fmt("%s/doc/%s/html/constrols.html", *prefix, LSP_ARTIFACT_ID);
//                lsp_trace("Checking path: %s", path.as_utf8());
//
//                if (path.exists())
//                {
//                    if (spath.fmt_utf8("file://%s", path.as_utf8()))
//                    {
//                        if ((res = follow_url(&spath)) == STATUS_OK)
//                            return res;
//                    }
//                }
//            }
//
//            // Follow the online documentation
//            if (spath.fmt_utf8("%s?page=manuals&section=controls", LSP_BASE_URI))
//            {
//                if ((res = follow_url(&spath)) == STATUS_OK)
//                    return res;
//            }
//
//            return STATUS_NOT_FOUND;
//        }
//
//        status_t PluginWindow::follow_url(const LSPString *url)
//        {
//            #ifdef PLATFORM_WINDOWS
//                ::ShellExecuteW(
//                    NULL,               // Not associated with window
//                    L"open",            // Open hyperlink
//                    sUrl.get_utf16(),   // The file to execute
//                    NULL,               // Parameters
//                    NULL,               // Directory
//                    SW_SHOWNORMAL       // Show command
//                );
//            #else
//                status_t res;
//                ipc::Process p;
//
//                if ((res = p.set_command("xdg-open")) != STATUS_OK)
//                    return STATUS_OK;
//                if ((res = p.add_arg(url)) != STATUS_OK)
//                    return STATUS_OK;
//                if ((res = p.launch()) != STATUS_OK)
//                    return STATUS_OK;
//                p.wait();
//            #endif /* PLATFORM_WINDOWS */
//
//            return STATUS_OK;
//        }
//
//        status_t PluginWindow::slot_debug_dump(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            if (__this->pUI != NULL)
//                __this->pUI->request_state_dump();
//
//            return STATUS_OK;
//        }
//
//        status_t PluginWindow::slot_show_menu_top(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            return __this->show_menu(0, data);
//        }
//
//        status_t PluginWindow::slot_show_menu_left(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            return __this->show_menu(1, data);
//        }
//
//        status_t PluginWindow::slot_show_menu_right(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            return __this->show_menu(2, data);
//        }
//
//        status_t PluginWindow::show_menu(size_t actor_id, void *data)
//        {
//            LSPWidget *actor = vMStud[actor_id];
//            if (data != NULL)
//            {
//                ws_event_t *ev = static_cast<ws_event_t *>(data);
//                return pMenu->show(actor, ev);
//            }
//
//            return pMenu->show(actor);
//        }
//
//        status_t PluginWindow::slot_call_export_settings_to_file(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            bool relative = __this->pRelPaths->get_value() >= 0.5f;
//            __this->pUI->export_settings(__this->pExport->selected_file(), relative);
//            return STATUS_OK;
//        }
//
//        status_t PluginWindow::slot_call_import_settings_to_file(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            __this->pUI->import_settings(__this->pImport->selected_file(), false);
//            return STATUS_OK;
//        }
//
//        status_t PluginWindow::slot_export_settings_to_clipboard(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            __this->pUI->export_settings_to_clipboard();
//            return STATUS_OK;
//        }
//
//        status_t PluginWindow::slot_import_settings_from_clipboard(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
//            __this->pUI->import_settings_from_clipboard();
//            return STATUS_OK;
//        }
//
        status_t PluginWindow::slot_message_close(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            if (__this->wMessage != NULL)
                __this->wMessage->visibility()->set(false);
            return STATUS_OK;
        }
//
//        status_t PluginWindow::slot_fetch_path(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
//            if ((_this == NULL) || (_this->pPath == NULL))
//                return STATUS_BAD_STATE;
//
//            LSPFileDialog *dlg = widget_cast<LSPFileDialog>(sender);
//            if (dlg == NULL)
//                return STATUS_OK;
//
//            dlg->set_path(_this->pPath->get_buffer<char>());
//            return STATUS_OK;
//        }
//
//        status_t PluginWindow::slot_commit_path(LSPWidget *sender, void *ptr, void *data)
//        {
//            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
//            if ((_this == NULL) || (_this->pPath == NULL))
//                return STATUS_BAD_STATE;
//
//            LSPFileDialog *dlg = widget_cast<LSPFileDialog>(sender);
//            if (dlg == NULL)
//                return STATUS_OK;
//
//            const char *path = dlg->path();
//            if (path != NULL)
//            {
//                _this->pPath->write(path, strlen(path));
//                _this->pPath->notify_all();
//            }
//
//            return STATUS_OK;
//        }

        tk::Label *PluginWindow::create_label(tk::WidgetContainer *dst, const char *key, float halign)
        {
            tk::Label *lbl = new tk::Label(pWidget->display());
            lbl->init();
            vWidgets.add(lbl);
            dst->add(lbl);

            lbl->text()->set(key);
            lbl->allocation()->set_fill(true);
            lbl->text_layout()->set(halign, 0.5f);

            return lbl;
        }

        tk::Label *PluginWindow::create_plabel(tk::WidgetContainer *dst, const char *key, const expr::Parameters *params, const float halign)
        {
            tk::Label *lbl = new tk::Label(pWidget->display());
            lbl->init();
            vWidgets.add(lbl);
            dst->add(lbl);

            lbl->text()->set(key, params);
            lbl->allocation()->set_fill(true);
            lbl->text_layout()->set(halign, 0.5f);

            return lbl;
        }

        tk::Hyperlink *PluginWindow::create_hlink(tk::WidgetContainer *dst, const char *text, float halign)
        {
            tk::Align *algn = new tk::Align(pWidget->display());
            algn->init();
            vWidgets.add(algn);
            algn->layout()->set_halign(halign);
            dst->add(algn);

            tk::Hyperlink *hlink = new tk::Hyperlink(pWidget->display());
            hlink->init();
            vWidgets.add(hlink);
            algn->add(hlink);

            hlink->url()->set_raw(text);
            hlink->text()->set_raw(text);
            return hlink;
        }


// TODO: take this from metadata
#define LSP_MAIN_VERSION                                "0.0.0"
#define LSP_FULL_NAME                                   "Linux Studio Plugins Project"
#define LSP_DONATION_URI1                               "https://salt.bountysource.com/teams/lsp-plugins"
#define LSP_DONATION_URI2                               "https://liberapay.com/sadko4u/donate"
#define LSP_BASE_URI                                    "http://lsp-plug.in/"

        status_t PluginWindow::show_notification()
        {
            tk::Window *wnd = tk::widget_cast<tk::Window>(pWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            // Check that we really need to show notification window
            if (pPVersion != NULL)
            {
// TODO
//                const char *v = pPVersion->buffer<char>();
//                if ((v != NULL) && (strcmp(LSP_MAIN_VERSION, v) == 0))
//                    return STATUS_OK;
//
//                pPVersion->write(LSP_MAIN_VERSION, strlen(LSP_MAIN_VERSION));
                pPVersion->notify_all();
            }

            lsp_trace("Showing notification dialog");

            if (wMessage == NULL)
            {
                wMessage = new tk::Window(pWidget->display());
                if (wMessage == NULL)
                    return STATUS_NO_MEM;

                vWidgets.add(wMessage);
                wMessage->init();
                wMessage->border_style()->set(ws::BS_DIALOG);
                wMessage->title()->set("titles.update_notification");
                wMessage->actions()->deny_all();
                wMessage->actions()->set_closeable(true);
                wMessage->padding()->set_all(16);

                tk::Box *vbox = new tk::Box(pWidget->display());
                vbox->init();
                vbox->orientation()->set_vertical();
                vbox->spacing()->set(8);
                vWidgets.add(vbox);
                wMessage->add(vbox);

                expr::Parameters p;

                tk::Label *lbl  = create_label(vbox, "headings.greetings");
                lbl->font()->set_size(24);
                lbl->font()->set_bold();

                p.clear();
                p.set_cstring("version", LSP_MAIN_VERSION);
                lbl  = create_plabel(vbox, "messages.greetings.0", &p);
                lbl->font()->set_bold();

                p.clear();
                p.set_cstring("project", LSP_FULL_NAME);
                lbl  = create_plabel(vbox, "messages.greetings.1", &p);
                lbl  = create_label(vbox, "messages.greetings.2");
                create_hlink(vbox, LSP_DONATION_URI1, 0.02);
                create_hlink(vbox, LSP_DONATION_URI2, 0.02);

                lbl  = create_label(vbox, "messages.greetings.3");
                lbl  = create_label(vbox, "messages.greetings.4");

                lbl  = create_label(vbox, "messages.greetings.5", 1.0f);
                lbl  = create_label(vbox, LSP_FULL_NAME, 1.0f);
                create_hlink(vbox, LSP_BASE_URI, 1.0f);

                tk::Align *algn = new tk::Align(pWidget->display());
                algn->init();
                algn->allocation()->set_fill(true);
                vWidgets.add(algn);
                vbox->add(algn);

                tk::Button *btn = new tk::Button(pWidget->display());
                btn->init();
                vWidgets.add(btn);
                algn->add(btn);
                btn->constraints()->set_min_width(96);
                btn->text()->set("actions.close");

                // Bind slots
                btn->slots()->bind(tk::SLOT_SUBMIT, slot_message_close, this);
                wMessage->slots()->bind(tk::SLOT_CLOSE, slot_message_close, this);
            }

            wMessage->show(wnd);
            return STATUS_OK;
        }

    }
}


