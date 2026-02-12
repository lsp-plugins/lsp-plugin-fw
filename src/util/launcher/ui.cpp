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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/plug-fw/meta/registry.h>
#include <lsp-plug.in/plug-fw/ui/const.h>
#include <lsp-plug.in/plug-fw/util/launcher/ui.h>
#include <lsp-plug.in/plug-fw/util/launcher/window.h>
#include <lsp-plug.in/plug-fw/ctl.h>

#include <private/ui/xml/Handler.h>
#include <private/ui/xml/RootNode.h>

// Launcher-specific ports
#define UI_LAUNCHER_PREFIX                      "_launcher_"
#define UI_LAUNCHER_WIDTH_ID                    UI_LAUNCHER_PREFIX "width"
#define UI_LAUNCHER_HEIGHT_ID                   UI_LAUNCHER_PREFIX "height"

namespace lsp
{
    namespace meta
    {
        static const meta::port_t launcher_config_metadata[] =
        {
            INT_CONTROL_ALL(UI_LAUNCHER_WIDTH_ID, "Plugin launcher window width", NULL, U_NONE, 0, 65536, 400, 1),
            INT_CONTROL_ALL(UI_LAUNCHER_HEIGHT_ID, "Plugin launcher window height", NULL, U_NONE, 0, 65536, 648, 1),
            PORTS_END
        };
    } /* namespace meta */

    namespace launcher
    {
        static const uint8_t category_ordering[] =
        {
            meta::B_EQUALIZERS,
            meta::B_DYNAMICS,
            meta::B_MB_DYNAMICS,
            meta::B_CONVOLUTION,
            meta::B_DELAYS,
            meta::B_ANALYZERS,
            meta::B_MB_PROCESSING,
            meta::B_SAMPLERS,
            meta::B_EFFECTS,
            meta::B_GENERATORS,
            meta::B_UTILITIES,
        };

        static const char * const category_keys[] =
        {
            "analyzers",
            "convolution",
            "delays",
            "dynamics",
            "effects",
            "equalizers",
            "generators",
            "mb_dynamics",
            "mb_processing",
            "samplers",
            "utilities"
        };

        static ssize_t category_weight(meta::bundle_group_t g)
        {
            size_t i=0;
            for (; i < sizeof(category_ordering) / sizeof(category_ordering[0]); ++i)
            {
                const uint8_t weight = category_ordering[i];
                if (weight == g)
                    return i;
            }
            return i;
        }

        UI::UI(resource::ILoader * loader, const meta::package_t *package, const meta::plugin_t **launch):
            ui::IWrapper(NULL, loader),
            sUIScaling(this),
            sFontScaling(this),
            sDocumentation(this)
        {
            pPackage            = package;
            pLaunch             = launch;

            launcher::init_config(sConfig);

            pWindowWidth        = NULL;
            pWindowHeight       = NULL;
            pLanguage           = NULL;
            pVisualSchema       = NULL;
            pAboutWindow        = NULL;
            nConfigChanges      = 0;

            wFilter             = NULL;
            wTabs               = NULL;
            wLanguageArea       = NULL;
            wLanguage           = NULL;
            wSchemaArea         = NULL;
            wVisualSchema       = NULL;
            wAllBundles         = NULL;
            wFavourites         = NULL;
        }

        UI::~UI()
        {
            do_destroy();
        }

        void UI::do_destroy()
        {
            sUIScaling.destroy();
            sFontScaling.destroy();
            sDocumentation.destroy();

            pPackage = NULL;

            // Destroy metadata if present
            for (lltl::iterator<category_t> it = vCategories.values(); it; ++it)
            {
                category_t * const c = it.get();
                if (c == NULL)
                    continue;
                delete c;
            }
            vCategories.flush();

            for (lltl::iterator<bundle_t> it = vBundles.values(); it; ++it)
            {
                bundle_t * const b = it.get();
                if (b == NULL)
                    continue;
                delete b;
            }
            vBundles.flush();

            for (lltl::iterator<plugin_t> it = vPlugins.values(); it; ++it)
            {
                plugin_t * const p = it.get();
                if (p == NULL)
                    continue;
                delete p;
            }
            vPlugins.flush();

            vLangSel.flush();
            vSchemaSel.flush();

            // Destroy config
            launcher::free_config(sConfig);
        }

        void UI::destroy()
        {
            do_destroy();
            ui::IWrapper::destroy();
        }

        ssize_t UI::plugin_cmp_function(const plugin_t *a, const plugin_t *b)
        {
            ssize_t delta;
            const meta::plugin_t *pa = a->pMeta;
            const meta::plugin_t *pb = b->pMeta;

            if ((delta = (category_weight(pa->bundle->group) - category_weight(pb->bundle->group))) != 0)
                return delta;
            if ((delta = strcmp(pa->bundle->name, pb->bundle->name)) != 0)
                return delta;
            if ((delta = (pa->bundle - pb->bundle)) != 0)
                return delta;
            return strcmp(pa->uid, pb->uid);
        }

        status_t UI::scan_metadata()
        {
            // Map plugins
            for (const meta::Registry * r = meta::Registry::root(); r != NULL; r = r->next())
            {
                const meta::plugin_t * meta = r->plugin();
                if ((meta == NULL) || (meta->bundle == NULL))
                    continue;

                plugin_t * const p = new plugin_t;
                if (p == NULL)
                    return STATUS_NO_MEM;

                p->pMeta        = meta;             // Plugin
                p->pBundle      = NULL;
                p->wImage       = NULL;
                p->wButton      = NULL;

                if (!vPlugins.add(p))
                {
                    delete p;
                    return STATUS_NO_MEM;
                }
            }
            vPlugins.qsort(plugin_cmp_function);

            // Map bundles and categories
            category_t *c = NULL;
            bundle_t *b = NULL;
            for (lltl::iterator<plugin_t> it = vPlugins.values(); it; ++it)
            {
                plugin_t * const p = it.get();
                if (p == NULL)
                    return STATUS_CORRUPTED;

                const meta::plugin_t * const mp = p->pMeta;
                if (mp == NULL)
                    return STATUS_CORRUPTED;

                const meta::bundle_t * const mb = mp->bundle;
                if (mb == NULL)
                    return STATUS_CORRUPTED;

                // Map category
                if ((c == NULL) || (c->enCategory != mb->group))
                {
                    if (mb->group >= sizeof(category_keys) / sizeof(category_keys[0]))
                    {
                        lsp_error("Unsupported category identifier id=%d", int(c->enCategory));
                        return STATUS_CORRUPTED;
                    }

                    c = new category_t;
                    if (c == NULL)
                        return STATUS_NO_MEM;

                    c->wRoot        = NULL;
                    c->wHeading     = NULL;
                    c->enCategory   = mb->group;
                    c->sUID         = category_keys[c->enCategory];
                    c->nVisibility  = 0;

                    lsp_trace("Added category id=%d uid=%s", int(mb->group), c->sUID);

                    if (!vCategories.add(c))
                    {
                        delete c;
                        return STATUS_NO_MEM;
                    }
                }

                // Map bundle
                if ((b == NULL) || (b->pMeta != mp->bundle))
                {
                    b = new bundle_t;
                    if (b == NULL)
                        return STATUS_NO_MEM;

                    b->pMeta        = mp->bundle;
                    b->pCategory    = c;
                    b->wRoot        = NULL;
                    b->wBundle      = NULL;
                    b->wHeading     = NULL;
                    b->wDescription = NULL;
                    b->wImages      = NULL;
                    b->wButtons     = NULL;
                    b->wFavouries   = NULL;
                    b->wHelp        = NULL;
                    b->nVisibility  = 0;
                    b->nActivePlugin= 0;

                    lsp_trace("Added bundle uid=%s, name=%s", mb->uid, mb->name);

                    if (!vBundles.add(b))
                    {
                        delete b;
                        return STATUS_NO_MEM;
                    }
                    if (!c->vBundles.add(b))
                        return STATUS_NO_MEM;
                }

                // Map plugin
                if (!b->vPlugins.add(p))
                    return STATUS_NO_MEM;

                p->pBundle      = b;
            }

            return STATUS_OK;
        }

        status_t UI::build_ui()
        {
            status_t res;

            wWindow     = new tk::Window(pDisplay);
            if (wWindow == NULL)
                return STATUS_NO_MEM;
            if ((res = wWindow->init()) != STATUS_OK)
                return res;

            // Create window controller
            launcher::Window *wnd  = new launcher::Window(this, wWindow);
            if (wnd == NULL)
                return STATUS_NO_MEM;
            if ((res = wnd->init()) != STATUS_OK)
                return res;

            // Form the location of the resource
            LSPString xpath;
            if (xpath.fmt_utf8(LSP_BUILTIN_PREFIX "ui/launcher.xml") <= 0)
                return STATUS_NO_MEM;

            // Create context
            ui::UIContext ctx(this, wnd->controllers(), wnd->widgets());
            if ((res = ctx.init()) != STATUS_OK)
                return res;

            // Parse the XML document
            ui::xml::RootNode root(&ctx, "window", wnd);
            ui::xml::Handler handler(resources());
            if ((res = handler.parse_resource(&xpath, &root)) != STATUS_OK)
                return res;

            // Append overlays to the window
            lltl::parray<ctl::Overlay> *overlays = ctx.overlays();
            for (size_t i=0, n=overlays->size(); i<n; ++i)
            {
                ctl::Overlay *ov = overlays->uget(i);
                if (ov == NULL)
                    continue;

                if ((res = wnd->add(&ctx, ov)) != STATUS_OK)
                    return res;
            }

            pWindow     = wnd;

            // Call post-initialization
            if ((res = wnd->post_init()) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        status_t UI::init(void *root_widget)
        {
            status_t res;

        #ifdef LSP_TRACE
            lsp_trace("Begin UI initialization");
            const system::time_millis_t start = system::get_time_millis();
            lsp_finally {
                const system::time_millis_t end = system::get_time_millis();
                lsp_trace("UI initialization time: %d ms", int(end - start));
            };
        #endif /* LSP_TRACE */

            // Load configuration file
            if ((res = launcher::load_config(sConfig)) != STATUS_OK)
            {
                if (res != STATUS_NOT_FOUND)
                    lsp_warn("Error loading launcher config, error code: %d", int(res));
                else
                    ++nConfigChanges;
            }

            if ((res = create_launcher_ports()) != STATUS_OK)
                return res;

            // Scan plugin metadata
            if ((res = scan_metadata()) != STATUS_OK)
                return res;

            // Initialize parent
            if ((res = IWrapper::init(root_widget)) != STATUS_OK)
                return res;

            // Create window widget
            if ((res = build_ui()) != STATUS_OK)
            {
                lsp_error("Failed to build launcher window UI");
                return STATUS_BAD_STATE;
            }

            // Call post-initialization
            if ((res = post_init()) != STATUS_OK)
                return res;

            // Show the window
            window()->show();

            return res;
        }

        status_t UI::post_init()
        {
            tk::Window * const root    = window();
            if (root == NULL)
            {
                lsp_error("No root window present!");
                return STATUS_BAD_STATE;
            }

            // Init UI scaling
            LSP_STATUS_ASSERT(sUIScaling.init(false));
            sUIScaling.bind_ui_scaling_show("trg_ui_scaling", tk::SLOT_SUBMIT);
            sUIScaling.bind_ui_scaling_zoom_in("trg_ui_zoom_in", tk::SLOT_SUBMIT);
            sUIScaling.bind_ui_scaling_zoom_out("trg_ui_zoom_out", tk::SLOT_SUBMIT);

            // Init Font scaling
            LSP_STATUS_ASSERT(sFontScaling.init());
            sFontScaling.bind_show("trg_font_scaling", tk::SLOT_SUBMIT);
            sFontScaling.bind_zoom_in("trg_font_zoom_in", tk::SLOT_SUBMIT);
            sFontScaling.bind_zoom_out("trg_font_zoom_out", tk::SLOT_SUBMIT);

            // Bind widgets
            tk::Registry * const registry = controller()->widgets();
            wFilter = registry->get<tk::Edit>("search_input");
            wTabs = registry->get<tk::TabControl>("tab_control");
            wAllBundles = registry->get<tk::WidgetContainer>("all_plugins_list");
            wFavourites = registry->get<tk::WidgetContainer>("favourites_list");

            // Bind ports
            BIND_PORT(this, pWindowWidth, UI_LAUNCHER_WIDTH_ID);
            BIND_PORT(this, pWindowHeight, UI_LAUNCHER_HEIGHT_ID);
            BIND_PORT(this, pLanguage, LANGUAGE_PORT);
            BIND_PORT(this, pVisualSchema, UI_VISUAL_SCHEMA_PORT);

            // Bind events to the display
            pDisplay->slots()->bind(tk::SLOT_IDLE, slot_display_idle, this);
            pDisplay->set_idle_interval(1000 / UI_FRAMES_PER_SECOND);

            // Bind events to the root window
            root->slots()->bind(tk::SLOT_CLOSE, slot_window_close, this);
            root->slots()->bind(tk::SLOT_RESIZE, slot_window_resize, this);
            root->slots()->bind(tk::SLOT_SHOW, slot_window_show, this);
            if (wFilter != NULL)
                wFilter->slots()->bind(tk::SLOT_CHANGE, slot_filter_change, this);
            if (wTabs != NULL)
                wTabs->slots()->bind(tk::SLOT_SUBMIT, slot_change_tab, this);

            bind_trigger("trg_about", tk::SLOT_SUBMIT, slot_show_about);

            // Deploy configuration values to ports
            deploy_config();

            // Init language support
            LSP_STATUS_ASSERT(init_i18n_support());
            sync_language_selection();

            // Init visual schema support
            LSP_STATUS_ASSERT(init_visual_schema_support());
            sync_visual_schema_selection();

            // Create plugin catalog
            LSP_STATUS_ASSERT(create_catalog());
            sync_widget_visibility();

            // Initialize manuals
            LSP_STATUS_ASSERT(sDocumentation.init());
            bind_trigger("trg_show_manual", tk::SLOT_SUBMIT, slot_show_ui_manual);

            // Synchronize state
            sUIScaling.sync_parameters();
            sFontScaling.sync_parameters();

            return STATUS_OK;
        }

        void UI::bind_trigger(const char *uid, tk::slot_t ev, tk::event_handler_t handler)
        {
            tk::Widget *w = controller()->widgets()->find(uid);
            if (w == NULL)
                return;
            w->slots()->bind(ev, handler, this);
        }

        template <typename T>
        T *UI::create_widget(const char *style)
        {
            T * const widget = new T(pDisplay);
            if (widget == NULL)
                return NULL;
            if (controller()->widgets()->add(widget) != STATUS_OK)
            {
                delete widget;
                return NULL;
            }
            if (widget->init() != STATUS_OK)
                return NULL;
            if (style != NULL)
            {
                tk::Style * const ws = pDisplay->schema()->get(style);
                if (ws != NULL)
                    widget->style()->add_parent(ws);
                else
                    lsp_warn("Could not inject non-existing style '%s'", style);
            }

            return widget;
        }

        status_t UI::create_launcher_ports()
        {
            // Create additional ports (launcher configuration)
            for (const meta::port_t *p = meta::launcher_config_metadata; p->id != NULL; ++p)
            {
                switch (p->role)
                {
                    case meta::R_CONTROL:
                    {
                        ui::IPort *up = new ui::ControlPort(p, this);
                        if (up != NULL)
                        {
                            lsp_trace("Created launcher configuration cotrol port id=%s", up->id());
                            bind_custom_port(up);
                        }
                        break;
                    }

                    case meta::R_PATH:
                    {
                        ui::IPort *up = new ui::PathPort(p, this);
                        if (up != NULL)
                        {
                            lsp_trace("Created launcher configuration path port id=%s", up->id());
                            bind_custom_port(up);
                        }
                        break;
                    }

                    default:
                        lsp_error("Could not instantiate configuration port id=%s", p->id);
                        return STATUS_UNKNOWN_ERR;
                }
            }

            return STATUS_OK;
        }

        void UI::deploy_config()
        {
            // Update configuration values
            if (pWindowWidth != NULL)
                pWindowWidth->set_value(sConfig.nWidth);
            if (pWindowHeight != NULL)
                pWindowHeight->set_value(sConfig.nHeight);
            if (pWindowWidth != NULL)
                pWindowWidth->notify_all(ui::PORT_NONE);
            if (pWindowHeight != NULL)
                pWindowHeight->notify_all(ui::PORT_NONE);
        }

        status_t UI::create_catalog()
        {
            LSPString tmp;
            LSPString path;

            // Iterate over categories
            for (lltl::iterator<category_t> ci = vCategories.values(); ci; ++ci)
            {
                category_t * const c = ci.get();

                // Add category heading widget
                tmp.fmt_ascii("bundles.groups.%s", c->sUID);
                if ((c->wHeading = create_widget<tk::Label>("LauncherWindow::Category::Heading")) == NULL)
                    return STATUS_NO_MEM;
                LSP_STATUS_ASSERT(c->wHeading->text()->set(&tmp));

                // Set root widget as heading
                c->wRoot            = c->wHeading;
            }

            // Iterate over bundles
            for (lltl::iterator<bundle_t> bi = vBundles.values(); bi; ++bi)
            {
                bundle_t * const b = bi.get();

                // Add category heading widget
                if ((b->wBundle = create_widget<tk::Grid>("LauncherWindow::Bundle::Grid")) == NULL)
                    return STATUS_NO_MEM;

                b->wBundle->rows()->set(3);
                b->wBundle->columns()->set(3);

                // Add box with plugin images
                {
                    tk::Align * const align = create_widget<tk::Align>("LauncherWindow::Bundle::Images::Align");
                    if (align == NULL)
                        return STATUS_NO_MEM;
                    LSP_STATUS_ASSERT(b->wBundle->add(align, 3, 1));

                    if ((b->wImages = create_widget<tk::Box>("LauncherWindow::Bundle::Images")) == NULL)
                        return STATUS_NO_MEM;
                    LSP_STATUS_ASSERT(align->add(b->wImages));
                }

                // Add plugin heading
                tmp.fmt_ascii("bundles.%s.name", b->pMeta->uid);
                if ((b->wHeading = create_widget<tk::Label>("LauncherWindow::Bundle::Heading")) == NULL)
                    return STATUS_NO_MEM;
                LSP_STATUS_ASSERT(b->wHeading->text()->set(&tmp));
                LSP_STATUS_ASSERT(b->wBundle->add(b->wHeading));

                // Add favourites and help buttons
                {
                    tk::Box * const btns = create_widget<tk::Box>(NULL);
                    if (btns == NULL)
                        return STATUS_NO_MEM;
                    btns->allocation()->set_hreduce(true);
                    LSP_STATUS_ASSERT(b->wBundle->add(btns));

                    if ((b->wFavouries = create_widget<tk::Button>("LauncherWindow::Bundle::Favourites")) == NULL)
                        return STATUS_NO_MEM;
                    LSP_STATUS_ASSERT(btns->add(b->wFavouries));
                    b->wFavouries->slots()->bind(tk::SLOT_SUBMIT, slot_toggle_favourites, this);

                    if ((b->wHelp = create_widget<tk::Button>("LauncherWindow::Bundle::Help")) == NULL)
                        return STATUS_NO_MEM;
                    b->wHelp->slots()->bind(tk::SLOT_SUBMIT, slot_show_bundle_manual, this);
                    LSP_STATUS_ASSERT(b->wHelp->text()->set("icons.main.help_nb"));
                    LSP_STATUS_ASSERT(btns->add(b->wHelp));
                }

                // Add bundle description
                tmp.fmt_ascii("bundles.%s.description", b->pMeta->uid);
                if ((b->wDescription = create_widget<tk::Label>("LauncherWindow::Bundle::Description")) == NULL)
                    return STATUS_NO_MEM;
                b->wDescription->allocation()->set_expand(true);
                LSP_STATUS_ASSERT(b->wDescription->text()->set(&tmp));
                LSP_STATUS_ASSERT(b->wBundle->add(b->wDescription, 1, 2));

                // Add launch button grid
                if ((b->wButtons = create_widget<tk::Grid>("LauncherWindow::Bundle::Buttons")) == NULL)
                    return STATUS_NO_MEM;
                LSP_STATUS_ASSERT(b->wBundle->add(b->wButtons, 1, 2));
                b->wButtons->rows()->set((b->vPlugins.size() + 4 - 1) / 4);
                b->wButtons->columns()->set(4);

                // Set up root widget
                b->wRoot            = b->wBundle;
            }

            // Iterate over plugins
            for (lltl::iterator<plugin_t> pi = vPlugins.values(); pi; ++pi)
            {
                plugin_t * const p = pi.get();
                if (p->pBundle == NULL)
                    return STATUS_CORRUPTED;

                // Add image
                if ((p->wImage = create_widget<tk::Image>("LauncherWindow::Plugin::Image")) == NULL)
                    return STATUS_NO_MEM;
                LSP_STATUS_ASSERT(p->pBundle->wImages->add(p->wImage));

                // Load plugin icon
                path.fmt_ascii(LSP_BUILTIN_PREFIX "icons/%s.xpm", p->pMeta->uid);
                io::IInStream * is = pLoader->read_stream(&path);
                if (is == NULL)
                {
                    path.set_ascii(LSP_BUILTIN_PREFIX "icons/default_icon.xpm");
                    is = pLoader->read_stream(&path);
                    if (is == NULL)
                        lsp_warn("Could not load icon for plugin uid='%s'", p->pMeta->uid);
                }

                if (is != NULL)
                {
                    lsp_finally {
                        is->close();
                        delete is;
                    };
                    status_t res = p->wImage->bitmap()->load(is, NULL);
                    if (res != STATUS_OK)
                    {
                        lsp_warn("Could not load icon for plugin uid='%s', path='%s'", p->pMeta->uid, path.get_utf8());
                        return res;
                    }
                }

                // Add launch button
                if ((p->wButton = create_widget<tk::Button>("LauncherWindow::Plugin::Button")) == NULL)
                    return STATUS_NO_MEM;
                p->sName.bind(pDisplay->schema()->root(), pDisplay->dictionary());
                tmp.fmt_ascii("bundles.launcher.%s", p->pMeta->uid);
                LSP_STATUS_ASSERT(p->wButton->text()->set(&tmp));
                p->wButton->slots()->bind(tk::SLOT_MOUSE_IN, slot_plugin_mouse_in, this);
                p->wButton->slots()->bind(tk::SLOT_SUBMIT, slot_plugin_submit, this);
                tmp.fmt_ascii("bundles.%s.description", p->pMeta->uid);
                LSP_STATUS_ASSERT(p->sName.set(&tmp));
                p->pBundle->wButtons->add(p->wButton);
            }

            return STATUS_OK;
        }

        i18n::IDictionary *UI::get_default_dict()
        {
            i18n::IDictionary * dict = display()->dictionary();
            if (dict == NULL)
                return dict;

            if (dict->lookup("default", &dict) != STATUS_OK)
                dict = NULL;

            return dict;
        }

        status_t UI::init_i18n_support()
        {
            wLanguageArea           = controller()->widgets()->get<tk::Widget>("trg_language_select_area");
            wLanguage               = controller()->widgets()->get<tk::ComboBox>("trg_language_select");
            if (wLanguage == NULL)
                return STATUS_OK;

            i18n::IDictionary *dict     = get_default_dict();
            if (dict == NULL)
                return STATUS_OK;

            // Perform lookup before loading list of languages
            status_t res                = dict->lookup("lang.target", &dict);
            if (res != STATUS_OK)
                return res;

            // Iterate all children and add language keys
            LSPString key, value;
            res_sel_t *lang;
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

                if ((lang = new res_sel_t()) == NULL)
                    return STATUS_NO_MEM;
                lsp_finally {
                    if (lang != NULL)
                        delete lang;
                };

                lang->wItem = create_widget<tk::ListBoxItem>(NULL);
                if (lang->wItem == NULL)
                    return STATUS_NO_MEM;
                LSP_STATUS_ASSERT(wLanguage->items()->add(lang->wItem));

                // Fill attributes
                lang->sLocation.swap(&key);
                lang->wItem->text()->set_raw(&value);

                // Add to list
                if (!vLangSel.add(lang))
                    return STATUS_NO_MEM;
                lang        = NULL;
            }

            wLanguage->slots()->bind(tk::SLOT_SUBMIT, slot_select_language, this);

            // Set menu item visible only if there are available languages
            if (pLanguage != NULL)
            {
                const char * const lang = pLanguage->buffer<char>();
                if ((lang != NULL) && (strlen(lang) > 0))
                {
                    tk::Display * const dpy     = display();
                    if ((dpy->schema()->set_lanugage(lang)) == STATUS_OK)
                    {
                        lsp_trace("System language set to: %s", lang);
                        pLanguage->notify_all(ui::PORT_NONE);
                    }
                }
            }

            return STATUS_OK;
        }

        status_t UI::init_visual_schema_support()
        {
            wSchemaArea                 = controller()->widgets()->get<tk::ComboBox>("trg_visual_schema_select_area");
            wVisualSchema               = controller()->widgets()->get<tk::ComboBox>("trg_visual_schema_select");
            if (wLanguage == NULL)
                return STATUS_OK;
            if ((pLoader == NULL) || (pVisualSchema == NULL))
                return STATUS_OK;

            // For that we need to scan all available schemas in the resource directory
            resource::resource_t *list = NULL;
            ssize_t count = pLoader->enumerate(LSP_BUILTIN_PREFIX "schema", &list);
            if ((count <= 0) || (list == NULL))
            {
                if (list != NULL)
                    free(list);
                return STATUS_OK;
            }
            lsp_finally { free(list); };

            // Generate the 'Visual Schema' menu items
            status_t res;
            res_sel_t *schema;

            for (ssize_t i=0; i<count; ++i)
            {
                tk::StyleSheet sheet;
                LSPString path;

                if (list[i].type != resource::RES_FILE)
                    continue;

                if (!path.fmt_ascii(LSP_BUILTIN_PREFIX SCHEMA_PATH "/%s", list[i].name))
                    return STATUS_NO_MEM;

                // Try to load schema
                if ((res = load_stylesheet(&sheet, &path)) != STATUS_OK)
                {
                    if (res == STATUS_NO_MEM)
                        return res;
                    continue;
                }

                // Append schema to the list
                if ((schema = new res_sel_t()) == NULL)
                    return STATUS_NO_MEM;
                lsp_finally {
                    if (schema != NULL)
                        delete schema;
                };

                schema->wItem = create_widget<tk::ListBoxItem>(NULL);
                if (schema->wItem == NULL)
                    return STATUS_NO_MEM;
                LSP_STATUS_ASSERT(wVisualSchema->items()->add(schema->wItem));

                // Fill attributes
                schema->sLocation.swap(&path);
                schema->wItem->text()->set_key(sheet.title());
                schema->wItem->text()->params()->set_string("file", &schema->sLocation);

                // Create closure and bind
                if (!vSchemaSel.add(schema))
                    return STATUS_NO_MEM;
                schema          = NULL;
            }

            wVisualSchema->slots()->bind(tk::SLOT_SUBMIT, slot_select_visual_schema, this);

            return STATUS_OK;
        }

        bool UI::match_filter(const plugin_t *p, const LSPString *filter, bool favourites)
        {
            // Filter favourite bundles
            const bundle_t * const b = p->pBundle;
            if ((favourites) && (!sConfig.vFavourites.contains(b->pMeta->uid)))
                return false;

            // Match if filter is empty
            if (filter->is_empty())
                return true;

            // Match bundle name
            if (b->sNativeName.index_of_nocase(filter) >= 0)
                return true;
            if (b->sDefaultName.index_of_nocase(filter) >= 0)
                return true;

            // Match plugin name
            if (p->sNativeName.index_of_nocase(filter) >= 0)
                return true;
            if (p->sDefaultName.index_of_nocase(filter) >= 0)
                return true;

            return false;
        }

        void UI::sync_favourites_state(bundle_t *b)
        {
            if (b->wFavouries == NULL)
                return;

            const bool is_favourite = sConfig.vFavourites.contains(b->pMeta->uid);
            if (b->wFavouries->down()->get() != is_favourite)
                b->wFavouries->down()->set(is_favourite);
            b->wFavouries->text()->set((is_favourite) ? "icons.markers.star_filled_nb" : "icons.markers.star_blank_nb");
        }

        void UI::sync_widget_visibility()
        {
            if (wTabs == NULL)
                return;

            // Obtain active tab and it's index
            tk::Tab *tab = wTabs->selected()->get();
            const size_t idx = (tab != NULL) ? wTabs->widgets()->index_of(tab) : 0;

            // Create query string
            LSPString query, tmp;
            if (wFilter != NULL)
                wFilter->text()->format(&query);

            // Iterate over categories
            for (lltl::iterator<category_t> ci = vCategories.values(); ci; ++ci)
            {
                category_t * const c = ci.get();
                c->nVisibility      = 0;
                c->wHeading->text()->format(&c->sNativeName);
                c->wHeading->text()->format(&c->sDefaultName, LSP_TK_PROP_DEFAULT_LANGUAGE);
            }

            // Iterate over bundles
            for (lltl::iterator<bundle_t> bi = vBundles.values(); bi; ++bi)
            {
                bundle_t * const b = bi.get();
                b->nVisibility      = 0;
                b->wHeading->text()->format(&b->sNativeName);
                b->wHeading->text()->format(&b->sDefaultName, LSP_TK_PROP_DEFAULT_LANGUAGE);
            }

            // Iterate over plugins
            for (lltl::iterator<plugin_t> pi = vPlugins.values(); pi; ++pi)
            {
                plugin_t * const p = pi.get();

                p->sName.format(&p->sNativeName);
                p->sName.format(&p->sDefaultName, LSP_TK_PROP_DEFAULT_LANGUAGE);

                // Apply filter
                if (match_filter(p, &query, idx == 1))
                {
                    ++p->pBundle->nVisibility;
                    ++p->pBundle->pCategory->nVisibility;
                }
            }

            // Add widgets to the box
            tk::WidgetContainer * const wc = (idx == 0) ? wAllBundles : wFavourites;

            wAllBundles->remove_all();
            wFavourites->remove_all();

            // Add all visible widgets to the list
            for (lltl::iterator<category_t> ci = vCategories.values(); ci; ++ci)
            {
                category_t * const c = ci.get();
                if (c->nVisibility <= 0)
                    continue;

                wc->add(c->wRoot);
                lsp_trace("Added category id=%s", c->sUID);

                for (lltl::iterator<bundle_t> bi = c->vBundles.values(); bi; ++bi)
                {
                    bundle_t * const b = bi.get();

                    // Sync favourites state
                    sync_favourites_state(b);

                    if (b->nVisibility <= 0)
                        continue;

                    lsp_trace("  Added bundle id=%s", b->pMeta->uid);
                    wc->add(b->wRoot);

                    for (lltl::iterator<plugin_t> pi = b->vPlugins.values(); pi; ++pi)
                    {
                        plugin_t * const p = pi.get();
                        if (p->wImage != NULL)
                            p->wImage->visibility()->set(pi.index() == b->nActivePlugin);
                    }
                }
            }
        }

        const meta::package_t *UI::package() const
        {
            return pPackage;
        }

        void UI::host_scaling_changed()
        {
            sUIScaling.host_scaling_changed();
        }

        void UI::main_iteration()
        {
            IWrapper::main_iteration();

            if (nConfigChanges > 0)
            {
                // Save changed config
                status_t res = launcher::save_config(sConfig);
                if (res != STATUS_OK)
                    lsp_warn("Error saving launcher configuration file, error code=%d", int(res));

                nConfigChanges = 0;
            }
        }

        status_t UI::main_loop()
        {
            tk::Display * const dpy = display();
            return (dpy != NULL) ? dpy->main() : STATUS_BAD_STATE;
        }

        void UI::sync_language_selection()
        {
            if (wLanguage == NULL)
                return;

            if (wLanguageArea != NULL)
                wLanguage->visibility()->set(wLanguage->items()->size() > 1);

            tk::Display * const dpy    = display();
            if (dpy == NULL)
                return;

            LSPString lang;
            if ((dpy->schema()->get_language(&lang)) != STATUS_OK)
                return;

            for (size_t i=0, n=vLangSel.size(); i<n; ++i)
            {
                res_sel_t *xsel    = vLangSel.uget(i);
                if ((xsel->wItem != NULL) && (xsel->sLocation.equals(&lang)))
                {
                    wLanguage->selected()->set(xsel->wItem);
                    break;
                }
            }
        }

        void UI::sync_visual_schema_selection()
        {
            if (wVisualSchema == NULL)
                return;

            // Update area visibility
            if (wSchemaArea != NULL)
                wSchemaArea->visibility()->set(wVisualSchema->items()->size() > 1);

            // Update selecetd item
            const char *path = (pVisualSchema != NULL) ? pVisualSchema->buffer<const char>() : NULL;
            if (path == NULL)
                return;
            LSPString xpath;
            if (!xpath.set_utf8(path))
                return;

            for (size_t i=0, n=vSchemaSel.size(); i<n; ++i)
            {
                res_sel_t *xsel    = vSchemaSel.uget(i);
                if ((xsel->wItem != NULL) && (xsel->sLocation.equals(&xpath)))
                {
                    wVisualSchema->selected()->set(xsel->wItem);
                    break;
                }
            }
        }

        void UI::notify(ui::IPort *port, size_t flags)
        {
            if (port == NULL)
                return;

            if (port == pLanguage)
                sync_language_selection();
            if (port == pWindowWidth)
            {
                sConfig.nWidth  = pWindowWidth->value();
                wWindow->size()->set_width(sConfig.nWidth);
            }
            if (port == pWindowHeight)
            {
                sConfig.nHeight = pWindowHeight->value();
                wWindow->size()->set_height(sConfig.nHeight);
            }
            if (port == pVisualSchema)
            {
                sync_visual_schema_selection();
                sync_language_selection();
                sUIScaling.sync_parameters();
                sFontScaling.sync_parameters();
            }
        }

        void UI::on_window_resize()
        {
            if (wWindow == NULL)
                return;

            size_t width, height;
            wWindow->size()->get(width, height);

            // Check that window parameters have changed
            const bool notify_width     = (pWindowWidth != NULL) && (ssize_t(width) != ssize_t(pWindowWidth->value()));
            const bool notify_height    = (pWindowHeight != NULL) && (ssize_t(height) != ssize_t(pWindowHeight->value()));
            if ((!notify_width) && (!notify_height))
                return;

            // Mark configuration as changed
            ++nConfigChanges;

            // Wrap with begin_edit()/end_edit() stuff
            if (notify_width)
                pWindowWidth->begin_edit();
            if (notify_height)
                pWindowHeight->begin_edit();
            lsp_finally {
                if (notify_width)
                    pWindowHeight->end_edit();
                if (notify_height)
                    pWindowWidth->end_edit();
            };

            // Change values
            if (notify_width)
                pWindowWidth->set_value(width);
            if (notify_height)
                pWindowHeight->set_value(height);

            if (notify_width)
                pWindowWidth->notify_all(ui::PORT_USER_EDIT);
            if (notify_height)
                pWindowHeight->notify_all(ui::PORT_USER_EDIT);
        }

        void UI::select_plugin_image(tk::Widget *sender, bool select)
        {
            plugin_t *p = NULL;

            for (lltl::iterator<plugin_t> pi = vPlugins.values(); pi; ++pi)
            {
                plugin_t * const xp = pi.get();
                if (xp->wButton == sender)
                {
                    p = xp;
                    break;
                }
            }

            if (p == NULL)
                return;

            // Update image visibility
            bundle_t * const b  = p->pBundle;
            b->nActivePlugin    = b->vPlugins.index_of(p);

            for (lltl::iterator<plugin_t> pi = b->vPlugins.values(); pi; ++pi)
            {
                plugin_t * const xp = pi.get();
                if (xp->wImage != NULL)
                    xp->wImage->visibility()->set(pi.index() == b->nActivePlugin);
            }
        }

        status_t UI::locate_window()
        {
            tk::Window * const wnd = window();
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

        status_t UI::slot_display_idle(tk::Widget *sender, void *ptr, void *data)
        {
//            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            UI * const self = static_cast<UI *>(ptr);
            if (self != NULL)
                self->main_iteration();

            return STATUS_OK;
        }

        status_t UI::slot_window_close(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self != NULL)
            {
                tk::Display * const dpy = self->display();
                if (dpy != NULL)
                    dpy->quit_main();
            }

            return STATUS_OK;
        }


        status_t UI::slot_window_resize(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self != NULL)
                self->on_window_resize();

            return STATUS_OK;
        }

        status_t UI::slot_filter_change(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self != NULL)
                self->sync_widget_visibility();

            return STATUS_OK;
        }

        status_t UI::slot_plugin_mouse_in(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self != NULL)
                self->select_plugin_image(sender, true);

            return STATUS_OK;
        }

        status_t UI::slot_change_tab(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self != NULL)
                self->sync_widget_visibility();

            return STATUS_OK;
        }

        status_t UI::slot_plugin_submit(tk::Widget *sender, void *ptr, void *data)
        {
            if (sender == NULL)
                return STATUS_OK;

            UI * const self = static_cast<UI *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if (self->pLaunch != NULL)
            {
                for (lltl::iterator<plugin_t> pi = self->vPlugins.values(); pi; ++pi)
                {
                    const plugin_t * const p = pi.get();
                    if ((p != NULL) && (p->wButton == sender))
                    {
                        *self->pLaunch    = p->pMeta;
                        break;
                    }
                }
            }

            // Ensure that configuration files have been saved
            self->main_iteration();

            // Leave the main event loop
            tk::Display * const dpy = self->display();
            if (dpy != NULL)
                dpy->quit_main();

            return STATUS_OK;
        }

        status_t UI::slot_toggle_favourites(tk::Widget *sender, void *ptr, void *data)
        {
            if (sender == NULL)
                return STATUS_OK;

            UI * const self = static_cast<UI *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            for (lltl::iterator<bundle_t> bi = self->vBundles.values(); bi; ++bi)
            {
                bundle_t * const b = bi.get();
                if ((b != NULL) && (b->wFavouries == sender))
                {
                    const bool favourite_on = b->wFavouries->down()->get();
                    char *id                = NULL;
                    lsp_finally {
                        if (id != NULL)
                            free(id);
                    };
                    if (favourite_on)
                    {
                        // Add to list
                        if ((id = strdup(b->pMeta->uid)) != NULL)
                        {
                            if (self->sConfig.vFavourites.put(id, &id))
                                ++self->nConfigChanges;
                        }
                    }
                    else
                    {
                        // Remove from list
                        if (self->sConfig.vFavourites.remove(b->pMeta->uid, &id))
                            ++self->nConfigChanges;
                    }

                    self->sync_favourites_state(b);
                    break;
                }
            }

            return STATUS_OK;
        }

        status_t UI::slot_select_language(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if (self->wLanguage == NULL)
                return STATUS_OK;

            tk::ListBoxItem * const item = self->wLanguage->selected()->get();
            if (item == NULL)
                return STATUS_OK;

            tk::Display * const dpy = sender->display();
            lsp_trace("dpy = %p", dpy);
            if (dpy == NULL)
                return STATUS_BAD_STATE;

            for (lltl::iterator<res_sel_t> it=self->vLangSel.values(); it; ++it)
            {
                const res_sel_t * const sel = it.get();
                if (sel->wItem == item)
                {
                    // Select language
                    tk::Schema * const schema  = dpy->schema();
                    lsp_trace("Select language: \"%s\"", sel->sLocation.get_native());
                    if ((schema->set_lanugage(&sel->sLocation)) != STATUS_OK)
                    {
                        lsp_warn("Failed to select language \"%s\"", sel->sLocation.get_native());
                        return STATUS_OK;
                    }

                    // Update parameter
                    const char * const dlang = sel->sLocation.get_utf8();
                    const char * const slang = self->pLanguage->buffer<char>();
                    lsp_trace("Current language in settings: \"%s\"", slang);
                    if ((slang == NULL) || (strcmp(slang, dlang)))
                    {
                        self->pLanguage->begin_edit();
                        self->pLanguage->write(dlang, strlen(dlang));
                        self->pLanguage->notify_all(ui::PORT_USER_EDIT);
                        self->pLanguage->end_edit();
                    }
                    lsp_trace("Language has been selected");
                    break;
                }
            }

            return STATUS_OK;
        }

        status_t UI::slot_select_visual_schema(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if (self->wVisualSchema == NULL)
                return STATUS_OK;

            tk::ListBoxItem * const item = self->wVisualSchema->selected()->get();
            if (item == NULL)
                return STATUS_OK;

            for (lltl::iterator<res_sel_t> it=self->vSchemaSel.values(); it; ++it)
            {
                const res_sel_t * const sel = it.get();
                if (sel->wItem == item)
                {
                    // Try to load schema first
                    if (self->load_visual_schema(&sel->sLocation) == STATUS_OK)
                    {
                        const char * const value = sel->sLocation.get_utf8();

                        if (self->pVisualSchema != NULL)
                        {
                            self->pVisualSchema->begin_edit();
                            self->pVisualSchema->write(value, strlen(value));
                            self->pVisualSchema->notify_all(ui::PORT_USER_EDIT);
                            self->pVisualSchema->end_edit();
                        }
                    }

                    lsp_trace("Visual schema has changed");
                    break;
                }
            }

            return STATUS_OK;
        }

        status_t UI::slot_show_about(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if (self->pAboutWindow == NULL)
            {
                ctl::AboutWindow *ctl = new ctl::AboutWindow(self);
                if (ctl == NULL)
                    return STATUS_NO_MEM;
                lsp_finally {
                    if (ctl != NULL)
                    {
                        ctl->destroy();
                        delete ctl;
                    }
                };
                LSP_STATUS_ASSERT(ctl->init());
                LSP_STATUS_ASSERT(self->controller()->controllers()->add(ctl));
                self->pAboutWindow    = release_ptr(ctl);
            }

            self->pAboutWindow->show(self->window());
            return STATUS_OK;
        }

        status_t UI::slot_show_bundle_manual(tk::Widget *sender, void *ptr, void *data)
        {
            if (sender == NULL)
                return STATUS_OK;

            UI * const self = static_cast<UI *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            for (lltl::iterator<bundle_t> bi = self->vBundles.values(); bi; ++bi)
            {
                bundle_t * const b = bi.get();
                if ((b != NULL) && (b->wHelp == sender))
                {
                    const plugin_t * const p = b->vPlugins.get(b->nActivePlugin);
                    const meta::plugin_t * const meta = (p != NULL) ? p->pMeta : NULL;
                    if (meta == NULL)
                        break;

                    self->sDocumentation.show_plugin_manual(meta);
                    break;
                }
            }

            return STATUS_OK;
        }

        status_t UI::slot_show_ui_manual(tk::Widget *sender, void *ptr, void *data)
        {
            if (sender == NULL)
                return STATUS_OK;

            UI * const self = static_cast<UI *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            self->sDocumentation.show_ui_manual();

            return STATUS_OK;
        }

        status_t UI::slot_window_show(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            self->locate_window();

            return STATUS_OK;
        }

    } /* namespace launcher */
} /* namespace lsp */


