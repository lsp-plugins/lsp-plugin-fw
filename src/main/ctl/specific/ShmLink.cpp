/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 17 авг. 2024 г.
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
#include <lsp-plug.in/plug-fw/meta/func.h>

#include <private/ui/xml/RootNode.h>
#include <private/ui/xml/Handler.h>

#define SHMLINK_STYLE_CONNECTED     "ShmLink::Connected"
#define SHMLINK_STYLE_NOT_CONNECTED "ShmLink::NotConnected"

namespace lsp
{
    //---------------------------------------------------------------------
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(ShmLink)
            status_t res;

            if (!name->equals_ascii("shmlink"))
                return STATUS_NOT_FOUND;

            tk::Button *w = new tk::Button(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::ShmLink *wc    = new ctl::ShmLink(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(ShmLink)

        //-----------------------------------------------------------------
        const tk::w_class_t ShmLink::PopupWindow::metadata      = { "ShmLink::PopupWindow", &tk::PopupWindow::metadata };

        ShmLink::PopupWindow::PopupWindow(ShmLink *link, ui::IWrapper *wrapper, tk::Display *dpy):
            tk::PopupWindow(dpy)
        {
            pClass          = &metadata;
            pWrapper        = wrapper;

            pLink           = link;
        }

        ShmLink::PopupWindow::~PopupWindow()
        {
            pLink           = NULL;
        }

        status_t ShmLink::PopupWindow::init()
        {
            // Initialize components
            status_t res = tk::PopupWindow::init();
            if (res != STATUS_OK)
                return res;

            // Create controller
            ctl::Window *wc = new ctl::Window(pWrapper, this);
            if (wc == NULL)
                return STATUS_NO_MEM;
            sControllers.add(wc);
            wc->init();

            ui::UIContext uctx(pWrapper, wc->controllers(), wc->widgets());
            if ((res = uctx.init()) != STATUS_OK)
                return res;

            // Parse the XML document
            ui::xml::RootNode root(&uctx, "window", wc);
            ui::xml::Handler handler(pWrapper->resources());
            if ((res = handler.parse_resource(LSP_BUILTIN_PREFIX "ui/shmlink.xml", &root)) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        void ShmLink::PopupWindow::destroy()
        {
            sControllers.destroy();
            sWidgets.destroy();

            tk::PopupWindow::destroy();
        }

        //-----------------------------------------------------------------
        const ctl_class_t ShmLink::metadata     = { "ShmLink", &Widget::metadata };

        const tk::tether_t ShmLink::popup_tether[] =
        {
            { tk::TF_LEFT | tk::TF_BOTTOM | tk::TF_HORIZONTAL,      1.0f,  1.0f    },
            { tk::TF_LEFT | tk::TF_TOP | tk::TF_HORIZONTAL,         1.0f, -1.0f    },
            { tk::TF_RIGHT | tk::TF_BOTTOM | tk::TF_HORIZONTAL,    -1.0f,  1.0f    },
            { tk::TF_RIGHT | tk::TF_TOP | tk::TF_HORIZONTAL,       -1.0f, -1.0f    },
        };

        ShmLink::ShmLink(ui::IWrapper *wrapper, tk::Button *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            wPopup          = NULL;
        }

        ShmLink::~ShmLink()
        {
            do_destroy();
        }

        void ShmLink::destroy()
        {
            do_destroy();
        }

        void ShmLink::do_destroy()
        {
            // Destroy popup window
            if (wPopup != NULL)
            {
                wPopup->destroy();
                delete wPopup;
                wPopup = NULL;
            }
        }

        status_t ShmLink::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn != NULL)
            {
                sColor.init(pWrapper, btn->color());
                sTextColor.init(pWrapper, btn->text_color());
                sBorderColor.init(pWrapper, btn->border_color());
                sHoverColor.init(pWrapper, btn->hover_color());
                sTextHoverColor.init(pWrapper, btn->text_hover_color());
                sBorderHoverColor.init(pWrapper, btn->border_hover_color());
                sDownColor.init(pWrapper, btn->down_color());
                sTextDownColor.init(pWrapper, btn->text_down_color());
                sBorderDownColor.init(pWrapper, btn->border_down_color());
                sDownHoverColor.init(pWrapper, btn->down_hover_color());
                sTextDownHoverColor.init(pWrapper, btn->text_down_hover_color());
                sBorderDownHoverColor.init(pWrapper, btn->border_down_hover_color());
                sHoleColor.init(pWrapper, btn->hole_color());

                sEditable.init(pWrapper, btn->editable());

                // Set style
                inject_style(btn, SHMLINK_STYLE_NOT_CONNECTED);

                // Bind slots
                btn->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
            }

            return STATUS_OK;
        }

        void ShmLink::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sColor.set("color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sHoverColor.set("hover.color", name, value);
                sHoverColor.set("hcolor", name, value);
                sTextHoverColor.set("text.hover.color", name, value);
                sTextHoverColor.set("thcolor", name, value);
                sBorderHoverColor.set("border.hover.color", name, value);
                sBorderHoverColor.set("bhcolor", name, value);
                sDownColor.set("down.color", name, value);
                sDownColor.set("dcolor", name, value);
                sTextDownColor.set("text.down.color", name, value);
                sTextDownColor.set("tdcolor", name, value);
                sBorderDownColor.set("border.down.color", name, value);
                sBorderDownColor.set("bdcolor", name, value);
                sDownHoverColor.set("down.hover.color", name, value);
                sDownHoverColor.set("dhcolor", name, value);
                sTextDownHoverColor.set("text.down.hover.color", name, value);
                sTextDownHoverColor.set("tdhcolor", name, value);
                sBorderDownHoverColor.set("border.down.hover.color", name, value);
                sBorderDownHoverColor.set("bdhcolor", name, value);

                sHoleColor.set("hole.color", name, value);

                sEditable.set("editable", name, value);
                sHover.set("hover", name, value);

                set_font(btn->font(), "font", name, value);
                set_constraints(btn->constraints(), name, value);
                set_param(btn->led(), "led", name, value);
                set_param(btn->hole(), "hole", name, value);
                set_param(btn->flat(), "flat", name, value);
                set_param(btn->text_clip(), "text.clip", name, value);
                set_param(btn->text_adjust(), "text.adjust", name, value);
                set_param(btn->text_clip(), "tclip", name, value);
                set_param(btn->font_scaling(), "font.scaling", name, value);
                set_param(btn->font_scaling(), "font.scale", name, value);
                set_param(btn->mode(), "mode", name, value);
                set_text_layout(btn->text_layout(), name, value);
            }

            Widget::set(ctx, name, value);
        }

        void ShmLink::end(ui::UIContext *ctx)
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn == NULL)
                return;

            btn->mode()->set_normal();
            sync_state();

            Widget::end(ctx);
        }

        void ShmLink::notify(ui::IPort *port, size_t flags)
        {
            if ((port == pPort) && (pPort != NULL))
                sync_state();
        }

        void ShmLink::sync_state()
        {
            tk::Button *btn = tk::widget_cast<tk::Button>(wWidget);
            if (btn == NULL)
                return;

            revoke_style(btn, SHMLINK_STYLE_CONNECTED);
            revoke_style(btn, SHMLINK_STYLE_NOT_CONNECTED);

            const char *lc_key = "labels.link.not_connected";
            const char *btn_style = SHMLINK_STYLE_NOT_CONNECTED;
            btn->text()->params()->clear();

            if ((pPort != NULL) && (meta::is_string_holding_port(pPort->metadata())))
            {
                const char *name = pPort->buffer<char>();
                btn->text()->params()->add_cstring("value", name);
                lc_key      = "labels.link.connected";
                btn_style   = SHMLINK_STYLE_CONNECTED;
            }

            btn->text()->set(lc_key);
            inject_style(btn, btn_style);
        }

        ShmLink::PopupWindow *ShmLink::create_popup_window()
        {
            if (wPopup != NULL)
                return wPopup;

            // Create window
            PopupWindow *w = new PopupWindow(this, pWrapper, wWidget->display());
            if (w == NULL)
                return NULL;
            lsp_finally {
                if (w != NULL)
                {
                    w->destroy();
                    delete w;
                }
            };

            status_t res = w->init();
            if (res != STATUS_OK)
                return NULL;

            lsp_trace("Created shmlink popup window ptr=%p", static_cast<tk::Widget *>(w));
            wPopup = release_ptr(w);

            return wPopup;
        }

        void ShmLink::init_popup_window()
        {
        }

        void ShmLink::show_selector()
        {
            if (wWidget == NULL)
                return;

            PopupWindow *popup      = create_popup_window();
            if (popup == NULL)
                return;

            init_popup_window();

            // Show the window and take focus
            ws::rectangle_t r;
            wWidget->get_padded_screen_rectangle(&r);
            popup->trigger_area()->set(&r);
            popup->trigger_widget()->set(wWidget);
            popup->set_tether(popup_tether, sizeof(popup_tether)/sizeof(tk::tether_t));
            popup->show(wWidget);
            popup->take_focus();
            popup->grab_events(ws::GRAB_DROPDOWN);
        }

        status_t ShmLink::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::ShmLink *_this     = static_cast<ctl::ShmLink *>(ptr);
            if (_this != NULL)
                _this->show_selector();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


