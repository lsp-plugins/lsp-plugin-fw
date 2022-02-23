/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 19 июл. 2021 г.
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
        CTL_FACTORY_IMPL_START(ComboGroup)
            status_t res;

            if (!name->equals_ascii("cgroup"))
                return STATUS_NOT_FOUND;

            tk::ComboGroup *w = new tk::ComboGroup(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::ComboGroup *wc  = new ctl::ComboGroup(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(ComboGroup)

        //-----------------------------------------------------------------
        const ctl_class_t ComboGroup::metadata  = { "ComboGroup", &Widget::metadata };

        ComboGroup::ComboGroup(ui::IWrapper *wrapper, tk::ComboGroup *cgroup): ctl::Widget(wrapper, cgroup)
        {
            pClass          = &metadata;

            pPort           = NULL;
            fMin            = 0.0f;
            fMax            = 0.0f;
            fStep           = 0.0f;
            nActive         = -1;
        }

        ComboGroup::~ComboGroup()
        {
        }

        status_t ComboGroup::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::ComboGroup *cgrp = tk::widget_cast<tk::ComboGroup>(wWidget);
            if (cgrp != NULL)
            {
                // Bind slots
                cgrp->slots()->bind(tk::SLOT_SUBMIT, slot_combo_submit, this);

                sColor.init(pWrapper, cgrp->color());
                sTextColor.init(pWrapper, cgrp->text_color());
                sSpinColor.init(pWrapper, cgrp->spin_color());
                sEmptyText.init(pWrapper, cgrp->empty_text());
                sTextPadding.init(pWrapper, cgrp->text_padding());
                sEmbed.init(pWrapper, cgrp->embedding());
                sActive.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void ComboGroup::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::ComboGroup *cgrp = tk::widget_cast<tk::ComboGroup>(wWidget);
            if (cgrp != NULL)
            {
                bind_port(&pPort, "id", name, value);
                set_expr(&sActive, "active", name, value);

                sColor.set("color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sSpinColor.set("spin.color", name, value);
                sSpinColor.set("scolor", name, value);
                sEmptyText.set("text", name, value);
                sTextPadding.set("text.padding", name, value);
                sEmbed.set("embed", name, value);

                set_font(cgrp->font(), "font", name, value);
                set_layout(cgrp->layout(), NULL, name, value);
                set_constraints(cgrp->constraints(), name, value);
                set_alignment(cgrp->heading(), "heading.alignment", name, value);
                set_alignment(cgrp->heading(), "heading.align", name, value);
                set_param(cgrp->text_ajdust(), "text.adjust", name, value);
                set_param(cgrp->border_size(), "border.size", name, value);
                set_param(cgrp->border_size(), "bsize", name, value);
                set_param(cgrp->border_radius(), "border.radius", name, value);
                set_param(cgrp->border_radius(), "bradius", name, value);
                set_param(cgrp->text_radius(), "text.radius", name, value);
                set_param(cgrp->text_radius(), "tradius", name, value);
                set_param(cgrp->spin_size(), "spin.size", name, value);
                set_param(cgrp->spin_spacing(), "spin.spacing", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        status_t ComboGroup::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::ComboGroup *cgrp = tk::widget_cast<tk::ComboGroup>(wWidget);
            return (cgrp != NULL) ? cgrp->widgets()->add(child->widget()) : STATUS_OK;
        }

        void ComboGroup::end(ui::UIContext *ctx)
        {
            if (pPort != NULL)
                sync_metadata(pPort);

            if (sActive.valid())
                select_active_widget();

            Widget::end(ctx);
        }

        void ComboGroup::notify(ui::IPort *port)
        {
            if (port == NULL)
                return;

            Widget::notify(port);

            if (sActive.depends(port))
                select_active_widget();

            if (pPort == port)
            {
                tk::ComboGroup *cgrp = tk::widget_cast<tk::ComboGroup>(wWidget);
                if (cgrp != NULL)
                {
                    ssize_t index = (pPort->value() - fMin) / fStep;

                    tk::ListBoxItem *li = cgrp->items()->get(index);
                    cgrp->selected()->set(li);
                }
            }
        }

        void ComboGroup::select_active_widget()
        {
            tk::ComboGroup *cgrp = tk::widget_cast<tk::ComboGroup>(wWidget);
            if (cgrp == NULL)
                return;

            ssize_t index = (sActive.valid()) ? sActive.evaluate_int() : -1;
            tk::Widget *w = (index >= 0) ? cgrp->widgets()->get(index) : NULL;
            cgrp->active()->set(w);
        }

        status_t ComboGroup::slot_combo_submit(tk::Widget *sender, void *ptr, void *data)
        {
            ComboGroup *_this   = static_cast<ComboGroup *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

        void ComboGroup::sync_metadata(ui::IPort *port)
        {
            tk::ComboGroup *cgrp = tk::widget_cast<tk::ComboGroup>(wWidget);
            if (cgrp == NULL)
                return;

            if (port != pPort)
                return;

            const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
            if (p == NULL)
                return;

            meta::get_port_parameters(p, &fMin, &fMax, &fStep);

            if (p->unit == meta::U_ENUM)
            {
                ssize_t value   = pPort->value();
                size_t i        = 0;

                tk::WidgetList<tk::ListBoxItem> *lst = cgrp->items();
                lst->clear();

                LSPString lck;
                tk::ListBoxItem *li;

                for (const meta::port_item_t *item = p->items; (item != NULL) && (item->text != NULL); ++item, ++i)
                {
                    li  = new tk::ListBoxItem(wWidget->display());
                    if (li == NULL)
                        return;
                    li->init();

                    ssize_t key     = fMin + fStep * i;
                    if (item->lc_key != NULL)
                    {
                        lck.set_ascii("lists.");
                        lck.append_ascii(item->lc_key);
                        li->text()->set(&lck);
                    }
                    else
                        li->text()->set_raw(item->text);
                    lst->madd(li);

                    if (key == value)
                        cgrp->selected()->set(li);
                }
            }
        }

        void ComboGroup::submit_value()
        {
            if (pPort == NULL)
                return;

            tk::ComboGroup *cgrp = tk::widget_cast<tk::ComboGroup>(wWidget);
            if (cgrp == NULL)
                return;

            ssize_t index = cgrp->items()->index_of(cgrp->selected()->get());

            float value = fMin + fStep * index;
            lsp_trace("index = %d, value=%f", int(index), value);

            pPort->set_value(value);
            pPort->notify_all();
        }

    } // ctl
} // lsp


