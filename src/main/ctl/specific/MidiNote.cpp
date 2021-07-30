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
        CTL_FACTORY_IMPL_END(MidiNote)

        MidiNote::MidiNote(ui::IWrapper *wrapper, tk::Indicator *widget): Widget(wrapper, widget)
        {
            nNote           = 0;
            nDigits         = 3;
            pNote           = NULL;
            pOctave         = NULL;
            pValue          = NULL;
        }

        MidiNote::~MidiNote()
        {
            do_destroy();
        }

        void MidiNote::do_destroy()
        {
        }

        status_t MidiNote::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Indicator *ind = tk::widget_cast<tk::Indicator>(wWidget);
            if (ind != NULL)
            {
                sColor.init(pWrapper, ind->color());
                sTextColor.init(pWrapper, ind->text_color());
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

        void MidiNote::schema_reloaded()
        {
            Widget::schema_reloaded();

            sColor.reload();
            sTextColor.reload();
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
            status_t res = meta::parse_value(&v, value->get_utf8(), meta);
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

    } // namespace ctl
} // namespace lsp
