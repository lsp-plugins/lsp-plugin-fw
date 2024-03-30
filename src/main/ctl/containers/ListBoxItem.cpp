/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 11 апр. 2024 г.
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

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(ListBoxItem)
            status_t res;

            if (!name->equals_ascii("option"))
                return STATUS_NOT_FOUND;

            tk::ListBoxItem *w = new tk::ListBoxItem(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::ListBoxItem *wc  = new ctl::ListBoxItem(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(ListBoxItem)

        //-----------------------------------------------------------------
        const ctl_class_t ListBoxItem::metadata    = { "ListBoxItem", &Widget::metadata };

        ListBoxItem::ListBoxItem(ui::IWrapper *wrapper, tk::ListBoxItem *widget):
            Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pSync           = NULL;
            bSelected       = false;
            fValue          = 0.0f;
        }

        ListBoxItem::~ListBoxItem()
        {
        }

        status_t ListBoxItem::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::ListBoxItem *item = tk::widget_cast<tk::ListBoxItem>(wWidget);
            if (item != NULL)
            {
                sText.init(pWrapper, item->text());
                sBgSelectedColor.init(pWrapper, item->bg_selected_color());
                sBgHoverColor.init(pWrapper, item->bg_hover_color());
                sTextColor.init(pWrapper, item->text_color());
                sTextSelectedColor.init(pWrapper, item->text_selected_color());
                sTextHoverColor.init(pWrapper, item->text_hover_color());

                sValue.init(pWrapper, this);
                sSelected.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void ListBoxItem::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::ListBoxItem *item = tk::widget_cast<tk::ListBoxItem>(wWidget);
            if (item != NULL)
            {
                set_param(item->text_adjust(), "text.adjust", name, value);
                set_param(item->text_adjust(), "tadjust", name, value);

                sText.set("text", name, value);
                sBgSelectedColor.set("bg.selected.color", name, value);
                sBgSelectedColor.set("bg.scolor", name, value);
                sBgHoverColor.set("bg.hover.color", name, value);
                sBgHoverColor.set("bg.hcolor", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sTextSelectedColor.set("text.selected.color", name, value);
                sTextSelectedColor.set("text.scolor", name, value);
                sTextHoverColor.set("text.hover.color", name, value);
                sTextHoverColor.set("text.hcolor", name, value);

                if (!strcmp(name, "selected"))
                    sSelected.parse(value);
                if (!strcmp(name, "value"))
                    sValue.parse(value);
            }

            return Widget::set(ctx, name, value);
        }

        void ListBoxItem::notify(ui::IPort *port, size_t flags)
        {
            bool notify = false;

            if (sSelected.depends(port))
            {
                bSelected = sSelected.evaluate_bool(false);
                notify = true;
            }
            if (sValue.depends(port))
            {
                fValue = sValue.evaluate_float(0.0f);
                notify = true;
            }

            if ((notify) && (pSync != NULL))
                pSync->child_changed(this);
        }

        void ListBoxItem::end(ui::UIContext *ctx)
        {
            if (sSelected.valid())
                bSelected = sSelected.evaluate_bool(false);
            if (sValue.valid())
                fValue = sValue.evaluate_float(0.0f);
        }

        void ListBoxItem::set_child_sync(IChildSync *sync)
        {
            pSync           = sync;
        }

        bool ListBoxItem::selected() const
        {
            return bSelected;
        }

        float ListBoxItem::value() const
        {
            return fValue;
        }

    } /* namespace ctl */
} /* namespace lsp */
