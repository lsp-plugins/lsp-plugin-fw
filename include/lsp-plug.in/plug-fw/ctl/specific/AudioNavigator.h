/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 28 сент. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIONAVIGATOR_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIONAVIGATOR_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        class AudioNavigator: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                enum action_t
                {
                    A_NONE,
                    A_FIRST,
                    A_LAST,
                    A_NEXT,
                    A_PREVIOUS,
                    A_FAST_FORWARD,
                    A_FAST_BACKWARD,
                    A_RANDOM,
                    A_CLEAR
                };

            protected:
                static action_t     parse_action(const char *action);

            protected:
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                sync_state();
                void                set_activity(bool active);
                void                update_styles();
                void                apply_action();

            protected:
                ui::IPort                  *pPort;

                ctl::Color                  sColor;
                ctl::Color                  sTextColor;
                ctl::Color                  sBorderColor;
                ctl::Color                  sHoverColor;
                ctl::Color                  sTextHoverColor;
                ctl::Color                  sBorderHoverColor;
                ctl::Color                  sHoleColor;

                ctl::Boolean                sEditable;
                ctl::Boolean                sHover;
                ctl::Padding                sTextPad;
                ctl::LCString               sText;

                bool                        bActive;        // Navigator is active
                action_t                    enAction;       // Actual action
                ctl::DirController          sDirController; // Directory controller

            public:
                explicit AudioNavigator(ui::IWrapper *wrapper, tk::Button *widget);
                AudioNavigator(const AudioNavigator &) = delete;
                AudioNavigator(AudioNavigator &&) = delete;
                virtual ~AudioNavigator() override;

                AudioNavigator & operator = (const AudioNavigator &) = delete;
                AudioNavigator & operator = (AudioNavigator &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
        };

    } /* namespace ctl */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIONAVIGATOR_H_ */
