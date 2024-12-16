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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_HELPERS_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_HELPERS_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        void inject_style(tk::Widget *widget, const char *style_name);
        void revoke_style(tk::Widget *widget, const char *style_name);
        void add_parent_style(tk::Widget *widget, const char *style_name);

        void inject_style(tk::Widget *widget, const LSPString *style_name);
        void revoke_style(tk::Widget *widget, const LSPString *style_name);
        void add_parent_style(tk::Widget *widget, const LSPString *style_name);

        void inject_style(tk::Widget *widget, const LSPString &style_name);
        void revoke_style(tk::Widget *widget, const LSPString &style_name);
        void add_parent_style(tk::Widget *widget, const LSPString &style_name);

        bool assign_styles(tk::Widget *widget, const char *style_list, bool remove_parents);
    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_HELPERS_H_ */
