/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_EDIT_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_EDIT_H_

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
         * Edit widget controller
         */
        class Edit: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;
                tk::Timer           sTimer;
                ssize_t             nInputDelay;

                ctl::LCString       sEmptyText;

                ctl::Color          sColor;
                ctl::Color          sBorderColor;
                ctl::Color          sBorderGapColor;
                ctl::Color          sCursorColor;
                ctl::Color          sTextColor;
                ctl::Color          sEmptyTextColor;
                ctl::Color          sTextSelectedColor;
                ctl::Color          sInactiveColor;
                ctl::Color          sInactiveBorderColor;
                ctl::Color          sInactiveBorderGapColor;
                ctl::Color          sInactiveCursorColor;
                ctl::Color          sInactiveTextColor;
                ctl::Color          sInactiveEmptyTextColor;
                ctl::Color          sInactiveTextSelectedColor;

                ctl::Integer        sBorderSize;
                ctl::Integer        sBorderGapSize;
                ctl::Integer        sBorderRadius;

            protected:
                static status_t     slot_key_up(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_change_value(tk::Widget *sender, void *ptr, void *data);
                static status_t     timer_fired(ws::timestamp_t sched, ws::timestamp_t time, void *arg);

            protected:
                void                commit_value();
                void                submit_value();
                void                setup_timer();
                const char         *get_input_style();

            public:
                explicit Edit(ui::IWrapper *wrapper, tk::Edit *widget);
                Edit(const Edit &) = delete;
                Edit(Edit &&) = delete;
                virtual ~Edit() override;
                Edit & operator = (const Edit &) = delete;
                Edit & operator = (Edit &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_EDIT_H_ */
