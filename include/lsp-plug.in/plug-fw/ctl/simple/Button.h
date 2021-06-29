/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_BUTTON_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_BUTTON_H_

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
         * Button widget controller
         */
        class Button: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                float               fValue;
                float               fDflValue;
                bool                bValueSet;
                ui::IPort          *pPort;

                ctl::Color          sColor;
                ctl::Color          sTextColor;
                ctl::Color          sBorderColor;
                ctl::Color          sHoverColor;
                ctl::Color          sTextHoverColor;
                ctl::Color          sBorderHoverColor;
                ctl::Color          sDownColor;
                ctl::Color          sTextDownColor;
                ctl::Color          sBorderDownColor;
                ctl::Color          sDownHoverColor;
                ctl::Color          sTextDownHoverColor;
                ctl::Color          sBorderDownHoverColor;
                ctl::Color          sHoleColor;

                ctl::Expression     sEditable;
                ctl::Boolean        sHover;
                ctl::Padding        sTextPad;
                ctl::LCString       sText;

            protected:
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);

                void                commit_value(float value);
                void                submit_value();
                float               next_value(bool down);
                void                trigger_expr();

            public:
                explicit Button(ui::IWrapper *wrapper, tk::Button *widget);
                virtual ~Button();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        notify(ui::IPort *port);
                virtual void        end(ui::UIContext *ctx);
                virtual void        schema_reloaded();
        };
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_BUTTON_H_ */
