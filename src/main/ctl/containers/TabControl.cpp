/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 нояб. 2022 г.
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
        CTL_FACTORY_IMPL_START(TabControl)
            status_t res;

            if (!name->equals_ascii("tabs"))
                return STATUS_NOT_FOUND;

            tk::TabControl *w = new tk::TabControl(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::TabControl *wc  = new ctl::TabControl(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(TabControl)

        //-----------------------------------------------------------------
        const ctl_class_t TabControl::metadata  = { "TabControl", &Widget::metadata };

        TabControl::TabControl(ui::IWrapper *wrapper, tk::TabControl *tc): ctl::Widget(wrapper, tc)
        {
            pClass          = &metadata;

            pPort           = NULL;
            fMin            = 0.0f;
            fMax            = 0.0f;
            fStep           = 0.0f;
        }

        TabControl::~TabControl()
        {
        }

        status_t TabControl::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::TabControl *tc = tk::widget_cast<tk::TabControl>(wWidget);
            if (tc != NULL)
            {
                // Bind slots
                tc->slots()->bind(tk::SLOT_SUBMIT, slot_submit, this);

                sBorderColor.init(pWrapper, tc->border_color());
                sHeadingColor.init(pWrapper, tc->heading_color());
                sHeadingSpacingColor.init(pWrapper, tc->heading_spacing_color());
                sHeadingGapColor.init(pWrapper, tc->heading_gap_color());
                sBorderSize.init(pWrapper, tc->border_size());
                sBorderRadius.init(pWrapper, tc->border_radius());
                sTabSpacing.init(pWrapper, tc->tab_spacing());
                sHeadingSpacing.init(pWrapper, tc->heading_spacing());
                sHeadingGap.init(pWrapper, tc->heading_gap());
                sHeadingGapBrightness.init(pWrapper, tc->heading_gap_brightness());
                sEmbedding.init(pWrapper, tc->embedding());
                sTabJoint.init(pWrapper, tc->tab_joint());
                sHeadingFill.init(pWrapper, tc->heading_fill());
                sHeadingSpacingFill.init(pWrapper, tc->heading_spacing_fill());
                sActive.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void TabControl::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::TabControl *tc = tk::widget_cast<tk::TabControl>(wWidget);
            if (tc != NULL)
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

                set_constraints(tc->constraints(), name, value);
                set_layout(tc->heading(), "heading", name, value);
                set_layout(tc->heading(), "head", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        tk::Tab *TabControl::create_new_tab(tk::Widget *child, tk::Registry *registry)
        {
            // We need to create tab wrapper
            tk::Tab *t      = new tk::Tab(wWidget->display());
            if (t == NULL)
                return NULL;
            lsp_finally {
                if (t != NULL)
                {
                    t->destroy();
                    delete t;
                }
            };

            if (t->init() != STATUS_OK)
                return NULL;
            if ((child != NULL) && (t->add(child) != STATUS_OK))
                return NULL;
            if ((registry != NULL) && (registry->add(t) != STATUS_OK))
                return NULL;

            return lsp::release(t);
        }

        status_t TabControl::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            tk::TabControl *tc = tk::widget_cast<tk::TabControl>(wWidget);
            if (tc == NULL)
                return STATUS_OK;

            // Cast passed widget to tab
            tk::Tab *tab = tk::widget_cast<tk::Tab>(child->widget());
            if (tab == NULL)
            {
                // We need to create tab wrapper
                if ((tab = create_new_tab(child->widget(), ctx->widgets())) == NULL)
                    return STATUS_NO_MEM;
            }

            // Register tab in the tab list
            if (!vTabs.add(tab))
                return STATUS_NO_MEM;

            return tc->add(tab);
        }

        void TabControl::end(ui::UIContext *ctx)
        {
            // Process widgets, create tab widgets if necessary
            if (pPort != NULL)
                sync_metadata(pPort);

            if (sActive.valid())
                select_active_widget();

            Widget::end(ctx);
        }

        void TabControl::notify(ui::IPort *port, size_t flags)
        {
            if (port == NULL)
                return;

            Widget::notify(port, flags);

            if (sActive.depends(port))
                select_active_widget();

            if (pPort == port)
            {
                tk::TabControl *tc = tk::widget_cast<tk::TabControl>(wWidget);
                if (tc != NULL)
                {
                    ssize_t index   = (pPort->value() - fMin) / fStep;

                    tk::Tab *tab    = tc->widgets()->get(index);
                    tc->selected()->set(tab);
                }
            }
        }

        void TabControl::select_active_widget()
        {
            tk::TabControl *tc = tk::widget_cast<tk::TabControl>(wWidget);
            if (tc == NULL)
                return;

            ssize_t index = (sActive.valid()) ? sActive.evaluate_int() : -1;
            tk::Tab *tab  = (index >= 0) ? tc->widgets()->get(index) : NULL;
            tc->selected()->set(tab);
        }

        status_t TabControl::slot_submit(tk::Widget *sender, void *ptr, void *data)
        {
            TabControl *_this   = static_cast<TabControl *>(ptr);
            if (_this != NULL)
                _this->submit_value();
            return STATUS_OK;
        }

        void TabControl::sync_metadata(ui::IPort *port)
        {
            tk::TabControl *tc = tk::widget_cast<tk::TabControl>(wWidget);
            if (tc == NULL)
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

                tk::WidgetList<tk::Tab> *lst = tc->widgets();
                lst->clear();

                LSPString lck;
                tk::Tab *tab;

                for (const meta::port_item_t *item = p->items; (item != NULL) && (item->text != NULL); ++item, ++i)
                {
                    tab = vTabs.get(i);
                    if (tab == NULL)
                    {
                        if ((tab = create_new_tab(NULL, NULL)) == NULL)
                            return;
                        lst->madd(tab);
                    }
                    else
                        lst->add(tab);

                    ssize_t key     = fMin + fStep * i;
                    if (item->lc_key != NULL)
                    {
                        lck.set_ascii("lists.");
                        lck.append_ascii(item->lc_key);
                        tab->text()->set(&lck);
                    }
                    else
                        tab->text()->set_raw(item->text);

                    if (key == value)
                        tc->selected()->set(tab);
                }
            }
        }

        void TabControl::submit_value()
        {
            if (pPort == NULL)
                return;

            tk::TabControl *tc = tk::widget_cast<tk::TabControl>(wWidget);
            if (tc == NULL)
                return;

            ssize_t index = tc->widgets()->index_of(tc->selected()->get());

            float value = fMin + fStep * index;
            lsp_trace("index = %d, value=%f", int(index), value);

            pPort->set_value(value);
            pPort->notify_all(ui::PORT_USER_EDIT);
        }

    } /* namespace ctl */
} /* namespace lsp */
