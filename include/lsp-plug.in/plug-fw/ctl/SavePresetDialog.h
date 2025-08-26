/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 июн. 2025 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SAVEPRESETDIALOG_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SAVEPRESETDIALOG_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * The dialog for saving preset
         */
        class SavePresetDialog: public ctl::Window, public ui::IPresetListener
        {
            public:
                static const ctl_class_t metadata;

            protected:
                tk::Edit           *wPresetName;            // Preset name
                tk::MessageBox     *wConfirmation;          // Confirmation meesage box
                tk::MessageBox     *wNotification;          // Notification message box
                tk::CheckBox       *wFavourites;            // Add to favourites check box
                tk::Button         *wSaveButton;            // Save button

            protected:
                // Slots
                static status_t     slot_preset_name_change(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_save_button_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_accept_save_preset(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_reject_save_preset(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_cancel_save_preset(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_close_notification(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_favourites_click(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                sync_save_button_state();
                const ui::preset_t *find_user_preset_by_name(const LSPString *name);
                bool                request_confirmation(const LSPString *name);
                void                save_preset(const LSPString *name);
                bool                show_show_save_error(const LSPString *name, status_t code);

            public:
                explicit SavePresetDialog(ui::IWrapper *src, tk::Window *widget);
                SavePresetDialog(const SavePresetDialog &) = delete;
                SavePresetDialog(SavePresetDialog &&) = delete;
                virtual ~SavePresetDialog() override;

                SavePresetDialog & operator = (const SavePresetDialog &) = delete;
                SavePresetDialog & operator = (SavePresetDialog &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                status_t            post_init();
                status_t            show(tk::Widget *actor, const LSPString *name, bool favourite);
        };


    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SAVEPRESETDIALOG_H_ */
