/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 6 мая 2021 г.
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

#define INPUT_STYLE_VALID       "Value::PopupWindow::ValidInput"
#define INPUT_STYLE_INVALID     "Value::PopupWindow::InvalidInput"
#define INPUT_STYLE_MISMATCH    "Value::PopupWindow::MismatchInput"

#define STATUS_STYLE_OK         "Value::Status::OK"
#define STATUS_STYLE_WARN       "Value::Status::Warn"
#define STATUS_STYLE_ERROR      "Value::Status::Error"

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Label)
            status_t res;
            label_type_t type;

            if (name->equals_ascii("label"))
                type = CTL_LABEL_TEXT;
            else if (name->equals_ascii("value"))
                type = CTL_LABEL_VALUE;
            else if (name->equals_ascii("status"))
                type = CTL_STATUS_CODE;
            else
                return STATUS_NOT_FOUND;

            tk::Label *w = new tk::Label(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Label *wc  = new ctl::Label(context->wrapper(), w, type);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Label)


        //-----------------------------------------------------------------
        Label::PopupWindow::PopupWindow(Label *label, tk::Display *dpy):
            tk::PopupWindow(dpy),
            sBox(dpy),
            sValue(dpy),
            sUnits(dpy),
            sApply(dpy),
            sCancel(dpy)
        {
            pClass      = &metadata;

            pLabel      = label;
        }

        Label::PopupWindow::~PopupWindow()
        {
            pLabel      = NULL;
        }

        status_t Label::PopupWindow::init()
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

            inject_style(&sBox, "Value::PopupWindow::Box");
            sBox.add(&sValue);
            sBox.add(&sUnits);
            sBox.add(&sApply);
            sBox.add(&sCancel);

            this->slots()->bind(tk::SLOT_MOUSE_DOWN, Label::slot_mouse_button, pLabel);
            this->slots()->bind(tk::SLOT_MOUSE_UP, Label::slot_mouse_button, pLabel);

            sValue.slots()->bind(tk::SLOT_KEY_UP, Label::slot_key_up, pLabel);
            sValue.slots()->bind(tk::SLOT_CHANGE, Label::slot_change_value, pLabel);
            inject_style(&sValue, INPUT_STYLE_VALID);

            inject_style(&sUnits, "Value::PopupWindow::Units");

            sApply.text()->set("actions.apply");
            sApply.slots()->bind(tk::SLOT_SUBMIT, Label::slot_submit_value, pLabel);
            inject_style(&sApply, "Value::PopupWindow::Apply");

            sCancel.text()->set("actions.cancel");
            sCancel.slots()->bind(tk::SLOT_SUBMIT, Label::slot_cancel_value, pLabel);
            inject_style(&sCancel, "Value::PopupWindow::Cancel");

            this->add(&sBox);
            inject_style(this, "Value::PopupWindow");

            return STATUS_OK;
        }

        void Label::PopupWindow::destroy()
        {
            sValue.destroy();
            sUnits.destroy();
            sApply.destroy();
            sBox.destroy();

            tk::PopupWindow::destroy();
        }

        //-----------------------------------------------------------------
        const ctl_class_t Label::metadata     = { "Label", &Widget::metadata };

        Label::Label(ui::IWrapper *wrapper, tk::Label *widget, label_type_t type):
            Widget(wrapper, widget)
        {
            pClass          = &metadata;

            enType          = type;
            pPort           = NULL;
            fValue          = 0.0f;
            bDetailed       = true;
            bSameLine       = false;
            bReadOnly       = false;
            nUnits          = meta::U_NONE - 1;
            nPrecision      = -1;
            wPopup          = NULL;
        }

        Label::~Label()
        {
            do_destroy();
        }

        status_t Label::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Label *lbl = tk::widget_cast<tk::Label>(wWidget);
            if (lbl != NULL)
            {
                sColor.init(pWrapper, lbl->color());
                sHoverColor.init(pWrapper, lbl->hover_color());
                sText.init(pWrapper, lbl->text());

                lbl->slot(tk::SLOT_MOUSE_DBL_CLICK)->bind(slot_dbl_click, this);
            }

            return STATUS_OK;
        }

        void Label::destroy()
        {
            do_destroy();
            ctl::Widget::destroy();
        }

        void Label::do_destroy()
        {
            // Destroy popup window
            if (wPopup != NULL)
            {
                wPopup->destroy();
                delete wPopup;
                wPopup = NULL;
            }
        }

        void Label::commit_value()
        {
            // Get metadata and value
            if (pPort == NULL)
                return;

            const meta::port_t *mdata = pPort->metadata();
            if (mdata == NULL)
                return;
            fValue      = pPort->value();

            // Get label widget
            tk::Label *lbl = tk::widget_cast<tk::Label>(wWidget);
            if (lbl == NULL)
                return;

            // Analyze type of the label
            bool detailed       = bDetailed;

            switch (enType)
            {
                case CTL_LABEL_TEXT:
                    if ((mdata != NULL) && (mdata->name != NULL))
                        lbl->text()->set_raw(mdata->name);
                    return;

                case CTL_LABEL_VALUE:
                {
                    // Encode units
                    tk::prop::String sunit;
                    sunit.bind(lbl->style(), lbl->display()->dictionary());

                    if (nUnits != (meta::U_NONE - 1))
                        sunit.set(meta::get_unit_lc_key(nUnits));
                    else
                        sunit.set(meta::get_unit_lc_key((meta::is_decibel_unit(mdata->unit)) ? meta::U_DB : mdata->unit));

                    // Format the value
                    char buf[TMP_BUF_SIZE];
                    expr::Parameters params;
                    LSPString text, funit;

                    meta::format_value(buf, TMP_BUF_SIZE, mdata, fValue, nPrecision);
                    text.set_ascii(buf);
                    sunit.format(&funit);
                    if (mdata->unit == meta::U_BOOL)
                    {
                        text.prepend_ascii("labels.bool.");
                        sunit.set(&text);
                        sunit.format(&text);
                        detailed = false;
                    }

                    // Update text
                    const char *key = "labels.values.fmt_value";
                    if ((detailed) && (funit.length() > 0))
                        key = (bSameLine) ? "labels.values.fmt_single_line" : "labels.values.fmt_multi_line";

                    params.add_string("value", &text);
                    params.add_string("unit", &funit);

                    lbl->text()->set(key, &params);
                    break;
                }

                case CTL_STATUS_CODE:
                {
                    status_t code = fValue;
                    const char *lc_key = get_status_lc_key(code);
                    LSPString key;

                    revoke_style(lbl, STATUS_STYLE_OK);
                    revoke_style(lbl, STATUS_STYLE_WARN);
                    revoke_style(lbl, STATUS_STYLE_ERROR);

                    if (status_is_success(code))
                        inject_style(lbl, STATUS_STYLE_OK);
                    else if (status_is_preliminary(code))
                        inject_style(lbl, STATUS_STYLE_WARN);
                    else
                        inject_style(lbl, STATUS_STYLE_ERROR);

                    if (key.set_ascii("statuses.std."))
                        key.append_ascii(lc_key);

                    lbl->text()->set(&key);
                    break;
                }

                default:
                    break;
            }
        }

        bool Label::apply_value(const LSPString *value)
        {
            const meta::port_t *meta = (pPort != NULL) ? pPort->metadata() : NULL;
            if ((meta == NULL) || (!meta::is_in_port(meta)))
                return false;

            float fv;
            status_t res = parse_value(&fv, value->get_utf8(), meta);
            if (res != STATUS_OK)
                return false;

            pPort->set_value(fv);
            pPort->notify_all();
            return true;
        }

        void Label::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Label *lbl = tk::widget_cast<tk::Label>(wWidget);
            if (lbl != NULL)
            {
                bind_port(&pPort, "id", name, value);

                set_text_layout(lbl->text_layout(), name, value);
                set_font(lbl->font(), "font", name, value);
                set_constraints(lbl->constraints(), name, value);
                set_param(lbl->text_adjust(), "text.adjust", name, value);
                set_param(lbl->hover(), "hover", name, value);
                set_param(lbl->font_scaling(), "font.scaling", name, value);
                set_param(lbl->font_scaling(), "font.scale", name, value);

                if (enType == CTL_LABEL_TEXT)
                    sText.set("text", name, value);

                set_value(&bDetailed, "detailed", name, value);
                set_value(&bSameLine, "value.same_line", name, value);
                set_value(&bSameLine, "same_line", name, value);
                set_value(&bSameLine, "same.line", name, value);
                set_value(&bSameLine, "sline", name, value);
                set_value(&bReadOnly, "read_only", name, value);
                set_value(&bReadOnly, "readonly", name, value);
                set_value(&bReadOnly, "rdonly", name, value);
                set_value(&nPrecision, "precision", name, value);

                sColor.set("color", name, value);
                sHoverColor.set("hover.color", name, value);
                sHoverColor.set("hcolor", name, value);
            }

            Widget::set(ctx, name, value);
        }

        void Label::notify(ui::IPort *port)
        {
            Widget::notify(port);
            if ((pPort != NULL) && (pPort == port))
                commit_value();
        }

        void Label::end(ui::UIContext *ctx)
        {
            commit_value();
            Widget::end(ctx);
        }

        status_t Label::slot_dbl_click(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::Label *_this = static_cast<ctl::Label *>(ptr);
            if ((_this == NULL) || (_this->enType != CTL_LABEL_VALUE) || (_this->bReadOnly))
                return STATUS_OK;

            // Get port metadata
            const meta::port_t *mdata = (_this->pPort != NULL) ? _this->pPort->metadata() : NULL;
            if ((mdata == NULL) || (!meta::is_in_port(mdata)))
                return STATUS_OK;

            // Set-up units
            const char *u_key = NULL;
            if (_this->nUnits != (meta::U_NONE - 1))
                u_key  = meta::get_unit_lc_key(_this->nUnits);
            else
                u_key  = meta::get_unit_lc_key((is_decibel_unit(mdata->unit)) ? meta::U_DB : mdata->unit);
            if ((mdata->unit == meta::U_BOOL) || (mdata->unit == meta::U_ENUM))
                u_key  = NULL;

            // Get label widget
            tk::Label *lbl = tk::widget_cast<tk::Label>(_this->wWidget);
            if (lbl == NULL)
                return STATUS_OK;

            // Create popup window if required
            PopupWindow *popup  = _this->wPopup;
            if (popup == NULL)
            {
                popup           = new PopupWindow(_this, lbl->display());
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
            format_value(buf, TMP_BUF_SIZE, mdata, _this->fValue, _this->nPrecision);
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
            popup->add_arrangement(tk::A_RIGHT, 0.0f, false);
            popup->show(_this->wWidget);
            popup->grab_events(ws::GRAB_DROPDOWN);
            popup->sValue.take_focus();

            return STATUS_OK;
        }

        status_t Label::slot_submit_value(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::Label *_this = static_cast<ctl::Label *>(ptr);
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

        status_t Label::slot_change_value(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::Label *_this = static_cast<ctl::Label *>(ptr);
            if ((_this == NULL) || (_this->wPopup == NULL))
                return STATUS_OK;

            // Get port metadata
            const meta::port_t *meta = (_this->pPort != NULL) ? _this->pPort->metadata() : NULL;
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
                if (meta::parse_value(&v, value.get_utf8(), meta) == STATUS_OK)
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

        status_t Label::slot_cancel_value(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::Label *_this = static_cast<ctl::Label *>(ptr);
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

        status_t Label::slot_key_up(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::Label *_this = static_cast<ctl::Label *>(ptr);
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

        status_t Label::slot_mouse_button(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::Label *_this = static_cast<ctl::Label *>(ptr);
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

    } /* namespace ctl */
} /* namespace lsp */


