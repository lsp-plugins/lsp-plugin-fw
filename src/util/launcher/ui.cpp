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
#include <lsp-plug.in/plug-fw/meta/registry.h>
#include <lsp-plug.in/plug-fw/ui/const.h>
#include <lsp-plug.in/plug-fw/util/launcher/ui.h>
#include <lsp-plug.in/plug-fw/util/launcher/window.h>
#include <lsp-plug.in/plug-fw/ctl.h>

#include <private/ui/xml/Handler.h>
#include <private/ui/xml/RootNode.h>

namespace lsp
{
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
            sFontScaling(this)
        {
            pPackage            = package;
            pLaunch             = launch;
            pWindowWidth        = NULL;
            pWindowHeight       = NULL;

            wFilter             = NULL;
            wTabs               = NULL;
            wAllBundles         = NULL;
            wFavourites         = NULL;
        }

        UI::~UI()
        {
            do_destroy();
        }

        void UI::do_destroy()
        {
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
            BIND_PORT(this, pWindowWidth, UI_LAUNCHER_WIDTH_PORT);
            BIND_PORT(this, pWindowHeight, UI_LAUNCHER_HEIGHT_PORT);

            // Bind events to the display
            pDisplay->slots()->bind(tk::SLOT_IDLE, slot_display_idle, this);
            pDisplay->set_idle_interval(1000 / UI_FRAMES_PER_SECOND);

            // Bind events to the root window
            root->slots()->bind(tk::SLOT_CLOSE, slot_window_close, this);
            root->slots()->bind(tk::SLOT_RESIZE, slot_window_resize, this);
            if (wFilter != NULL)
                wFilter->slots()->bind(tk::SLOT_CHANGE, slot_filter_change, this);
            if (wTabs != NULL)
                wTabs->slots()->bind(tk::SLOT_SUBMIT, slot_change_tab, this);

            // Notify port changes
            if (pWindowWidth != NULL)
                pWindowWidth->notify_all(ui::PORT_NONE);
            if (pWindowHeight != NULL)
                pWindowHeight->notify_all(ui::PORT_NONE);

            // Create plugin catalog
            LSP_STATUS_ASSERT(create_catalog());

            // Synchronize state
            sync_widget_visibility();
            sUIScaling.sync_parameters();
            sFontScaling.sync_parameters();

            return STATUS_OK;
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
                    LSP_STATUS_ASSERT(b->wFavouries->text()->set("icons.actions.star"));
                    LSP_STATUS_ASSERT(btns->add(b->wFavouries));

                    if ((b->wHelp = create_widget<tk::Button>("LauncherWindow::Bundle::Help")) == NULL)
                        return STATUS_NO_MEM;
                    LSP_STATUS_ASSERT(b->wHelp->text()->set("icons.main.help"));
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
                    is = pLoader->read_stream(LSP_BUILTIN_PREFIX "icons/default_icon.xpm");
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
                p->wButton->slots()->bind(tk::SLOT_MOUSE_OUT, slot_plugin_mouse_out, this);
                p->wButton->slots()->bind(tk::SLOT_SUBMIT, slot_plugin_submit, this);
                tmp.fmt_ascii("bundles.%s.description", p->pMeta->uid);
                LSP_STATUS_ASSERT(p->sName.set(&tmp));
                p->pBundle->wButtons->add(p->wButton);
            }

            return STATUS_OK;
        }

        bool UI::match_filter(const plugin_t *p, const LSPString *filter)
        {
            if (filter->is_empty())
                return true;
            if (p->sNativeName.index_of_nocase(filter) >= 0)
                return true;
            if (p->sDefaultName.index_of_nocase(filter) >= 0)
                return true;

            const bundle_t * const b = p->pBundle;
            if (b->sNativeName.index_of_nocase(filter) >= 0)
                return true;
            if (b->sDefaultName.index_of_nocase(filter) >= 0)
                return true;

            return false;
        }

        void UI::sync_widget_visibility()
        {
            if (wTabs == NULL)
                return;

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
                if (match_filter(p, &query))
                {
                    ++p->pBundle->nVisibility;
                    ++p->pBundle->pCategory->nVisibility;
                }
            }

            // Add widgets to the box
            tk::Tab *tab = wTabs->selected()->get();
            const size_t idx = (tab != NULL) ? wTabs->widgets()->index_of(tab) : 0;
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

        status_t UI::main_loop()
        {
            tk::Display * const dpy = display();
            return (dpy != NULL) ? dpy->main() : STATUS_BAD_STATE;
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

        void UI::notify(ui::IPort *port, size_t flags)
        {
            if (port == NULL)
                return;

            if (port == pWindowWidth)
                wWindow->size()->set_width(pWindowWidth->value());
            if (port == pWindowHeight)
                wWindow->size()->set_height(pWindowHeight->value());
        }

        void UI::on_window_resize()
        {
            if (wWindow == NULL)
                return;

            size_t width, height;
            wWindow->size()->get(width, height);

            if (pWindowWidth != NULL)
                pWindowWidth->begin_edit();
            if (pWindowHeight != NULL)
                pWindowHeight->begin_edit();
            lsp_finally {
                if (pWindowHeight != NULL)
                    pWindowHeight->end_edit();
                if (pWindowWidth != NULL)
                    pWindowWidth->end_edit();
            };

            bool notify_width = false;
            bool notify_height = false;
            if ((pWindowWidth != NULL) && (ssize_t(width) != ssize_t(pWindowWidth->value())))
            {
                pWindowWidth->set_value(width);
                notify_width = true;
            }
            if ((pWindowHeight != NULL) && (ssize_t(height) != ssize_t(pWindowHeight->value())))
            {
                pWindowHeight->set_value(height);
                notify_height = true;
            }

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

        status_t UI::slot_display_idle(tk::Widget *sender, void *ptr, void *data)
        {
//            lsp_trace("sender = %p, ptr = %p, data = %p", sender, ptr, data);
            UI * const self = static_cast<UI *>(ptr);
            if (self != NULL)
                self->main_iteration();

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

        status_t UI::slot_plugin_mouse_out(tk::Widget *sender, void *ptr, void *data)
        {
            UI * const self = static_cast<UI *>(ptr);
            if (self != NULL)
                self->select_plugin_image(NULL, false);

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
            UI * const self = static_cast<UI *>(ptr);
            if (self != NULL)
            {
                if (self->pLaunch != NULL)
                {
                    for (lltl::iterator<plugin_t> pi = self->vPlugins.values(); pi; ++pi)
                    {
                        const plugin_t *p = pi.get();
                        if ((p != NULL) && (p->wButton == sender))
                        {
                            *self->pLaunch    = p->pMeta;
                            break;
                        }
                    }
                }

                tk::Display * const dpy = self->display();
                if (dpy != NULL)
                    dpy->quit_main();
            }

            return STATUS_OK;
        }

    } /* namespace launcher */
} /* namespace lsp */


