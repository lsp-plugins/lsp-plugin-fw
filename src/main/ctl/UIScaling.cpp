/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 8 февр. 2026 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ctl
    {
        static constexpr ssize_t SCALING_FACTOR_BEGIN       = 50;
        static constexpr ssize_t SCALING_FACTOR_STEP        = 25;
        static constexpr ssize_t SCALING_FACTOR_END         = 400;

        static const tk::tether_t top_tether[] =
        {
            { tk::TF_BOTTOM | tk::TF_LEFT,      1.0f,  1.0f },
            { tk::TF_TOP | tk::TF_LEFT,         1.0f, -1.0f },
        };

        static const tk::tether_t bottom_tether[] =
        {
            { tk::TF_TOP | tk::TF_LEFT,         1.0f, -1.0f },
            { tk::TF_BOTTOM | tk::TF_LEFT,      1.0f,  1.0f },
        };

        UIScaling::UIScaling(ui::IWrapper *src)
        {
            pWrapper            = src;
            wUIScaling          = NULL;
            wBundleScaling      = NULL;
            wPreferHost         = NULL;

            pUIScaling          = NULL;
            pUIScalingHost      = NULL;
            pBundleScaling      = NULL;
        }

        UIScaling::~UIScaling()
        {
            destroy();
            pWrapper        = NULL;
        }

        tk::MenuItem *UIScaling::create_menu_item(tk::Menu *dst)
        {
            tk::MenuItem *item = new tk::MenuItem(dst->display());
            if (item == NULL)
                return NULL;
            lsp_finally {
                if (item != NULL)
                {
                    item->destroy();
                    delete item;
                }
            };

            status_t res = item->init();
            if (res != STATUS_OK)
                return NULL;

            if (pWrapper->controller()->widgets()->add(item) != STATUS_OK)
                return NULL;

            if ((res = dst->add(item)) != STATUS_OK)
                return NULL;

            return release_ptr(item);
        }

        tk::Menu *UIScaling::create_menu()
        {
            status_t res;
            tk::Menu *menu = new tk::Menu(pWrapper->display());
            if (menu == NULL)
                return NULL;
            lsp_finally {
                if (menu != NULL)
                {
                    menu->destroy();
                    delete menu;
                }
            };

            if ((res = menu->init()) != STATUS_OK)
                return NULL;
            if (pWrapper->controller()->widgets()->add(menu) != STATUS_OK)
                return NULL;

            return release_ptr(menu);
        }

        status_t UIScaling::init(bool bundle_scaling)
        {
            status_t res;
            if ((res = init_ui_scaling()) != STATUS_OK)
                return res;
            if (bundle_scaling)
            {

                if ((res = init_bundle_scaling()) != STATUS_OK)
                    return res;
            }

            return STATUS_OK;
        }

        status_t UIScaling::init_ui_scaling()
        {
            // Bind ports
            BIND_PORT(pWrapper, pUIScaling, UI_SCALING_PORT);
            BIND_PORT(pWrapper, pUIScalingHost, UI_SCALING_HOST_PORT);

            // Create submenu
            wUIScaling              = create_menu();
            if (wUIScaling == NULL)
                return STATUS_NO_MEM;

            // Initialize 'Prefer the host' settings
            tk::MenuItem *item;
            if ((item = create_menu_item(wUIScaling)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.ui_scaling.prefer_host");
            item->type()->set_check();
            item->slots()->bind(tk::SLOT_SUBMIT, slot_ui_scaling_prefer_host, this);
            wPreferHost     = item;

            // Add the 'Zoom in' setting
            if ((item = create_menu_item(wUIScaling)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.ui_scaling.zoom_in");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_ui_scaling_zoom_in, this);

            // Add the 'Zoom out' setting
            if ((item = create_menu_item(wUIScaling)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.ui_scaling.zoom_out");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_ui_scaling_zoom_out, this);

            // Add the separator
            if ((item = create_menu_item(wUIScaling)) == NULL)
                return STATUS_NO_MEM;
            item->type()->set_separator();

            // Generate the 'Set scaling' menu items
            for (size_t scale = SCALING_FACTOR_BEGIN; scale <= SCALING_FACTOR_END; scale += SCALING_FACTOR_STEP)
                add_scaling_menu_item(vUIScalingSel, wUIScaling, "actions.ui_scaling.value:pc", scale, slot_ui_scaling_select);

            return STATUS_OK;
        }

        status_t UIScaling::init_bundle_scaling()
        {
            // Bind ports
            BIND_PORT(pWrapper, pBundleScaling, UI_BUNDLE_SCALING_PORT);

            // Create submenu
            wBundleScaling          = create_menu();
            if (wBundleScaling == NULL)
                return STATUS_NO_MEM;

            // Add the 'Zoom in' setting
            tk::MenuItem *item;
            if ((item = create_menu_item(wBundleScaling)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.bundle_scaling.zoom_in");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_bundle_scaling_zoom_in, this);

            // Add the 'Zoom out' setting
            if ((item = create_menu_item(wBundleScaling)) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.bundle_scaling.zoom_out");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_bundle_scaling_zoom_out, this);

            // Add the separator
            if ((item = create_menu_item(wBundleScaling)) == NULL)
                return STATUS_NO_MEM;
            item->type()->set_separator();

            // Generate the exact value menu items
            add_scaling_menu_item(vBundleScalingSel, wBundleScaling, "actions.bundle_scaling.default", 0, slot_bundle_scaling_select);
            for (size_t scale = SCALING_FACTOR_BEGIN; scale <= SCALING_FACTOR_END; scale += SCALING_FACTOR_STEP)
                add_scaling_menu_item(vBundleScalingSel, wBundleScaling, "actions.bundle_scaling.value:pc", scale, slot_bundle_scaling_select);

            return STATUS_OK;
        }

        void UIScaling::destroy()
        {
            // Delete UI scaling bindings
            for (size_t i=0, n=vUIScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *s = vUIScalingSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vUIScalingSel.flush();

            // Delete UI bundle scaling bindings
            for (size_t i=0, n=vBundleScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *s = vBundleScalingSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vBundleScalingSel.flush();
        }

        status_t UIScaling::add_scaling_menu_item(
            lltl::parray<scaling_sel_t> & list,
            tk::Menu *menu, const char *key, size_t scale,
            tk::event_handler_t handler)
        {
            tk::MenuItem * const item = create_menu_item(menu);
            if (item == NULL)
                return STATUS_NO_MEM;
            item->type()->set_radio();
            item->text()->set_key(key);
            item->text()->params()->set_int("value", scale);

            // Add scaling record
            scaling_sel_t * const sel = new scaling_sel_t();
            if (sel == NULL)
                return STATUS_NO_MEM;

            sel->ctl        = this;
            sel->scaling    = scale;
            sel->item       = item;

            if (!list.add(sel))
            {
                delete sel;
                return STATUS_NO_MEM;
            }

            item->slots()->bind(tk::SLOT_SUBMIT, handler, sel);

            return STATUS_OK;
        }

        status_t UIScaling::show_menu(tk::Menu *menu, tk::Widget *actor)
        {
            if (menu == NULL)
                return STATUS_OK;

            if (actor != NULL)
            {
                ws::rectangle_t xr, wr;

                pWrapper->window()->get_rectangle(&wr);
                actor->get_rectangle(&xr);

                if (xr.nTop > (wr.nHeight / 2))
                    menu->set_tether(bottom_tether, sizeof(bottom_tether)/sizeof(tk::tether_t));
                else
                    menu->set_tether(top_tether, sizeof(top_tether)/sizeof(tk::tether_t));
                menu->show(actor);
            }
            else
                menu->show();

            return STATUS_OK;
        }

        void UIScaling::notify(ui::IPort *port, size_t flags)
        {
            if (port == NULL)
                return;
            if ((port != pUIScaling) && (port != pUIScalingHost) && (port != pBundleScaling))
                return;

            tk::Display *dpy            = pWrapper->display();
            if (dpy == NULL)
                return;

            const bool sync_host        = (pUIScalingHost->value() >= 0.5f);
            const float ui_scaling      = (pUIScaling != NULL) ? pUIScaling->value() : 100.0f;
            const float bundle_scaling  = (pBundleScaling != NULL) ? pBundleScaling->value() : 0.0f;

            lsp_trace("sync_host=%s, ui_scaling=%f, ui_bundle_scaling=%f",
                (sync_host) ? "true" : "false", ui_scaling, bundle_scaling);

            // Compute actual scaling
            float actual_scaling        = ui_scaling;
            if (bundle_scaling >= float(SCALING_FACTOR_BEGIN))
                actual_scaling              = bundle_scaling;
            else if (sync_host)
                actual_scaling              = pWrapper->ui_scaling_factor(actual_scaling);

            lsp_trace("computed actual_scaling=%f", actual_scaling);

            // Update the UI ui_scaling
            dpy->schema()->scaling()->set(actual_scaling * 0.01f);

            // Synchronize state of menu check boxes
            if (wPreferHost != NULL)
                wPreferHost->checked()->set(sync_host);

            for (size_t i=0, n=vUIScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *xsel = vUIScalingSel.uget(i);
                if (xsel->item != NULL)
                    xsel->item->checked()->set(fabs(xsel->scaling - ui_scaling) < 1e-4f);
            }

            for (size_t i=0, n=vBundleScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *xsel = vBundleScalingSel.uget(i);
                if (xsel->item != NULL)
                    xsel->item->checked()->set(fabs(xsel->scaling - bundle_scaling) < 1e-4f);
            }
        }

        status_t UIScaling::bind_event(const char *id, tk::slot_t slot, tk::event_handler_t handler)
        {
            tk::Widget *w = pWrapper->controller()->widgets()->find(id);
            if (w == NULL)
                return STATUS_NOT_FOUND;

            tk::handler_id_t ev = w->slots()->bind(slot, handler, this);
            return (ev >= 0) ? STATUS_OK : STATUS_NOT_BOUND;
        }

        ssize_t UIScaling::get_bundle_scaling()
        {
            if (pBundleScaling == NULL)
                return -1;

            const ssize_t value     = pBundleScaling->value();
            if (value >= SCALING_FACTOR_BEGIN)
                return value;

            tk::Display * const dpy = pWrapper->display();
            if (dpy == NULL)
                return -1;

            return ssize_t(dpy->schema()->scaling()->get() * 100.0f);
        }

        void UIScaling::host_scaling_changed()
        {
            if (pUIScalingHost != NULL)
                pUIScalingHost->notify_all(ui::PORT_NONE);
            else if (pUIScaling != NULL)
                pUIScaling->notify_all(ui::PORT_NONE);
        }

        status_t UIScaling::slot_ui_scaling_prefer_host(tk::Widget *sender, void *ptr, void *data)
        {
            UIScaling * const self   = static_cast<UIScaling *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            const bool prefer   = (self->pUIScalingHost->value() >= 0.5f) ? 0.0f : 1.0f;
            self->pUIScalingHost->set_value((prefer) ? 1.0f : 0.0f);

            // Wrap with begin_edit()/end_edit()
            self->pUIScaling->begin_edit();
            if (prefer)
                self->pUIScalingHost->begin_edit();
            lsp_finally {
                self->pUIScaling->end_edit();
                if (prefer)
                    self->pUIScalingHost->end_edit();
            };

            // Do the logic
            if (prefer)
            {
                const ssize_t value     = self->pUIScaling->value();
                const ssize_t new_value = self->pWrapper->ui_scaling_factor(value);

                self->pUIScaling->set_value(new_value);
                self->pUIScaling->notify_all(ui::PORT_USER_EDIT);
            }
            self->pUIScalingHost->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t UIScaling::slot_ui_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data)
        {
            UIScaling * const self   = static_cast<UIScaling *>(ptr);
            if ((self == NULL) || (self->pUIScaling == NULL))
                return STATUS_OK;

            ssize_t value       = self->pUIScaling->value();
            ssize_t new_value   = ((value / SCALING_FACTOR_STEP) + 1) * SCALING_FACTOR_STEP;
            value               = lsp_limit(new_value , SCALING_FACTOR_BEGIN, SCALING_FACTOR_END);

            // Wrap with begin_edit()/end_edit()
            self->pUIScalingHost->begin_edit();
            self->pUIScaling->begin_edit();
            lsp_finally {
                self->pUIScalingHost->end_edit();
                self->pUIScaling->end_edit();
            };

            // Do logic
            self->pUIScalingHost->set_value(0.0f);
            self->pUIScaling->set_value(value);

            self->pUIScalingHost->notify_all(ui::PORT_USER_EDIT);
            self->pUIScaling->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t UIScaling::slot_ui_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data)
        {
            UIScaling *const self    = static_cast<UIScaling *>(ptr);
            if ((self == NULL) || (self->pUIScaling == NULL))
                return STATUS_OK;

            ssize_t value       = self->pUIScaling->value();
            ssize_t new_value   = ((value / SCALING_FACTOR_STEP) - 1) * SCALING_FACTOR_STEP;
            value               = lsp_limit(new_value, SCALING_FACTOR_BEGIN, SCALING_FACTOR_END);

            // Wrap with begin_edit()/end_edit()
            self->pUIScalingHost->begin_edit();
            self->pUIScaling->begin_edit();
            lsp_finally {
                self->pUIScalingHost->end_edit();
                self->pUIScaling->end_edit();
            };

            // Do main logic
            self->pUIScalingHost->set_value(0.0f);
            self->pUIScaling->set_value(value);

            self->pUIScalingHost->notify_all(ui::PORT_USER_EDIT);
            self->pUIScaling->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t UIScaling::slot_ui_scaling_select(tk::Widget *sender, void *ptr, void *data)
        {
            scaling_sel_t *sel  = static_cast<scaling_sel_t *>(ptr);
            if (sel == NULL)
                return STATUS_OK;

            UIScaling *self     = sel->ctl;
            if ((self != NULL) && (self->pUIScaling != NULL))
            {
                // Wrap with begin_edit()/end_edit()
                self->pUIScalingHost->begin_edit();
                self->pUIScaling->begin_edit();
                lsp_finally {
                    self->pUIScalingHost->end_edit();
                    self->pUIScaling->end_edit();
                };

                // Do main logic
                self->pUIScalingHost->set_value(0.0f);
                self->pUIScaling->set_value(sel->scaling);

                self->pUIScalingHost->notify_all(ui::PORT_USER_EDIT);
                self->pUIScaling->notify_all(ui::PORT_USER_EDIT);
            }

            return STATUS_OK;
        }

        status_t UIScaling::slot_ui_scaling_show(tk::Widget *sender, void *ptr, void *data)
        {
            UIScaling * const self  = static_cast<UIScaling *>(ptr);
            return self->show_menu(self->wUIScaling, sender);
        }

        status_t UIScaling::slot_bundle_scaling_zoom_in(tk::Widget *sender, void *ptr, void *data)
        {
            UIScaling * const self  = static_cast<UIScaling *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            // Get actual scaling value
            ssize_t value       = self->get_bundle_scaling();
            if (value < 0)
                return STATUS_OK;

            // Update value and commit
            ssize_t new_value   = ((value / SCALING_FACTOR_STEP) + 1) * SCALING_FACTOR_STEP;
            value               = lsp_limit(new_value , SCALING_FACTOR_BEGIN, SCALING_FACTOR_END);

            self->pBundleScaling->begin_edit();
            self->pBundleScaling->set_value(value);
            self->pBundleScaling->notify_all(ui::PORT_USER_EDIT);
            self->pBundleScaling->end_edit();

            return STATUS_OK;
        }

        status_t UIScaling::slot_bundle_scaling_zoom_out(tk::Widget *sender, void *ptr, void *data)
        {
            UIScaling * const self  = static_cast<UIScaling *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            // Get actual scaling value
            ssize_t value       = self->get_bundle_scaling();
            if (value < 0)
                return STATUS_OK;

            // Update value and commit
            ssize_t new_value   = ((value / SCALING_FACTOR_STEP) - 1) * SCALING_FACTOR_STEP;
            value               = lsp_limit(new_value , SCALING_FACTOR_BEGIN, SCALING_FACTOR_END);

            self->pBundleScaling->begin_edit();
            self->pBundleScaling->set_value(value);
            self->pBundleScaling->notify_all(ui::PORT_USER_EDIT);
            self->pBundleScaling->end_edit();

            return STATUS_OK;
        }

        status_t UIScaling::slot_bundle_scaling_select(tk::Widget *sender, void *ptr, void *data)
        {
            scaling_sel_t * const sel   = static_cast<scaling_sel_t *>(ptr);
            if (sel == NULL)
                return STATUS_OK;

            UIScaling * const self  = sel->ctl;
            if ((self != NULL) && (self->pBundleScaling != NULL))
            {
                self->pBundleScaling->begin_edit();
                self->pBundleScaling->set_value(sel->scaling);
                self->pBundleScaling->notify_all(ui::PORT_USER_EDIT);
                self->pBundleScaling->end_edit();
            }

            return STATUS_OK;
        }

        status_t UIScaling::slot_bundle_scaling_show(tk::Widget *sender, void *ptr, void *data)
        {
            UIScaling * const self  = static_cast<UIScaling *>(ptr);
            return self->show_menu(self->wBundleScaling, sender);
        }

    } /* namespace ctl */
} /* namespace lsp */


