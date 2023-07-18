/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 окт. 2021 г.
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
#include <lsp-plug.in/ipc/Thread.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(ThreadComboBox)
            status_t res;

            if (!name->equals_ascii("threadcombo"))
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

            ctl::ThreadComboBox *wc  = new ctl::ThreadComboBox(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(ThreadComboBox)

        //-----------------------------------------------------------------
        const ctl_class_t ThreadComboBox::metadata    = { "ThreadComboBox", &Widget::metadata };

        ThreadComboBox::ThreadComboBox(ui::IWrapper *wrapper, tk::ComboBox *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
        }

        ThreadComboBox::~ThreadComboBox()
        {
        }

        status_t ThreadComboBox::init()
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
                sEmptyText.init(pWrapper, cbox->empty_text());

                // Bind slots
                cbox->slots()->bind(tk::SLOT_SUBMIT, slot_combo_submit, this);
            }

            return STATUS_OK;
        }

        void ThreadComboBox::set(ui::UIContext *ctx, const char *name, const char *value)
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

        void ThreadComboBox::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            tk::ComboBox *cbox = tk::widget_cast<tk::ComboBox>(wWidget);
            if (cbox == NULL)
                return;

            if ((pPort == port) && (cbox != NULL))
            {
                ssize_t index = pPort->value();

                tk::ListBoxItem *selected = cbox->items()->get(index - 1);
                if (selected != NULL)
                    cbox->selected()->set(selected);
            }
        }

        void ThreadComboBox::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            tk::ComboBox *cbox = tk::widget_cast<tk::ComboBox>(wWidget);
            if (cbox != NULL)
            {
                LSPString v;

                for (size_t i=1, cores=ipc::Thread::system_cores(); i<=cores; ++i)
                {
                    if (!v.fmt_ascii("%d", int(i)))
                        continue;

                    tk::ListBoxItem *w = new tk::ListBoxItem(cbox->display());
                    if (w == NULL)
                        continue;
                    if (w->init() != STATUS_OK)
                    {
                        w->destroy();
                        delete w;
                        continue;
                    }

                    w->text()->set_raw(&v);
                    w->tag()->set(i);

                    if (cbox->items()->madd(w) != STATUS_OK)
                    {
                        w->destroy();
                        delete w;
                    }
                }
            }
        }

        status_t ThreadComboBox::slot_combo_submit(tk::Widget *sender, void *ptr, void *data)
        {
            ThreadComboBox *_this     = static_cast<ThreadComboBox *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

        void ThreadComboBox::submit_value()
        {
            if (pPort == NULL)
                return;

            const meta::port_t *meta = pPort->metadata();
            if (meta == NULL)
                return;

            tk::ComboBox *cbox = tk::widget_cast<tk::ComboBox>(wWidget);
            if (cbox == NULL)
                return;

            tk::ListBoxItem *selected = cbox->selected()->get();
            ssize_t index   = (selected != NULL) ? selected->tag()->get() : 1;
            float v         = meta::limit_value(meta, index);
            ssize_t nindex  = v;

            // The value does not match?
            if (index != nindex)
            {
                selected = cbox->items()->get(index - 1);
                if (selected != NULL)
                    cbox->selected()->set(selected);
            }

            lsp_trace("index = %d, value=%f", int(index), v);

            pPort->set_value(v);
            pPort->notify_all(ui::PORT_USER_EDIT);
        }
    } /* namespace ctl */
} /* namespace lsp */





