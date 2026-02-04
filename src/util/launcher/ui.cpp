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

        UI::UI(resource::ILoader * loader):
            ui::IWrapper(NULL, loader)
        {
            pPackage            = NULL;
            pWindowWidth        = NULL;
            pWindowHeight       = NULL;

            wAllBundles         = NULL;
            wFavourites         = NULL;
        }

        UI::~UI()
        {
            do_destroy();
        }

        void UI::do_destroy()
        {
            // Destroy manifest if present
            if (pPackage != NULL)
            {
                meta::free_manifest(pPackage);
                pPackage = NULL;
            }

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

        status_t UI::load_package_info()
        {
            // Load package information
            status_t res;
            io::IInStream *is = resources()->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
            if (is == NULL)
            {
                lsp_error("No manifest.json found in resources");
                return STATUS_BAD_STATE;
            }
            lsp_finally {
                is->close();
                delete is;
            };

            res = meta::load_manifest(&pPackage, is);

            if (res != STATUS_OK)
            {
                lsp_error("Error while reading manifest file, error: %d", int(res));
                return res;
            }

            return STATUS_OK;
        }

        ssize_t UI::plugin_cmp_function(const plugin_t *a, const plugin_t *b)
        {
            ssize_t delta;
            const meta::plugin_t *pa = a->pPlugin;
            const meta::plugin_t *pb = b->pPlugin;

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

                p->pPlugin      = meta;             // Plugin
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

                const meta::plugin_t * const mp = p->pPlugin;
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
                    c->enCategory   = mb->group;
                    c->sUID         = category_keys[c->enCategory];
                    c->nVisibiity   = 0;

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
                    b->nVisibiity   = 0;

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

            // Call post-initialization
            if ((res = wnd->post_init()) != STATUS_OK)
                return res;

            pWindow     = wnd;
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

            // Load package information
            if ((res = load_package_info()) != STATUS_OK)
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

            tk::Window * const root    = window();
            if (root == NULL)
            {
                lsp_error("No root window present!");
                return STATUS_BAD_STATE;
            }

            // Bind widgets
            wAllBundles = controller()->widgets()->get<tk::WidgetContainer>("all_plugins_list");
            wFavourites = controller()->widgets()->get<tk::WidgetContainer>("favourites_list");

            // Bind ports
            BIND_PORT(this, pWindowWidth, UI_LAUNCHER_WIDTH_PORT);
            BIND_PORT(this, pWindowHeight, UI_LAUNCHER_HEIGHT_PORT);

            // Bind events to the display
            pDisplay->slots()->bind(tk::SLOT_IDLE, slot_display_idle, this);
            pDisplay->set_idle_interval(1000 / UI_FRAMES_PER_SECOND);

            // Bind events to the root window
            root->slots()->bind(tk::SLOT_CLOSE, slot_window_close, this);
            root->slots()->bind(tk::SLOT_RESIZE, slot_window_resize, this);

            // Notify port changes
            if (pWindowWidth != NULL)
                pWindowWidth->notify_all(ui::PORT_NONE);
            if (pWindowHeight != NULL)
                pWindowHeight->notify_all(ui::PORT_NONE);

            // Show the window
            root->show();

            return res;
        }

        const meta::package_t *UI::package() const
        {
            return pPackage;
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

    } /* namespace launcher */
} /* namespace lsp */


