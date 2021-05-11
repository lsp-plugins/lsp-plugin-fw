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
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/io/OutStringSequence.h>
#include <lsp-plug.in/io/InStringSequence.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(PluginWindow)
            status_t res;
            if (!name->equals_ascii("plugin"))
                return STATUS_NOT_FOUND;

            tk::Window *w       = new tk::Window(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->add_widget(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            PluginWindow *wc    = new PluginWindow(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(PluginWindow)

        //-----------------------------------------------------------------
        PluginWindow::ConfigSink::ConfigSink(ui::IWrapper *wrapper)
        {
            pWrapper = wrapper;
        }

        void PluginWindow::ConfigSink::unbind()
        {
            pWrapper        = NULL;
        }

        status_t PluginWindow::ConfigSink::receive(const LSPString *text, const char *mime)
        {
            ui::IWrapper *wrapper = pWrapper;
            if (wrapper == NULL)
                return STATUS_NOT_BOUND;

            io::InStringSequence is(text);
            return wrapper->import_settings(&is);
        }

        //-----------------------------------------------------------------
        // Plugin window
        const ctl_class_t PluginWindow::metadata = { "PluginWindow", &Widget::metadata };

        PluginWindow::PluginWindow(ui::IWrapper *src, tk::Window *widget): Widget(src, widget)
        {
            pClass          = &metadata;

            bResizable      = false;

            wBox            = NULL;
            wMessage        = NULL;
            wRack[0]        = NULL;
            wRack[1]        = NULL;
            wRack[2]        = NULL;
            wMenu           = NULL;
            wExport         = NULL;
            wImport         = NULL;

            pPMStud         = NULL;
            pPVersion       = NULL;
            pPBypass        = NULL;
            pPath           = NULL;
            pR3DBackend     = NULL;
            pLanguage       = NULL;
            pRelPaths       = NULL;

            pConfigSink     = NULL;
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
            // Unbind configuration sink
            if (pConfigSink != NULL)
            {
                pConfigSink->unbind();
                pConfigSink->release();
            }

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

            // Delete language selection bindings
            for (size_t i=0, n=vLangSel.size(); i<n; ++i)
            {
                lang_sel_t *s = vLangSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vLangSel.flush();

//            vBackendSel.flush();

            wBox            = NULL;
            wMessage        = NULL;
            wRack[0]        = NULL;
            wRack[1]        = NULL;
            wRack[2]        = NULL;
            wMenu           = NULL;
            wExport         = NULL;
            wImport         = NULL;
        }

        status_t PluginWindow::slot_window_close(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this->pWrapper != NULL)
                _this->pWrapper->quit_main_loop();
            return STATUS_OK;
        }

        status_t PluginWindow::slot_window_show(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            __this->show_notification();
            return STATUS_OK;
        }

        void PluginWindow::set(const char *name, const char *value)
        {
            if (!strcmp(name, "resizable"))
                PARSE_BOOL(value, bResizable = __)

            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd != NULL)
                set_constraints(wnd->constraints(), name, value);
            Widget::set(name, value);
        }

        status_t PluginWindow::init()
        {
            Widget::init();

            // Get window handle
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
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
            wnd->set_class(meta->uid, "lsp-plugins");
            wnd->role()->set("audio-plugin");
            wnd->title()->set_raw(meta->name);
            wnd->layout()->set_scale(1.0f);

            if (!wnd->nested())
                wnd->actions()->deny(ws::WA_RESIZE);

            LSP_STATUS_ASSERT(create_main_menu());
            LSP_STATUS_ASSERT(init_window_layout());

            // Bind event handlers
            wnd->slots()->bind(tk::SLOT_CLOSE, slot_window_close, this);
            wnd->slots()->bind(tk::SLOT_SHOW, slot_window_show, this);

            return STATUS_OK;
        }

        i18n::IDictionary  *PluginWindow::get_default_dict(tk::Widget *src)
        {
            i18n::IDictionary *dict = src->display()->dictionary();
            if (dict == NULL)
                return dict;

            if (dict->lookup("default", &dict) != STATUS_OK)
                dict = NULL;

            return dict;
        }

        status_t PluginWindow::create_main_menu()
        {
            tk::Window *wnd             = tk::widget_cast<tk::Window>(wWidget);
            tk::Display *dpy            = wnd->display();
            const meta::plugin_t *meta  = pWrapper->ui()->metadata();

            // Initialize menu
            wMenu = new tk::Menu(dpy);
            pWrapper->ui()->map_widget(WUID_MAIN_MENU, wMenu);
            vWidgets.add(wMenu);
            wMenu->init();

            // Initialize menu items
            {
                // Add 'Plugin manual' menu item
                tk::MenuItem *itm       = new tk::MenuItem(dpy);
                vWidgets.add(itm);
                itm->init();
                itm->text()->set("actions.plugin_manual");
                itm->slots()->bind(tk::SLOT_SUBMIT, slot_show_plugin_manual, this);
                wMenu->add(itm);

                // Add 'UI manual' menu item
                itm                     = new tk::MenuItem(dpy);
                vWidgets.add(itm);
                itm->init();
                itm->text()->set("actions.ui_manual");
                itm->slots()->bind(tk::SLOT_SUBMIT, slot_show_ui_manual, this);
                wMenu->add(itm);

                // Add separator
                itm     = new tk::MenuItem(dpy);
                vWidgets.add(itm);
                itm->init();
                itm->type()->set_separator();
                wMenu->add(itm);

                // Create 'Export' submenu and bind to parent
                tk::Menu *submenu = new tk::Menu(dpy);
                vWidgets.add(submenu);
                submenu->init();
                pWrapper->ui()->map_widget(WUID_EXPORT_MENU, submenu);

                itm = new tk::MenuItem(dpy);
                vWidgets.add(itm);
                itm->init();
                itm->text()->set("actions.export");
                itm->menu()->set(submenu);
                wMenu->add(itm);

                // Create 'Export' menu items
                {
                    tk::MenuItem *child = new tk::MenuItem(dpy);
                    vWidgets.add(child);
                    child->init();
                    child->text()->set("actions.export_settings_to_file");
                    child->slots()->bind(tk::SLOT_SUBMIT, slot_export_settings_to_file, this);
                    submenu->add(child);

                    child = new tk::MenuItem(dpy);
                    vWidgets.add(child);
                    child->init();
                    child->text()->set("actions.export_settings_to_clipboard");
                    child->slots()->bind(tk::SLOT_SUBMIT, slot_export_settings_to_clipboard, this);
                    submenu->add(child);
                }

                // Create 'Import' menu and bind to parent
                submenu = new tk::Menu(dpy);
                vWidgets.add(submenu);
                submenu->init();
                pWrapper->ui()->map_widget(WUID_IMPORT_MENU, submenu);

                itm = new tk::MenuItem(dpy);
                vWidgets.add(itm);
                itm->init();
                itm->text()->set("actions.import");
                itm->menu()->set(submenu);
                wMenu->add(itm);

                // Create import menu items
                {
                    tk::MenuItem *child = new tk::MenuItem(dpy);
                    vWidgets.add(child);
                    child->init();
                    child->text()->set("actions.import_settings_from_file");
                    child->slots()->bind(tk::SLOT_SUBMIT, slot_import_settings_from_file, this);
                    submenu->add(child);

                    child = new tk::MenuItem(dpy);
                    vWidgets.add(child);
                    child->init();
                    child->text()->set("actions.import_settings_from_clipboard");
                    child->slots()->bind(tk::SLOT_SUBMIT, slot_import_settings_from_clipboard, this);
                    submenu->add(child);
                }

                // Add separator
                itm     = new tk::MenuItem(dpy);
                vWidgets.add(itm);
                itm->init();
                itm->type()->set_separator();
                wMenu->add(itm);

                // Create 'Toggle rack mount' menu item
                itm     = new tk::MenuItem(dpy);
                vWidgets.add(itm);
                itm->init();
                itm->text()->set("actions.toggle_rack_mount");
                itm->slots()->bind(tk::SLOT_SUBMIT, slot_toggle_rack_mount, this);
                wMenu->add(itm);

                // Create 'Dump state' menu item if supported
                if (meta->extensions & meta::E_DUMP_STATE)
                {
                    itm     = new tk::MenuItem(dpy);
                    vWidgets.add(itm);
                    itm->init();
                    itm->text()->set("actions.debug_dump");
                    itm->slots()->bind(tk::SLOT_SUBMIT, slot_debug_dump, this);
                    wMenu->add(itm);
                }

                // Create language selection menu
                init_i18n_support(wMenu);
//
//                // Add support of 3D rendering backend switch
//                if (meta->extensions & E_3D_BACKEND)
//                    init_r3d_support(pMenu);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::init_window_layout()
        {
            tk::Window *wnd             = tk::widget_cast<tk::Window>(wWidget);
            tk::Display *dpy            = wnd->display();
            const meta::plugin_t *meta  = pWrapper->ui()->metadata();
            inject_style(wnd, "PluginWindow");

            // Get default dictionary
            i18n::IDictionary *dict     = get_default_dict(wnd);

            // Initialize main grid
            tk::Grid *grd = new tk::Grid(dpy);
            vWidgets.add(grd);
            wnd->add(grd);

            grd->init();
            grd->rows()->set(2);
            grd->columns()->set((pPBypass != NULL) ? 4 : 3);
            inject_style(grd, "PluginWindow::Grid");

            // Initialize rack ears
            LSPString buf, acronym;
            if (dict != NULL)
                dict->lookup("project.acronym", &acronym);
            buf.fmt_utf8("%s %s", acronym.get_utf8(), meta->acronym);

            // Rack ear at the top
            tk::RackEars *rk_ear = new tk::RackEars(dpy);
            wRack[0] = rk_ear;
            vWidgets.add(rk_ear);
            rk_ear->init();
            rk_ear->angle()->set(1);
            rk_ear->text()->set_raw(&buf);
            rk_ear->slots()->bind(tk::SLOT_SUBMIT, slot_show_menu, this);
            inject_style(rk_ear, "PluginWindow::RackEarTop");
            grd->add(rk_ear, 1, (pPBypass != NULL) ? 4 : 3);

            rk_ear   = new tk::RackEars(dpy);
            wRack[1] = rk_ear;
            vWidgets.add(rk_ear);
            rk_ear->init();
            rk_ear->angle()->set(2);
            rk_ear->text()->set_raw(&acronym);
            rk_ear->slots()->bind(tk::SLOT_SUBMIT, slot_show_menu, this);
            inject_style(rk_ear, "PluginWindow::RackEarSide");
            grd->add(rk_ear);

            if (pPBypass != NULL)
            {
                tk::Box *box = new tk::Box(dpy);
                vWidgets.add(box);
                box->init();
                box->orientation()->set_vertical();
                inject_style(box, "PluginWindow::Bypass::Box");
                grd->add(box);

                tk::Label *lbl = new tk::Label(dpy);
                vWidgets.add(lbl);
                lbl->init();
                lbl->text()->set("labels.bypass");
                inject_style(lbl, "PluginWindow::Bypass::Label");
                box->add(lbl);

                tk::Switch *sw  = new tk::Switch(dpy);
                vWidgets.add(sw);
                sw->init();
                inject_style(sw, "PluginWindow::Bypass::Switch");
                box->add(sw);

                tk::Led *led = new tk::Led(dpy);
                vWidgets.add(led);
                led->init();
                inject_style(led, "PluginWindow::Bypass::Led");
                box->add(led);

                // Create controllers
                ctl::Widget *ctl = new ctl::Switch(pWrapper, sw);
                ctl->init();
                ctl->set("id", pPBypass->metadata()->id);
                ctl->begin();
                ctl->end();
                pWrapper->add_controller(ctl);

                ctl = new ctl::Led(pWrapper, led);
                ctl->init();
                ctl->set("id", pPBypass->metadata()->id);
                ctl->begin();
                ctl->end();
                pWrapper->add_controller(ctl);
            }

            wBox    = new tk::Box(dpy);
            vWidgets.add(wBox);
            wBox->init();
            inject_style(wBox, "PluginWindow::Content");
            grd->add(wBox);

            rk_ear  = new tk::RackEars(dpy);
            wRack[2]= rk_ear;
            vWidgets.add(rk_ear);
            rk_ear->init();
            rk_ear->angle()->set(0);
            rk_ear->text()->set_raw(meta->acronym);
            rk_ear->slots()->bind(tk::SLOT_SUBMIT, slot_show_menu, this);
            inject_style(rk_ear, "PluginWindow::RackEarSide");
            grd->add(rk_ear);

            return STATUS_OK;
        }

        status_t PluginWindow::init_i18n_support(tk::Menu *menu)
        {
            if (menu == NULL)
                return STATUS_OK;

            tk::Display *dpy            = menu->display();
            i18n::IDictionary *dict     = get_default_dict(menu);
            if (dict == NULL)
                return STATUS_OK;

            // Perform lookup before loading list of languages
            status_t res = dict->lookup("lang.target", &dict);
            if (res != STATUS_OK)
                return res;

            // Create submenu item
            tk::MenuItem *root          = new tk::MenuItem(menu->display());
            if (root == NULL)
                return STATUS_NO_MEM;
            if ((res = root->init()) != STATUS_OK)
            {
                delete root;
                return res;
            }
            if (!vWidgets.add(root))
            {
                root->destroy();
                delete root;
                return STATUS_NO_MEM;
            }
            root->text()->set("actions.select_language");
            if ((res = menu->add(root)) != STATUS_OK)
                return res;

            // Create submenu
            menu                = new tk::Menu(menu->display());
            if (menu == NULL)
                return STATUS_NO_MEM;
            if ((res = menu->init()) != STATUS_OK)
            {
                menu->destroy();
                delete menu;
                return res;
            }
            if (!vWidgets.add(menu))
            {
                menu->destroy();
                delete menu;
                return STATUS_NO_MEM;
            }
            root->menu()->set(menu);

            // Iterate all children and add language keys
            LSPString key, value;
            lang_sel_t *lang;
            size_t added = 0;
            for (size_t i=0, n=dict->size(); i<n; ++i)
            {
                // Fetch placeholder for language selection key
                if ((res = dict->get_value(i, &key, &value)) != STATUS_OK)
                {
                    // Skip nested dictionaries
                    if (res == STATUS_BAD_TYPE)
                        continue;
                    return res;
                }
                if ((lang = new lang_sel_t()) == NULL)
                    return STATUS_NO_MEM;
                if (!lang->lang.set(&key))
                {
                    delete lang;
                    return STATUS_NO_MEM;
                }
                if (!vLangSel.add(lang))
                {
                    delete lang;
                    return STATUS_NO_MEM;
                }
                lang->ctl   = this;
                lang->item  = NULL;

                // Create menu item
                tk::MenuItem *item = new tk::MenuItem(dpy);
                if (item == NULL)
                    continue;
                if ((res = item->init()) != STATUS_OK)
                {
                    item->destroy();
                    delete item;
                    continue;
                }
                if (!vWidgets.add(item))
                {
                    item->destroy();
                    delete item;
                    continue;
                }

                item->text()->set_raw(&value);
                item->type()->set_radio();
                menu->add(item);
                lang->item      = item;

                // Create closure and bind
                item->slots()->bind(tk::SLOT_SUBMIT, slot_select_language, lang);

                ++added;
            }

            // Set menu item visible only if there are available languages
            root->visibility()->set(added > 0);
            if (pLanguage != NULL)
            {
                const char *lang = pLanguage->buffer<char>();
                if ((lang != NULL) && (strlen(lang) > 0))
                {
                    if ((dpy->schema()->set_lanugage(lang)) == STATUS_OK)
                    {
                        lsp_trace("System language set to: %s", lang);
                        pLanguage->notify_all();
                    }
                }
            }

            return STATUS_OK;
        }

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

        bool PluginWindow::has_path_ports()
        {
            for (size_t i = 0, n = pWrapper->ports(); i < n; ++i)
            {
                ui::IPort *p = pWrapper->port(i);
                const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                if ((meta != NULL) && (meta->role == meta::R_PATH))
                    return true;
            }
            return false;
        }

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
        status_t PluginWindow::slot_select_language(tk::Widget *sender, void *ptr, void *data)
        {
            lang_sel_t *sel = reinterpret_cast<lang_sel_t *>(ptr);
            lsp_trace("sender=%p, sel=%p", sender, sel);
            if ((sender == NULL) || (sel == NULL) || (sel->ctl == NULL) || (sel->item == NULL))
                return STATUS_BAD_ARGUMENTS;

            tk::Display *dpy = sender->display();
            lsp_trace("dpy = %p", dpy);
            if (dpy == NULL)
                return STATUS_BAD_STATE;

            // Select language
            tk::Schema *schema  = dpy->schema();
            lsp_trace("Select language: \"%s\"", sel->lang.get_native());
            if ((schema->set_lanugage(&sel->lang)) != STATUS_OK)
            {
                lsp_warn("Failed to select language \"%s\"", sel->lang.get_native());
                return STATUS_OK;
            }

            // Update parameter
            const char *dlang = sel->lang.get_utf8();
            const char *slang = sel->ctl->pLanguage->buffer<char>();
            lsp_trace("Current language in settings: \"%s\"", slang);
            if ((slang == NULL) || (strcmp(slang, dlang)))
            {
                sel->ctl->pLanguage->write(dlang, strlen(dlang));
                sel->ctl->pLanguage->notify_all();
            }

            lsp_trace("Language has been selected");

            return STATUS_OK;
        }

        void PluginWindow::sync_language_selection()
        {
            tk::Display *dpy    = wWidget->display();
            if (dpy == NULL)
                return;

            LSPString lang;
            if ((dpy->schema()->get_language(&lang)) != STATUS_OK)
                return;

            for (size_t i=0, n=vLangSel.size(); i<n; ++i)
            {
                lang_sel_t *xsel    = vLangSel.uget(i);
                if (xsel->item == NULL)
                    continue;
                xsel->item->checked()->set(xsel->lang.equals(&lang));
            }
        }

        void PluginWindow::end()
        {
            // Check widget pointer
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);

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

            if (port == pPMStud)
            {
                bool top    = pPMStud->value() < 0.5f;
                wRack[0]->visibility()->set(top);
                wRack[1]->visibility()->set(!top);
                wRack[2]->visibility()->set(!top);
            }

            if (port == pLanguage)
                sync_language_selection();
        }

        status_t PluginWindow::add(ctl::Widget *child)
        {
            // Check widget pointer
            if (wBox == NULL)
                return STATUS_BAD_STATE;

            return wBox->add(child->widget());
        }

        tk::FileFilters *PluginWindow::create_config_filters(tk::FileDialog *dlg)
        {
            tk::FileFilters *f = dlg->filter();
            if (f == NULL)
                return f;

            tk::FileMask *ffi = f->add();
            if (ffi != NULL)
            {
                ffi->pattern()->set("*.cfg");
                ffi->title()->set("files.config.lsp");
                ffi->extensions()->set_raw(".cfg");
            }

            ffi = f->add();
            if (ffi != NULL)
            {
                ffi->pattern()->set("*");
                ffi->title()->set("files.all");
                ffi->extensions()->set_raw("");
            }

            return f;
        }

        status_t PluginWindow::slot_export_settings_to_file(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this     = static_cast<PluginWindow *>(ptr);
            tk::Display *dpy        = _this->wWidget->display();
            tk::FileDialog *dlg     = _this->wExport;

            if (dlg == NULL)
            {
                dlg = new tk::FileDialog(dpy);
                _this->vWidgets.add(dlg);
                _this->wExport = dlg;

                dlg->init();
                dlg->mode()->set(tk::FDM_SAVE_FILE);
                dlg->title()->set("titles.export_settings");
                dlg->action_text()->set("actions.save");
                dlg->use_confirm()->set(true);
                dlg->confirm_message()->set("messages.file.confirm_overwrite");

                create_config_filters(dlg);

                // Add 'Relative paths' option
                tk::Box *wc = new tk::Box(dpy);
                _this->vWidgets.add(wc);
                wc->init();
                wc->orientation()->set_vertical();
                wc->allocation()->set_hfill(true);

                if (_this->has_path_ports())
                {
                    tk::Box *op_rpath       = new tk::Box(dpy);
                    _this->vWidgets.add(op_rpath);
                    op_rpath->init();
                    op_rpath->orientation()->set_horizontal();
                    op_rpath->spacing()->set(4);

                    // Add switch button
                    tk::CheckBox *ck_rpath  = new tk::CheckBox(dpy);
                    _this->vWidgets.add(ck_rpath);
                    ck_rpath->init();
                    op_rpath->add(ck_rpath);

                    // Add label
                    tk::Label *lbl_rpath     = new tk::Label(dpy);
                    _this->vWidgets.add(lbl_rpath);
                    lbl_rpath->init();

                    lbl_rpath->allocation()->set_expand(true);
                    lbl_rpath->text_layout()->set_halign(-1.0f);
                    lbl_rpath->text()->set("labels.relative_paths");
                    op_rpath->add(lbl_rpath);

                    // Add option to dialog
                    wc->add(op_rpath);
                }

                // Bind actions
                if (wc->items()->size() > 0)
                    dlg->options()->set(wc);
                dlg->slots()->bind(tk::SLOT_SUBMIT, slot_call_export_settings_to_file, ptr);
                dlg->slots()->bind(tk::SLOT_SHOW, slot_fetch_path, _this);
                dlg->slots()->bind(tk::SLOT_HIDE, slot_commit_path, _this);
            }

            dlg->show(_this->wWidget);
            return STATUS_OK;
        }

        status_t PluginWindow::slot_export_settings_to_clipboard(tk::Widget *sender, void *ptr, void *data)
        {
            status_t res;
            LSPString buf;
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);

            // Export settings to text buffer
            io::OutStringSequence sq(&buf);
            if ((res = _this->pWrapper->export_settings(&sq)) != STATUS_OK)
                return STATUS_OK;
            sq.close();

            // Now 'buf' contains serialized configuration, put it to clipboard
            tk::TextDataSource *ds = new tk::TextDataSource();
            if (ds == NULL)
                return STATUS_NO_MEM;
            ds->acquire();
            res = ds->set_text(&buf);
            if (res == STATUS_OK)
                res = _this->wWidget->display()->set_clipboard(ws::CBUF_CLIPBOARD, ds);
            ds->release();

            return STATUS_OK;
        }

        status_t PluginWindow::slot_import_settings_from_file(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this     = static_cast<PluginWindow *>(ptr);
            tk::Display *dpy        = _this->wWidget->display();
            tk::FileDialog *dlg     = _this->wImport;
            if (dlg == NULL)
            {
                dlg     = new tk::FileDialog(dpy);
                _this->vWidgets.add(dlg);
                _this->wImport      = dlg;

                dlg->init();
                dlg->mode()->set(tk::FDM_OPEN_FILE);
                dlg->title()->set("titles.import_settings");
                dlg->action_text()->set("actions.open");

                create_config_filters(dlg);

                dlg->slots()->bind(tk::SLOT_SUBMIT, slot_call_import_settings_from_file, ptr);
                dlg->slots()->bind(tk::SLOT_SHOW, slot_fetch_path, _this);
                dlg->slots()->bind(tk::SLOT_HIDE, slot_commit_path, _this);
            }

            dlg->show(_this->wWidget);
            return STATUS_OK;
        }

        status_t PluginWindow::slot_import_settings_from_clipboard(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            tk::Display *dpy        = _this->wWidget->display();

            // Create new sink
            ConfigSink *ds = new ConfigSink(_this->pWrapper);
            if (ds == NULL)
                return STATUS_NO_MEM;
            ds->acquire();

            // Release previously used
            ConfigSink *old = _this->pConfigSink;
            _this->pConfigSink = ds;

            if (old != NULL)
            {
                old->unbind();
                old->release();
            }

            // Request clipboard data
            return dpy->get_clipboard(ws::CBUF_CLIPBOARD, ds);
        }


        status_t PluginWindow::slot_toggle_rack_mount(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            ui::IPort *mstud = __this->pPMStud;
            if (mstud != NULL)
            {
                bool x = mstud->value() >= 0.5f;
                mstud->set_value((x) ? 0.0f : 1.0f);
                mstud->notify_all();
            }

            return STATUS_OK;
        }

        static const char * manual_prefixes[] =
        {
        #ifdef LSP_LIB_PREFIX
            LSP_LIB_PREFIX("/share"),
            LSP_LIB_PREFIX("/local/share"),
        #endif /*  LSP_LIB_PREFIX */
            "/usr/share",
            "/usr/local/share",
            "/share",
            NULL
        };

        status_t PluginWindow::slot_show_plugin_manual(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            const meta::plugin_t *meta = __this->pWrapper->ui()->metadata();

            io::Path path;
            LSPString spath;
            status_t res;

            // Try to open local documentation
            for (const char **prefix = manual_prefixes; *prefix != NULL; ++prefix)
            {
                path.fmt("%s/doc/%s/html/plugins/%s.html",
                        *prefix, "lsp-plugins", meta->uid
                    );

                lsp_trace("Checking path: %s", path.as_utf8());

                if (path.exists())
                {
                    if (spath.fmt_utf8("file://%s", path.as_utf8()))
                    {
                        if ((res = system::follow_url(&spath)) == STATUS_OK)
                            return res;
                    }
                }
            }

            // Follow the online documentation
            if (spath.fmt_utf8("%s?page=manuals&section=%s", "https://lsp-plug.in/", meta->uid))
            {
                if ((res = system::follow_url(&spath)) == STATUS_OK)
                    return res;
            }

            return STATUS_NOT_FOUND;
        }

        status_t PluginWindow::slot_show_ui_manual(tk::Widget *sender, void *ptr, void *data)
        {
            io::Path path;
            LSPString spath;
            status_t res;

            // Try to open local documentation
            for (const char **prefix = manual_prefixes; *prefix != NULL; ++prefix)
            {
                path.fmt("%s/doc/%s/html/constrols.html", *prefix, "lsp-plugins");
                lsp_trace("Checking path: %s", path.as_utf8());

                if (path.exists())
                {
                    if (spath.fmt_utf8("file://%s", path.as_utf8()))
                    {
                        if ((res = system::follow_url(&spath)) == STATUS_OK)
                            return res;
                    }
                }
            }

            // Follow the online documentation
            if (spath.fmt_utf8("%s?page=manuals&section=controls", "https://lsp-plug.in/"))
            {
                if ((res = system::follow_url(&spath)) == STATUS_OK)
                    return res;
            }

            return STATUS_NOT_FOUND;
        }

        status_t PluginWindow::slot_debug_dump(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            if (__this->pWrapper != NULL)
                __this->pWrapper->dump_state_request();

            return STATUS_OK;
        }

        status_t PluginWindow::slot_show_menu(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            return __this->show_menu(sender, data);
        }

        status_t PluginWindow::show_menu(tk::Widget *actor, void *data)
        {
            tk::Menu *menu = tk::widget_ptrcast<tk::Menu>(wMenu);
            if (menu == NULL)
                return STATUS_OK;

            menu->show();
            return STATUS_OK;
        }

        status_t PluginWindow::slot_call_export_settings_to_file(tk::Widget *sender, void *ptr, void *data)
        {
            LSPString path;
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            status_t res = _this->wExport->selected_file()->format(&path);

            if (res == STATUS_OK)
            {
                bool relative = (_this->pRelPaths != NULL) ? _this->pRelPaths->value() >= 0.5f : false;
                _this->pWrapper->export_settings(&path, relative);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::slot_call_import_settings_from_file(tk::Widget *sender, void *ptr, void *data)
        {
            LSPString path;
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            status_t res = _this->wImport->selected_file()->format(&path);

            if (res == STATUS_OK)
                _this->pWrapper->import_settings(&path);

            return STATUS_OK;
        }


        status_t PluginWindow::slot_message_close(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            if (__this->wMessage != NULL)
                __this->wMessage->visibility()->set(false);
            return STATUS_OK;
        }

        status_t PluginWindow::slot_fetch_path(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if ((_this == NULL) || (_this->pPath == NULL))
                return STATUS_BAD_STATE;

            tk::FileDialog *dlg = tk::widget_cast<tk::FileDialog>(sender);
            if (dlg == NULL)
                return STATUS_OK;

            dlg->path()->set_raw(_this->pPath->buffer<char>());
            return STATUS_OK;
        }

        status_t PluginWindow::slot_commit_path(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if ((_this == NULL) || (_this->pPath == NULL))
                return STATUS_BAD_STATE;

            tk::FileDialog *dlg = tk::widget_cast<tk::FileDialog>(sender);
            if (dlg == NULL)
                return STATUS_OK;

            LSPString tmp_path;
            if (dlg->path()->format(&tmp_path) == STATUS_OK)
            {
                const char *path = tmp_path.get_utf8();
                if (path != NULL)
                {
                    _this->pPath->write(path, strlen(path));
                    _this->pPath->notify_all();
                }
            }

            return STATUS_OK;
        }

        tk::Label *PluginWindow::create_label(tk::WidgetContainer *dst, const char *key, const char *style_name)
        {
            tk::Label *lbl = new tk::Label(wWidget->display());
            lbl->init();
            vWidgets.add(lbl);
            dst->add(lbl);

            lbl->text()->set(key);
            inject_style(lbl, style_name);

            return lbl;
        }

        tk::Label *PluginWindow::create_plabel(tk::WidgetContainer *dst, const char *key, const expr::Parameters *params, const char *style_name)
        {
            tk::Label *lbl = new tk::Label(wWidget->display());
            lbl->init();
            vWidgets.add(lbl);
            dst->add(lbl);

            lbl->text()->set(key, params);
            inject_style(lbl, style_name);

            return lbl;
        }

        tk::Hyperlink *PluginWindow::create_hlink(tk::WidgetContainer *dst, const char *url, const char *text, const expr::Parameters *params, const char *style_name)
        {
            tk::Hyperlink *hlink = new tk::Hyperlink(wWidget->display());
            hlink->init();
            vWidgets.add(hlink);
            dst->add(hlink);

            hlink->url()->set(url);
            hlink->text()->set(text);
            if (params != NULL)
                hlink->text()->set_params(params);
            inject_style(hlink, style_name);

            return hlink;
        }

        status_t PluginWindow::show_notification()
        {
            LSPString key, value;
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            // Get default dictionary
            const meta::package_t *pkg  = pWrapper->package();
            const meta::plugin_t *meta  = pWrapper->ui()->metadata();
            i18n::IDictionary *dict     = get_default_dict(wnd);

            LSPString pkgver, plugver;
            pkgver.fmt_ascii("%d.%d.%d",
                    int(pkg->version.major),
                    int(pkg->version.minor),
                    int(pkg->version.micro)
            );
            if (pkg->version.branch)
                pkgver.fmt_append_utf8("-%s", pkg->version.branch);

            plugver.fmt_ascii("%d.%d.%d",
                    int(LSP_MODULE_VERSION_MAJOR(meta->version)),
                    int(LSP_MODULE_VERSION_MINOR(meta->version)),
                    int(LSP_MODULE_VERSION_MICRO(meta->version))
            );

            // Check that we really need to show notification window
            if (pPVersion != NULL)
            {
                const char *v = pPVersion->buffer<char>();
                if ((v != NULL) && (pkgver.equals_utf8(v)))
                    return STATUS_OK;

                const char *vstring = pkgver.get_utf8();
                pPVersion->write(vstring, strlen(vstring));
                pPVersion->notify_all();
            }

            lsp_trace("Showing notification dialog");

            if (wMessage == NULL)
            {
                wMessage = new tk::Window(wWidget->display());
                if (wMessage == NULL)
                    return STATUS_NO_MEM;

                vWidgets.add(wMessage);

                wMessage->init();
                wMessage->border_style()->set(ws::BS_DIALOG);
                wMessage->title()->set("titles.update_notification");
                wMessage->actions()->deny_all();
                wMessage->actions()->set_closeable(true);
                inject_style(wMessage, "GreetingDialog");

                tk::Box *vbox = new tk::Box(wWidget->display());
                vbox->init();
                vbox->orientation()->set_vertical();
                vbox->spacing()->set(8);
                vWidgets.add(vbox);
                wMessage->add(vbox);

                expr::Parameters p;
                p.set_string("package", &pkgver);
                p.set_string("version", &plugver);
                p.set_cstring("artifact", pkg->artifact);
                p.set_cstring("project", pkg->full_name);
                p.set_cstring("acronym", pkg->short_name);
                p.set_cstring("site", pkg->site);

                tk::Label *lbl  = create_label(vbox, "headings.greetings", "GreetingDialog::Heading");
                lbl  = create_plabel(vbox, "messages.greetings.0", &p, "GreetingDialog::Text");
                lbl->font()->set_bold();
                lbl  = create_plabel(vbox, "messages.greetings.1", &p, "GreetingDialog::Text");
                lbl  = create_label(vbox, "messages.greetings.2", "GreetingDialog::Text");

                // Create list of donation URLs
                if (dict)
                {
                    for (int i=0; ; ++i) {
                        if (!key.fmt_utf8("project.donations.%d", i))
                            break;
                        if (dict->lookup(&key, &value) != STATUS_OK)
                            break;
                        create_hlink(vbox, key.get_utf8(), key.get_utf8(), NULL, "GreetingDialog::Hlink");
                    }
                }

                lbl  = create_plabel(vbox, "messages.greetings.3", &p, "GreetingDialog::Text");
                lbl  = create_plabel(vbox, "messages.greetings.4", &p, "GreetingDialog::Text");

                lbl  = create_plabel(vbox, "messages.greetings.5", &p, "GreetingDialog::Postscript");
                lbl  = create_plabel(vbox, "messages.postscript", &p, "GreetingDialog::Postscript");
                create_hlink(vbox, pkg->site, "messages.site", &p, "GreetingDialog::PostscriptHlink");

                tk::Align *algn = new tk::Align(wWidget->display());
                algn->init();
                algn->allocation()->set_fill(true);
                vWidgets.add(algn);
                vbox->add(algn);

                tk::Button *btn = new tk::Button(wWidget->display());
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


