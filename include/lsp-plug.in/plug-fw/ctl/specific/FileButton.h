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
                enum file_status_t
                {
                    FB_SELECT_FILE,
                    FB_PROGRESS,
                    FB_SUCCESS,
                    FB_ERROR
                };

                class DragInSink: public tk::URLSink
                {
                    protected:
                        FileButton     *pButton;

                    public:
                        explicit DragInSink(FileButton *button);
                        virtual ~DragInSink() override;

                        void unbind();
                        virtual status_t    commit_url(const LSPString *url) override;
                };

            protected:
                size_t              nStatus;        // Current status of file load
                bool                bSave;
                ui::IPort          *pPort;          // Port that contains name of actual audio file
                ui::IPort          *pCommand;       // Port that triggers command for save/load operation
                ui::IPort          *pProgress;      // Port that indicates the loading progress
                ui::IPort          *pPathPort;      // Port that contains the current navigation path of file dialog

                DragInSink         *pDragInSink;
                tk::FileDialog     *pDialog;
                lltl::parray<file_format_t>     vFormats;

                ctl::Expression     sStatus;
                ctl::Expression     sProgress;
                ctl::Padding        sTextPadding;
                ctl::Boolean        sGradient;
                ctl::Integer        sBorderSize;
                ctl::Integer        sBorderPressedSize;
                ctl::Color          sColor;
                ctl::Color          sInvColor;
                ctl::Color          sBorderColor;
                ctl::Color          sInvBorderColor;
                ctl::Color          sLineColor;
                ctl::Color          sInvLineColor;
                ctl::Color          sTextColor;
                ctl::Color          sInvTextColor;

            protected:
                void                update_state();
                void                show_file_dialog();
                void                update_path();
                void                commit_file();

            protected:
                static status_t     slot_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_drag_request(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dialog_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dialog_hide(tk::Widget *sender, void *ptr, void *data);

            public:
                explicit FileButton(ui::IWrapper *wrapper, tk::FileButton *widget, bool save);
                virtual ~FileButton() override;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;
        };

    } /* namespace ctl */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_FILEBUTTON_H_ */
