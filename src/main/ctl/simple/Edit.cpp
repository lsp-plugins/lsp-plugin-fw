/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 15 апр. 2022 г.
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
        CTL_FACTORY_IMPL_START(Edit)
            status_t res;

            if (!name->equals_ascii("edit"))
                return STATUS_NOT_FOUND;

            tk::Edit *w     = new tk::Edit(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Edit *wc   = new ctl::Edit(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Edit)

        //-----------------------------------------------------------------
        const ctl_class_t Edit::metadata        = { "Edit", &Widget::metadata };

        Edit::Edit(ui::IWrapper *wrapper, tk::Edit *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;
        }

        Edit::~Edit()
        {
        }

        status_t Edit::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::Edit *ed = tk::widget_cast<tk::Edit>(wWidget);
            if (ed != NULL)
            {
                sEmptyText.init(pWrapper, ed->empty_text());
                sColor.init(pWrapper, ed->color());
                sBorderColor.init(pWrapper, ed->border_color());
                sBorderGapColor.init(pWrapper, ed->border_gap_color());
                sCursorColor.init(pWrapper, ed->cursor_color());
                sTextColor.init(pWrapper, ed->text_color());
                sTextSelectedColor.init(pWrapper, ed->text_selected_color());
                sEmptyTextColor.init(pWrapper, ed->placeholder_text_color());

                sBorderSize.init(pWrapper, ed->border_size());
                sBorderGapSize.init(pWrapper, ed->border_size());
                sBorderRadius.init(pWrapper, ed->border_radius());
            }

            return STATUS_OK;
        }

        void Edit::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Edit *ed = tk::widget_cast<tk::Edit>(wWidget);
            if (ed != NULL)
            {
                sEmptyText.set("text.empty", name, value);
                sEmptyText.set("etext", name, value);
                sColor.set("color", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sBorderGapColor.set("border.gap.color", name, value);
                sBorderGapColor.set("bgap.color", name, value);
                sCursorColor.set("cursor.color", name, value);
                sCursorColor.set("ccolor", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sEmptyTextColor.set("text.empty.color", name, value);
                sEmptyTextColor.set("etext.color", name, value);
                sTextSelectedColor.set("text.selected.color", name, value);
                sTextSelectedColor.set("tsel.color", name, value);

                sBorderSize.set("border.size", name, value);
                sBorderSize.set("bsize", name, value);
                sBorderGapSize.set("border.gap.size", name, value);
                sBorderGapSize.set("bgap.size", name, value);
                sBorderRadius.set("border.radius", name, value);
                sBorderRadius.set("bradius", name, value);

                set_constraints(ed->constraints(), name, value);
            }

            Widget::set(ctx, name, value);
        }

    } /* namespace ctl */
} /* namespace lsp */



