/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 сент. 2024 г.
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


#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOFOLDER_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOFOLDER_H_

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
         * AudioFolder controller
         */
        class AudioFolder: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;
                ui::IPort          *pAutoload;

                ctl::Enum           sHScroll;
                ctl::Enum           sVScroll;

                bool                bActive;        // Navigator is active
                ctl::DirController  sDirController; // Directory controller

            protected:
                static status_t     slot_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                sync_state();
                void                set_activity(bool active);
                void                update_styles();
                void                apply_action();
                bool                sync_list();

            public:
                explicit AudioFolder(ui::IWrapper *wrapper, tk::ListBox *widget);
                AudioFolder(const AudioFolder &) = delete;
                AudioFolder(AudioFolder &&) = delete;
                virtual ~AudioFolder() override;

                AudioFolder & operator = (const AudioFolder &) = delete;
                AudioFolder & operator = (AudioFolder &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOFOLDER_H_ */
