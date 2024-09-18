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

#define SHMLINK_STYLE_CONNECTED             "ShmLink::Connected"
#define SHMLINK_STYLE_CONNECTED_SEND        "ShmLink::Connected::Send"
#define SHMLINK_STYLE_CONNECTED_RETURN      "ShmLink::Connected::Return"
#define SHMLINK_STYLE_NOT_CONNECTED         "ShmLink::NotConnected"

#define SHMLINK_FILTER_VALID                "ShmLink::Filter::ValidInput"
#define SHMLINK_FILTER_INVALID              "ShmLink::Filter::InvalidInput"

#define SHMLINK_ITEM_CONNECTED              "ShmLink::ListBoxItem::Connected"

namespace lsp
{
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
        const tk::tether_t ShmLink::popup_tether[] =
        {
            { tk::TF_LEFT | tk::TF_BOTTOM | tk::TF_HORIZONTAL,      1.0f,  1.0f    },
            { tk::TF_LEFT | tk::TF_TOP | tk::TF_HORIZONTAL,         1.0f, -1.0f    },
            { tk::TF_RIGHT | tk::TF_BOTTOM | tk::TF_HORIZONTAL,    -1.0f,  1.0f    },
            { tk::TF_RIGHT | tk::TF_TOP | tk::TF_HORIZONTAL,       -1.0f, -1.0f    },
        };

        const tk::w_class_t ShmLink::Selector::metadata      = { "ShmLink::Selector", &tk::PopupWindow::metadata };

        ShmLink::Selector::Selector(ShmLink *link, ui::IWrapper *wrapper, tk::Display *dpy):
            tk::PopupWindow(dpy)
        {
            pClass          = &metadata;
            pWrapper        = wrapper;

            pLink           = link;

            wName           = NULL;
            wConnections    = NULL;
            wConnect        = NULL;
            wDisconnect     = NULL;
        }

        ShmLink::Selector::~Selector()
        {
            pLink           = NULL;
        }

        status_t ShmLink::Selector::init()
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

            ui::UIContext uctx(pWrapper, &sControllers, &sWidgets);
            if ((res = uctx.init()) != STATUS_OK)
                return res;

            // Parse the XML document
            ui::xml::RootNode root(&uctx, "window", wc);
            ui::xml::Handler handler(pWrapper->resources());
            if ((res = handler.parse_resource(LSP_BUILTIN_PREFIX "ui/shmlink.xml", &root)) != STATUS_OK)
                return res;

            // Bind widgets
            wName           = sWidgets.get<tk::Edit>("name");
            wConnections    = sWidgets.get<tk::ListBox>("connections");
            wConnect        = sWidgets.get<tk::Button>("connect");
            wDisconnect     = sWidgets.get<tk::Button>("disconnect");

            if (wName != NULL)
            {
                wName->slots()->bind(tk::SLOT_CHANGE, slot_filter_change, this);
                wName->slots()->bind(tk::SLOT_KEY_UP, slot_key_up, this);
            }
            if (wConnections != NULL)
            {
                wConnections->slots()->bind(tk::SLOT_SUBMIT, slot_connections_submit, this);
                wConnections->slots()->bind(tk::SLOT_KEY_UP, slot_key_up, this);
            }
            if (wConnect != NULL)
            {
                wConnect->slots()->bind(tk::SLOT_SUBMIT, slot_connect, this);
                wConnect->slots()->bind(tk::SLOT_KEY_UP, slot_key_up, this);
            }
            if (wDisconnect != NULL)
            {
                wDisconnect->slots()->bind(tk::SLOT_SUBMIT, slot_disconnect, this);
                wDisconnect->slots()->bind(tk::SLOT_KEY_UP, slot_key_up, this);
            }

            return STATUS_OK;
        }

        void ShmLink::Selector::destroy()
        {
            sControllers.destroy();
            sWidgets.destroy();

            tk::PopupWindow::destroy();
        }

        void ShmLink::Selector::show(tk::Widget *actor)
        {
            // Apply filter
            if (wName != NULL)
                wName->text()->clear();
            apply_filter();

            // Show the window and take focus
            ws::rectangle_t r;
            actor->get_padded_screen_rectangle(&r);
            trigger_area()->set(&r);
            trigger_widget()->set(actor);
            set_tether(popup_tether, sizeof(popup_tether)/sizeof(tk::tether_t));
            tk::PopupWindow::show(actor);
            take_focus();
            if (wName != NULL)
                wName->take_focus();
//            grab_events(ws::GRAB_DROPDOWN);
        }

        ssize_t ShmLink::Selector::compare_strings(const LSPString *a, const LSPString *b)
        {
            ssize_t res = a->compare_to_nocase(b);
            return (res != 0) ? res : a->compare_to(b);
        }

        bool ShmLink::is_valid_ending_char(lsp_wchar_t c)
        {
            switch (c)
            {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    return false;
            };

            return true;
        }

        const char *ShmLink::valid_name(const LSPString *s)
        {
            if (s == NULL)
                return NULL;

            // Validate length
            const size_t length = s->length();
            if (length <= 0)
                return "";
            else if (length > MAX_SHM_SEGMENT_NAME_CHARS)
                return NULL;

            // Validate not containing spaces
            if (!is_valid_ending_char(s->first()))
                return NULL;
            if (!is_valid_ending_char(s->last()))
                return NULL;

            // Validate size in bytes
            const char *utf8 = s->get_utf8();
            if (utf8 == NULL)
                return NULL;
            if (strlen(utf8) >= MAX_SHM_SEGMENT_NAME_BYTES)
                return NULL;

            return utf8;
        }

        void ShmLink::shorten_name(LSPString *dst, const char *name)
        {
            dst->set_utf8(name);
            const size_t length = dst->length();
            const size_t max_length = lsp_max(nMaxNameLength, 2u);
            if (length <= max_length)
                return;

            size_t prefix = lsp_max((max_length * 3) / 4, 1u);
            size_t suffix = max_length - prefix;

            // Subtract one character for '...'
            if (suffix > 2)
                --suffix;
            else if (prefix > 2)
                --prefix;
            else
                return;

            dst->set(prefix, 0x2026);
            dst->remove(prefix + 1, dst->length() - suffix);
        }

        void ShmLink::Selector::apply_filter()
        {
            status_t res;
            LSPString filter, tmp, name;

            // Apply widget visibility
            if (wName != NULL)
            {
                wName->text()->format(&filter);

                // Update style depending on the length of the filter
                revoke_style(wName, SHMLINK_FILTER_VALID);
                revoke_style(wName, SHMLINK_FILTER_INVALID);
                inject_style(wName, (valid_name(&filter) != NULL) ? SHMLINK_FILTER_VALID : SHMLINK_FILTER_INVALID);
            }

            if (wDisconnect != NULL)
                wDisconnect->visibility()->set(filter.length() <= 0);
            if (wConnect != NULL)
                wConnect->visibility()->set(filter.length() > 0);

            // Get actual name of filter
            ui::IPort *port = (pLink != NULL) ? pLink->pPort : NULL;
            if ((port != NULL) && (meta::is_string_holding_port(port->metadata())))
            {
                const char *conn_name = port->buffer<char>();
                if (conn_name != NULL)
                    name.set_utf8(conn_name);
            }

            // Fill list of connections
            if (wConnections != NULL)
            {
                // Form the list of items that match the filter
                lltl::parray<LSPString> list;
                lsp_finally {
                    for (size_t i=0, n=list.size(); i<n; ++i)
                    {
                        LSPString *s    = list.uget(i);
                        if (s != NULL)
                            delete s;
                    }
                };

                const core::ShmState *state = pWrapper->shm_state();
                if (state != NULL)
                {
                    for (size_t i=0, n=state->size(); i<n; ++i)
                    {
                        const core::ShmRecord *item = state->get(i);
                        if (!tmp.set_utf8(item->name))
                            return;

                        if (tmp.index_of_nocase(&filter) < 0)
                            continue;

                        LSPString *s = tmp.clone();
                        if (s == NULL)
                            return;
                        if (!list.add(s))
                        {
                            delete s;
                            return;
                        }
                    }

                    // Sort the list
                    list.qsort(compare_strings);
                }

                // Fill the widget
                wConnections->items()->clear();

                for (size_t i=0, n=list.size(); i<n; ++i)
                {
                    LSPString *conn = list.uget(i);
                    if (conn == NULL)
                        return;

                    // Create item
                    tk::ListBoxItem *li = new tk::ListBoxItem(wConnections->display());
                    if (li == NULL)
                        return;
                    lsp_finally {
                        if (li != NULL)
                        {
                            li->destroy();
                            delete li;
                        }
                    };
                    if ((res = li->init()) != STATUS_OK)
                        return;

                    if ((res = wConnections->items()->madd(li)) != STATUS_OK)
                        return;

                    // Set text and update style
                    li->text()->set_raw(conn);
                    if (conn->equals(&name))
                        inject_style(li, SHMLINK_ITEM_CONNECTED);

                    li  = NULL;
                }
            }
        }

        void ShmLink::Selector::connect_by_list()
        {
            if (wConnections == NULL)
                return;
            if (wConnections->selected()->size() < 1)
                return;

            lsp_finally { hide(); };

            ui::IPort *port = (pLink != NULL) ? pLink->pPort : NULL;
            if (port == NULL)
                return;

            tk::ListBoxItem *item = wConnections->selected()->any();
            if (item == NULL)
                return;

            LSPString name;
            if (item->text()->format(&name) != STATUS_OK)
                return;

            const char *c_name = valid_name(&name);
            if (c_name == NULL)
                c_name = "";

            lsp_trace("write value '%s' to port id=%s", c_name, port->id());

            port->write(c_name, strlen(c_name));
            port->notify_all(ui::PORT_NONE);
        }

        void ShmLink::Selector::connect_by_filter()
        {
            lsp_finally { hide(); };

            if (wName == NULL)
                return;

            ui::IPort *port = (pLink != NULL) ? pLink->pPort : NULL;
            if (port == NULL)
                return;

            LSPString name;
            if (wName->text()->format(&name) != STATUS_OK)
                return;

            const char *c_name = valid_name(&name);
            if (c_name == NULL)
                c_name = "";

            lsp_trace("write value '%s' to port id=%s", c_name, port->id());

            port->write(c_name, strlen(c_name));
            port->notify_all(ui::PORT_NONE);
        }

        void ShmLink::Selector::disconnect()
        {
            lsp_finally { hide(); };

            ui::IPort *port = (pLink != NULL) ? pLink->pPort : NULL;
            if (port == NULL)
                return;

            port->set_default();
            port->notify_all(ui::PORT_NONE);
        }

        status_t ShmLink::Selector::slot_filter_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::ShmLink::Selector *_this   = static_cast<ctl::ShmLink::Selector *>(ptr);
            if (_this != NULL)
                _this->apply_filter();
            return STATUS_OK;
        }

        status_t ShmLink::Selector::slot_connections_submit(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::ShmLink::Selector *_this   = static_cast<ctl::ShmLink::Selector *>(ptr);
            if (_this != NULL)
                _this->connect_by_list();
            return STATUS_OK;
        }

        status_t ShmLink::Selector::slot_connect(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::ShmLink::Selector *_this   = static_cast<ctl::ShmLink::Selector *>(ptr);
            if (_this != NULL)
                _this->connect_by_filter();
            return STATUS_OK;
        }

        status_t ShmLink::Selector::slot_disconnect(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::ShmLink::Selector *_this   = static_cast<ctl::ShmLink::Selector *>(ptr);
            if (_this != NULL)
                _this->disconnect();
            return STATUS_OK;
        }

        status_t ShmLink::Selector::slot_key_up(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            Selector *self = static_cast<Selector *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            // Should be keyboard event
            ws::event_t *ev = reinterpret_cast<ws::event_t *>(data);
            if ((ev == NULL) || (ev->nType != ws::UIE_KEY_UP))
                return STATUS_BAD_ARGUMENTS;

            // Do the action
            return self->process_key_up(sender, ev);
        }

        status_t ShmLink::Selector::process_key_up(tk::Widget *sender, const ws::event_t *ev)
        {
            const ws::code_t key = tk::KeyboardHandler::translate_keypad(ev->nCode);
            if (key == ws::WSK_RETURN)
            {
                hide();

                if ((wName != NULL) && (!wName->text()->is_empty()))
                    connect_by_filter();
                else
                    disconnect();
            }
            else if (key == ws::WSK_ESCAPE)
                hide();

            return STATUS_OK;
        }

        //-----------------------------------------------------------------
        const ctl_class_t ShmLink::metadata     = { "ShmLink", &Widget::metadata };

        ShmLink::ShmLink(ui::IWrapper *wrapper, tk::Button *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            wPopup          = NULL;

            nMaxNameLength  = 12;
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

                set_value(&nMaxNameLength, "value.max_length", name, value);
                set_value(&nMaxNameLength, "value.maxlen", name, value);

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

            const meta::port_t *port = (pPort != NULL) ? pPort->metadata() : NULL;

            LSPString tmp;

            revoke_style(btn, SHMLINK_STYLE_CONNECTED);
            revoke_style(btn, SHMLINK_STYLE_CONNECTED_SEND);
            revoke_style(btn, SHMLINK_STYLE_CONNECTED_RETURN);
            revoke_style(btn, SHMLINK_STYLE_NOT_CONNECTED);

            const char *lc_key = (meta::is_send_name(port)) ?
                "labels.link.send.not_connected" :
                "labels.link.return.not_connected";
            const char *btn_style = SHMLINK_STYLE_NOT_CONNECTED;
            btn->text()->params()->clear();

            if ((pPort != NULL) && (meta::is_string_holding_port(pPort->metadata())))
            {
                const char *name = pPort->buffer<char>();
                if ((name != NULL) && (strlen(name) > 0))
                {
                    shorten_name(&tmp, name);

                    btn->text()->params()->add_string("value", &tmp);

                    // Set style depending on the type of metadata
                    if (meta::is_send_name(port))
                    {
                        lc_key      = "labels.link.send.connected";
                        btn_style   = SHMLINK_STYLE_CONNECTED_SEND;
                    }
                    else if (meta::is_return_name(port))
                    {
                        lc_key      = "labels.link.return.connected";
                        btn_style   = SHMLINK_STYLE_CONNECTED_RETURN;
                    }
                    else
                    {
                        lc_key      = "labels.link.other.connected";
                        btn_style   = SHMLINK_STYLE_CONNECTED;
                    }
                }
            }

            btn->text()->set_key(lc_key);
            inject_style(btn, btn_style);

            // Update widget size estimations
            btn->clear_text_estimations();
            tk::String *est = btn->add_text_estimation();
            if (est != NULL)
            {
                tmp.clear();
                for (size_t i=0, n=lsp_max(nMaxNameLength, 2u); i<n; ++i)
                    tmp.append('W');

                est->set_key("labels.link.send.connected");
                est->params()->add_string("value", &tmp);
            }
        }

        ShmLink::Selector *ShmLink::create_selector()
        {
            if (wPopup != NULL)
                return wPopup;

            // Create window
            Selector *w = new Selector(this, pWrapper, wWidget->display());
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

        void ShmLink::show_selector()
        {
            if (wWidget == NULL)
                return;

            Selector *popup      = create_selector();
            if (popup != NULL)
                popup->show(wWidget);
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


