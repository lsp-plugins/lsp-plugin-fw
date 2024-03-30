/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 15 апр. 2022 г.
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

namespace lsp
{
    namespace ctl
    {
        static constexpr size_t TMP_BUF_SIZE                = 128;
        static constexpr size_t INPUT_DELAY                 = 2000;     // 2 seconds

        static constexpr const char *INPUT_STYLE_VALID      = "Edit::ValidInput";
        static constexpr const char *INPUT_STYLE_INVALID    = "Edit::InvalidInput";
        static constexpr const char *INPUT_STYLE_MISMATCH   = "Edit::MismatchInput";

        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Edit)
            status_t res;

            if (!name->equals_ascii("edit"))
                return STATUS_NOT_FOUND;

            tk::Edit *w     = new tk::Edit(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Edit *wc   = new ctl::Edit(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Edit)

        //-----------------------------------------------------------------
        const ctl_class_t Edit::metadata        = { "Edit", &Widget::metadata };

        Edit::Edit(ui::IWrapper *wrapper, tk::Edit *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
            pPort           = NULL;
            nInputDelay     = INPUT_DELAY;
        }

        Edit::~Edit()
        {
            sTimer.cancel();
        }

        void Edit::destroy()
        {
            sTimer.cancel();
        }

        status_t Edit::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            // Set timer to show the greeting window
            sTimer.set_handler(timer_fired, this);
            sTimer.bind(pWrapper->display());

            tk::Edit *ed = tk::widget_cast<tk::Edit>(wWidget);
            if (ed != NULL)
            {
                ed->slots()->bind(tk::SLOT_KEY_UP, slot_key_up, this);
                ed->slots()->bind(tk::SLOT_CHANGE, slot_change_value, this);
                inject_style(ed, INPUT_STYLE_VALID);

                sEmptyText.init(pWrapper, ed->empty_text());
                sColor.init(pWrapper, ed->color());
                sBorderColor.init(pWrapper, ed->border_color());
                sBorderGapColor.init(pWrapper, ed->border_gap_color());
                sCursorColor.init(pWrapper, ed->cursor_color());
                sTextColor.init(pWrapper, ed->text_color());
                sTextSelectedColor.init(pWrapper, ed->text_selected_color());
                sEmptyTextColor.init(pWrapper, ed->placeholder_text_color());

                sBorderSize.init(pWrapper, ed->border_size());
                sBorderGapSize.init(pWrapper, ed->border_size());
                sBorderRadius.init(pWrapper, ed->border_radius());
            }

            return STATUS_OK;
        }

        void Edit::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Edit *ed = tk::widget_cast<tk::Edit>(wWidget);
            if (ed != NULL)
            {
                bind_port(&pPort, "id", name, value);

                set_value(&nInputDelay, "input_delay", name, value);
                set_value(&nInputDelay, "autocommit", name, value);

                sEmptyText.set("text.empty", name, value);
                sEmptyText.set("etext", name, value);
                sColor.set("color", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sBorderGapColor.set("border.gap.color", name, value);
                sBorderGapColor.set("bgap.color", name, value);
                sCursorColor.set("cursor.color", name, value);
                sCursorColor.set("ccolor", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sEmptyTextColor.set("text.empty.color", name, value);
                sEmptyTextColor.set("etext.color", name, value);
                sTextSelectedColor.set("text.selected.color", name, value);
                sTextSelectedColor.set("tsel.color", name, value);

                sBorderSize.set("border.size", name, value);
                sBorderSize.set("bsize", name, value);
                sBorderGapSize.set("border.gap.size", name, value);
                sBorderGapSize.set("bgap.size", name, value);
                sBorderRadius.set("border.radius", name, value);
                sBorderRadius.set("bradius", name, value);

                set_constraints(ed->constraints(), name, value);
            }

            Widget::set(ctx, name, value);
        }

        void Edit::notify(ui::IPort *port, size_t flags)
        {
            if ((port == pPort) && (port != NULL))
                commit_value();

            Widget::notify(port, flags);
        }

        void Edit::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
            commit_value();
        }

        void Edit::submit_value()
        {
            if (pPort == NULL)
                return;
            sTimer.cancel();

            tk::Edit *ed = tk::widget_cast<tk::Edit>(wWidget);
            if ((ed == NULL) || (pPort == NULL))
                return;

            // Get the text
            LSPString value;
            if (ed->text()->format(&value) != STATUS_OK)
                return;

            // Do the stuff depending on the metadata type
            const meta::port_t *meta = pPort->metadata();
            if (meta::is_path_port(meta))
            {
                const char *path = value.get_utf8();
                const size_t length = strlen(path);
                if ((path != NULL) && (length < PATH_MAX))
                {
                    pPort->write(path, length);
                    pPort->notify_all(ui::PORT_USER_EDIT);
                }
            }
            else if (meta::is_string_port(meta))
            {
                const char *str = value.get_utf8();
                if ((str != NULL) && (value.length() <= size_t(meta->max)))
                {
                    pPort->write(str, strlen(str));
                    pPort->notify_all(ui::PORT_USER_EDIT);
                }
            }
            else
            {
                float v;
                if (meta::parse_value(&v, value.get_utf8(), meta, false) == STATUS_OK)
                {
                    pPort->set_value(v);
                    pPort->notify_all(ui::PORT_USER_EDIT);
                }
            }
        }

        void Edit::commit_value()
        {
            if (pPort == NULL)
                return;
            sTimer.cancel();

            const meta::port_t *meta = pPort->metadata();
            if (meta == NULL)
                return;

            tk::Edit *ed = tk::widget_cast<tk::Edit>(wWidget);
            if ((ed == NULL) || (pPort == NULL))
                return;

            // Do the stuff depending on the metadata
            if (meta::is_path_port(meta))
            {
                const char *path    = pPort->buffer<char>();
                ed->text()->set_raw(path);
            }
            else if (meta::is_string_port(meta))
            {
                const char *string  = pPort->buffer<char>();
                ed->text()->set_raw(string);
            }
            else
            {
                // Set-up value
                float value         = pPort->value();

                char buf[TMP_BUF_SIZE];
                format_value(buf, TMP_BUF_SIZE, meta, value, -1, false);
                ed->text()->set_raw(buf);
                ed->selection()->clear();
            }

            // Update style
            revoke_style(ed, INPUT_STYLE_INVALID);
            revoke_style(ed, INPUT_STYLE_MISMATCH);
            revoke_style(ed, INPUT_STYLE_VALID);
            inject_style(ed, INPUT_STYLE_VALID);
        }

        void Edit::setup_timer()
        {
            if (pPort == NULL)
                sTimer.cancel();
            else if (nInputDelay > 0)
                sTimer.launch(1, nInputDelay, nInputDelay);
        }

        const char *Edit::get_input_style()
        {
            tk::Edit *ed = tk::widget_cast<tk::Edit>(wWidget);
            if ((ed == NULL) || (pPort == NULL))
                return INPUT_STYLE_VALID;

            // Get the text
            LSPString value;
            if (ed->text()->format(&value) != STATUS_OK)
                return INPUT_STYLE_INVALID;

            lsp_trace("checking style for value=%s", value.get_utf8());

            // Do the stuff depending on the metadata type
            const meta::port_t *meta = pPort->metadata();
            if (meta::is_path_port(meta))
            {
                const char *path = value.get_utf8();
                if (path == NULL)
                    return INPUT_STYLE_INVALID;
                if (strlen(path) > PATH_MAX)
                    return INPUT_STYLE_MISMATCH;
            }
            else if (meta::is_string_port(meta))
            {
                if (value.length() > size_t(meta->max))
                    return INPUT_STYLE_MISMATCH;
            }
            else
            {
                float v;
                if (meta::parse_value(&v, value.get_utf8(), meta, false) == STATUS_OK)
                {
                    if (!meta::range_match(meta, v))
                        return INPUT_STYLE_MISMATCH;
                }
                else
                    return INPUT_STYLE_INVALID;
            }

            return INPUT_STYLE_VALID;
        }

        status_t Edit::slot_key_up(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::Edit *self = static_cast<ctl::Edit *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            // Should be keyboard event
            ws::event_t *ev = reinterpret_cast<ws::event_t *>(data);
            if ((ev == NULL) || (ev->nType != ws::UIE_KEY_UP))
                return STATUS_BAD_ARGUMENTS;

            // Hide popup window
            ws::code_t key = tk::KeyboardHandler::translate_keypad(ev->nCode);
            if (key == ws::WSK_RETURN)
                self->submit_value();

            return STATUS_OK;
        }

        status_t Edit::slot_change_value(tk::Widget *sender, void *ptr, void *data)
        {
            // Get control pointer
            ctl::Edit *self = static_cast<ctl::Edit *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            // Update style
            tk::Widget *v = self->wWidget;
            if (v != NULL)
            {
                const char *style = self->get_input_style();

                revoke_style(v, INPUT_STYLE_INVALID);
                revoke_style(v, INPUT_STYLE_MISMATCH);
                revoke_style(v, INPUT_STYLE_VALID);
                inject_style(v, style);
            }

            // Set-up commit timer
            self->setup_timer();

            return STATUS_OK;
        }

        status_t Edit::timer_fired(ws::timestamp_t sched, ws::timestamp_t time, void *arg)
        {
            lsp_trace("timer fired this=%p", arg);

            // Get control pointer
            ctl::Edit *self = static_cast<ctl::Edit *>(arg);
            if (self == NULL)
                return STATUS_OK;

            self->submit_value();

            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */



