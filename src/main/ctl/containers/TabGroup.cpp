/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 дек. 2024 г.
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
        CTL_FACTORY_IMPL_START(TabGroup)
            status_t res;

            if (!name->equals_ascii("tgroup"))
                return STATUS_NOT_FOUND;

            tk::TabGroup *w = new tk::TabGroup(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::TabGroup *wc  = new ctl::TabGroup(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(TabGroup)

        //-----------------------------------------------------------------
        const ctl_class_t TabGroup::metadata  = { "TabGroup", &Widget::metadata };

        TabGroup::TabGroup(ui::IWrapper *wrapper, tk::TabGroup *cgroup): ctl::Widget(wrapper, cgroup)
        {
            pClass          = &metadata;

            pPort           = NULL;
            fMin            = 0.0f;
            fMax            = 0.0f;
            fStep           = 0.0f;
            nActive         = -1;
        }

        TabGroup::~TabGroup()
        {
        }

        status_t TabGroup::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::TabGroup *tg = tk::widget_cast<tk::TabGroup>(wWidget);
            if (tg != NULL)
            {
                // Bind slots
                tg->slots()->bind(tk::SLOT_SUBMIT, slot_tab_submit, this);

                sBorderColor.init(pWrapper, tg->border_color());
                sHeadingColor.init(pWrapper, tg->heading_color());
                sHeadingSpacingColor.init(pWrapper, tg->heading_spacing_color());
                sHeadingGapColor.init(pWrapper, tg->heading_gap_color());
                sBorderSize.init(pWrapper, tg->border_size());
                sBorderRadius.init(pWrapper, tg->border_radius());
                sTabSpacing.init(pWrapper, tg->tab_spacing());
                sHeadingSpacing.init(pWrapper, tg->heading_spacing());
                sHeadingGap.init(pWrapper, tg->heading_gap());
                sHeadingGapBrightness.init(pWrapper, tg->heading_gap_brightness());
                sEmbedding.init(pWrapper, tg->embedding());
                sTabJoint.init(pWrapper, tg->tab_joint());
                sHeadingFill.init(pWrapper, tg->heading_fill());
                sHeadingSpacingFill.init(pWrapper, tg->heading_spacing_fill());
                sActive.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void TabGroup::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::TabGroup *tg = tk::widget_cast<tk::TabGroup>(wWidget);
            if (tg != NULL)
            {
                bind_port(&pPort, "id", name, value);
                set_expr(&sActive, "active", name, value);

                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sHeadingColor.set("heading.color", name, value);
                sHeadingColor.set("hcolor", name, value);
                sHeadingSpacingColor.set("heading.spacing.color", name, value);
                sHeadingSpacingColor.set("hscolor", name, value);
                sHeadingGapColor.set("heading.gap.color", name, value);
                sHeadingGapColor.set("hgcolor", name, value);
                sBorderSize.set("border.size", name, value);
                sBorderSize.set("bsize", name, value);
                sBorderRadius.set("border.radius", name, value);
                sBorderRadius.set("bradius", name, value);

                sTabSpacing.set("tab.spacing", name, value);
                sHeadingSpacing.set("hspacing", name, value);
                sHeadingSpacing.set("heading.spacing", name, value);
                sHeadingGap.set("hgap", name, value);
                sHeadingGap.set("heading.gap", name, value);
                sHeadingGapBrightness.set("heading.gap.brightness", name, value);
                sHeadingGapBrightness.set("hgap.brightness", name, value);
                sEmbedding.set("embedding", name, value);
                sEmbedding.set("embed", name, value);
                sTabJoint.set("tab.joint", name, value);
                sHeadingFill.set("heading.fill", name, value);
                sHeadingSpacingFill.set("heading.spacing.fill", name, value);
                sHeadingSpacingFill.set("hspacing.fill", name, value);

                set_constraints(tg->constraints(), name, value);
                set_layout(tg->heading(), "heading", name, value);
                set_layout(tg->heading(), "head", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        status_t TabGroup::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::TabGroup *tg = tk::widget_cast<tk::TabGroup>(wWidget);
            return (tg != NULL) ? tg->widgets()->add(child->widget()) : STATUS_OK;
        }

        void TabGroup::end(ui::UIContext *ctx)
        {
            if (pPort != NULL)
                sync_metadata(pPort);

            if (sActive.valid())
                select_active_widget();

            Widget::end(ctx);
        }

        void TabGroup::notify(ui::IPort *port, size_t flags)
        {
            if (port == NULL)
                return;

            Widget::notify(port, flags);

            if (sActive.depends(port))
                select_active_widget();

            if (pPort == port)
            {
                tk::TabGroup *tg = tk::widget_cast<tk::TabGroup>(wWidget);
                if (tg != NULL)
                {
                    ssize_t index = (pPort->value() - fMin) / fStep;

                    tk::TabItem *ti = tg->items()->get(index);
                    tg->selected()->set(ti);
                }
            }
        }

        void TabGroup::select_active_widget()
        {
            tk::TabGroup *tg = tk::widget_cast<tk::TabGroup>(wWidget);
            if (tg == NULL)
                return;

            ssize_t index = (sActive.valid()) ? sActive.evaluate_int() : -1;
            tk::Widget *w = (index >= 0) ? tg->widgets()->get(index) : NULL;
            tg->active()->set(w);
        }

        status_t TabGroup::slot_tab_submit(tk::Widget *sender, void *ptr, void *data)
        {
            TabGroup *self   = static_cast<TabGroup *>(ptr);
            if (self != NULL)
                self->submit_value();
            return STATUS_OK;
        }

        void TabGroup::sync_metadata(ui::IPort *port)
        {
            tk::TabGroup *tg = tk::widget_cast<tk::TabGroup>(wWidget);
            if (tg == NULL)
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

                tk::WidgetList<tk::TabItem> *lst = tg->items();
                lst->clear();

                LSPString lck;
                tk::TabItem *ti;

                for (const meta::port_item_t *item = p->items; (item != NULL) && (item->text != NULL); ++item, ++i)
                {
                    ti  = new tk::TabItem(wWidget->display());
                    if (ti == NULL)
                        return;
                    ti->init();

                    ssize_t key     = fMin + fStep * i;
                    if (item->lc_key != NULL)
                    {
                        lck.set_ascii("lists.");
                        lck.append_ascii(item->lc_key);
                        ti->text()->set(&lck);
                    }
                    else
                        ti->text()->set_raw(item->text);
                    lst->madd(ti);

                    if (key == value)
                        tg->selected()->set(ti);
                }
            }
        }

        void TabGroup::submit_value()
        {
            if (pPort == NULL)
                return;

            tk::TabGroup *tg = tk::widget_cast<tk::TabGroup>(wWidget);
            if (tg == NULL)
                return;

            ssize_t index = tg->items()->index_of(tg->selected()->get());

            float value = fMin + fStep * index;
            lsp_trace("index = %d, value=%f", int(index), value);

            pPort->set_value(value);
            pPort->notify_all(ui::PORT_USER_EDIT);
        }

    } /* namespace ctl */
} /* namespace lsp */



