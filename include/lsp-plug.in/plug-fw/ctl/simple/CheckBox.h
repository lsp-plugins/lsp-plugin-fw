/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 17 дек. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_CHECKBOX_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_CHECKBOX_H_

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
         * Switch button controller
         */
        class CheckBox: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ctl::Integer    sBorderSize;
                ctl::Integer    sBorderRadius;
                ctl::Integer    sBorderGapSize;
                ctl::Integer    sCheckRadius;
                ctl::Integer    sCheckGapSize;
                ctl::Integer    sCheckMinSize;

                ctl::Color      sColor;
                ctl::Color      sHoverColor;
                ctl::Color      sFillColor;
                ctl::Color      sFillHoverColor;
                ctl::Color      sBorderColor;
                ctl::Color      sBorderHoverColor;
                ctl::Color      sBorderGapColor;
                ctl::Color      sBorderGapHoverColor;

                ui::IPort      *pPort;
                float           fValue;
                bool            bInvert;

            protected:
                static status_t     slot_submit(tk::Widget *sender, void *ptr, void *data);

                void                commit_value(float value);
                void                submit_value();

            public:
                explicit CheckBox(ui::IWrapper *wrapper, tk::CheckBox *widget);
                virtual ~CheckBox() override;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
        };

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_CHECKBOX_H_ */
