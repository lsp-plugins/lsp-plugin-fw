/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 сент. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_TAPBUTTON_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_TAPBUTTON_H_

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
         * Led Meter widget controller
         */
        class TempoTap: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;
                ssize_t             nThresh;
                uint64_t            nLastTap;
                float               fTempo;

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

                ctl::Boolean        sEditable;
                ctl::Boolean        sHover;
                ctl::Padding        sTextPad;
                ctl::LCString       sText;

            protected:
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);

            protected:
                static uint64_t     time();
                void                submit_value();

            public:
                explicit TempoTap(ui::IWrapper *wrapper, tk::Button *widget);
                virtual ~TempoTap();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        end(ui::UIContext *ctx);

        };
    } // namespace ctl
} // namespace lsp




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_TAPBUTTON_H_ */
