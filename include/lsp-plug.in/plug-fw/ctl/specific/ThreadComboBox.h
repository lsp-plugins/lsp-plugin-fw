/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 окт. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_THREADCOMBOBOX_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_THREADCOMBOBOX_H_

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
         * ComboBox controller that allows to select number of threads utilized by system
         */
        class ThreadComboBox: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;
                ctl::Color          sColor;
                ctl::Color          sSpinColor;
                ctl::Color          sTextColor;
                ctl::Color          sSpinTextColor;
                ctl::Color          sBorderColor;
                ctl::Color          sBorderGapColor;
                ctl::LCString       sEmptyText;

            protected:
                static status_t     slot_combo_submit(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                submit_value();

            public:
                explicit ThreadComboBox(ui::IWrapper *wrapper, tk::ComboBox *widget);
                virtual ~ThreadComboBox();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        notify(ui::IPort *port);
                virtual void        end(ui::UIContext *ctx);
        };

    } /* namespace ctl */
} /* namespace lsp */




#endif /* INCLUDE_LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_THREADCOMBOBOX_H_ */
