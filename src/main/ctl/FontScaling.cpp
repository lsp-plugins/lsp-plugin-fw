/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 февр. 2026 г.
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

namespace lsp
{
    namespace ctl
    {
        static constexpr ssize_t FONT_SCALING_FACTOR_BEGIN      = 50;
        static constexpr ssize_t FONT_SCALING_FACTOR_STEP       = 10;
        static constexpr ssize_t FONT_SCALING_FACTOR_END        = 200;

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

        FontScaling::FontScaling(ui::IWrapper *src)
        {
            pWrapper            = src;
            wMenu               = NULL;
            pFontScaling        = NULL;
        }

        FontScaling::~FontScaling()
        {
            destroy();
        }

        void FontScaling::destroy()
        {
            // Delete UI font scaling bindings
            for (size_t i=0, n=vFontScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *s = vFontScalingSel.uget(i);
                if (s != NULL)
                    delete s;
            }
            vFontScalingSel.flush();
        }

        status_t FontScaling::init()
        {
            // Bint ports
            BIND_PORT(pWrapper, pFontScaling, UI_FONT_SCALING_PORT);

            // Create menu
            if ((wMenu = create_menu()) == NULL)
                return STATUS_NO_MEM;

            // Add the 'Zoom In' setting
            tk::MenuItem *item;
            if ((item = create_menu_item()) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.font_scaling.zoom_in");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_zoom_in, this);

            // Add the 'Zoom out' setting
            if ((item = create_menu_item()) == NULL)
                return STATUS_NO_MEM;
            item->text()->set_key("actions.font_scaling.zoom_out");
            item->slots()->bind(tk::SLOT_SUBMIT, slot_zoom_out, this);

            // Add the separator
            if ((item = create_menu_item()) == NULL)
                return STATUS_NO_MEM;
            item->type()->set_separator();

            // Generate the 'Set font scaling' menu items
            for (size_t scale = FONT_SCALING_FACTOR_BEGIN; scale <= FONT_SCALING_FACTOR_END; scale += FONT_SCALING_FACTOR_STEP)
                LSP_STATUS_ASSERT(add_scaling_menu_item(scale));

            sync_parameters();

            return STATUS_OK;
        }

        tk::MenuItem *FontScaling::create_menu_item()
        {
            tk::MenuItem *item = new tk::MenuItem(pWrapper->display());
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

            wMenu->add(item);

            return release_ptr(item);
        }

        tk::Menu *FontScaling::create_menu()
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

        status_t FontScaling::add_scaling_menu_item(size_t scale)
        {
            tk::MenuItem * const item = create_menu_item();
            if (item == NULL)
                return STATUS_NO_MEM;
            item->type()->set_radio();
            item->text()->set_key("actions.font_scaling.value:pc");
            item->text()->params()->set_int("value", scale);

            // Add scaling record
            scaling_sel_t * const sel = new scaling_sel_t();
            if (sel == NULL)
                return STATUS_NO_MEM;

            sel->ctl        = this;
            sel->scaling    = scale;
            sel->item       = item;

            if (!vFontScalingSel.add(sel))
            {
                delete sel;
                return STATUS_NO_MEM;
            }

            item->slots()->bind(tk::SLOT_SUBMIT, slot_select, sel);

            return STATUS_OK;
        }

        status_t FontScaling::show_menu(tk::Widget *actor)
        {
            if (actor != NULL)
            {
                ws::rectangle_t xr, wr;

                pWrapper->window()->get_rectangle(&wr);
                actor->get_rectangle(&xr);

                if (xr.nTop > (wr.nHeight >> 1))
                    wMenu->set_tether(bottom_tether, sizeof(bottom_tether)/sizeof(tk::tether_t));
                else
                    wMenu->set_tether(top_tether, sizeof(top_tether)/sizeof(tk::tether_t));
                wMenu->show(actor);
            }
            else
                wMenu->show();

            return STATUS_OK;
        }

        status_t FontScaling::bind_event(const char *id, tk::slot_t slot, tk::event_handler_t handler)
        {
            tk::Widget *w = pWrapper->controller()->widgets()->find(id);
            if (w == NULL)
                return STATUS_NOT_FOUND;

            tk::handler_id_t ev = w->slots()->bind(slot, handler, this);
            return (ev >= 0) ? STATUS_OK : STATUS_NOT_BOUND;
        }

        void FontScaling::notify(ui::IPort *port, size_t flags)
        {
            if ((port == NULL) || (port != pFontScaling))
                return;

            tk::Display * const dpy = pWrapper->display();
            if (dpy == NULL)
                return;

            float scaling           = (pFontScaling != NULL) ? pFontScaling->value() : 100.0f;

            // Update the UI scaling
            dpy->schema()->font_scaling()->set(scaling * 0.01f);
            scaling                 = dpy->schema()->font_scaling()->get() * 100.0f;

            for (size_t i=0, n=vFontScalingSel.size(); i<n; ++i)
            {
                scaling_sel_t *xsel = vFontScalingSel.uget(i);
                if (xsel->item != NULL)
                    xsel->item->checked()->set(fabs(xsel->scaling - scaling) < 1e-4f);
            }
        }

        void FontScaling::sync_parameters()
        {
            if (pFontScaling != NULL)
                notify(pFontScaling, ui::PORT_NONE);
        }

        status_t FontScaling::slot_zoom_in(tk::Widget *sender, void *ptr, void *data)
        {
            FontScaling * const self = static_cast<FontScaling *>(ptr);
            if ((self == NULL) || (self->pFontScaling == NULL))
                return STATUS_OK;

            ssize_t value   = self->pFontScaling->value();
            value           = lsp_limit(value + FONT_SCALING_FACTOR_STEP, FONT_SCALING_FACTOR_BEGIN, FONT_SCALING_FACTOR_END);

            self->pFontScaling->begin_edit();
            self->pFontScaling->set_value(value);
            self->pFontScaling->notify_all(ui::PORT_USER_EDIT);
            self->pFontScaling->end_edit();

            return STATUS_OK;
        }

        status_t FontScaling::slot_zoom_out(tk::Widget *sender, void *ptr, void *data)
        {
            FontScaling * const self = static_cast<FontScaling *>(ptr);
            if ((self == NULL) || (self->pFontScaling == NULL))
                return STATUS_OK;

            ssize_t value   = self->pFontScaling->value();
            value           = lsp_limit(value - FONT_SCALING_FACTOR_STEP, FONT_SCALING_FACTOR_BEGIN, FONT_SCALING_FACTOR_END);

            self->pFontScaling->begin_edit();
            self->pFontScaling->set_value(value);
            self->pFontScaling->notify_all(ui::PORT_USER_EDIT);
            self->pFontScaling->end_edit();

            return STATUS_OK;
        }

        status_t FontScaling::slot_select(tk::Widget *sender, void *ptr, void *data)
        {
            scaling_sel_t * const sel  = static_cast<scaling_sel_t *>(ptr);
            if (sel == NULL)
                return STATUS_OK;

            FontScaling * const self = sel->ctl;
            if ((self != NULL) && (self->pFontScaling != NULL))
            {
                self->pFontScaling->begin_edit();
                self->pFontScaling->set_value(sel->scaling);
                self->pFontScaling->notify_all(ui::PORT_USER_EDIT);
                self->pFontScaling->end_edit();
            }

            return STATUS_OK;
        }

        status_t FontScaling::slot_show(tk::Widget *sender, void *ptr, void *data)
        {
            FontScaling * const self = static_cast<FontScaling *>(ptr);
            return self->show_menu(sender);
        }
    } /* namespace ctl */
} /* namespace lsp */

