/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 27 июн. 2025 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ctl
    {
        //-----------------------------------------------------------------
        const ctl_class_t SavePresetDialog::metadata = { "SavePresetDialog", &Window::metadata };

        SavePresetDialog::SavePresetDialog(ui::IWrapper *src, tk::Window *widget):
            ctl::Window(src, widget)
        {
            wPresetName     = NULL;
            wConfirmation   = NULL;
            wNotification   = NULL;
            wFavourites     = NULL;
            wSaveButton     = NULL;
        }

        SavePresetDialog::~SavePresetDialog()
        {
        }

        void SavePresetDialog::destroy()
        {
            ctl::Window::destroy();
        }

        status_t SavePresetDialog::init()
        {
            LSP_STATUS_ASSERT(ctl::Window::init());

            return STATUS_OK;
        }

        status_t SavePresetDialog::post_init()
        {
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            wnd->actions()->set(ws::WA_DIALOG);
            if (wnd->border_style()->get() != ws::BS_DIALOG)
                lsp_trace("Not a dialog");

            wPresetName     = widgets()->get<tk::Edit>("preset_name");
            wFavourites     = widgets()->get<tk::CheckBox>("favourites_check");
            wSaveButton     = widgets()->get<tk::Button>("btn_save");

            wWidget->slots()->add(tk::SLOT_SUBMIT);
            wWidget->slots()->add(tk::SLOT_CANCEL);
            wWidget->slots()->bind(tk::SLOT_CLOSE, slot_cancel_save_preset, self());

            bind_slot("preset_name", tk::SLOT_CHANGE, slot_preset_name_change);
            bind_slot("btn_save", tk::SLOT_SUBMIT, slot_save_button_submit);
            bind_slot("btn_cancel", tk::SLOT_SUBMIT, slot_cancel_save_preset);
            bind_slot("favourites_label", tk::SLOT_MOUSE_CLICK, slot_favourites_click);
            bind_shortcut(wnd, ws::WSK_ESCAPE, tk::KM_NONE, slot_cancel_save_preset);
            bind_shortcut(wnd, ws::WSK_RETURN, tk::KM_NONE, slot_save_button_submit);
            bind_shortcut(wnd, ws::WSK_KEYPAD_ENTER, tk::KM_NONE, slot_save_button_submit);

            return STATUS_OK;
        }

        status_t SavePresetDialog::show(tk::Widget *actor, const LSPString *name, bool favourite)
        {
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            // Set preset name
            if (wPresetName != NULL)
            {
                wPresetName->text()->set_raw(name);
                wPresetName->selection()->set_all();
            }

            // Set favourites check
            if (wFavourites != NULL)
                wFavourites->checked()->set(favourite);

            sync_save_button_state();
            wnd->show(actor);

            if (wPresetName != NULL)
                wPresetName->take_focus();

            return STATUS_OK;
        }

        void SavePresetDialog::sync_save_button_state()
        {
            if (wSaveButton == NULL)
                return;

            LSPString name;
            status_t res = wPresetName->text()->format(&name);
            name.trim();

            const bool active = (res == STATUS_OK) && (name.length() > 0);
            wSaveButton->active()->set(active);
            wSaveButton->editable()->set(active);
        }

        const ui::preset_t *SavePresetDialog::find_user_preset_by_name(const LSPString *name)
        {
            if (wPresetName == NULL)
                return NULL;

            const ui::preset_t *list = pWrapper->all_presets();
            const size_t count = pWrapper->num_presets();

            for (size_t i=0; i<count; ++i)
            {
                const ui::preset_t *preset = &list[i];
                if (!(preset->flags & ui::PRESET_FLAG_USER))
                    continue;
                if (preset->name.equals_nocase(name))
                    return preset;
            }

            return NULL;
        }

        //-----------------------------------------------------------------
        // Slots
        status_t SavePresetDialog::slot_preset_name_change(tk::Widget *sender, void *ptr, void *data)
        {
            SavePresetDialog *self = static_cast<SavePresetDialog *>(ptr);
            if (self != NULL)
                self->sync_save_button_state();

            return STATUS_OK;
        }

        bool SavePresetDialog::request_confirmation(const LSPString *name)
        {
            // Create confirmation dialog if needed
            if (wConfirmation == NULL)
            {
                // Create and initialize dialog object
                tk::MessageBox *dialog = new tk::MessageBox(wWidget->display());
                if (dialog == NULL)
                    return false;
                lsp_finally {
                    if (dialog != NULL)
                    {
                        dialog->destroy();
                        delete dialog;
                    }
                };

                status_t res = dialog->init();
                if (res != STATUS_OK)
                {
                    lsp_trace("init failed");
                    return false;
                }

                dialog->title()->set("titles.confirmation");
                dialog->heading()->set("headings.confirmation");
                dialog->add("actions.confirm.yes", slot_accept_save_preset, self());
                dialog->add("actions.confirm.no", slot_reject_save_preset, self());
                dialog->add("actions.cancel", slot_cancel_save_preset, self());

                dialog->buttons()->get(0)->constraints()->set_min_width(96);
                dialog->buttons()->get(1)->constraints()->set_min_width(96);
                dialog->buttons()->get(2)->constraints()->set_min_width(96);

                bind_shortcut(dialog, ws::WSK_ESCAPE, tk::KM_NONE, slot_cancel_save_preset);
                bind_shortcut(dialog, 'c', tk::KM_NONE, slot_cancel_save_preset);
                bind_shortcut(dialog, 'C', tk::KM_NONE, slot_cancel_save_preset);
                bind_shortcut(dialog, ws::WSK_BACKSPACE, tk::KM_NONE, slot_reject_save_preset);
                bind_shortcut(dialog, 'n', tk::KM_NONE, slot_reject_save_preset);
                bind_shortcut(dialog, 'N', tk::KM_NONE, slot_reject_save_preset);
                bind_shortcut(dialog, ws::WSK_RETURN, tk::KM_NONE, slot_accept_save_preset);
                bind_shortcut(dialog, ws::WSK_KEYPAD_ENTER, tk::KM_NONE, slot_accept_save_preset);
                bind_shortcut(dialog, 'y', tk::KM_NONE, slot_accept_save_preset);
                bind_shortcut(dialog, 'Y', tk::KM_NONE, slot_accept_save_preset);

                // Commit dialog
                if (widgets()->add(dialog) != STATUS_OK)
                    return false;
                wConfirmation   = release_ptr(dialog);
            }

            // Update text
            expr::Parameters params;
            params.set_string("name", name);
            wConfirmation->message()->set(
                "messages.presets.confirm_overwrite",
                &params);

            wConfirmation->show(wWidget);

            return true;
        }

        bool SavePresetDialog::show_show_save_error(const LSPString *name, status_t code)
        {
            // Create confirmation dialog if needed
            if (wNotification == NULL)
            {
                // Create and initialize dialog object
                tk::MessageBox *dialog = new tk::MessageBox(wWidget->display());
                if (dialog == NULL)
                    return false;
                lsp_finally {
                    if (dialog != NULL)
                    {
                        dialog->destroy();
                        delete dialog;
                    }
                };

                status_t res = dialog->init();
                if (res != STATUS_OK)
                    return false;

                dialog->title()->set("titles.attention");
                dialog->heading()->set("headings.attention");
                dialog->add("actions.ok", slot_close_notification, self());
                dialog->add("actions.cancel", slot_cancel_save_preset, self());

                dialog->buttons()->get(0)->constraints()->set_min_width(96);
                dialog->buttons()->get(1)->constraints()->set_min_width(96);

                bind_shortcut(dialog, ws::WSK_ESCAPE, tk::KM_NONE, slot_cancel_save_preset);
                bind_shortcut(dialog, ws::WSK_RETURN, tk::KM_NONE, slot_close_notification);
                bind_shortcut(dialog, ws::WSK_KEYPAD_ENTER, tk::KM_NONE, slot_close_notification);
                bind_shortcut(dialog, ws::WSK_BACKSPACE, tk::KM_NONE, slot_close_notification);

                // Commit dialog
                if (widgets()->add(dialog) != STATUS_OK)
                    return false;
                wNotification   = release_ptr(dialog);
            }

            // Update text
            expr::Parameters params;
            params.set_string("name", name);
            wNotification->message()->set(
                "messages.presets.confirm_overwrite",
                &params);

            wNotification->show(wWidget);

            return true;
        }

        void SavePresetDialog::save_preset(const LSPString *name)
        {
            const bool favourite = (wFavourites != NULL) ? wFavourites->checked()->get() : false;

            size_t flags = ui::PRESET_FLAG_USER;
            if (favourite)
                flags      |= ui::PRESET_FLAG_FAVOURITE;

            status_t res = pWrapper->save_preset(name, flags);
            if (res == STATUS_OK)
            {
                wWidget->hide();
                wWidget->slots()->execute(tk::SLOT_SUBMIT, wWidget);
            }
            else
                show_show_save_error(name, res);
        }

        status_t SavePresetDialog::slot_save_button_submit(tk::Widget *sender, void *ptr, void *data)
        {
            SavePresetDialog *self = static_cast<SavePresetDialog *>(ptr);
            if (self == NULL)
                return STATUS_OK;
            if ((self->wSaveButton == NULL) || (!self->wSaveButton->editable()))
                return STATUS_OK;

            // Check that we need confirmation
            LSPString name;
            if (self->wPresetName->text()->format(&name) != STATUS_OK)
                return STATUS_OK;

            const ui::preset_t *active = self->pWrapper->active_preset();
            const ui::preset_t *save = self->find_user_preset_by_name(&name);
            if (save != NULL)
            {
                const bool need_confirm = (active != save);
                if (need_confirm)
                {
                    self->request_confirmation(&name);
                    return STATUS_OK;
                }
            }

            self->save_preset(&name);

            return STATUS_OK;
        }

        status_t SavePresetDialog::slot_accept_save_preset(tk::Widget *sender, void *ptr, void *data)
        {
            SavePresetDialog *self = static_cast<SavePresetDialog *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if (self->wConfirmation != NULL)
                self->wConfirmation->hide();

            LSPString name;
            if (self->wPresetName->text()->format(&name) != STATUS_OK)
                return STATUS_OK;

            self->save_preset(&name);

            return STATUS_OK;
        }

        status_t SavePresetDialog::slot_reject_save_preset(tk::Widget *sender, void *ptr, void *data)
        {
            SavePresetDialog *self = static_cast<SavePresetDialog *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if (self->wConfirmation != NULL)
                self->wConfirmation->hide();

            return STATUS_OK;
        }

        status_t SavePresetDialog::slot_cancel_save_preset(tk::Widget *sender, void *ptr, void *data)
        {
            SavePresetDialog *self = static_cast<SavePresetDialog *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if (self->wNotification != NULL)
                self->wNotification->hide();
            if (self->wConfirmation != NULL)
                self->wConfirmation->hide();

            self->wWidget->hide();
            self->wWidget->slots()->execute(tk::SLOT_CANCEL, self->wWidget);

            return STATUS_OK;
        }

        status_t SavePresetDialog::slot_close_notification(tk::Widget *sender, void *ptr, void *data)
        {
            SavePresetDialog *self = static_cast<SavePresetDialog *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if (self->wNotification != NULL)
                self->wNotification->hide();

            return STATUS_OK;
        }

        status_t SavePresetDialog::slot_favourites_click(tk::Widget *sender, void *ptr, void *data)
        {
            SavePresetDialog *self = static_cast<SavePresetDialog *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if (self->wFavourites == NULL)
                return STATUS_OK;

            const ws::event_t *ev = static_cast<ws::event_t *>(data);
            if (ev == NULL)
                return STATUS_OK;

            if (ev->nCode == ws::MCB_LEFT)
                self->wFavourites->checked()->toggle();

            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


