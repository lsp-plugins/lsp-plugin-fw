/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 22 сент. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_FILEBUTTON_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_FILEBUTTON_H_

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
         * Rack widget controller
         */
        class FileButton: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                bool                bSave;
                ui::IPort          *pFile;
                ui::IPort          *pCommand;
                ui::IPort          *pProgress;
                ui::IPort          *pPath;

                ctl::Expression     sStatus;
                ctl::Expression     sProgress;
                ctl::Padding        sTextPadding;
                ctl::Color          sColor;
                ctl::Color          sInvColor;
                ctl::Color          sLineColor;
                ctl::Color          sInvLineColor;
                ctl::Color          sTextColor;
                ctl::Color          sInvTextColor;

            protected:
                void                trigger_expr();
                void                on_submit();

            protected:
                static status_t     slot_submit(tk::Widget *sender, void *ptr, void *data);

            public:
                explicit FileButton(ui::IWrapper *wrapper, tk::FileButton *widget, bool save);
                virtual ~FileButton();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        end(ui::UIContext *ctx);
                virtual void        notify(ui::IPort *port);
                virtual void        reloaded(const tk::StyleSheet *sheet);
        };

    } /* namespace ctl */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_FILEBUTTON_H_ */
