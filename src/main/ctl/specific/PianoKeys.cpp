/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 сент. 2025 г.
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

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(PianoKeys)
            status_t res;

            if (!name->equals_ascii("piano"))
                return STATUS_NOT_FOUND;

            tk::PianoKeys *w = new tk::PianoKeys(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::PianoKeys *wc  = new ctl::PianoKeys(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(PianoKeys);

        //-----------------------------------------------------------------
        const ctl_class_t PianoKeys::metadata       = { "PianoKeys", &Widget::metadata };

        PianoKeys::PianoKeys(ui::IWrapper *wrapper, tk::PianoKeys *widget): Widget(wrapper, widget)
        {
            pClass              = &metadata;

            pSelectionStart     = NULL;
            pSelectionEnd       = NULL;
        }

        PianoKeys::~PianoKeys()
        {
            do_destroy();
        }

        status_t PianoKeys::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::PianoKeys *pk = tk::widget_cast<tk::PianoKeys>(wWidget);
            if (pk != NULL)
            {
                sBorder.init(pWrapper, pk->padding());
                sSplitSize.init(pWrapper, pk->split_size());
                sMinNote.init(pWrapper, pk->min_note());
                sMaxNote.init(pWrapper, pk->max_note());
                sAngle.init(pWrapper, pk->angle());
                sKeyAspect.init(pWrapper, pk->key_aspect());
                sNatural.init(pWrapper, pk->natural());
                sEditable.init(pWrapper, pk->editable());
                sSelectable.init(pWrapper, pk->selectable());
                sClearSelection.init(pWrapper, pk->clear_selection());

                pk->slots()->bind(tk::SLOT_SUBMIT, slot_submit_key, this);
                pk->slots()->bind(tk::SLOT_CHANGE, slot_change_selection, this);
            }

            return STATUS_OK;
        }

        void PianoKeys::destroy()
        {
            do_destroy();
            Widget::destroy();
        }

        void PianoKeys::do_destroy()
        {
        }

        void PianoKeys::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            Widget::set(ctx, name, value);

            tk::PianoKeys *pk = tk::widget_cast<tk::PianoKeys>(wWidget);
            if (pk != NULL)
            {
                bind_port(&pSelectionStart, "selection.start", name, value);
                bind_port(&pSelectionStart, "sel.start", name, value);
                bind_port(&pSelectionEnd, "selection.end", name, value);
                bind_port(&pSelectionEnd, "sel.end", name, value);

                set_constraints(pk->size_constraints(), name, value);

                sBorder.set("border", name, value);
                sSplitSize.set("split", name, value);
                sMinNote.set("note.min", name, value);
                sMaxNote.set("note.max", name, value);
                sAngle.set("angle", name, value);
                sKeyAspect.set("key.aspect", name, value);
                sKeyAspect.set("aspect", name, value);
                sNatural.set("natural", name, value);
                sEditable.set("editable", name, value);
                sSelectable.set("selectable", name, value);
                sClearSelection.set("selection.clear", name, value);
            }
        }

        void PianoKeys::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
        }

        void PianoKeys::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);
            if (port == NULL)
                return;

            tk::PianoKeys *pk = tk::widget_cast<tk::PianoKeys>(wWidget);
            if (pk == NULL)
                return;

            if (port == pSelectionStart)
                pk->selection_start()->set(pSelectionStart->value());
            if (port == pSelectionEnd)
                pk->selection_end()->set(pSelectionEnd->value());
        }

        status_t PianoKeys::slot_submit_key(tk::Widget *sender, void *ptr, void *data)
        {
            return STATUS_OK;
        }

        status_t PianoKeys::slot_change_selection(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::PianoKeys *self = ctl::ctl_ptrcast<ctl::PianoKeys>(ptr);
            if (self == NULL)
                return STATUS_OK;

            tk::PianoKeys *pk = tk::widget_cast<tk::PianoKeys>(sender);
            if (pk == NULL)
                return STATUS_OK;

            const ssize_t sel_start = pk->selection_start()->get();
            const ssize_t sel_end   = pk->selection_end()->get();
            bool notify_start       = false;
            bool notify_end         = false;
            if ((self->pSelectionStart != NULL) && (ssize_t(self->pSelectionStart->value()) != sel_start))
            {
                self->pSelectionStart->set_value(sel_start);
                notify_start            = true;
            }
            if ((self->pSelectionEnd != NULL) && (ssize_t(self->pSelectionEnd->value()) != sel_end))
            {
                self->pSelectionEnd->set_value(sel_end);
                notify_end              = true;
            }

            if (notify_start)
                self->pSelectionStart->notify_all(ui::PORT_USER_EDIT);
            if (notify_end)
                self->pSelectionEnd->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */

