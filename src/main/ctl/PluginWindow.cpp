/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/plug-fw/core/presets.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/io/OutStringSequence.h>
#include <lsp-plug.in/io/InStringSequence.h>
#include <lsp-plug.in/tk/tk.h>

#include <lsp-plug.in/plug-fw/ui.h>
#include <private/ui/xml/RootNode.h>
#include <private/ui/xml/Handler.h>
#include <private/ctl/PluginWindowTemplate.h>

#define SCALING_FACTOR_BEGIN        50
#define SCALING_FACTOR_STEP         25
#define SCALING_FACTOR_END          400

#define FONT_SCALING_FACTOR_BEGIN   50
#define FONT_SCALING_FACTOR_STEP    10
#define FONT_SCALING_FACTOR_END     200

namespace lsp
{
    namespace ctl
    {
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
            return wrapper->import_settings(&is, ui::IMPORT_FLAG_NONE);
        }

        //-----------------------------------------------------------------
        // Plugin window
        const ctl_class_t PluginWindow::metadata = { "PluginWindow", &Window::metadata };

        const tk::tether_t PluginWindow::top_tether[] =
        {
            { tk::TF_BOTTOM | tk::TF_LEFT,      1.0f,  1.0f },
            { tk::TF_TOP | tk::TF_LEFT,         1.0f, -1.0f },
        };

        const tk::tether_t PluginWindow::bottom_tether[] =
        {
            { tk::TF_TOP | tk::TF_LEFT,         1.0f, -1.0f },
            { tk::TF_BOTTOM | tk::TF_LEFT,      1.0f,  1.0f },
        };

        const tk::tether_t PluginWindow::presets_tether[] =
        {
            { tk::TF_BOTTOM | tk::TF_LEFT,      1.0f,  1.0f },
            { tk::TF_TOP | tk::TF_LEFT,         1.0f, -1.0f },
            { tk::TF_BOTTOM | tk::TF_RIGHT,    -1.0f,  1.0f },
            { tk::TF_TOP | tk::TF_RIGHT,       -1.0f, -1.0f },
        };

        PluginWindow::PluginWindow(ui::IWrapper *src, tk::Window *widget): Window(src, widget)
        {
            pClass                      = &metadata;

            bResizable                  = false;

            pUserPaths                  = NULL;
            pPresetsWnd                 = NULL;

            wContent                    = NULL;
            wPresetsW                   = NULL;
            wGreeting                   = NULL;
            wAbout                      = NULL;
            wUserPaths                  = NULL;
            wMenu                       = NULL;
            wPresets                    = NULL;
            wUIScaling                  = NULL;
            wFontScaling                = NULL;
            wResetSettings              = NULL;
            wExport                     = NULL;
            wImport                     = NULL;
            wPreferHost                 = NULL;
            wKnobScaleEnable            = NULL;
            wOverrideHydrogen           = NULL;
            wRelPaths                   = NULL;
            wInvertVScroll              = NULL;
            wInvertGraphDotVScroll      = NULL;
            wZoomableSpectrum           = NULL;

            pPVersion                   = NULL;
            pPBypass                    = NULL;
            pPath                       = NULL;
            pFileType                   = NULL;
            pR3DBackend                 = NULL;
            pLanguage                   = NULL;
            pRelPaths                   = NULL;
            pUIScaling                  = NULL;
            pUIScalingHost              = NULL;
            pUIFontScaling              = NULL;
            pVisualSchema               = NULL;
            pKnobScaleEnable            = NULL;
            pOverrideHydrogen           = NULL;
            pInvertVScroll              = NULL;
            pInvertGraphDotVScroll      = NULL;
            pZoomableSpectrum           = NULL;

            init_enum_menu(&sFilterPointThickness);

            pConfigSink                 = NULL;

            sWndScale.nMFlags           = 0;
            sWndScale.sSize.nLeft       = 0;
            sWndScale.sSize.nTop        = 0;
            sWndScale.sSize.nWidth      = 0;
            sWndScale.sSize.nHeight     = 0;
            sWndScale.bActive           = false;
            sWndScale.nMouseX           = 0;
            sWndScale.nMouseY           = 0;
        }

        PluginWindow::~PluginWindow()
        {
            do_destroy();
        }

        void PluginWindow::destroy()
        {
            do_destroy();
            Window::destroy();
        }

        void PluginWindow::do_destroy()
        {
            // Cancel greeting timer
            wGreetingTimer.cancel();

            // Unbind configuration sink
            if (pConfigSink != NULL)
            {
                pConfigSink->unbind();
                pConfigSink->release();
                pConfigSink = NULL;
            }

            // Delete UI rendering backend bindings
            for (size_t i=0, n=vBackendSel.size(); i<n; ++i)
            {
                backend_sel_t *s = vBackendSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vBackendSel.flush();

            // Delete language selection bindings
            for (size_t i=0, n=vLangSel.size(); i<n; ++i)
            {
                lang_sel_t *s = vLangSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vLangSel.flush();

            // Delete UI scaling bindings
            for (size_t i=0, n=vScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *s = vScalingSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vScalingSel.flush();

            // Delete UI font scaling bindings
            for (size_t i=0, n=vFontScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *s = vFontScalingSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vFontScalingSel.flush();

            // Delete UI schema selection bindings
            for (size_t i=0, n=vSchemaSel.size(); i<n; ++i)
            {
                schema_sel_t *s = vSchemaSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vSchemaSel.flush();

            // Delete preset list
            for (size_t i=0, n=vPresetSel.size(); i<n; ++i)
            {
                preset_sel_t *s = vPresetSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vPresetSel.flush();

            pUserPaths      = NULL;

            wContent        = NULL;
            wPresetsW       = NULL;
            wGreeting       = NULL;
            wAbout          = NULL;
            wUserPaths      = NULL;
            wMenu           = NULL;
            wPresets        = NULL;
            wResetSettings  = NULL;
            wExport         = NULL;
            wImport         = NULL;
            wPreferHost     = NULL;
            wPresetsW       = NULL;
        }

        void PluginWindow::init_enum_menu(enum_menu_t *menu)
        {
            menu->pWnd      = this;
            menu->pMenu     = NULL;
            menu->pPort     = NULL;
        }

        void PluginWindow::bind_trigger(const char *uid, tk::slot_t ev, tk::event_handler_t handler)
        {
            tk::Widget *w = widgets()->find(uid);
            if (w == NULL)
                return;
            w->slots()->bind(ev, handler, this);
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
            PluginWindow *self = static_cast<PluginWindow *>(ptr);
            self->locate_window();
            self->set_greeting_timer();
            return STATUS_OK;
        }

        void PluginWindow::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            set_value(&bResizable, "resizable", name, value);
            Window::set(ctx, name, value);
        }

        void PluginWindow::set_preset_button_text(const char *text)
        {
            tk::Button *presetButton = tk::widget_cast<tk::Button>(widgets()->find("trg_presets_menu"));

            if (presetButton != NULL)
            {
                presetButton->text()->set_raw(text);
            }
        }

        status_t PluginWindow::init()
        {
            Window::init();

            // Get window handle
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            // Bind ports
            BIND_PORT(pWrapper, pPVersion, VERSION_PORT);
            BIND_PORT(pWrapper, pPath, CONFIG_PATH_PORT);
            BIND_PORT(pWrapper, pFileType, CONFIG_FTYPE_PORT);
            BIND_PORT(pWrapper, pPBypass, meta::PORT_NAME_BYPASS);
            BIND_PORT(pWrapper, pR3DBackend, R3D_BACKEND_PORT);
            BIND_PORT(pWrapper, pLanguage, LANGUAGE_PORT);
            BIND_PORT(pWrapper, pRelPaths, REL_PATHS_PORT);
            BIND_PORT(pWrapper, pUIScaling, UI_SCALING_PORT);
            BIND_PORT(pWrapper, pUIScalingHost, UI_SCALING_HOST);
            BIND_PORT(pWrapper, pUIFontScaling, UI_FONT_SCALING_PORT);
            BIND_PORT(pWrapper, pVisualSchema, UI_VISUAL_SCHEMA_PORT);
            BIND_PORT(pWrapper, pKnobScaleEnable, UI_ENABLE_KNOB_SCALE_ACTIONS_PORT);
            BIND_PORT(pWrapper, pOverrideHydrogen, UI_OVERRIDE_HYDROGEN_KITS_PORT);
            BIND_PORT(pWrapper, pInvertVScroll, UI_INVERT_VSCROLL_PORT);
            BIND_PORT(pWrapper, pInvertGraphDotVScroll, UI_GRAPH_DOT_INVERT_VSCROLL_PORT);
            BIND_PORT(pWrapper, pZoomableSpectrum, UI_ZOOMABLE_SPECTRUM_GRAPH_PORT);
            BIND_PORT(pWrapper, sFilterPointThickness.pPort, UI_FILTER_POINT_THICK_PORT);

            const meta::plugin_t *meta   = pWrapper->ui()->metadata();

            // Initialize window
            wnd->set_class(meta->uid, "lsp-plugins");
            wnd->role()->set("audio-plugin");
            wnd->title()->set_raw(meta->name);
            wnd->layout()->set_scale(1.0f);

            if (!wnd->nested())
                wnd->actions()->deny(ws::WA_RESIZE);

            LSP_STATUS_ASSERT(create_main_menu());
            // LSP_STATUS_ASSERT(create_reset_settings_menu());

            // Bind event handlers
            wnd->slots()->bind(tk::SLOT_CLOSE, slot_window_close, this);
            wnd->slots()->bind(tk::SLOT_SHOW, slot_window_show, this);
            wnd->slots()->bind(tk::SLOT_RESIZE, slot_window_resize, this);

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
            widgets()->add(WUID_MAIN_MENU, wMenu);
            wMenu->init();

            // TODO: Move to a separate method
            wPresets = new tk::Menu(dpy);
            widgets()->add(WUID_PRESETS_MENU, wPresets);
            wPresets->init();

            // Initialize menu items
            {
                // Add 'Plugin manual' menu item
                tk::MenuItem *itm       = new tk::MenuItem(dpy);
                widgets()->add(itm);
                itm->init();
                itm->text()->set("actions.plugin_manual");
                itm->slots()->bind(tk::SLOT_SUBMIT, slot_show_plugin_manual, this);
                wMenu->add(itm);

                // Add 'UI manual' menu item
                itm                     = new tk::MenuItem(dpy);
                widgets()->add(itm);
                itm->init();
                itm->text()->set("actions.ui_manual");
                itm->slots()->bind(tk::SLOT_SUBMIT, slot_show_ui_manual, this);
                wMenu->add(itm);

                // init_presets(wMenu, true);
                init_presets(wPresets, false);

                // Add separator
                itm     = new tk::MenuItem(dpy);
                widgets()->add(itm);
                itm->init();
                itm->type()->set_separator();
                wMenu->add(itm);

                // Create 'Export' submenu and bind to parent
                tk::Menu *submenu = new tk::Menu(dpy);
                widgets()->add(WUID_EXPORT_MENU, submenu);
                submenu->init();

                itm = new tk::MenuItem(dpy);
                widgets()->add(itm);
                itm->init();
                itm->text()->set("actions.export");
                itm->menu()->set(submenu);
                wMenu->add(itm);

                // Create 'Export' menu items
                {
                    tk::MenuItem *child = new tk::MenuItem(dpy);
                    widgets()->add(child);
                    child->init();
                    child->text()->set("actions.export_settings_to_file");
                    child->slots()->bind(tk::SLOT_SUBMIT, slot_export_settings_to_file, this);
                    submenu->add(child);

                    child = new tk::MenuItem(dpy);
                    widgets()->add(child);
                    child->init();
                    child->text()->set("actions.export_settings_to_clipboard");
                    child->slots()->bind(tk::SLOT_SUBMIT, slot_export_settings_to_clipboard, this);
                    submenu->add(child);
                }

                // Create 'Import' menu and bind to parent
                submenu = new tk::Menu(dpy);
                widgets()->add(WUID_IMPORT_MENU, submenu);
                submenu->init();

                itm = new tk::MenuItem(dpy);
                widgets()->add(itm);
                itm->init();
                itm->text()->set("actions.import");
                itm->menu()->set(submenu);
                wMenu->add(itm);

                // Create import menu items
                {
                    tk::MenuItem *child = new tk::MenuItem(dpy);
                    widgets()->add(child);
                    child->init();
                    child->text()->set("actions.import_settings_from_file");
                    child->slots()->bind(tk::SLOT_SUBMIT, slot_import_settings_from_file, this);
                    submenu->add(child);

                    child = new tk::MenuItem(dpy);
                    widgets()->add(child);
                    child->init();
                    child->text()->set("actions.import_settings_from_clipboard");
                    child->slots()->bind(tk::SLOT_SUBMIT, slot_import_settings_from_clipboard, this);
                    submenu->add(child);

                    child = new tk::MenuItem(dpy);
                    widgets()->add(child);
                    child->init();
                    child->text()->set("actions.reset_settings");
                    // TODO: Add confirmation dialog here
                    child->slots()->bind(tk::SLOT_SUBMIT, slot_confirm_reset_settings, this);
                    submenu->add(child);
                }

                itm     = new tk::MenuItem(dpy);
                widgets()->add(itm);
                itm->init();
                itm->text()->set("actions.user_paths");
                itm->slots()->bind(tk::SLOT_SUBMIT, slot_show_user_paths_dialog, this);
                wMenu->add(itm);

                // Create 'Dump state' menu item if supported
                if (meta->extensions & meta::E_DUMP_STATE)
                {
                    itm     = new tk::MenuItem(dpy);
                    widgets()->add(itm);
                    itm->init();
                    itm->text()->set("actions.debug_dump");
                    itm->slots()->bind(tk::SLOT_SUBMIT, slot_debug_dump, this);
                    wMenu->add(itm);
                }

                // Add separator
                itm     = new tk::MenuItem(dpy);
                widgets()->add(itm);
                itm->init();
                itm->type()->set_separator();
                wMenu->add(itm);

                // Create UI scaling menu
                init_scaling_support(wMenu);

                // Create UI scaling menu
                init_font_scaling_support(wMenu);

                // Create schema selection support menu
                init_visual_schema_support(wMenu);

                // Create language selection menu
                init_i18n_support(wMenu);

                // Add support of 3D rendering backend switch
                if (meta->extensions & meta::E_3D_BACKEND)
                    init_r3d_support(wMenu);

                // Create UI behaviour menu
                init_ui_behaviour(wMenu);
            }

            return STATUS_OK;
        }

        // status_t PluginWindow::create_reset_settings_menu()
        // {
        //     tk::Window *wnd             = tk::widget_cast<tk::Window>(wWidget);
        //     tk::Display *dpy            = wnd->display();

        //     // Initialize menu
        //     wResetSettings              = new tk::Menu(dpy);
        //     widgets()->add(WUID_RESET_SETTINGS_MENU, wResetSettings);
        //     wResetSettings->init();
        //     inject_style(wResetSettings, "PluginWindow::ResetMenu");

        //     // Initialize menu items
        //     {
        //         // Add 'Reset' menu item
        //         tk::MenuItem *itm       = new tk::MenuItem(dpy);
        //         widgets()->add(itm);
        //         itm->init();
        //         itm->text()->set("actions.reset");
        //         inject_style(itm, "PluginWindow::ResetMenu::Reset");
        //         itm->slots()->bind(tk::SLOT_SUBMIT, slot_confirm_reset_settings, this);
        //         wResetSettings->add(itm);
        //     }

        //     return STATUS_OK;
        // }

        tk::MenuItem *PluginWindow::create_menu_item(tk::Menu *dst)
        {
            tk::MenuItem *item = new tk::MenuItem(dst->display());
            if (item == NULL)
                return NULL;

            status_t res = item->init();
            if (res != STATUS_OK)
            {
                item->destroy();
                delete item;
                return NULL;
            }

            if (widgets()->add(item) != STATUS_OK)
            {
                item->destroy();
                delete item;
                return NULL;
            }

            dst->add(item);

            return item;
        }

        tk::Menu *PluginWindow::create_menu()
        {
            status_t res;
            tk::Menu *menu = new tk::Menu(wWidget->display());
            if (menu == NULL)
                return NULL;

            if ((res = menu->init()) != STATUS_OK)
            {
                menu->destroy();
                delete menu;
                return NULL;
            }
            if (widgets()->add(menu) != STATUS_OK)
            {
                menu->destroy();
                delete menu;
                return NULL;
            }

            return menu;
        }

        tk::Menu *PluginWindow::create_enum_menu(enum_menu_t *em, tk::Menu *parent, const char *label)
        {
            // Obtain port metadata
            lltl::parray<tk::MenuItem> list;
            const meta::port_t *meta = (em->pPort != NULL) ? em->pPort->metadata() : NULL;
            if ((meta == NULL) || (!meta::is_enum_unit(meta->unit)))
                return NULL;

            // Create root menu
            tk::Menu *menu = create_menu();
            if (menu == NULL)
                return menu;

            // Add items from the enumeration
            LSPString lc_key;
            for (const meta::port_item_t *item = meta->items; item->text != NULL; ++item)
            {
                // Create submenu item
                tk::MenuItem *mi            = create_menu_item(menu);
                if (item == NULL)
                    return NULL;

                // Set the text value
                mi->type()->set_radio();
                if (item->lc_key != NULL)
                {
                    if (!lc_key.set_ascii("lists."))
                        return NULL;
                    if (!lc_key.append_ascii(item->lc_key))
                        return NULL;
                    mi->text()->set(&lc_key);
                }
                else
                    mi->text()->set_raw(item->text);

                // Bind slots
                mi->slots()->bind(tk::SLOT_SUBMIT, slot_submit_enum_menu_item, em);

                // Add to list
                if (!list.add(mi))
                    return NULL;
            }

            // Return result
            list.swap(em->vItems);

            // Add to parent
            if (parent != NULL)
            {
                tk::MenuItem *item          = create_menu_item(parent);
                if (item != NULL)
                {
                    item->text()->set(label);
                    item->menu()->set(menu);
                }
            }

            return menu;
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
            tk::MenuItem *root          = create_menu_item(menu);
            if (root == NULL)
                return STATUS_NO_MEM;
            root->text()->set("actions.select_language");

            // Create submenu
            menu                        = create_menu();
            if (menu == NULL)
                return STATUS_NO_MEM;
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
                lsp_trace("i18n: key=%s, value=%s", key.get_utf8(), value.get_utf8());

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
                tk::MenuItem *item = create_menu_item(menu);
                if (item == NULL)
                    return STATUS_NO_MEM;
                item->text()->set_raw(&value);
                item->type()->set_radio();
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
                        pLanguage->notify_all(ui::PORT_NONE);
                    }
                }
            }

            return STATUS_OK;
        }

        status_t PluginWindow::init_scaling_support(tk::Menu *menu)
        {
            // Create submenu item
            tk::MenuItem *item          = create_menu_item(menu);
            if (item == NULL)
                return STATUS_NO_MEM;
            item->text()->set("actions.ui_scaling.select");

            // Create submenu
            menu                        = create_menu();
            if (menu == NULL)
                return STATUS_NO_MEM;
            item->menu()->set(menu);
            wUIScaling      = menu;

            // Initialize 'Prefer the host' settings
            if ((item = create_menu_item(menu)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.ui_scaling.prefer_host");
            item->type()->set_check();
            item->slots()->bind(tk::SLOT_SUBMIT, slot_scaling_toggle_prefer_host, this);
            wPreferHost     = item;

            // Add the 'Zoom in' setting
            if ((item = create_menu_item(menu)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.ui_scaling.zoom_in");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_scaling_zoom_in, this);

            // Add the 'Zoom out' setting
            if ((item = create_menu_item(menu)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.ui_scaling.zoom_out");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_scaling_zoom_out, this);

            // Add the separator
            if ((item = create_menu_item(menu)) == NULL)
                return STATUS_NO_MEM;
            item->type()->set_separator();

            // Generate the 'Set scaling' menu items
            scaling_sel_t *sel;

            for (size_t scale = SCALING_FACTOR_BEGIN; scale <= SCALING_FACTOR_END; scale += SCALING_FACTOR_STEP)
            {
                if ((item = create_menu_item(menu)) == NULL)
                    return STATUS_NO_MEM;
                item->type()->set_radio();
                item->text()->set_key("actions.ui_scaling.value:pc");
                item->text()->params()->set_int("value", scale);

                // Add scaling record
                if ((sel = new scaling_sel_t()) == NULL)
                    return STATUS_NO_MEM;

                sel->ctl        = this;
                sel->scaling    = scale;
                sel->item       = item;

                if (!vScalingSel.add(sel))
                {
                    delete sel;
                    return STATUS_NO_MEM;
                }

                item->slots()->bind(tk::SLOT_SUBMIT, slot_scaling_select, sel);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::init_font_scaling_support(tk::Menu *menu)
        {
            // Create submenu item
            tk::MenuItem *item          = create_menu_item(menu);
            if (item == NULL)
                return STATUS_NO_MEM;
            item->text()->set("actions.font_scaling.select");

            // Create submenu
            menu                        = create_menu();
            item->menu()->set(menu);
            wFontScaling    = menu;

            // Add the 'Zoom in' setting
            if ((item = create_menu_item(menu)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.font_scaling.zoom_in");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_font_scaling_zoom_in, this);

            // Add the 'Zoom out' setting
            if ((item = create_menu_item(menu)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.font_scaling.zoom_out");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_font_scaling_zoom_out, this);

            // Add the separator
            if ((item = create_menu_item(menu)) == NULL)
                return STATUS_NO_MEM;
            item->type()->set_separator();

            // Generate the 'Set font scaling' menu items
            scaling_sel_t *sel;

            for (size_t scale = FONT_SCALING_FACTOR_BEGIN; scale <= FONT_SCALING_FACTOR_END; scale += FONT_SCALING_FACTOR_STEP)
            {
                if ((item = create_menu_item(menu)) == NULL)
                    return STATUS_NO_MEM;
                item->type()->set_radio();
                item->text()->set_key("actions.font_scaling.value:pc");
                item->text()->params()->set_int("value", scale);

                // Add scaling record
                if ((sel = new scaling_sel_t()) == NULL)
                    return STATUS_NO_MEM;

                sel->ctl        = this;
                sel->scaling    = scale;
                sel->item       = item;

                if (!vFontScalingSel.add(sel))
                {
                    delete sel;
                    return STATUS_NO_MEM;
                }

                item->slots()->bind(tk::SLOT_SUBMIT, slot_font_scaling_select, sel);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::init_visual_schema_support(tk::Menu *menu)
        {
            status_t res;
            resource::ILoader *loader = pWrapper->resources();
            if ((loader == NULL) || (pVisualSchema == NULL))
                return STATUS_OK;

            // Create submenu item
            tk::MenuItem *item          = create_menu_item(menu);
            if (item == NULL)
                return STATUS_NO_MEM;
            item->text()->set("actions.visual_schema.select");

            // Create submenu
            menu                        = create_menu();
            if (menu == NULL)
                return STATUS_NO_MEM;
            item->menu()->set(menu);
            tk::MenuItem *root  = item;

            // For that we need to scan all available schemas in the resource directory
            resource::resource_t *list = NULL;
            ssize_t count = loader->enumerate(LSP_BUILTIN_PREFIX "schema", &list);
            if ((count <= 0) || (list == NULL))
            {
                if (list != NULL)
                    free(list);
                return STATUS_OK;
            }

            // Generate the 'Visual Schema' menu items
            schema_sel_t *sel;

            for (ssize_t i=0; i<count; ++i)
            {
                tk::StyleSheet sheet;
                LSPString path;

                if (list[i].type != resource::RES_FILE)
                    continue;

                if (!path.fmt_ascii(LSP_BUILTIN_PREFIX "schema/%s", list[i].name))
                {
                    free(list);
                    return STATUS_NO_MEM;
                }

                // Try to load schema
                if ((res = pWrapper->load_stylesheet(&sheet, &path)) != STATUS_OK)
                {
                    if (res == STATUS_NO_MEM)
                    {
                        free(list);
                        return res;
                    }
                    continue;
                }

                // Append menu item
                if ((item = create_menu_item(menu)) == NULL)
                    return STATUS_NO_MEM;
                item->type()->set_radio();
                item->text()->set_key(sheet.title());
                item->text()->params()->set_string("file", &path);

                // Append the shema to the menu
                if ((sel = new schema_sel_t()) == NULL)
                {
                    free(list);
                    return STATUS_NO_MEM;
                }

                sel->ctl        = this;
                sel->item       = item;
                sel->location.swap(&path);

                if (!vSchemaSel.add(sel))
                {
                    delete sel;
                    free(list);
                    return STATUS_NO_MEM;
                }

                item->slots()->bind(tk::SLOT_SUBMIT, slot_visual_schema_select, sel);
            }

            free(list);
            root->visibility()->set(vSchemaSel.size() > 0);

            return STATUS_OK;
        }

        status_t PluginWindow::init_ui_behaviour(tk::Menu *menu)
        {
            // Create submenu item
            tk::MenuItem *item          = create_menu_item(menu);
            if (item == NULL)
                return STATUS_NO_MEM;
            item->text()->set("actions.ui_behavior");

            // Create submenu
            menu                        = create_menu();
            if (menu == NULL)
                return STATUS_NO_MEM;
            item->menu()->set(menu);

            // Thickness of the enum menu item
            wFilterPointThickness = create_enum_menu(&sFilterPointThickness, menu, "actions.ui_behavior.filter_point_thickness");

            // Create menu items
            if ((wKnobScaleEnable = create_menu_item(menu)) != NULL)
            {
                wKnobScaleEnable->type()->set_check();
                wKnobScaleEnable->text()->set("actions.ui_behavior.ediable_knob_scale");
                wKnobScaleEnable->slots()->bind(tk::SLOT_SUBMIT, slot_enable_slot_scale_changed, this);
            }

            if ((wOverrideHydrogen = create_menu_item(menu)) != NULL)
            {
                wOverrideHydrogen->type()->set_check();
                wOverrideHydrogen->text()->set("actions.ui_behavior.override_hydrogen_kits");
                wOverrideHydrogen->slots()->bind(tk::SLOT_SUBMIT, slot_override_hydrogen_kits_changed, this);
            }

            // Vertical scroll inversion
            if ((wInvertVScroll = create_menu_item(menu)) != NULL)
            {
                wInvertVScroll->type()->set_check();
                wInvertVScroll->text()->set("actions.ui_behavior.vscroll.invert_global");
                wInvertVScroll->slots()->bind(tk::SLOT_SUBMIT, slot_invert_vscroll_changed, this);
            }

            if ((wInvertGraphDotVScroll = create_menu_item(menu)) != NULL)
            {
                wInvertGraphDotVScroll->type()->set_check();
                wInvertGraphDotVScroll->text()->set("actions.ui_behavior.vscroll.invert_graph_dot");
                wInvertGraphDotVScroll->slots()->bind(tk::SLOT_SUBMIT, slot_invert_graph_dot_vscroll_changed, this);
            }

            // Auto scale spectrum
            if ((wZoomableSpectrum = create_menu_item(menu)) != NULL)
            {
                wZoomableSpectrum->type()->set_check();
                wZoomableSpectrum->text()->set("actions.ui_behavior.enable_zoomable_spectrum");
                wZoomableSpectrum->slots()->bind(tk::SLOT_SUBMIT, slot_zoomable_spectrum_changed, this);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::init_presets(tk::Menu *menu, bool add_submenu)
        {
            status_t res;
            if (menu == NULL)
                return STATUS_OK;

            // Enumerate presets
            lltl::darray<resource::resource_t> presets;
            const meta::plugin_t *metadata = pWrapper->ui()->metadata();
            if ((metadata == NULL))
                return STATUS_OK;

            tk::MenuItem *item;

            if (add_submenu)
            {
                // Create submenu item
                item          = create_menu_item(menu);
                if (item == NULL)
                    return STATUS_NO_MEM;
                item->text()->set("actions.load_preset");

                // Create submenu
                menu                        = create_menu();
                if (menu == NULL)
                    return STATUS_NO_MEM;
                item->menu()->set(menu);
            }

            preset_sel_t *sel;
            io::Path path;
            LSPString tmp;

            if ((item = create_menu_item(menu)) == NULL) return STATUS_NO_MEM;
            item->text()->set_raw("Initial preset");
            // item->text()->set("actions.reset_settings");
            // TODO: Add confirmation dialog here
            item->slots()->bind(tk::SLOT_SUBMIT, slot_confirm_reset_settings, this);

            if ((item = create_menu_item(menu)) == NULL) return STATUS_NO_MEM;
            item->text()->set_raw("Presets window sketch");
            // item->text()->set("actions.reset_settings");
            // TODO: Add confirmation dialog here
            item->slots()->bind(tk::SLOT_SUBMIT, slot_show_presets_window, this);

            if ((item = create_menu_item(menu)) == NULL) return STATUS_NO_MEM;
            item->type()->set_separator();

            if ((item = create_menu_item(menu)) == NULL) return STATUS_NO_MEM;
            item->text()->set("actions.import_settings_from_file");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_import_settings_from_file, this);

            if ((item = create_menu_item(menu)) == NULL) return STATUS_NO_MEM;
            item->text()->set("actions.import_settings_from_clipboard");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_import_settings_from_clipboard, this);

            // if ((item = create_menu_item(menu)) == NULL) return STATUS_NO_MEM;
            // item->text()->set("*Open presets folder*");

            if (core::scan_presets(&presets, pWrapper->resources(), metadata->ui_presets) != STATUS_OK)
                return STATUS_OK;
            if (presets.is_empty())
                return STATUS_OK;
            core::sort_presets(&presets, true);

            if ((item = create_menu_item(menu)) == NULL)
                return STATUS_NO_MEM;
            item->type()->set_separator();

            for (size_t i=0, n=presets.size(); i<n; ++i)
            {
                // Enumerate next backend information
                const resource::resource_t *preset = presets.uget(i);
                if ((res = path.set(preset->name)) != STATUS_OK)
                    return res;

                // Create menu item
                if ((item = create_menu_item(menu)) == NULL)
                    return STATUS_NO_MEM;

                // Get name of the preset/patch without an extension
                if ((res = path.get_last_noext(&tmp)) != STATUS_OK)
                    return res;
                item->text()->set_raw(&tmp);
                if ((res = path.get_ext(&tmp)) != STATUS_OK)
                    return res;

                // Create backend information
                if ((sel = new preset_sel_t()) == NULL)
                    return STATUS_NO_MEM;

                sel->ctl    = this;
                sel->item   = item;
                sel->patch  = tmp.equals_ascii("patch");
                sel->location.fmt_utf8(LSP_BUILTIN_PREFIX "presets/%s/%s", metadata->ui_presets, preset->name);

                if (!vPresetSel.add(sel))
                {
                    delete sel;
                    return STATUS_NO_MEM;
                }

                item->slots()->bind(tk::SLOT_SUBMIT, slot_select_preset, sel);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::init_r3d_support(tk::Menu *menu)
        {
            if (menu == NULL)
                return STATUS_OK;

            ws::IDisplay *dpy           = menu->display()->display();
            if (dpy == NULL)
                return STATUS_OK;

            // Create submenu item
            tk::MenuItem *item          = create_menu_item(menu);
            if (item == NULL)
                return STATUS_NO_MEM;
            item->text()->set("actions.3d_rendering");

            // Get backend port
            const char *backend = (pR3DBackend != NULL) ? pR3DBackend->buffer<char>() : NULL;

            // Create submenu
            menu                        = create_menu();
            if (menu == NULL)
                return STATUS_NO_MEM;
            item->menu()->set(menu);

            backend_sel_t *sel;

            for (size_t id=0; ; ++id)
            {
                // Enumerate next backend information
                const ws::R3DBackendInfo *info = dpy->enum_backend(id);
                if (info == NULL)
                    break;

                // Create menu item
                if ((item = create_menu_item(menu)) == NULL)
                    return STATUS_NO_MEM;

                item->type()->set_radio();
                if (!info->lc_key.is_empty())
                {
                    LSPString tmp;
                    tmp.set_ascii("lists.rendering.");
                    tmp.append(&info->lc_key);
                    item->text()->set_key(&tmp);
                }
                else
                    item->text()->set_raw(&info->display);

                // Create backend information
                if ((sel = new backend_sel_t()) == NULL)
                    return STATUS_NO_MEM;

                sel->ctl    = this;
                sel->item   = item;
                sel->id     = id;
                item->slots()->bind(tk::SLOT_SUBMIT, slot_select_backend, sel);
                item->checked()->set((backend != NULL) && (info->uid.equals_ascii(backend)));

                if (!vBackendSel.add(sel))
                {
                    delete sel;
                    return STATUS_NO_MEM;
                }
            }

            // Select fallback backend if none
            if ((backend == NULL) && (vBackendSel.size() > 0))
            {
                sel = vBackendSel.uget(0);
                if (sel != NULL)
                    slot_select_backend(sel->item, sel, NULL);
            }

            return STATUS_OK;
        }

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

        status_t PluginWindow::slot_select_backend(tk::Widget *sender, void *ptr, void *data)
        {
            backend_sel_t *sel = reinterpret_cast<backend_sel_t *>(ptr);
            if ((sender == NULL) || (sel == NULL) || (sel->ctl == NULL))
                return STATUS_BAD_ARGUMENTS;

            ws::IDisplay *dpy = sender->display()->display();
            if (dpy == NULL)
                return STATUS_BAD_STATE;

            const ws::R3DBackendInfo *info = dpy->enum_backend(sel->id);
            if (info == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Mark backend as selected
            dpy->select_backend_id(sel->id);

            // Synchronize state of radio buttons
            for (size_t i=0, n=sel->ctl->vBackendSel.size(); i<n; ++i)
            {
                backend_sel_t *xsel     = sel->ctl->vBackendSel.uget(i);
                if (xsel->item == NULL)
                    continue;
                xsel->item->checked()->set(xsel->id == sel->id);
            }

            // Need to commit backend identifier to config file?
            const char *value = info->uid.get_ascii();
            if (value == NULL)
                return STATUS_NO_MEM;

            if (sel->ctl->pR3DBackend != NULL)
            {
                const char *backend = sel->ctl->pR3DBackend->buffer<char>();
                if ((backend == NULL) || (strcmp(backend, value)))
                {
                    sel->ctl->pR3DBackend->write(value, strlen(value));
                    sel->ctl->pR3DBackend->notify_all(ui::PORT_USER_EDIT);
                }
            }

            return STATUS_OK;
        }

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
                sel->ctl->pLanguage->notify_all(ui::PORT_USER_EDIT);
            }

            lsp_trace("Language has been selected");

            return STATUS_OK;
        }

        status_t PluginWindow::slot_select_preset(tk::Widget *sender, void *ptr, void *data)
        {
            preset_sel_t *sel = reinterpret_cast<preset_sel_t *>(ptr);
            lsp_trace("sender=%p, sel=%p", sender, sel);
            if ((sender == NULL) || (sel == NULL) || (sel->ctl == NULL) || (sel->item == NULL))
                return STATUS_BAD_ARGUMENTS;

            sel->ctl->set_preset_button_text(sel->item->text()->raw()->get_utf8());

            lsp_trace("Loading preset %s", sel->location.get_native());
            size_t flags = ui::IMPORT_FLAG_PRESET;
            if (sel->patch)
                flags      |= ui::IMPORT_FLAG_PATCH;
            sel->ctl->pWrapper->import_settings(&sel->location, flags);

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

        void PluginWindow::sync_ui_scaling()
        {
            tk::Display *dpy    = wWidget->display();
            if (dpy == NULL)
                return;

            bool sync_host      = (pUIScalingHost->value() >= 0.5f);
            float scaling       = (pUIScaling != NULL) ? pUIScaling->value() : 100.0f;

            lsp_trace("sync_host = %s, scaling=%f", (sync_host) ? "true" : "false", scaling);

            if (sync_host)
                scaling             = pWrapper->ui_scaling_factor(scaling);

            // Update the UI scaling
            dpy->schema()->scaling()->set(scaling * 0.01f);
            scaling             = dpy->schema()->scaling()->get() * 100.0f;

            // Synchronize state of menu check boxes
            if (wPreferHost != NULL)
                wPreferHost->checked()->set(sync_host);

            for (size_t i=0, n=vScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *xsel = vScalingSel.uget(i);
                if (xsel->item != NULL)
                    xsel->item->checked()->set(fabs(xsel->scaling - scaling) < 1e-4f);
            }
        }

        void PluginWindow::host_scaling_changed()
        {
            if (pUIScalingHost != NULL)
                pUIScalingHost->notify_all(ui::PORT_NONE);
            else if (pUIScaling != NULL)
                pUIScaling->notify_all(ui::PORT_NONE);
        }

        void PluginWindow::sync_invert_vscroll(ui::IPort *port)
        {
            tk::Display *dpy    = wWidget->display();
            if (dpy == NULL)
                return;

            bool invert_global  = (pInvertVScroll != NULL) ? pInvertVScroll->value() >= 0.5f : false;
            bool invert_gdot    = (pInvertGraphDotVScroll != NULL) ? (pInvertGraphDotVScroll->value() >= 0.5f) ^ invert_global : invert_global;

            lsp_trace("mouse vscroll invert: global=%s, graph_dot=%s",
                (invert_global) ? "true" : "false",
                (invert_gdot) ? "true" : "false");

            if ((port == pInvertVScroll) && (wInvertVScroll != NULL))
                wInvertVScroll->checked()->set(invert_global);
            if ((port == pInvertGraphDotVScroll) && (wInvertGraphDotVScroll != NULL))
                wInvertGraphDotVScroll->checked()->set(invert_gdot);

            // Set global configuration
            dpy->schema()->invert_mouse_vscroll()->set(invert_global);

            tk::Style *gdot_style = dpy->schema()->get("GraphDot");
            if (gdot_style != NULL)
                gdot_style->set_bool("mouse.vscroll.invert", invert_gdot);
        }

        void PluginWindow::sync_zoomable_spectrum()
        {
            tk::Display *dpy    = wWidget->display();
            if (dpy == NULL)
                return;

            bool zoomable  = (pZoomableSpectrum != NULL) ? pZoomableSpectrum->value() >= 0.5f : false;

            if (wZoomableSpectrum != NULL)
                wZoomableSpectrum->checked()->set(zoomable);
        }

        void PluginWindow::sync_enum_menu(enum_menu_t *menu, ui::IPort *port)
        {
            if ((port == NULL) || (port != menu->pPort))
                return;
            const meta::port_t *meta = menu->pPort->metadata();
            if (meta == NULL)
                return;

            tk::Display *dpy    = wWidget->display();
            if (dpy == NULL)
                return;

            const ssize_t index = menu->pPort->value() - meta->min;
            for (lltl::iterator<tk::MenuItem> it = menu->vItems.values(); it; ++it)
            {
                tk::MenuItem *mi = it.get();
                mi->checked()->set(ssize_t(it.index()) == index);
            }
        }

        void PluginWindow::sync_font_scaling()
        {
            tk::Display *dpy    = wWidget->display();
            if (dpy == NULL)
                return;

            float scaling       = (pUIFontScaling != NULL) ? pUIFontScaling->value() : 100.0f;

            // Update the UI scaling
            dpy->schema()->font_scaling()->set(scaling * 0.01f);
            scaling             = dpy->schema()->font_scaling()->get() * 100.0f;

            for (size_t i=0, n=vFontScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *xsel = vFontScalingSel.uget(i);
                if (xsel->item != NULL)
                    xsel->item->checked()->set(fabs(xsel->scaling - scaling) < 1e-4);
            }
        }

        void PluginWindow::sync_visual_schemas()
        {
            const char *path = (pVisualSchema != NULL) ? pVisualSchema->buffer<const char>() : NULL;

            // Update the UI scaling
            for (size_t i=0, n=vSchemaSel.size(); i<n; ++i)
            {
                schema_sel_t *xsel = vSchemaSel.uget(i);
                if (xsel->item != NULL)
                {
                    bool checked = (path != NULL) && xsel->location.equals_utf8(path);
                    xsel->item->checked()->set(checked);
                }
            }
        }

        void PluginWindow::sync_knob_scale_enabled()
        {
            // Update the knob scale
            bool knob_enable    = (pKnobScaleEnable != NULL) ? pKnobScaleEnable->value() >= 0.5f : true;
            if (wKnobScaleEnable != NULL)
                wKnobScaleEnable->checked()->set(knob_enable);
        }

        void PluginWindow::sync_override_hydrogen()
        {
            // Update the knob scale
            bool set_override   = (pOverrideHydrogen != NULL) ? pOverrideHydrogen->value() >= 0.5f : true;
            if (wOverrideHydrogen != NULL)
                wOverrideHydrogen->checked()->set(set_override);
        }

        void PluginWindow::begin(ui::UIContext *ctx)
        {
            Window::begin(ctx);
            status_t res;

            // Create context
            ui::UIContext uctx(pWrapper, controllers(), widgets());
            if ((res = init_context(&uctx)) != STATUS_OK)
                return;

            // Parse the XML document
            ctl::PluginWindowTemplate wnd(pWrapper, this);
            if (wnd.init() != STATUS_OK)
                return;

            ui::xml::RootNode root(&uctx, "window", &wnd);
            ui::xml::Handler handler(pWrapper->resources());
            res = handler.parse_resource(LSP_BUILTIN_PREFIX "ui/window.xml", &root);
            if (res != STATUS_OK)
                lsp_warn("Error parsing resource: %s, error: %d", LSP_BUILTIN_PREFIX "ui/window.xml", int(res));
            wnd.destroy();

            // Get proper widgets and initialize window layout
            wContent        = tk::widget_cast<tk::WidgetContainer>(widgets()->find("plugin_content"));

            // Header menu
            bind_trigger("trg_main_menu", tk::SLOT_SUBMIT, slot_show_main_menu);
            bind_trigger("trg_presets_menu", tk::SLOT_SUBMIT, slot_show_presets_window);
            bind_trigger("trg_export_settings", tk::SLOT_SUBMIT, slot_export_settings_to_file);
            bind_trigger("trg_import_settings", tk::SLOT_SUBMIT, slot_import_settings_from_file);
            bind_trigger("trg_reset_settings", tk::SLOT_SUBMIT, slot_reset_settings);
            bind_trigger("trg_about", tk::SLOT_SUBMIT, slot_show_about);

            // Footer
            bind_trigger("trg_ui_scaling", tk::SLOT_SUBMIT, slot_show_ui_scaling_menu);
            bind_trigger("trg_font_scaling", tk::SLOT_SUBMIT, slot_show_font_scaling_menu);
            bind_trigger("trg_ui_zoom_in", tk::SLOT_SUBMIT, slot_scaling_zoom_in);
            bind_trigger("trg_ui_zoom_out", tk::SLOT_SUBMIT, slot_scaling_zoom_out);
            bind_trigger("trg_font_zoom_in", tk::SLOT_SUBMIT, slot_font_scaling_zoom_in);
            bind_trigger("trg_font_zoom_out", tk::SLOT_SUBMIT, slot_font_scaling_zoom_out);
            bind_trigger("trg_plugin_manual", tk::SLOT_SUBMIT, slot_show_plugin_manual);

            // Window scaling
            bind_trigger("trg_window_scale", tk::SLOT_MOUSE_DOWN, slot_scale_mouse_down);
            bind_trigger("trg_window_scale", tk::SLOT_MOUSE_UP, slot_scale_mouse_up);
            bind_trigger("trg_window_scale", tk::SLOT_MOUSE_MOVE, slot_scale_mouse_move);
        }

        void PluginWindow::end(ui::UIContext *ctx)
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

            if (pVisualSchema != NULL)
                notify(pVisualSchema, ui::PORT_NONE);
            if (pUIScalingHost != NULL)
                notify(pUIScalingHost, ui::PORT_NONE);
            if (pUIScaling != NULL)
                notify(pUIScaling, ui::PORT_NONE);
            if (pUIFontScaling != NULL)
                notify(pUIFontScaling, ui::PORT_NONE);
            if (pKnobScaleEnable != NULL)
                notify(pKnobScaleEnable, ui::PORT_NONE);
            if (pOverrideHydrogen != NULL)
                notify(pOverrideHydrogen, ui::PORT_NONE);
            if (pInvertVScroll != NULL)
                notify(pInvertVScroll, ui::PORT_NONE);
            if (pInvertGraphDotVScroll != NULL)
                notify(pInvertGraphDotVScroll, ui::PORT_NONE);
            if (pZoomableSpectrum != NULL)
                notify(pZoomableSpectrum, ui::PORT_NONE);
            if (sFilterPointThickness.pPort != NULL)
                notify(sFilterPointThickness.pPort, ui::PORT_NONE);

            // Call for parent class method
            Window::end(ctx);
        }

        void PluginWindow::notify(ui::IPort *port, size_t flags)
        {
            Window::notify(port, flags);

            if (port == pLanguage)
                sync_language_selection();
            if ((port == pUIScaling) || (port == pUIScalingHost))
                sync_ui_scaling();
            if (port == pUIFontScaling)
                sync_font_scaling();
            if (port == pVisualSchema)
                sync_visual_schemas();
            if (port == pKnobScaleEnable)
                sync_knob_scale_enabled();
            if (port == pOverrideHydrogen)
                sync_override_hydrogen();
            if ((port == pInvertVScroll) || (port == pInvertGraphDotVScroll))
                sync_invert_vscroll(port);
            if (port == pZoomableSpectrum)
                sync_zoomable_spectrum();

            sync_enum_menu(&sFilterPointThickness, port);
        }

        status_t PluginWindow::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            return (wContent != NULL) ? wContent->add(child->widget()) : STATUS_BAD_STATE;
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
                _this->widgets()->add(dlg);
                _this->wExport = dlg;

                dlg->init();
                dlg->mode()->set(tk::FDM_SAVE_FILE);
                dlg->title()->set("titles.export_settings");
                dlg->action_text()->set("actions.save");
                dlg->use_confirm()->set(true);
                dlg->confirm_message()->set("messages.file.confirm_overwrite");

                create_config_filters(dlg);

                // Add 'Relative paths' option if present
                tk::Box *wc = new tk::Box(dpy);
                _this->widgets()->add(wc);
                wc->init();
                wc->orientation()->set_vertical();
                wc->allocation()->set_hfill(true);

                if (_this->has_path_ports())
                {
                    tk::Box *op_rpath       = new tk::Box(dpy);
                    _this->widgets()->add(op_rpath);
                    op_rpath->init();
                    op_rpath->orientation()->set_horizontal();
                    op_rpath->spacing()->set(4);

                    // Add switch button
                    tk::CheckBox *ck_rpath  = new tk::CheckBox(dpy);
                    _this->widgets()->add(ck_rpath);
                    ck_rpath->init();
                    ck_rpath->slots()->bind(tk::SLOT_SUBMIT, slot_relative_path_changed, _this);
                    _this->wRelPaths        = ck_rpath;
                    op_rpath->add(ck_rpath);

                    // Add label
                    tk::Label *lbl_rpath     = new tk::Label(dpy);
                    _this->widgets()->add(lbl_rpath);
                    lbl_rpath->init();

                    lbl_rpath->allocation()->set_hexpand(true);
                    lbl_rpath->allocation()->set_hfill(true);
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

            // Initialize and show the window
            if ((_this->wRelPaths != NULL) && (_this->pRelPaths != NULL))
            {
                bool checked = _this->pRelPaths->value() >= 0.5f;
                _this->wRelPaths->checked()->set(checked);
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
                _this->widgets()->add(dlg);
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

        status_t PluginWindow::slot_reset_settings(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            return __this->show_menu(__this->wResetSettings, sender, data);
        }

        status_t PluginWindow::slot_confirm_reset_settings(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            __this->set_preset_button_text("Select preset");
            return __this->pWrapper->reset_settings();
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
            PluginWindow *self = static_cast<PluginWindow *>(ptr);
            self->show_plugin_manual();
            return STATUS_OK;
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

        status_t PluginWindow::slot_show_about(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            if (__this != NULL)
                __this->show_about_window();

            return STATUS_OK;
        }

        status_t PluginWindow::slot_show_presets_window(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            if (__this != NULL)
                __this->show_presets_window();

            return STATUS_OK;
        }

        status_t PluginWindow::slot_debug_dump(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            if (__this->pWrapper != NULL)
                __this->pWrapper->dump_state_request();

            return STATUS_OK;
        }

        status_t PluginWindow::slot_show_main_menu(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            return __this->show_menu(__this->wMenu, sender, data);
        }

        status_t PluginWindow::slot_show_presets_menu(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            return __this->show_menu(__this->wPresets, sender, data);
        }

        status_t PluginWindow::slot_show_ui_scaling_menu(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            return __this->show_menu(__this->wUIScaling, sender, data);
        }

        status_t PluginWindow::slot_show_font_scaling_menu(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            return __this->show_menu(__this->wFontScaling, sender, data);
        }

        status_t PluginWindow::show_menu(tk::Widget *menu, tk::Widget *actor, void *data)
        {
            tk::Menu *xmenu = tk::widget_ptrcast<tk::Menu>(menu);
            if (xmenu == NULL)
                return STATUS_OK;

            if (actor != NULL)
            {
                ws::rectangle_t xr, wr;

                wWidget->get_rectangle(&wr);
                actor->get_rectangle(&xr);

                if (xr.nTop > (wr.nHeight >> 1))
                    xmenu->set_tether(bottom_tether, sizeof(bottom_tether)/sizeof(tk::tether_t));
                else
                    xmenu->set_tether(top_tether, sizeof(top_tether)/sizeof(tk::tether_t));
                xmenu->show(actor);
            }
            else
                xmenu->show();
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
                _this->pWrapper->import_settings(&path, ui::IMPORT_FLAG_NONE);

            return STATUS_OK;
        }


        status_t PluginWindow::slot_greeting_close(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            if (__this->wGreeting != NULL)
                __this->wGreeting->visibility()->set(false);
            return STATUS_OK;
        }

        status_t PluginWindow::slot_about_close(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *__this = static_cast<PluginWindow *>(ptr);
            if (__this->wAbout != NULL)
                __this->wAbout->visibility()->set(false);
            return STATUS_OK;
        }

        status_t PluginWindow::slot_fetch_path(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            tk::FileDialog *dlg = tk::widget_cast<tk::FileDialog>(sender);
            if (dlg == NULL)
                return STATUS_OK;

            // Set-up path
            if (_this->pPath != NULL)
            {
                dlg->path()->set_raw(_this->pPath->buffer<char>());
            }
            // Set-up file type
            if (_this->pFileType != NULL)
            {
                size_t filter = _this->pFileType->value();
                if (filter < dlg->filter()->size())
                    dlg->selected_filter()->set(filter);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::slot_commit_path(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_STATE;

            tk::FileDialog *dlg = tk::widget_cast<tk::FileDialog>(sender);
            if (dlg == NULL)
                return STATUS_OK;

            // Update file path
            if (_this->pPath != NULL)
            {
                LSPString tmp_path;
                if (dlg->path()->format(&tmp_path) == STATUS_OK)
                {
                    const char *path = tmp_path.get_utf8();
                    if (path != NULL)
                    {
                        _this->pPath->write(path, strlen(path));
                        _this->pPath->notify_all(ui::PORT_USER_EDIT);
                    }
                }
            }
            // Update filter
            if (_this->pFileType != NULL)
            {
                _this->pFileType->set_value(dlg->selected_filter()->get());
                _this->pFileType->notify_all(ui::PORT_USER_EDIT);
            }

            return STATUS_OK;
        }

        tk::Label *PluginWindow::create_label(tk::WidgetContainer *dst, const char *key, const char *style_name)
        {
            tk::Label *lbl = new tk::Label(wWidget->display());
            lbl->init();
            widgets()->add(lbl);
            dst->add(lbl);

            lbl->text()->set(key);
            inject_style(lbl, style_name);

            return lbl;
        }

        tk::Label *PluginWindow::create_plabel(tk::WidgetContainer *dst, const char *key, const expr::Parameters *params, const char *style_name)
        {
            tk::Label *lbl = new tk::Label(wWidget->display());
            lbl->init();
            widgets()->add(lbl);
            dst->add(lbl);

            lbl->text()->set(key, params);
            inject_style(lbl, style_name);

            return lbl;
        }

        tk::Hyperlink *PluginWindow::create_hlink(tk::WidgetContainer *dst, const char *url, const char *text, const expr::Parameters *params, const char *style_name)
        {
            tk::Hyperlink *hlink = new tk::Hyperlink(wWidget->display());
            hlink->init();
            widgets()->add(hlink);
            dst->add(hlink);

            hlink->url()->set(url);
            hlink->text()->set(text);
            if (params != NULL)
                hlink->text()->set_params(params);
            inject_style(hlink, style_name);

            return hlink;
        }

        status_t PluginWindow::set_greeting_timer()
        {
            if (pPVersion == NULL)
                return STATUS_OK;

            LSPString pkgver;
            status_t res = fmt_package_version(pkgver);
            if (res != STATUS_OK)
                return res;

            const char *v = pPVersion->buffer<char>();
            if ((v != NULL) && (pkgver.equals_utf8(v)))
                return STATUS_OK;

            // Skip if current version contains "dev"
            if (strstr(pkgver.get_utf8(), "dev") != NULL)
                return STATUS_OK;

            // Set timer to show the greeting window
            wGreetingTimer.set_handler(timer_show_greeting, this);
            wGreetingTimer.bind(pWrapper->display());
            wGreetingTimer.launch(1, 0, 1000);

            return STATUS_OK;
        }

        status_t PluginWindow::fmt_package_version(LSPString &pkgver)
        {
            const meta::package_t *pkg  = pWrapper->package();
            if (pkg == NULL)
                return STATUS_NO_DATA;

            const meta::plugin_t *meta  = pWrapper->ui()->metadata();
            if (meta == NULL)
                return STATUS_NO_DATA;

            pkgver.fmt_ascii("%d.%d.%d",
                int(pkg->version.major),
                int(pkg->version.minor),
                int(pkg->version.micro));
            if (pkg->version.branch)
                pkgver.fmt_append_utf8("-%s", pkg->version.branch);

            return STATUS_OK;
        }

        status_t PluginWindow::timer_show_greeting(ws::timestamp_t sched, ws::timestamp_t time, void *arg)
        {
            if (arg == NULL)
                return STATUS_OK;

            PluginWindow *self = static_cast<PluginWindow *>(arg);
            self->wGreetingTimer.cancel();
            self->show_greeting_window();
            return STATUS_OK;
        }
        status_t PluginWindow::show_presets_window()
        {
            status_t res;
            LSPString key, value;
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            lsp_trace("Showing presets dialog");

            if (wPresetsW == NULL)
            {
                res = create_presets_window();
                if (res != STATUS_OK)
                    return res;
            }

            // Show the widget relative to the actor
            tk::Widget *actor = widgets()->find("trg_presets_menu");
            if (actor != NULL)
            {
                ws::rectangle_t r;
                actor->get_screen_rectangle(&r);

                wPresetsW->set_tether(presets_tether, sizeof(presets_tether)/sizeof(tk::tether_t));
                wPresetsW->trigger_widget()->set(actor);
                wPresetsW->trigger_area()->set(&r);
                wPresetsW->show();
            }
            else
                wPresetsW->show(wnd);

            wPresetsW->grab_events(ws::GRAB_DROPDOWN);

            return STATUS_OK;
        }

        status_t PluginWindow::show_greeting_window()
        {
            status_t res;
            if (pPVersion == NULL)
                return STATUS_BAD_STATE;
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            LSPString pkgver;
            if ((res = fmt_package_version(pkgver)) != STATUS_OK)
                return res;

            // Check that we really need to show notification window
            const char *vstring = pkgver.get_utf8();
        #ifdef LSP_TRACE
            const char *old_pkgver = pPVersion->buffer<char>();
            lsp_trace("Updating last version from %s to %s", old_pkgver, vstring);
        #endif /* LSP_TRACE */

            pPVersion->write(vstring, strlen(vstring));
            pPVersion->notify_all(ui::PORT_NONE);

            lsp_trace("Showing greeting dialog");

            if (wGreeting == NULL)
            {
                ctl::Window *ctl = NULL;
                res = create_dialog_window(&ctl, &wGreeting, LSP_BUILTIN_PREFIX "ui/greeting.xml");
                if (res != STATUS_OK)
                    return res;

                // Bind slots
                tk::Widget *btn = ctl->widgets()->find("submit");
                if (btn != NULL)
                    btn->slots()->bind(tk::SLOT_SUBMIT, slot_greeting_close, this);
                wGreeting->slots()->bind(tk::SLOT_CLOSE, slot_greeting_close, this);
            }

            wGreeting->show(wnd);
            return STATUS_OK;
        }

        void PluginWindow::read_path_param(tk::String *value, const char *port_id)
        {
            ui::IPort *p = pWrapper->port(port_id);
            const char *path = NULL;
            if ((p != NULL) && (meta::is_path_port(p->metadata())))
                path = p->buffer<const char>();
            value->set_raw((path != NULL) ? path : "");
        }

        void PluginWindow::read_bool_param(tk::Boolean *value, const char *port_id)
        {
            ui::IPort *p = pWrapper->port(port_id);
            bool ck = false;
            if (p != NULL)
                ck = p->value() >= 0.5f;
            value->set(ck);
        }

        void PluginWindow::commit_path_param(tk::String *value, const char *port_id)
        {
            ui::IPort *p = pWrapper->port(port_id);
            if ((p != NULL) && (meta::is_path_port(p->metadata())))
            {
                LSPString text;
                value->format(&text);
                const char *path = text.get_utf8();
                if (path != NULL)
                    p->write(path, strlen(path));
                else
                    p->write("", 0);
                p->notify_all(ui::PORT_USER_EDIT);
            }
        }

        void PluginWindow::commit_bool_param(tk::Boolean *value, const char *port_id)
        {
            ui::IPort *p = pWrapper->port(port_id);
            if (p != NULL)
            {
                p->set_value((value->get()) ? 1.0f : 0.0f);
                p->notify_all(ui::PORT_USER_EDIT);
            }
        }

        status_t PluginWindow::show_user_paths_window()
        {
            status_t res;
            tk::Widget *btn;
            tk::Edit *edit;
            tk::CheckBox *check;
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            lsp_trace("Showing user paths dialog");

            if ((wUserPaths == NULL) || (pUserPaths == NULL))
            {
                res = create_dialog_window(&pUserPaths, &wUserPaths, LSP_BUILTIN_PREFIX "ui/user_paths.xml");
                if (res != STATUS_OK)
                    return res;

                // Bind slots
                if ((btn = pUserPaths->widgets()->find("submit")) != NULL)
                    btn->slots()->bind(tk::SLOT_SUBMIT, slot_user_paths_submit, this);
                if ((btn = pUserPaths->widgets()->find("cancel")) != NULL)
                    btn->slots()->bind(tk::SLOT_SUBMIT, slot_user_paths_close, this);
                wUserPaths->slots()->bind(tk::SLOT_CLOSE, slot_user_paths_close, this);
            }

            if ((edit = pUserPaths->widgets()->get<tk::Edit>("user_hydrogen_kit_path")) != NULL)
                read_path_param(edit->text(), UI_USER_HYDROGEN_KIT_PATH_PORT);
            if ((edit = pUserPaths->widgets()->get<tk::Edit>("override_hydrogen_kit_path")) != NULL)
                read_path_param(edit->text(), UI_OVERRIDE_HYDROGEN_KIT_PATH_PORT);
            if ((check = pUserPaths->widgets()->get<tk::CheckBox>("override_hydrogen_kits_check")) != NULL)
                read_bool_param(check->checked(), UI_OVERRIDE_HYDROGEN_KITS_PORT);

            wUserPaths->show(wnd);

            return STATUS_OK;
        }

        void PluginWindow::apply_user_paths_settings()
        {
            tk::Edit *edit;
            tk::CheckBox *check;

            if ((edit = pUserPaths->widgets()->get<tk::Edit>("user_hydrogen_kit_path")) != NULL)
                commit_path_param(edit->text(), UI_USER_HYDROGEN_KIT_PATH_PORT);
            if ((edit = pUserPaths->widgets()->get<tk::Edit>("override_hydrogen_kit_path")) != NULL)
                commit_path_param(edit->text(), UI_OVERRIDE_HYDROGEN_KIT_PATH_PORT);
            if ((check = pUserPaths->widgets()->get<tk::CheckBox>("override_hydrogen_kits_check")) != NULL)
                commit_bool_param(check->checked(), UI_OVERRIDE_HYDROGEN_KITS_PORT);
        }

        status_t PluginWindow::init_context(ui::UIContext *uctx)
        {
            status_t res;
            if ((res = uctx->init()) != STATUS_OK)
                return res;

            const meta::package_t *pkg = pWrapper->package();
            if (pkg != NULL)
                uctx->root()->set_string("package_id", pkg->artifact);

            const meta::plugin_t *plug = pWrapper->metadata();
            if (plug != NULL)
            {
                uctx->root()->set_string("plugin_id", plug->uid);
                const meta::bundle_t *bundle = plug->bundle;
                if (bundle != NULL)
                    uctx->root()->set_string("bundle_id", bundle->uid);
            }

            return res;
        }

        status_t PluginWindow::show_about_window()
        {
            lsp_trace("Showing about dialog");
            status_t res;

            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            if (wAbout == NULL)
            {
                ctl::Window *ctl = NULL;
                res = create_dialog_window(&ctl, &wAbout, LSP_BUILTIN_PREFIX "ui/about.xml");
                if (res != STATUS_OK)
                    return res;

                // Bind slots
                tk::Widget *btn = ctl->widgets()->find("submit");
                if (btn != NULL)
                    btn->slots()->bind(tk::SLOT_SUBMIT, slot_about_close, this);
                wAbout->slots()->bind(tk::SLOT_CLOSE, slot_about_close, this);
            }

            wAbout->show(wnd);
            return STATUS_OK;
        }

        status_t PluginWindow::show_plugin_manual()
        {
            const meta::plugin_t *meta = pWrapper->ui()->metadata();

            io::Path path;
            LSPString spath;
            status_t res;

            // Try to open local documentation
            for (const char **prefix = manual_prefixes; *prefix != NULL; ++prefix)
            {
                path.fmt("%s/doc/%s/html/plugins/%s.html",
                    *prefix, "lsp-plugins", meta->uid);

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

        status_t PluginWindow::create_dialog_window(ctl::Window **ctl, tk::Window **dst, const char *path)
        {
            status_t res;

            // Create window
            tk::Window *w = new tk::Window(wWidget->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            widgets()->add(w);
            w->init();

            // Create controller
            ctl::Window *wc = new ctl::Window(pWrapper, w);
            if (wc == NULL)
                return STATUS_NO_MEM;
            controllers()->add(wc);
            wc->init();

            ui::UIContext uctx(pWrapper, wc->controllers(), wc->widgets());
            if ((res = init_context(&uctx)) != STATUS_OK)
                return res;

            // Parse the XML document
            ui::xml::RootNode root(&uctx, "window", wc);
            ui::xml::Handler handler(pWrapper->resources());
            if ((res = handler.parse_resource(path, &root)) != STATUS_OK)
                return res;

            if (ctl != NULL)
                *ctl    = wc;
            if (dst != NULL)
                *dst    = w;

            return STATUS_OK;
        }

        status_t PluginWindow::create_presets_window()
        {
            status_t res;

            PluginWindow *self = this;

            // Create window
            tk::PopupWindow *w = new tk::PopupWindow(wWidget->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            widgets()->add(w);
            w->init();
            w->auto_close()->set(true);

            // Create controller
            ctl::PresetsWindow *wc = new ctl::PresetsWindow(pWrapper, w, self);
            if (wc == NULL)
                return STATUS_NO_MEM;
            controllers()->add(wc);
            wc->init();

            ui::UIContext uctx(pWrapper, wc->controllers(), wc->widgets());
            if ((res = init_context(&uctx)) != STATUS_OK)
                return res;

            // Parse the XML document
            ui::xml::RootNode root(&uctx, "window", wc);
            ui::xml::Handler handler(pWrapper->resources());
            if ((res = handler.parse_resource(LSP_BUILTIN_PREFIX "ui/presets.xml", &root)) != STATUS_OK)
                return res;

            if ((res = wc->post_init()) != STATUS_OK)
                return res;

            wPresetsW = w;
            pPresetsWnd = wc;

            return STATUS_OK;
        }

        status_t PluginWindow::slot_scaling_toggle_prefer_host(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            const bool prefer   = (_this->pUIScalingHost->value() >= 0.5f) ? 0.0f : 1.0f;
            _this->pUIScalingHost->set_value((prefer) ? 1.0f : 0.0f);

            if (prefer)
            {
                ssize_t value       = _this->pUIScaling->value();
                ssize_t new_value   = _this->pWrapper->ui_scaling_factor(value);

                _this->pUIScaling->set_value(new_value);
                _this->pUIScaling->notify_all(ui::PORT_USER_EDIT);
            }
            _this->pUIScalingHost->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if ((_this == NULL) || (_this->pUIScaling == NULL))
                return STATUS_OK;

            ssize_t value       = _this->pUIScaling->value();
            ssize_t new_value   = ((value / SCALING_FACTOR_STEP) + 1) * SCALING_FACTOR_STEP;
            value               = lsp_limit(new_value , SCALING_FACTOR_BEGIN, SCALING_FACTOR_END);

            _this->pUIScalingHost->set_value(0.0f);
            _this->pUIScaling->set_value(value);

            _this->pUIScalingHost->notify_all(ui::PORT_USER_EDIT);
            _this->pUIScaling->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if ((_this == NULL) || (_this->pUIScaling == NULL))
                return STATUS_OK;

            ssize_t value   = _this->pUIScaling->value();
            ssize_t new_value   = ((value / SCALING_FACTOR_STEP) - 1) * SCALING_FACTOR_STEP;
            value           = lsp_limit(new_value, SCALING_FACTOR_BEGIN, SCALING_FACTOR_END);

            _this->pUIScalingHost->set_value(0.0f);
            _this->pUIScaling->set_value(value);

            _this->pUIScalingHost->notify_all(ui::PORT_USER_EDIT);
            _this->pUIScaling->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_scaling_select(tk::Widget *sender, void *ptr, void *data)
        {
            scaling_sel_t *sel  = static_cast<scaling_sel_t *>(ptr);
            if (sel == NULL)
                return STATUS_OK;

            PluginWindow *_this = sel->ctl;
            if ((_this != NULL) && (_this->pUIScaling != NULL))
            {
                _this->pUIScalingHost->set_value(0.0f);
                _this->pUIScaling->set_value(sel->scaling);

                _this->pUIScalingHost->notify_all(ui::PORT_USER_EDIT);
                _this->pUIScaling->notify_all(ui::PORT_USER_EDIT);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::slot_font_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if ((_this == NULL) || (_this->pUIFontScaling == NULL))
                return STATUS_OK;

            ssize_t value   = _this->pUIFontScaling->value();
            value           = lsp_limit(value + FONT_SCALING_FACTOR_STEP, FONT_SCALING_FACTOR_BEGIN, FONT_SCALING_FACTOR_END);

            _this->pUIFontScaling->set_value(value);
            _this->pUIFontScaling->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_font_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if ((_this == NULL) || (_this->pUIFontScaling == NULL))
                return STATUS_OK;

            ssize_t value   = _this->pUIFontScaling->value();
            value           = lsp_limit(value - FONT_SCALING_FACTOR_STEP, FONT_SCALING_FACTOR_BEGIN, FONT_SCALING_FACTOR_END);

            _this->pUIFontScaling->set_value(value);
            _this->pUIFontScaling->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_font_scaling_select(tk::Widget *sender, void *ptr, void *data)
        {
            scaling_sel_t *sel  = static_cast<scaling_sel_t *>(ptr);
            if (sel == NULL)
                return STATUS_OK;

            PluginWindow *_this = sel->ctl;
            if ((_this != NULL) && (_this->pUIFontScaling != NULL))
            {
                _this->pUIFontScaling->set_value(sel->scaling);
                _this->pUIFontScaling->notify_all(ui::PORT_USER_EDIT);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::slot_visual_schema_select(tk::Widget *sender, void *ptr, void *data)
        {
            schema_sel_t *sel  = static_cast<schema_sel_t *>(ptr);
            if (sel == NULL)
                return STATUS_OK;

            PluginWindow *_this = sel->ctl;
            if (_this == NULL)
                return STATUS_OK;

            // Try to load schema first
            if (_this->pWrapper->load_visual_schema(&sel->location) == STATUS_OK)
            {
                const char *value = sel->location.get_utf8();

                if (_this->pVisualSchema != NULL)
                {
                    _this->pVisualSchema->write(value, strlen(value));
                    _this->pVisualSchema->notify_all(ui::PORT_USER_EDIT);
                }

                // Notify other parameters
                if (_this->pUIFontScaling != NULL)
                    _this->pUIFontScaling->notify_all(ui::PORT_USER_EDIT);
                if (_this->pUIScaling != NULL)
                    _this->pUIScaling->notify_all(ui::PORT_USER_EDIT);
                if (_this->pLanguage != NULL)
                    _this->pLanguage->notify_all(ui::PORT_USER_EDIT);
                if (_this->pKnobScaleEnable != NULL)
                    _this->pKnobScaleEnable->notify_all(ui::PORT_USER_EDIT);
                if (_this->pInvertVScroll != NULL)
                    _this->pInvertVScroll->notify_all(ui::PORT_USER_EDIT);
                if (_this->pInvertGraphDotVScroll != NULL)
                    _this->pInvertGraphDotVScroll->notify_all(ui::PORT_USER_EDIT);
                if (_this->pZoomableSpectrum != NULL)
                    _this->pZoomableSpectrum->notify_all(ui::PORT_USER_EDIT);
                if (_this->sFilterPointThickness.pPort != NULL)
                    _this->sFilterPointThickness.pPort->notify_all(ui::PORT_USER_EDIT);
            }

            return STATUS_OK;
        }

        status_t PluginWindow::slot_window_resize(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            const ws::rectangle_t *r = static_cast<const ws::rectangle_t *>(data);
            if (r == NULL)
                return STATUS_OK;

//            lsp_trace("Resize: x=%d, y=%d, w=%d, h=%d", int(r->nLeft), int(r->nTop), int(r->nWidth), int(r->nHeight));

            tk::Window *wnd = tk::widget_cast<tk::Window>(_this->wWidget);
            if ((wnd == NULL) || (wnd->has_parent()))
                return STATUS_OK;

            ws::rectangle_t wp = *r;
            ssize_t sw = 0, sh = 0;
            wnd->display()->screen_size(wnd->screen(), &sw, &sh);

            if (wp.nLeft >= sw)
                wp.nLeft    = sw - r->nWidth;
            if (wp.nTop >= sh)
                wp.nTop     = sh - r->nHeight;
            if ((wp.nLeft + wp.nWidth) < 0)
                wp.nLeft    = 0;
            if ((wp.nTop + wp.nHeight) < 0)
                wp.nTop     = 0;

            wnd->position()->set(wp.nLeft, wp.nTop);

            return STATUS_OK;
        }

        status_t PluginWindow::locate_window()
        {
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if ((wnd == NULL) || (wnd->has_parent()))
                return STATUS_OK;

            ws::rectangle_t r;

            // Estimate the real window size if it is still not known
            {
                ws::size_limit_t sr;
                wnd->get_padded_screen_rectangle(&r);
                wnd->get_padded_size_limits(&sr);
                if ((sr.nMinWidth >= 0) && (r.nWidth < sr.nMinWidth))
                    r.nWidth = sr.nMinWidth;
                if ((sr.nMinHeight >= 0) && (r.nHeight < sr.nMinHeight))
                    r.nHeight = sr.nMinHeight;
            }

            // Find the matching monitor (if it is present) and align window to the center
            bool aligned = false;
            {
                size_t num_monitors = 0;
                const ws::MonitorInfo * x = wnd->display()->enum_monitors(&num_monitors);
                if (x != NULL)
                {
                    for (size_t i=0; i<num_monitors; ++i)
                    {
                        if (tk::Position::inside(&x->rect, r.nLeft, r.nHeight))
                        {
                            r.nLeft = (x->rect.nWidth  - r.nWidth)  >> 1;
                            r.nTop  = (x->rect.nHeight - r.nHeight) >> 1;
                            aligned = true;
                            break;
                        }
                    }
                }
            }
            if (!aligned)
            {
                ssize_t sw = 0, sh = 0;
                wnd->display()->screen_size(wnd->screen(), &sw, &sh);

                r.nLeft = (sw - r.nWidth)  >> 1;
                r.nTop  = (sh - r.nHeight) >> 1;
            }

            // Set the actual position of the window
            wnd->position()->set(r.nLeft, r.nTop);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_scale_mouse_down(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            ws::event_t *ev = static_cast<ws::event_t *>(data);
            if (ev == NULL)
                return STATUS_OK;

            size_t old = _this->sWndScale.nMFlags;
            _this->sWndScale.nMFlags   |= 1 << ev->nCode;
            if (old == 0)
            {
                _this->sWndScale.bActive    = ev->nCode == ws::MCB_LEFT;

                if (_this->sWndScale.bActive)
                {
                    _this->wWidget->get_padded_screen_rectangle(&_this->sWndScale.sSize);
                    _this->sWndScale.nMouseX    = ev->nLeft;
                    _this->sWndScale.nMouseY    = ev->nTop;
                }
            }

            return STATUS_OK;
        }

        status_t PluginWindow::slot_scale_mouse_move(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            ws::event_t *ev = static_cast<ws::event_t *>(data);
            if (ev == NULL)
                return STATUS_OK;

            if (_this->sWndScale.bActive)
            {
                tk::Window *wnd = tk::widget_cast<tk::Window>(_this->wWidget);
                if (wnd == NULL)
                    return STATUS_OK;

                ws::size_limit_t sr;
                ws::rectangle_t xr  = _this->sWndScale.sSize;
                ssize_t width       = xr.nWidth  + ev->nLeft - _this->sWndScale.nMouseX;
                ssize_t height      = xr.nHeight + ev->nTop  - _this->sWndScale.nMouseY;

                _this->wWidget->get_padded_size_limits(&sr);
                tk::SizeConstraints::apply(&xr, &sr);

                if ((width != xr.nWidth) || (height != xr.nHeight))
                {
                    if (_this->pWrapper->accept_window_size(wnd, width, height))
                        wnd->resize_window(width, height);
                }
            }

            return STATUS_OK;
        }

        status_t PluginWindow::slot_scale_mouse_up(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            ws::event_t *ev = static_cast<ws::event_t *>(data);
            if (ev == NULL)
                return STATUS_OK;

            _this->sWndScale.nMFlags   &= ~(size_t(1) << ev->nCode);
            if (_this->sWndScale.nMFlags == 0)
                _this->sWndScale.bActive    = false;

            return STATUS_OK;
        }

        status_t PluginWindow::slot_relative_path_changed(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;
            if (_this->pRelPaths == NULL)
                return STATUS_OK;

            tk::CheckBox *ck_box = tk::widget_cast<tk::CheckBox>(sender);
            if (ck_box == NULL)
                return STATUS_OK;

            float value = ck_box->checked()->get() ? 1.0f : 0.0f;
            _this->pRelPaths->set_value(value);
            _this->pRelPaths->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_enable_slot_scale_changed(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;
            if (_this->pKnobScaleEnable == NULL)
                return STATUS_OK;
            if (_this->wKnobScaleEnable == NULL)
                return STATUS_OK;

            _this->wKnobScaleEnable->checked()->toggle();
            bool checked = _this->wKnobScaleEnable->checked()->get();
            _this->pKnobScaleEnable->set_value((checked) ? 1.0f : 0.0f);
            _this->pKnobScaleEnable->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_override_hydrogen_kits_changed(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;
            if (_this->pOverrideHydrogen == NULL)
                return STATUS_OK;
            if (_this->wOverrideHydrogen == NULL)
                return STATUS_OK;

            _this->wOverrideHydrogen->checked()->toggle();
            bool checked = _this->wOverrideHydrogen->checked()->get();
            _this->pOverrideHydrogen->set_value((checked) ? 1.0f : 0.0f);
            _this->pOverrideHydrogen->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_invert_vscroll_changed(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;
            if (_this->pInvertVScroll == NULL)
                return STATUS_OK;
            if (_this->wInvertVScroll == NULL)
                return STATUS_OK;

            _this->wInvertVScroll->checked()->toggle();
            bool checked = _this->wInvertVScroll->checked()->get();
            _this->pInvertVScroll->set_value((checked) ? 1.0f : 0.0f);
            _this->pInvertVScroll->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_invert_graph_dot_vscroll_changed(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;
            if (_this->pInvertGraphDotVScroll == NULL)
                return STATUS_OK;
            if (_this->wInvertGraphDotVScroll == NULL)
                return STATUS_OK;

            _this->wInvertGraphDotVScroll->checked()->toggle();
            bool checked = _this->wInvertGraphDotVScroll->checked()->get();
            _this->pInvertGraphDotVScroll->set_value((checked) ? 1.0f : 0.0f);
            _this->pInvertGraphDotVScroll->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_zoomable_spectrum_changed(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;
            if (_this->pZoomableSpectrum == NULL)
                return STATUS_OK;
            if (_this->wZoomableSpectrum == NULL)
                return STATUS_OK;

            _this->wZoomableSpectrum->checked()->toggle();
            bool checked = _this->wZoomableSpectrum->checked()->get();
            _this->pZoomableSpectrum->set_value((checked) ? 1.0f : 0.0f);
            _this->pZoomableSpectrum->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_submit_enum_menu_item(tk::Widget *sender, void *ptr, void *data)
        {
            enum_menu_t *em = static_cast<enum_menu_t *>(ptr);
            if ((em == NULL) || (em->pPort == NULL))
                return STATUS_OK;
            tk::MenuItem *mi = tk::widget_cast<tk::MenuItem>(sender);
            if (mi == NULL)
                return STATUS_OK;
            ssize_t index = em->vItems.index_of(mi);
            if (index < 0)
                return STATUS_OK;
            const meta::port_t *meta = em->pPort->metadata();
            if (meta == NULL)
                return STATUS_OK;

            float value = meta->min + index;
            lsp_trace("id = %s, index = %d, value=%f", meta->id, int(index), value);

            em->pPort->set_value(value);
            em->pPort->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PluginWindow::slot_show_user_paths_dialog(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            return _this->show_user_paths_window();
        }

        status_t PluginWindow::slot_user_paths_submit(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            _this->wUserPaths->visibility()->set(false);
            _this->apply_user_paths_settings();

            return STATUS_OK;
        }

        status_t PluginWindow::slot_user_paths_close(tk::Widget *sender, void *ptr, void *data)
        {
            PluginWindow *_this = static_cast<PluginWindow *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            _this->wUserPaths->visibility()->set(false);

            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


