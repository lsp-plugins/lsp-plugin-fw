/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 23 мая 2021 г.
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
        CTL_FACTORY_IMPL_START(Rack)
            status_t res;

            if (!name->equals_ascii("rack"))
                return STATUS_NOT_FOUND;

            tk::RackEars *w = new tk::RackEars(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->add_widget(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Rack *wc  = new ctl::Rack(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Rack)

        //-----------------------------------------------------------------
        const ctl_class_t Rack::metadata        = { "Rack", &Widget::metadata };

        Rack::Rack(ui::IWrapper *wrapper, tk::RackEars *widget): Widget(wrapper, widget)
        {
            pClass      = &metadata;
        }

        Rack::~Rack()
        {
        }

        status_t Rack::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::RackEars *re = tk::widget_cast<tk::RackEars>(wWidget);
            if (re != NULL)
            {
                sColor.init(pWrapper, re->color());
                sTextColor.init(pWrapper, re->text_color());
                sScrewColor.init(pWrapper, re->screw_color());
                sHoleColor.init(pWrapper, re->hole_color());

                sButtonPadding.init(pWrapper, re->button_padding());
                sScrewPadding.init(pWrapper, re->screw_padding());
                sTextPadding.init(pWrapper, re->text_padding());

                sText.init(pWrapper, re->text());
            }

            return STATUS_OK;
        }

        void Rack::set(const char *name, const char *value)
        {
            tk::RackEars *re = tk::widget_cast<tk::RackEars>(wWidget);
            if (re != NULL)
            {
                set_font(re->font(), "font", name, value);
                set_param(re->angle(), "angle", name, value);
                set_param(re->screw_size(), "screw.size", name, value);

                sColor.set("color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sScrewColor.set("screw.color", name, value);
                sScrewColor.set("scolor", name, value);
                sHoleColor.set("hole.color", name, value);
                sHoleColor.set("hcolor", name, value);

                sButtonPadding.set("button.padding", name, value);
                sButtonPadding.set("bpadding", name, value);
                sButtonPadding.set("bpad", name, value);
                sScrewPadding.set("screw.padding", name, value);
                sScrewPadding.set("spadding", name, value);
                sScrewPadding.set("spad", name, value);
                sTextPadding.set("text.padding", name, value);
                sTextPadding.set("tpadding", name, value);
                sTextPadding.set("tpad", name, value);

                sText.set("text", name, value);
            }

            return Widget::set(name, value);
        }

        void Rack::end()
        {
            Widget::end();
        }
    }
}


