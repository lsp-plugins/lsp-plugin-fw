/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 31 июл. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_LEDMETER_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_LEDMETER_H_

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
        class LedMeter: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ctl::LCString       sEstText;
                ctl::Color          sColor;
                lltl::parray<Widget> vChildren;

            protected:
                static status_t     slot_mouse_click(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                on_mouse_click(const ws::event_t *ev);

            public:
                explicit LedMeter(ui::IWrapper *wrapper, tk::LedMeter *widget);
                LedMeter(const LedMeter &) = delete;
                LedMeter(LedMeter &&) = delete;
                virtual ~LedMeter() override;

                LedMeter & operator = (const LedMeter &) = delete;
                LedMeter & operator = (LedMeter &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual status_t    add(ui::UIContext *ctx, ctl::Widget *child) override;
        };
    } /* namespace ctl */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_LEDMETER_H_ */
