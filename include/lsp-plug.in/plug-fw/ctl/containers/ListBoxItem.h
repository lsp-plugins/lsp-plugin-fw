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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_LISTBOXITEM_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_LISTBOXITEM_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * ComboBox controller
         */
        class ListBoxItem: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                IChildSync         *pSync;
                bool                bSelected;
                float               fValue;
                ctl::Expression     sSelected;
                ctl::Expression     sValue;
                ctl::LCString       sText;
                ctl::Color          sBgSelectedColor;
                ctl::Color          sBgHoverColor;
                ctl::Color          sTextColor;
                ctl::Color          sTextSelectedColor;
                ctl::Color          sTextHoverColor;

            public:
                explicit ListBoxItem(ui::IWrapper *wrapper, tk::ListBoxItem *widget);
                ListBoxItem(const ListBoxItem &) = delete;
                ListBoxItem(ListBoxItem &&) = delete;
                virtual ~ListBoxItem() override;

                ListBoxItem & operator = (const ListBoxItem &) = delete;
                ListBoxItem & operator = (ListBoxItem &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;

            public:
                void                set_child_sync(IChildSync *sync);
                bool                selected() const;
                float               value() const;
        };

    } /* namespace ctl */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_LISTBOXITEM_H_ */
