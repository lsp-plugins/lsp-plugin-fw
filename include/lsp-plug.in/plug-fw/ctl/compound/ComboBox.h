/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 июн. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_COMPOUND_COMBOBOX_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_COMPOUND_COMBOBOX_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        class ListBoxItem;

        /**
         * ComboBox controller
         */
        class ComboBox: public Widget, public IChildSync
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort                  *pPort;

                ctl::Color                  sColor;
                ctl::Color                  sSpinColor;
                ctl::Color                  sTextColor;
                ctl::Color                  sSpinTextColor;
                ctl::Color                  sBorderColor;
                ctl::Color                  sBorderGapColor;
                ctl::Color                  sInactiveColor;
                ctl::Color                  sInactiveSpinColor;
                ctl::Color                  sInactiveTextColor;
                ctl::Color                  sInactiveSpinTextColor;
                ctl::Color                  sInactiveBorderColor;
                ctl::Color                  sInactiveBorderGapColor;

                ctl::LCString               sEmptyText;
                lltl::parray<ListBoxItem>   vItems;             // Custom items

                float                       fMin;
                float                       fMax;
                float                       fStep;

            protected:
                static status_t     slot_combo_submit(tk::Widget *sender, void *ptr, void *data);

            protected:
                virtual void        sync_metadata(ui::IPort *port) override;
                void                submit_value();
                void                do_destroy();
                void                update_selection();

            public:
                explicit ComboBox(ui::IWrapper *wrapper, tk::ComboBox *widget);
                ComboBox(const ComboBox &) = delete;
                ComboBox(ComboBox &&) = delete;
                virtual ~ComboBox() override;

                ComboBox & operator = (const ComboBox &) = delete;
                ComboBox & operator = (ComboBox &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public: // ctl::Widget
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual status_t    add(ui::UIContext *ctx, ctl::Widget *child) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;

            public: // ctl::IChildSync
                virtual void        child_changed(Widget *child) override;

        };

    } /* namespace ctl */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_COMPOUND_COMBOBOX_H_ */
