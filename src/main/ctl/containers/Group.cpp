/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 апр. 2021 г.
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
        CTL_FACTORY_IMPL_START(Group)
            status_t res;

            if (!name->equals_ascii("group"))
                return STATUS_NOT_FOUND;

            tk::Group *w = new tk::Group(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->add_widget(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Group *wc  = new ctl::Group(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Group)

        //-----------------------------------------------------------------
        const ctl_class_t Group::metadata     = { "Group", &Widget::metadata };

        Group::Group(ui::IWrapper *wrapper, tk::Group *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        Group::~Group()
        {
        }

        status_t Group::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Group *grp  = tk::widget_cast<tk::Group>(wWidget);
            if (grp != NULL)
            {
                sTextColor.init(pWrapper, grp->text_color());
                sColor.init(pWrapper, grp->text_color());
                sEmbed.init(pWrapper, grp->embedding());
            }

            return STATUS_OK;
        }

        void Group::set(const char *name, const char *value)
        {
            tk::Group *grp  = tk::widget_cast<tk::Group>(wWidget);
            if (grp != NULL)
            {
                set_constraints(grp->constraints(), name, value);
                set_layout(grp->layout(), name, value);
                set_lc_attr(grp->text(), "text", name, value);
                set_font(grp->font(), "font", name, value);
                set_param(grp->show_text(), "text.show", name, value);
                set_param(grp->text_border(), "text.border", name, value);
                set_param(grp->text_radius(), "text.radius", name, value);
                set_param(grp->text_radius(), "text.r", name, value);
                set_param(grp->border_size(), "border.size", name, value);
                set_param(grp->border_size(), "border.sz", name, value);
                set_param(grp->border_radius(), "border.radius", name, value);
                set_param(grp->border_radius(), "border.r", name, value);
                set_padding(grp->ipadding(), "ipadding", name, value);
                set_padding(grp->ipadding(), "ipad", name, value);

                sTextColor.set("text.color", name, value);
                sColor.set("color", name, value);
            }

            sEmbed.set("embed", name, value);
            Widget::set(name, value);
        }

        status_t Group::add(ctl::Widget *child)
        {
            tk::Group *grp  = tk::widget_cast<tk::Group>(wWidget);
            return (grp != NULL) ? grp->add(child->widget()) : STATUS_BAD_STATE;
        }

    } /* namespace ctl */
} /* namespace lsp */




