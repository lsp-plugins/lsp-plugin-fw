/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 2 окт. 2021 г.
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
        CTL_FACTORY_IMPL_START(Fraction)
            status_t res;
            if (!name->equals_ascii("frac"))
                return STATUS_NOT_FOUND;

            tk::Fraction *w = new tk::Fraction(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Fraction *wc  = new ctl::Fraction(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Fraction)

        //-----------------------------------------------------------------
        const ctl_class_t Fraction::metadata            = { "Fraction", &Widget::metadata };

        Fraction::Fraction(ui::IWrapper *wrapper, tk::Fraction *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            pDen            = NULL;

            fSig            = 0.0f;
            fMaxSig         = 2.0f;
            nNum            = 0;
            nDenom          = 4;
            nDenomMin       = 1;
            nDenomMax       = 64;
        }

        Fraction::~Fraction()
        {
        }

        status_t Fraction::init()
        {
            status_t res = Widget::init();
            if (res != STATUS_OK)
                return res;

            tk::Fraction *fr = tk::widget_cast<tk::Fraction>(wWidget);
            if (fr != NULL)
            {
                sAngle.init(pWrapper, fr->angle());
                sTextPad.init(pWrapper, fr->text_pad());
                sThick.init(pWrapper, fr->thickness());

                sColor.init(pWrapper, fr->color());
                sNumColor.init(pWrapper, fr->num_color());
                sDenColor.init(pWrapper, fr->den_color());

                // Bind slots
                fr->slots()->bind(tk::SLOT_CHANGE, slot_change, this);
                fr->slots()->bind(tk::SLOT_CHANGE, slot_submit, this);
            }

            return STATUS_OK;
        }

        void Fraction::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Fraction *fr = tk::widget_cast<tk::Fraction>(wWidget);
            if (fr != NULL)
            {
                // Bind ports
                bind_port(&pPort, "id", name, value);
                bind_port(&pDen, "denominator.id", name, value);
                bind_port(&pDen, "denom.id", name, value);
                bind_port(&pDen, "den.id", name, value);

                // Set simple properties
                set_font(fr->font(), "font", name, value);
                set_value(&fMaxSig, "max", name, value);

                // Set complicated properties
                sColor.set("color", name, value);
                sNumColor.set("numerator.color", name, value);
                sNumColor.set("num.color", name, value);
                sDenColor.set("denominator.color", name, value);
                sDenColor.set("denom.color", name, value);
                sDenColor.set("den.color", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        status_t Fraction::add_list_item(tk::WidgetList<tk::ListBoxItem> *list, int i, const char *text)
        {
            // Create item
            tk::ListBoxItem *item = new tk::ListBoxItem(wWidget->display());
            if (item == NULL)
                return STATUS_NO_MEM;
            status_t res = item->init();
            if (res != STATUS_OK)
            {
                delete item;
                return res;
            }

            // Add item to list
            if ((list->madd(item)) != STATUS_OK)
            {
                item->destroy();
                delete item;
                return STATUS_NO_MEM;
            }

            if (text == NULL)
            {
                LSPString v;
                v.fmt_ascii("%d", int(i));
                item->text()->set_raw(&v);
            }
            else
                item->text()->set(text);
            item->tag()->set(i);

            return STATUS_OK;
        }

        void Fraction::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            tk::Fraction *fr = tk::widget_cast<tk::Fraction>(wWidget);
            if (fr == NULL)
                return;

            tk::WidgetList<tk::ListBoxItem> *dl = fr->den_items();
            dl->clear();

            if (pDen != NULL)
            {
                const meta::port_t *p = pDen->metadata();
                if (p == NULL)
                    return;

                if (p->flags & meta::F_LOWER)
                    nDenomMin   = p->min;
                if (meta::is_enum_unit(p->unit))
                    nDenomMax   = nDenomMin + meta::list_size(p->items);
                else if (p->flags & meta::F_UPPER)
                    nDenomMax   = p->max;

                // Generate denominator list
                if (meta::is_enum_unit(p->unit))
                {
                    for (ssize_t i=nDenomMin; i<=nDenomMax; ++i)
                        add_list_item(dl, i, p->items[i].text);
                }
                else
                {
                    for (ssize_t i=nDenomMin; i<=nDenomMax; ++i)
                        add_list_item(dl, i, NULL);
                }
            }
            else
            {
                for (ssize_t i=nDenomMin; i<=nDenomMax; ++i)
                    add_list_item(dl, i, NULL);
            }

            if (nDenom < nDenomMin)
                nDenom = nDenomMin;
            else if (nDenom > nDenomMax)
                nDenom = nDenomMax;

            // Call for values update
            update_values(NULL);
        }

        void Fraction::notify(ui::IPort *port, size_t flags)
        {
            if ((port == pPort) || (port == pDen))
                update_values(port);

            Widget::notify(port, flags);
        }

        void Fraction::update_values(ui::IPort *port)
        {
            tk::Fraction *fr = tk::widget_cast<tk::Fraction>(wWidget);
            if (fr == NULL)
                return;

            if ((port == pDen) && (pDen != NULL))
            {
                nDenom      = pDen->value();
                lsp_trace("nDenom = %d", int(nDenom));
            }
            if ((port == pPort) && (pPort != NULL))
            {
                fSig        = lsp_limit(pPort->value(), 0.0f, fMaxSig);
            }

            tk::ListBoxItem *sel = fr->den_items()->get(nDenom - 1);
            fr->den_selected()->set(sel);

            sync_numerator();
        }

        void Fraction::submit_value()
        {
            tk::Fraction *fr = tk::widget_cast<tk::Fraction>(wWidget);
            if (fr == NULL)
                return;

            tk::ListBoxItem *num_item   = fr->num_selected()->get();
            tk::ListBoxItem *den_item   = fr->den_selected()->get();

            nNum        = (num_item != NULL) ? fr->num_items()->index_of(num_item) : 0;
            nDenom      = (den_item != NULL) ? fr->den_items()->index_of(den_item) + 1 : 1;

            ssize_t num_min = 0;
            ssize_t num_max = fMaxSig * nDenom;
            if (nNum < num_min)
                nNum    = num_min;
            else if (nNum > num_max)
                nNum    = num_max;

            fSig        = float(nNum) / float(nDenom);
            sync_numerator();

            if (pPort != NULL)
                pPort->set_value(fSig);
            if (pDen != NULL)
                pDen->set_value(nDenom);

            if (pPort != NULL)
                pPort->notify_all(ui::PORT_USER_EDIT);
            if (pDen != NULL)
                pDen->notify_all(ui::PORT_USER_EDIT);
        }

        void Fraction::sync_numerator()
        {
            tk::Fraction *fr = tk::widget_cast<tk::Fraction>(wWidget);
            if (fr == NULL)
                return;

            tk::WidgetList<tk::ListBoxItem> *nl = fr->num_items();

            // Add missing list items
            ssize_t num_max = fMaxSig * nDenom + 0.5f;
            for (ssize_t i = nl->size(); i <= num_max; ++i)
                add_list_item(nl, i, NULL);

            nl->truncate(num_max + 1);

            // Update selected item
            nNum            = fSig * nDenom;
            tk::ListBoxItem *sel = fr->num_items()->get(nNum);
            fr->num_selected()->set(sel);
        }

        status_t Fraction::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Fraction *_this    = static_cast<ctl::Fraction *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

        status_t Fraction::slot_submit(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Fraction *_this    = static_cast<ctl::Fraction *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */



