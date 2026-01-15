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
#include <lsp-plug.in/plug-fw/ui/const.h>
#include <lsp-plug.in/plug-fw/util/launcher/ui.h>
#include <lsp-plug.in/plug-fw/util/launcher/ui_config.h>
#include <lsp-plug.in/plug-fw/util/launcher/window.h>

#include <private/ui/xml/Handler.h>
#include <private/ui/xml/RootNode.h>

namespace lsp
{
    namespace launcher
    {
        UI::UI(resource::ILoader * loader):
            ui::IWrapper(NULL, loader)
        {
            pPackage            = NULL;
//            pDisplay            = display;
//            wWindow             = NULL;
//
//            pLoader             = loader;
//
//            init_config(sConfig);
//            bConfigDirty        = false;
        }

        UI::~UI()
        {
            do_destroy();
        }

        void UI::do_destroy()
        {
            if (pPackage != NULL)
            {
                meta::free_manifest(pPackage);
                pPackage = NULL;
            }
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
        #ifdef LSP_TRACE
            lsp_trace("Begin UI initialization");
            const system::time_millis_t start = system::get_time_millis();
            lsp_finally {
                const system::time_millis_t end = system::get_time_millis();
                lsp_trace("UI initialization time: %d ms", int(end - start));
            };
        #endif /* LSP_TRACE */

            // Load package information
            status_t res = load_package_info();
            if (res != STATUS_OK)
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

            // Bind events to the root window
            root->slots()->bind(tk::SLOT_CLOSE, slot_window_close, this);

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

//        template <typename T>
//        void UI::destroy(T * & w)
//        {
//            if (w == NULL)
//                return;
//
//            w->destroy();
//            delete w;
//            w = NULL;
//        }
//
//        void UI::destroy()
//        {
//            destroy(wWindow);
//
//            destroy_plugin_metadata(sRegistry);
//            destroy_visual_schemas(sSchemas);
//        }
//
//        template <typename T>
//        status_t UI::create(T * & widget)
//        {
//            T * result = new T(pDisplay);
//            if (result == NULL)
//                return STATUS_NO_MEM;
//
//            status_t res = result->init();
//            if (res != STATUS_OK)
//            {
//                result->destroy();
//                delete result;
//                return res;
//            }
//
//            widget = result;
//
//            return res;
//        }
//
//        status_t UI::init()
//        {
//            if (pDisplay == NULL)
//                return STATUS_BAD_STATE;
//
//            // Read plugin metadata from JSON
//            LSP_STATUS_ASSERT(read_plugin_metadata(sRegistry, pLoader, LSP_BUILTIN_PREFIX "loader/plugins.json"));
//
//            // Read global configuration
//            LSP_STATUS_ASSERT(load_config(sConfig));
//
//            // Read UI schemas
//            LSP_STATUS_ASSERT(load_visual_schemas(sSchemas, pLoader));
//
//            // Update visual schema
//            if (update_visual_schema() != STATUS_OK)
//                lsp_warn("Error loading visual schema");
//
//            // Bind the display idle handler
//            pDisplay->slots()->bind(tk::SLOT_IDLE, slot_display_idle, this);
//            pDisplay->set_idle_interval(1000 / UI_FRAMES_PER_SECOND);
//
//            // Create main window
//            LSP_STATUS_ASSERT(create(wWindow));
//            wWindow->size()->set(sConfig.nWidth, sConfig.nHeight);
//            wWindow->set_class("launcher", "lsp-plugins");
//            wWindow->role()->set("audio-plugin");
//            wWindow->title()->set_raw(&sRegistry.package.name);
//            wWindow->layout()->set_scale(1.0f);
//
//            wWindow->slots()->bind(tk::SLOT_CLOSE, slot_window_close, this);
//            wWindow->slots()->bind(tk::SLOT_RESIZE, slot_window_resize, this);
//
//
//            // Show the window
//            wWindow->show();
//
//            return STATUS_OK;
//        }
//
//        status_t UI::apply_visual_schema(tk::StyleSheet *sheet)
//        {
//            // Apply schema
//            status_t res = pDisplay->schema()->apply(sheet, pLoader);
//            if (res != STATUS_OK)
//                return res;
//
//            return res;
//        }
//
//        status_t UI::update_visual_schema()
//        {
//            visual_schema_t *dfl_a = NULL;
//            visual_schema_t *dfl_b = NULL;
//
//            // First try to apply schema from configuration.
//            status_t res;
//            for (lltl::iterator<visual_schema_t> it=sSchemas.values(); it; ++it)
//            {
//                visual_schema_t *s = it.get();
//                if (s->path.equals(&sConfig.sSchema))
//                {
//                    if ((res = apply_visual_schema(&s->sheet)) == STATUS_OK)
//                        return STATUS_OK;
//
//                    dfl_a       = s;
//                    break;
//                }
//            }
//
//            // Second, try to load default schema
//            if (!sConfig.sSchema.equals_ascii(LSP_BUILTIN_PREFIX DEFAULT_SCHEMA_PATH))
//            {
//                for (lltl::iterator<visual_schema_t> it=sSchemas.values(); it; ++it)
//                {
//                    visual_schema_t *s = it.get();
//                    if (dfl_a == s)
//                        continue;
//
//                    if (s->path.equals_ascii(LSP_BUILTIN_PREFIX DEFAULT_SCHEMA_PATH))
//                    {
//                        if ((res = apply_visual_schema(&s->sheet)) == STATUS_OK)
//                        {
//                            if (!sConfig.sSchema.set_ascii(LSP_BUILTIN_PREFIX DEFAULT_SCHEMA_PATH))
//                                return STATUS_NO_MEM;
//
//                            bConfigDirty        = true;
//                            return STATUS_OK;
//                        }
//                        dfl_b       = s;
//                    }
//                }
//            }
//
//            // Try to load any rest schema
//            for (lltl::iterator<visual_schema_t> it=sSchemas.values(); it; ++it)
//            {
//                visual_schema_t *s = it.get();
//                if ((dfl_a == s) || (dfl_b == s))
//                    continue;
//
//                if ((res = apply_visual_schema(&s->sheet)) == STATUS_OK)
//                {
//                    if (!sConfig.sSchema.set(&s->path))
//                        return STATUS_NO_MEM;
//                    return STATUS_OK;
//                }
//            }
//
//            return STATUS_NOT_FOUND;
//        }
//
//        status_t UI::slot_display_idle(tk::Widget *sender, void *ptr, void *data)
//        {
//            UI * const self = static_cast<UI *>(ptr);
//            if (self == NULL)
//                return STATUS_OK;
//
//            if (self->bConfigDirty)
//            {
//                status_t res = save_config(self->sConfig);
//                if (res != STATUS_OK)
//                    lsp_warn("Error saving configuration file: code=%d", int(res));
//                self->bConfigDirty = false;
//            }
//
//            return STATUS_OK;
//        }
//
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
//
//        status_t UI::slot_window_resize(tk::Widget *sender, void *ptr, void *data)
//        {
//            UI * const self = static_cast<UI *>(ptr);
//            if (self != NULL)
//            {
//                size_t width, height;
//                ui_config_t & config = self->sConfig;
//
//                self->wWindow->size()->get(width, height);
//
//                if ((width != config.nWidth) ||
//                    (height != config.nHeight))
//                {
//                    config.nWidth       = width;
//                    config.nHeight      = height;
//                    self->bConfigDirty  = true;
//                }
//            }
//
//            return STATUS_OK;
//        }

    } /* namespace launcher */
} /* namespace lsp */


