/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 апр. 2021 г.
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
        void inject_style(tk::Widget *widget, const char *style_name)
        {
            tk::Style *style = widget->display()->schema()->get(style_name);
            if (style != NULL)
                widget->style()->inject_parent(style);
        }

        void add_parent_style(tk::Widget *widget, const char *style_name)
        {
            tk::Style *style = widget->display()->schema()->get(style_name);
            if (style != NULL)
                widget->style()->add_parent(style);
        }

        void revoke_style(tk::Widget *widget, const char *style_name)
        {
            tk::Style *style = widget->display()->schema()->get(style_name);
            if (style != NULL)
                widget->style()->remove_parent(style);
        }

        bool assign_styles(tk::Widget *widget, const char *style_list, bool remove_parents)
        {
            if (widget == NULL)
                return false;

            tk::Style *s = widget->style();
            if (s == NULL)
                return false;

            LSPString cname, text;
            if (!text.set_utf8(style_list))
                return false;

            if (remove_parents)
                s->remove_all_parents();

            // Parse list of
            ssize_t first = 0, last = -1, len = text.length();

            while (true)
            {
                last = text.index_of(first, ',');
                if (last < 0)
                {
                    last = len;
                    break;
                }

                if (!cname.set(&text, first, last))
                    return false;

                add_parent_style(widget, cname.get_utf8());
                first = last + 1;
            }

            // Last token pending?
            if (last > first)
            {
                if (!cname.set(&text, first, last))
                    return false;
                add_parent_style(widget, cname.get_utf8());
            }

            return true;
        }

    } /* namespace ctl */
} /* namespace lsp */


