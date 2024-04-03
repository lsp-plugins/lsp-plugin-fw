/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 31 мар. 2024 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/core/presets.h>
#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        const ctl_class_t PresetsWindow::metadata = { "PresetsWindow", &Window::metadata };

        PresetsWindow::PresetsWindow(ui::IWrapper *src, tk::Window *widget, PluginWindow *pluginWindow):
            ctl::Window(src, widget)
        {
            pClass          = &metadata;

            pPluginWindow   = pluginWindow;
            wPresetsList    = NULL;
        }

        PresetsWindow::~PresetsWindow()
        {
        }

        status_t PresetsWindow::init()
        {
            status_t res = ctl::Window::init();
            if (res != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        status_t PresetsWindow::post_init()
        {
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            bind_slot("btn_new", tk::SLOT_SUBMIT, slot_preset_new_click);
            bind_slot("btn_delete", tk::SLOT_SUBMIT, slot_preset_delete_click);
            bind_slot("btn_override", tk::SLOT_SUBMIT, slot_preset_override_click);
            bind_slot("btn_import", tk::SLOT_SUBMIT, slot_import_click);
            bind_slot("btn_export", tk::SLOT_SUBMIT, slot_export_click);
            bind_slot("btn_copy", tk::SLOT_SUBMIT, slot_state_copy_click);
            bind_slot("btn_paste", tk::SLOT_SUBMIT, slot_state_paste_click);
            bind_slot("presets_list", tk::SLOT_SUBMIT, slot_preset_select);
            bind_slot("presets_list", tk::SLOT_MOUSE_DBL_CLICK, slot_preset_dblclick);

            refresh_presets();

            wPresetsList = widgets()->get<tk::ListBox>("presets_list");
            sync_preset_selection();

            return STATUS_OK;
        }

        void PresetsWindow::destroy()
        {
            ctl::Window::destroy();
        }

        void PresetsWindow::bind_slot(const char *widget_id, tk::slot_t id, tk::event_handler_t handler)
        {
            tk::Widget *w = widgets()->find(widget_id);
            if (w == NULL)
            {
                lsp_warn("Not found widget: ui:id=%s", widget_id);
                return;
            }

            w->slots()->bind(id, handler, this);
        }

        status_t PresetsWindow::refresh_presets()
        {
            status_t res;
            lltl::darray<resource::resource_t> build_in_presets;
            const meta::plugin_t *metadata = pWrapper->ui()->metadata();
            if (metadata == NULL)
                return STATUS_NOT_FOUND;

            if ((res = core::scan_presets(&build_in_presets, pWrapper->resources(), metadata->ui_presets)) != STATUS_OK)
                return res;
            if (build_in_presets.is_empty())
                return STATUS_NOT_FOUND;
            core::sort_presets(&build_in_presets, true);

            // Initial preset
            resource::resource_t initial_preset;
            initial_preset.type = resource::RES_DIR;
            strcpy(initial_preset.name, "-- Initial preset --");
            build_in_presets.insert(0, initial_preset);

            put_presets_to_list(&build_in_presets);

            return STATUS_OK;
        }

        void PresetsWindow::put_presets_to_list(lltl::darray<resource::resource_t> *presets)
        {
            tk::ListBox *lb = widgets()->get<tk::ListBox>("presets_list");
            if (lb == NULL)
                return;

            lb->items()->clear();
            status_t res;
            io::Path path;
            LSPString preset_name;

            for (size_t i=0, n=presets->size(); i<n; ++i)
            {
                const resource::resource_t *r = presets->uget(i);
                if (r == NULL)
                    continue;

                // Obtain preset name
                if ((res = path.set(r->name)) != STATUS_OK)
                    continue;
                if ((res = path.get_last_noext(&preset_name)) != STATUS_OK)
                    continue;

                // Create list box item
                tk::ListBoxItem *item = new tk::ListBoxItem(lb->display());
                if (item == NULL)
                    continue;
                lsp_finally {
                    if (item != NULL)
                    {
                        item->destroy();
                        delete item;
                    }
                };

                if ((res = item->init()) != STATUS_OK)
                    continue;

                // Fill item
                item->text()->set_raw(&preset_name);
                item->tag()->set(i);

                // Add item to list
                if ((res = lb->items()->madd(item)) == STATUS_OK)
                    item    = NULL;
            }
        }

        void PresetsWindow::sync_preset_selection()
        {
            if (wPresetsList == NULL)
                return;

            tk::ListBoxItem *item = wPresetsList->selected()->any();
            const bool editable = (item != NULL);

            tk::Button *btn_delete = widgets()->get<tk::Button>("btn_delete");
            if (btn_delete != NULL)
                btn_delete->editable()->set(editable);

            tk::Button *btn_override = widgets()->get<tk::Button>("btn_override");
            if (btn_override != NULL)
                btn_override->editable()->set(editable);

            // TODO: Save selected preset index to iSelectedPreset
        }

        // Slots

        status_t PresetsWindow::slot_window_close(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            tk::Window *wnd = tk::widget_cast<tk::Window>(self->wWidget);
            if (wnd != NULL)
                wnd->visibility()->set(false);
            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_new_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_new_click");

            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);

            // TODO: Ask for name using prompt dialog
            // Validate the name
            // TODO: Save preset to the folder
            self->refresh_presets();

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_delete_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_delete_click");

            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);

            // TODO: Ask for deletion confirmation
            // TODO: Delete preset from the folder
            self->refresh_presets();

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_override_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_override_click");

            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);

            // TODO: Ask for override confirmation
            // TODO: Update preset in the folder

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_import_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_import_click");

            // TODO: Show import context menu

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_export_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_export_click");

            // TODO: Show export context menu

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_state_copy_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_state_copy_click");

            // TODO: Copy current plugin state to clipboard

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_state_paste_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_state_paste_click");

            // TODO: Paste plugin state from clipboard

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_select(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_select");

            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            self->sync_preset_selection();

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_dblclick(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_dblclick");

            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);

            if (self->iSelectedPreset < 0)
                return STATUS_OK; // Impossible

            // TODO: If iSelectedPreset == 0 -> PluginWindow.slot_confirm_reset_settings();
            // TODO: Else -> load preset

            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */

