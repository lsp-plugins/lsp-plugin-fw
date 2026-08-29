/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 июн. 2021 г.
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
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(ComboBox)
            status_t res;

            if (!name->equals_ascii("combo"))
                return STATUS_NOT_FOUND;

            tk::ComboBox *w = new tk::ComboBox(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::ComboBox *wc  = new ctl::ComboBox(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(ComboBox)

        //-----------------------------------------------------------------
        const ctl_class_t ComboBox::metadata    = { "ComboBox", &Widget::metadata };

        ComboBox::ComboBox(ui::IWrapper *wrapper, tk::ComboBox *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            fMin            = 0.0f;
            fMax            = 0.0f;
            fStep           = 0.0f;
        }

        ComboBox::~ComboBox()
        {
            do_destroy();
        }

        void ComboBox::destroy()
        {
            do_destroy();
            Widget::destroy();
        }

        void ComboBox::do_destroy()
        {
            // Clear all stored items in the combo box.
            tk::ComboBox * const cbox = tk::widget_cast<tk::ComboBox>(wWidget);
            if (cbox != NULL)
                cbox->items()->clear();

            // Clear added items
            if (!vItems.is_empty())
            {
                for (lltl::iterator<ListBoxItem> it = vItems.values(); it; ++it)
                {
                    ListBoxItem * const item = it.get();
                    if (item != NULL)
                        item->set_child_sync(NULL);
                }
                vItems.flush();
            }

            // Clear generated items
            drop_port_items();
        }

        void ComboBox::drop_port_items()
        {
            if (!vPortItems.is_empty())
            {
                for (lltl::iterator<port_item_t> it = vPortItems.values(); it; ++it)
                {
                    port_item_t * const item = it.get();
                    if (item->pItem != NULL)
                        item->pItem->destroy();
                    delete item->pItem;
                }

                vPortItems.flush();
            }
        }

        status_t ComboBox::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::ComboBox *cbox = tk::widget_cast<tk::ComboBox>(wWidget);
            if (cbox != NULL)
            {
                sColor.init(pWrapper, cbox->color());
                sSpinColor.init(pWrapper, cbox->spin_color());
                sTextColor.init(pWrapper, cbox->text_color());
                sSpinTextColor.init(pWrapper, cbox->spin_text_color());
                sBorderColor.init(pWrapper, cbox->border_color());
                sBorderGapColor.init(pWrapper, cbox->border_gap_color());

                sInactiveColor.init(pWrapper, cbox->inactive_color());
                sInactiveSpinColor.init(pWrapper, cbox->inactive_spin_color());
                sInactiveTextColor.init(pWrapper, cbox->inactive_text_color());
                sInactiveSpinTextColor.init(pWrapper, cbox->inactive_spin_text_color());
                sInactiveBorderColor.init(pWrapper, cbox->inactive_border_color());
                sInactiveBorderGapColor.init(pWrapper, cbox->inactive_border_gap_color());

                sEmptyText.init(pWrapper, cbox->empty_text());

                // Bind slots
                cbox->slots()->bind(tk::SLOT_SUBMIT, slot_combo_submit, this);
            }

            return STATUS_OK;
        }

        void ComboBox::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::ComboBox *cbox = tk::widget_cast<tk::ComboBox>(wWidget);
            if (cbox != NULL)
            {
                bind_port(&pPort, "id", name, value);

                set_param(cbox->border_size(), "border.size", name, value);
                set_param(cbox->border_size(), "bsize", name, value);
                set_param(cbox->border_gap(), "border.gap", name, value);
                set_param(cbox->border_gap(), "bgap", name, value);
                set_param(cbox->border_radius(), "border.radius", name, value);
                set_param(cbox->border_radius(), "bradius", name, value);
                set_param(cbox->spin_size(), "spin.size", name, value);
                set_param(cbox->spin_separator(), "spin.separator", name, value);
                set_param(cbox->text_adjust(), "text.ajust", name, value);

                sColor.set("color", name, value);
                sSpinColor.set("spin.color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sSpinTextColor.set("spin.text.color", name, value);
                sSpinTextColor.set("spin.tcolor", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sBorderGapColor.set("border.gap.color", name, value);
                sBorderGapColor.set("bgap.color", name, value);

                sInactiveColor.set("inactive.color", name, value);
                sInactiveSpinColor.set("inactive.spin.color", name, value);
                sInactiveTextColor.set("inactive.text.color", name, value);
                sInactiveTextColor.set("inactive.tcolor", name, value);
                sInactiveSpinTextColor.set("inactive.spin.text.color", name, value);
                sInactiveSpinTextColor.set("inactive.spin.tcolor", name, value);
                sInactiveBorderColor.set("inactive.border.color", name, value);
                sInactiveBorderColor.set("inactive.bcolor", name, value);
                sInactiveBorderGapColor.set("inactive.border.gap.color", name, value);
                sInactiveBorderGapColor.set("inactive.bgap.color", name, value);

                sEmptyText.set("text.empty", name, value);

                set_text_fitness(cbox->text_fit(), "text.fitness", name, value);
                set_text_fitness(cbox->text_fit(), "tfitness", name, value);
                set_text_fitness(cbox->text_fit(), "tfit", name, value);
                set_font(cbox->font(), "font", name, value);
                set_constraints(cbox->constraints(), name, value);
                set_text_layout(cbox->text_layout(), name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void ComboBox::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            bool update = false;
            if ((port != NULL) && (pPort == port))
                update = true;
            else if (!vItems.is_empty())
                update = true;

            if (update)
                update_selection();
        }

        status_t ComboBox::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            ctl::ListBoxItem * const item = ctl::ctl_cast<ctl::ListBoxItem>(child);
            if (item == NULL)
                return STATUS_BAD_ARGUMENTS;

            if (!vItems.add(item))
                return STATUS_NO_MEM;

            item->set_child_sync(this);
            return STATUS_OK;
        }

        void ComboBox::end(ui::UIContext *ctx)
        {
            sync_metadata(pPort);

            Widget::end(ctx);
        }

        void ComboBox::update_selection()
        {
            tk::ComboBox * const cbox = tk::widget_cast<tk::ComboBox>(wWidget);
            if (cbox == NULL)
                return;

            if (!vItems.is_empty())
            {
                ctl::ListBoxItem *sel = vItems.uget(0);
                for (size_t i=1, n=vItems.size(); i < n; ++i)
                {
                    ctl::ListBoxItem * const item = vItems.uget(i);
                    if ((item != NULL) && (item->selected()))
                    {
                        sel = item;
                        break;
                    }
                }

                tk::Widget *li = (sel != NULL) ? sel->widget() : NULL;
                cbox->selected()->set(tk::widget_cast<tk::ListBoxItem>(li));
            }
            else if (!vPortItems.is_empty())
            {
                const ssize_t key = (pPort->value() - fMin) / fStep;
                port_item_t *sel = NULL;
                for (size_t i=0, n=vPortItems.size(); i < n; ++i)
                {
                    port_item_t * const item = vPortItems.uget(i);
                    if ((item != NULL) && (item->pItem != NULL) && (item->nKey == key))
                    {
                        sel = item;
                        break;
                    }
                }
                if (sel == NULL)
                    sel     = vPortItems.uget(0);

                cbox->selected()->set(sel->pItem);
            }
        }

        void ComboBox::sync_metadata(ui::IPort *port)
        {
            tk::ComboBox *cbox = tk::widget_cast<tk::ComboBox>(wWidget);
            if (cbox == NULL)
                return;

            if (!vItems.is_empty())
            {
                // Need to initialize items?
                if (cbox->items()->is_empty())
                {
                    for (lltl::iterator<ListBoxItem> it=vItems.values(); it; ++it)
                    {
                        ListBoxItem *item = it.get();
                        if (item == NULL)
                            continue;

                        tk::ListBoxItem *li = tk::widget_cast<tk::ListBoxItem>(item->widget());
                        if (li == NULL)
                            continue;

                        cbox->items()->add(li);
                    }
                }

                update_selection();
            }
            else if (port == pPort)
            {
                const meta::port_t *p = (pPort != NULL) ? pPort->metadata() : NULL;
                if (p == NULL)
                    return;

                meta::get_port_parameters(p, &fMin, &fMax, &fStep);
                if (p->unit != meta::U_ENUM)
                    return;

                const ssize_t value   = pPort->value();

                tk::WidgetList<tk::ListBoxItem> * const lst = cbox->items();
                lst->clear();

                bool success = false;
                drop_port_items();
                lsp_finally {
                    if (!success)
                    {
                        lst->clear();
                        drop_port_items();
                    }
                };

                LSPString lck;

                const bool is_sorted    = meta::is_sorted(p);
                size_t i                = 0;
                for (const meta::port_item_t *item = p->items; (item != NULL) && (item->text != NULL); ++item, ++i)
                {
                    port_item_t * const pi  = vPortItems.append();
                    if (pi == NULL)
                        return;

                    pi->pItem               = NULL;
                    pi->nIndex              = (is_sorted) ? item->order : i;
                    pi->nKey                = fMin + fStep * i;

                    // Initialize list box item
                    pi->pItem               = new tk::ListBoxItem(wWidget->display());
                    if (pi->pItem == NULL)
                        return;
                    status_t res            = pi->pItem->init();
                    if (res != STATUS_OK)
                        return;

                    // Set up list box item text
                    if (item->lc_key != NULL)
                    {
                        lck.set_ascii("lists.");
                        lck.append_ascii(item->lc_key);
                        pi->pItem->text()->set(&lck);
                    }
                    else
                        pi->pItem->text()->set_raw(item->text);
                }

                if (is_sorted)
                    vPortItems.ssort(compare_port_item);

                success = true;
                for (lltl::iterator<port_item_t> it = vPortItems.values(); it; ++it)
                {
                    port_item_t * const pi = it.get();
                    if (pi == NULL)
                        continue;

                    lst->add(pi->pItem);
                    if (pi->nKey == value)
                        cbox->selected()->set(pi->pItem);
                }
            }
        }

        ssize_t ComboBox::compare_port_item(const port_item_t *a, const port_item_t *b)
        {
            return a->nIndex - b->nIndex;
        }

        status_t ComboBox::slot_combo_submit(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::ComboBox * const self  = static_cast<ctl::ComboBox *>(ptr);
            if (self != NULL)
                self->submit_value();
            return STATUS_OK;
        }

        void ComboBox::submit_value()
        {
            tk::ComboBox * const cbox = tk::widget_cast<tk::ComboBox>(wWidget);
            if (cbox == NULL)
                return;

            if (pPort == NULL)
                return;

            tk::ListBoxItem * const li = cbox->selected()->get();
            float value = 0.0f;

            if (!vItems.is_empty())
            {
                // Find the item that matches the list box
                ListBoxItem *found = NULL;
                for (lltl::iterator<ListBoxItem> it=vItems.values(); it; ++it)
                {
                    ListBoxItem * const item = it.get();
                    if ((item != NULL) && (item->widget() == li))
                    {
                        found = item;
                        break;
                    }
                }

                if (found == NULL)
                    return;

                value   = found->value();
            }
            else if (!vPortItems.is_empty())
            {
                // Find the item that matches the list box
                port_item_t *found = NULL;
                for (lltl::iterator<port_item_t> it=vPortItems.values(); it; ++it)
                {
                    port_item_t * const item = it.get();
                    if ((item != NULL) && (item->pItem == li))
                    {
                        found = item;
                        break;
                    }
                }

                if (found == NULL)
                    return;

                value   = float(found->nKey);
            }
            else
                return;

            lsp_trace("value=%f", value);

            pPort->begin_edit();
            pPort->set_value(value);
            pPort->notify_all(ui::PORT_USER_EDIT);
            pPort->end_edit();
        }

        void ComboBox::child_changed(Widget *child)
        {
            update_selection();
        }

    } /* namespace ctl */
} /* namespace lsp */


