/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 июл. 2021 г.
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
#include <lsp-plug.in/plug-fw/meta/func.h>

#define TMP_BUF_SIZE            128

#define INPUT_STYLE_VALID       "MidiNote::PopupWindow::ValidInput"
#define INPUT_STYLE_INVALID     "MidiNote::PopupWindow::InvalidInput"
#define INPUT_STYLE_MISMATCH    "MidiNote::PopupWindow::MismatchInput"

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(MidiNote)
            status_t res;

            if (!name->equals_ascii("midinote"))
                return STATUS_NOT_FOUND;

            tk::Indicator *w = new tk::Indicator(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::MidiNote *wc  = new ctl::MidiNote(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(MidiNote);

        //-----------------------------------------------------------------
        MidiNote::PopupWindow::PopupWindow(MidiNote *label, tk::Display *dpy):
            tk::PopupWindow(dpy),
            sBox(dpy),
            sValue(dpy),
            sUnits(dpy),
            sApply(dpy),
            sCancel(dpy)
        {
            pClass          = &metadata;

            pLabel          = label;
        }

        MidiNote::PopupWindow::~PopupWindow()
        {
            pLabel          = NULL;
        }

        status_t MidiNote::PopupWindow::init()
        {
            // Initialize components
            status_t res = tk::PopupWindow::init();
            if (res == STATUS_OK)
                res = sBox.init();
            if (res == STATUS_OK)
                res = sValue.init();
            if (res == STATUS_OK)
                res = sUnits.init();
            if (res == STATUS_OK)
                res = sApply.init();
            if (res == STATUS_OK)
                res = sCancel.init();

            if (res != STATUS_OK)
                return res;

            inject_style(&sBox, "MidiNote::PopupWindow::Box");
            sBox.add(&sValue);
            sBox.add(&sUnits);
            sBox.add(&sApply);
            sBox.add(&sCancel);

            this->slots()->bind(tk::SLOT_MOUSE_DOWN, MidiNote::slot_mouse_button, pLabel);
            this->slots()->bind(tk::SLOT_MOUSE_UP, MidiNote::slot_mouse_button, pLabel);

            sValue.slots()->bind(tk::SLOT_KEY_UP, MidiNote::slot_key_up, pLabel);
            sValue.slots()->bind(tk::SLOT_CHANGE, MidiNote::slot_change_value, pLabel);
            inject_style(&sValue, INPUT_STYLE_VALID);

            inject_style(&sUnits, "MidiNote::PopupWindow::Units");

            sApply.text()->set("actions.apply");
            sApply.slots()->bind(tk::SLOT_SUBMIT, MidiNote::slot_submit_value, pLabel);
            inject_style(&sApply, "MidiNote::PopupWindow::Apply");

            sCancel.text()->set("actions.cancel");
            sCancel.slots()->bind(tk::SLOT_SUBMIT, MidiNote::slot_cancel_value, pLabel);
            inject_style(&sCancel, "MidiNote::PopupWindow::Cancel");

            this->add(&sBox);
            inject_style(this, "MidiNote::PopupWindow");

            return STATUS_OK;
        }

        void MidiNote::PopupWindow::destroy()
        {
            sValue.destroy();
            sUnits.destroy();
            sApply.destroy();
            sBox.destroy();

            tk::PopupWindow::destroy();
        }

        //-----------------------------------------------------------------
        const ctl_class_t MidiNote::metadata     = { "MidiNote", &Widget::metadata };

        const tk::tether_t MidiNote::popup_tether[] =
        {
            { tk::TF_RIGHT | tk::TF_TOP,    -1.0f,  1.0f    },
            { tk::TF_RIGHT | tk::TF_BOTTOM, -1.0f, -1.0f    },
        };

        MidiNote::MidiNote(ui::IWrapper *wrapper, tk::Indicator *widget): Widget(wrapper, widget)
        {
            nNote           = 0;
            nDigits         = 3;
            pNote           = NULL;
            pOctave         = NULL;
            pValue          = NULL;
            wPopup          = NULL;
        }

        MidiNote::~MidiNote()
        {
            do_destroy();
        }

        void MidiNote::do_destroy()
        {
            // Destroy popup window
            if (wPopup != NULL)
            {
                wPopup->destroy();
                delete wPopup;
                wPopup = NULL;
            }
        }

        status_t MidiNote::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Indicator *ind = tk::widget_cast<tk::Indicator>(wWidget);
            if (ind != NULL)
            {
                sColor.init(pWrapper, ind->color());
                sTextColor.init(pWrapper, ind->text_color());

                ind->slot(tk::SLOT_MOUSE_DBL_CLICK)->bind(slot_dbl_click, this);
                ind->slot(tk::SLOT_MOUSE_SCROLL)->bind(slot_mouse_scroll, this);
            }

            return STATUS_OK;
        }

        void MidiNote::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Indicator *ind = tk::widget_cast<tk::Indicator>(wWidget);
            if (ind != NULL)
            {
                bind_port(&pValue, "id", name, value);
                bind_port(&pNote, "note_id", name, value);
                bind_port(&pNote, "note.id", name, value);
                bind_port(&pOctave, "octave_id", name, value);
                bind_port(&pOctave, "octave.id", name, value);
                bind_port(&pOctave, "oct_id", name, value);
                bind_port(&pOctave, "oct.id", name, value);

                sColor.set("color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sIPadding.set("ipadding", name, value);
                sIPadding.set("ipad", name, value);

                set_param(ind->modern(), "modern", name, value);

                set_param(ind->spacing(), "spacing", name, value);
                set_param(ind->dark_text(), "text.dark", name, value);
                set_param(ind->dark_text(), "tdark", name, value);
                set_font(ind->font(), "font", name, value);

                set_value(&nDigits, "digits", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void MidiNote::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            notify(pValue);
        }

        void MidiNote::notify(ui::IPort *port)
        {
            Widget::notify(port);

            if ((port != NULL) && (port == pValue))
            {
                commit_value(port->value());
            }
        }

        void MidiNote::commit_value(float value)
        {
            tk::Indicator *ind = tk::widget_cast<tk::Indicator>(wWidget);
            if (ind == NULL)
                return;

            nNote   = value;

            LSPString tmp;
            tmp.fmt_ascii("%d", int(nNote));

            ind->rows()->set(1);
            ind->columns()->set(nDigits);
            ind->text_shift()->set(ssize_t(tmp.length()) - ssize_t(nDigits));
            ind->text()->set_raw(tmp.get_utf8());
        }

        bool MidiNote::apply_value(const LSPString *value)
        {
            lsp_trace("Apply text value: %s", value->get_utf8());
            if (pValue == NULL)
                return false;

            const meta::port_t *meta = pValue->metadata();
            if (meta == NULL)
                return false;

            float v;
            status_t res = meta::parse_value(&v, value->get_utf8(), meta, false);
            if (res != STATUS_OK)
                return res;

            apply_value(v);
            return true;
        }

        void MidiNote::apply_value(ssize_t value)
        {
            lsp_trace("Apply value: %d", int(value));

            value           = lsp_limit(value, 0, 127);
            size_t note     = value % 12;
            size_t octave   = value / 12;

            const meta::port_t *meta;
            if (pNote != NULL)
            {
                if (((meta = pNote->metadata()) != NULL) && (meta->flags & meta::F_LOWER))
                    pNote->set_value(meta->min + note);
                else
                    pNote->set_value(note);
            }

            if (pOctave != NULL)
            {
                if (((meta = pOctave->metadata()) != NULL) && (meta->flags & meta::F_LOWER))
                    pOctave->set_value(meta->min + octave);
                else
                    pOctave->set_value(octave);
            }

            nNote           = value;
            if (pNote != NULL)
                pNote->notify_all();
            if (pOctave != NULL)
                pOctave->notify_all();
        }

        status_t MidiNote::slot_submit_value(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::MidiNote *_this = static_cast<ctl::MidiNote *>(ptr);
            if ((_this == NULL) || (_this->wPopup == NULL))
                return STATUS_OK;

            // Apply value
            PopupWindow *popup  = _this->wPopup;
            LSPString value;
            if (popup->sValue.text()->format(&value) == STATUS_OK)
            {
                // The deploy should be always successful
                if (!_this->apply_value(&value))
                    return STATUS_OK;
            }

            // Hide the popup window
            if (popup != NULL)
            {
                popup->hide();
                if (popup->queue_destroy() == STATUS_OK)
                    _this->wPopup  = NULL;
            }

            return STATUS_OK;
        }

        status_t MidiNote::slot_change_value(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::MidiNote *_this = static_cast<ctl::MidiNote *>(ptr);
            if ((_this == NULL) || (_this->wPopup == NULL))
                return STATUS_OK;

            // Get port metadata
            const meta::port_t *meta = (_this->pValue != NULL) ? _this->pValue->metadata() : NULL;
            if ((meta == NULL) || (!meta::is_in_port(meta)))
                return false;

            // Get popup window
            PopupWindow *popup  = _this->wPopup;
            if (popup == NULL)
                return STATUS_OK;

            // Validate input
            LSPString value;
            const char *style = INPUT_STYLE_INVALID;
            if (popup->sValue.text()->format(&value) == STATUS_OK)
            {
                float v;
                if (meta::parse_value(&v, value.get_utf8(), meta, false) == STATUS_OK)
                {
                    style = INPUT_STYLE_VALID;
                    if (!meta::range_match(meta, v))
                        style = INPUT_STYLE_MISMATCH;
                }
            }

            // Update color
            tk::Widget *v = &popup->sValue;
            revoke_style(v, INPUT_STYLE_INVALID);
            revoke_style(v, INPUT_STYLE_MISMATCH);
            revoke_style(v, INPUT_STYLE_VALID);
            inject_style(v, style);

            return STATUS_OK;
        }

        status_t MidiNote::slot_cancel_value(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::MidiNote *_this = static_cast<ctl::MidiNote *>(ptr);
            if ((_this == NULL) || (_this->wPopup == NULL))
                return STATUS_OK;

            // Hide the widget and queue for destroy
            PopupWindow *popup  = _this->wPopup;
            if (popup != NULL)
            {
                popup->hide();
                if (popup->queue_destroy() == STATUS_OK)
                    _this->wPopup  = NULL;
            }

            return STATUS_OK;
        }

        status_t MidiNote::slot_key_up(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::MidiNote *_this = static_cast<ctl::MidiNote *>(ptr);
            if ((_this == NULL) || (_this->wPopup == NULL))
                return STATUS_OK;

            // Should be keyboard event
            ws::event_t *ev = reinterpret_cast<ws::event_t *>(data);
            if ((ev == NULL) || (ev->nType != ws::UIE_KEY_UP))
                return STATUS_BAD_ARGUMENTS;

            // Hide popup window
            ws::code_t key = tk::KeyboardHandler::translate_keypad(ev->nCode);

            PopupWindow *popup  = _this->wPopup;
            if (key == ws::WSK_RETURN)
            {
                // Deploy new value
                LSPString value;
                if (popup->sValue.text()->format(&value) == STATUS_OK)
                {
                    if (!_this->apply_value(&value))
                        return STATUS_OK;
                }
            }

            if ((key == ws::WSK_RETURN) || (key == ws::WSK_ESCAPE))
            {
                popup->hide();
                if (popup->queue_destroy() == STATUS_OK)
                    _this->wPopup  = NULL;
            }
            return STATUS_OK;
        }

        status_t MidiNote::slot_mouse_button(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::MidiNote *_this = static_cast<ctl::MidiNote *>(ptr);
            if ((_this == NULL) || (_this->wPopup == NULL))
                return STATUS_OK;

            // Get event
            ws::event_t *ev = reinterpret_cast<ws::event_t *>(data);
            if (ev == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Hide popup window without any action
            PopupWindow *popup  = _this->wPopup;
            if (!popup->inside(ev->nLeft, ev->nTop))
            {
                popup->hide();
                if (popup->queue_destroy() == STATUS_OK)
                    _this->wPopup  = NULL;
            }

            return STATUS_OK;
        }

        status_t MidiNote::slot_dbl_click(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            MidiNote *_this = static_cast<MidiNote *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            // Get port metadata
            const meta::port_t *mdata = (_this->pValue != NULL) ? _this->pValue->metadata() : NULL;
            if (mdata == NULL)
                return STATUS_OK;

            // Set-up units
            const char *u_key = meta::get_unit_lc_key((is_decibel_unit(mdata->unit)) ? meta::U_DB : mdata->unit);
            if ((mdata->unit == meta::U_BOOL) || (mdata->unit == meta::U_ENUM))
                u_key  = NULL;

            // Get label widget
            tk::Indicator *ind = tk::widget_cast<tk::Indicator>(_this->wWidget);
            if (ind == NULL)
                return STATUS_OK;

            // Create popup window if required
            PopupWindow *popup  = _this->wPopup;
            if (popup == NULL)
            {
                popup           = new PopupWindow(_this, ind->display());
                status_t res    = popup->init();
                if (res != STATUS_OK)
                {
                    delete popup;
                    return res;
                }

                _this->wPopup   = popup;
            }

            // Set-up value
            char buf[TMP_BUF_SIZE];
            format_value(buf, TMP_BUF_SIZE, mdata, _this->nNote, _this->nDigits, false);
            popup->sValue.text()->set_raw(buf);
            popup->sValue.selection()->set_all();

            if (u_key != NULL)
            {
                if (popup->sUnits.text()->set(u_key) != STATUS_OK)
                    u_key = NULL;
            }

            popup->sUnits.visibility()->set(u_key != NULL);

            // Show the window and take focus
            ws::rectangle_t r;
            _this->wWidget->get_padded_screen_rectangle(&r);
            r.nWidth    = 0;
            popup->trigger_area()->set(&r);
            popup->trigger_widget()->set(_this->wWidget);
            popup->set_tether(popup_tether, sizeof(popup_tether)/sizeof(tk::tether_t));
            popup->show(_this->wWidget);
            popup->grab_events(ws::GRAB_DROPDOWN);
            popup->sValue.take_focus();

            return STATUS_OK;
        }

        status_t MidiNote::slot_mouse_scroll(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            MidiNote *_this = static_cast<MidiNote *>(ptr);
            if (_this == NULL)
                return STATUS_OK;

            // Should be keyboard event
            ws::event_t *ev = static_cast<ws::event_t *>(data);
            if ((ev == NULL) || (ev->nType != ws::UIE_MOUSE_SCROLL))
                return STATUS_BAD_ARGUMENTS;

            ssize_t delta = (ev->nCode == ws::MCD_UP) ? -1 : 1; // 1 semitone
            if (ev->nState & ws::MCF_CONTROL)
                delta      *= 12; // 1 octave

            _this->apply_value(_this->nNote + delta);
            return STATUS_OK;
        }

    } // namespace ctl
} // namespace lsp
