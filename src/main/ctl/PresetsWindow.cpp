/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

        static const char *preset_lists_ids[] =
        {
            "factory_presets_list",
            "user_presets_list",
            "favourites_presets_list",
            "all_presets_list"
        };

        static const tk::tether_t presets_tether[] =
        {
            { tk::TF_BOTTOM | tk::TF_LEFT,      1.0f,  1.0f },
            { tk::TF_TOP | tk::TF_LEFT,         1.0f, -1.0f },
            { tk::TF_BOTTOM | tk::TF_RIGHT,    -1.0f,  1.0f },
            { tk::TF_TOP | tk::TF_RIGHT,       -1.0f, -1.0f },
        };

        PresetsWindow::PresetsWindow(ui::IWrapper *src, tk::Window *widget, PluginWindow *pluginWindow):
            ctl::Window(src, widget)
        {
            pClass          = &metadata;

            pPluginWindow   = pluginWindow;
            wExport         = NULL;
            wImport         = NULL;
            wRelPaths       = NULL;

            for (size_t i=0; i<PLT_TOTAL; ++i)
            {
                preset_list_t *plist = &vPresetsLists[i];
                plist->wList    = NULL;
            }

            pPath           = NULL;
            pFileType       = NULL;
            pRelPaths       = NULL;
        }

        PresetsWindow::~PresetsWindow()
        {
        }

        status_t PresetsWindow::init()
        {
            status_t res = ctl::Window::init();
            if (res != STATUS_OK)
                return res;

            BIND_PORT(pWrapper, pPath, CONFIG_PATH_PORT);
            BIND_PORT(pWrapper, pFileType, CONFIG_FTYPE_PORT);
            BIND_PORT(pWrapper, pRelPaths, REL_PATHS_PORT);

            return STATUS_OK;
        }

        status_t PresetsWindow::post_init()
        {
            tk::Window *wnd = tk::widget_cast<tk::Window>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            bind_slot("btn_new", tk::SLOT_SUBMIT, slot_preset_new_click);
            bind_slot("btn_delete", tk::SLOT_SUBMIT, slot_preset_delete_click);
            bind_slot("btn_save", tk::SLOT_SUBMIT, slot_preset_save_click);
            bind_slot("btn_import", tk::SLOT_SUBMIT, slot_import_click);
            bind_slot("btn_export", tk::SLOT_SUBMIT, slot_submit_export_settings);
            bind_slot("btn_copy", tk::SLOT_SUBMIT, slot_state_copy_click);
            bind_slot("btn_paste", tk::SLOT_SUBMIT, slot_state_paste_click);

            for (size_t i=0; i<PLT_TOTAL; ++i)
            {
                preset_list_t *plist = &vPresetsLists[i];

                tk::ListBox *lbox   = widgets()->get<tk::ListBox>(preset_lists_ids[i]);
                if (lbox == NULL)
                    continue;

                plist->wList        = lbox;
                lbox->slots()->bind(tk::SLOT_SUBMIT, slot_preset_select, this);
                lbox->slots()->bind(tk::SLOT_MOUSE_DBL_CLICK, slot_preset_dblclick, this);
            }

            refresh_presets();
            sync_preset_selection(&vPresetsLists[PLT_FACTORY]);

            return STATUS_OK;
        }

        void PresetsWindow::destroy()
        {
            ctl::Window::destroy();

            wExport         = NULL; // Will be destroyed by wrapper
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

        status_t PresetsWindow::refresh_user_presets()
        {
            return STATUS_OK; // TODO
        }

        void PresetsWindow::put_presets_to_list(lltl::darray<resource::resource_t> *presets)
        {
            tk::ListBox *lb = widgets()->get<tk::ListBox>("factory_presets_list");
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

        void PresetsWindow::sync_preset_selection(preset_list_t *list)
        {
            if (list->wList == NULL)
                return;

            tk::ListBoxItem *item = list->wList->selected()->any();
            const bool editable = (item != NULL);

            tk::Button *btn_delete = widgets()->get<tk::Button>("btn_delete");
            if (btn_delete != NULL)
            {
                btn_delete->editable()->set(editable);
                btn_delete->active()->set(editable);
            }

            tk::Button *btn_save = widgets()->get<tk::Button>("btn_save");
            if (btn_save != NULL)
            {
                btn_save->editable()->set(editable);
                btn_save->active()->set(editable);
            }

            // TODO: Save selected preset index to iSelectedPreset
        }

        bool PresetsWindow::has_path_ports()
        {
            for (size_t i = 0, n = pWrapper->ports(); i < n; ++i)
            {
                ui::IPort *p = pWrapper->port(i);
                const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
                if ((meta != NULL) && (meta->role == meta::R_PATH))
                    return true;
            }
            return false;
        }

        tk::FileFilters *PresetsWindow::create_config_filters(tk::FileDialog *dlg)
        {
            tk::FileFilters *f = dlg->filter();
            if (f == NULL)
                return f;

            tk::FileMask *ffi = f->add();
            if (ffi != NULL)
            {
                ffi->pattern()->set("*.cfg");
                ffi->title()->set("files.config.lsp");
                ffi->extensions()->set_raw(".cfg");
            }

            ffi = f->add();
            if (ffi != NULL)
            {
                ffi->pattern()->set("*");
                ffi->title()->set("files.all");
                ffi->extensions()->set_raw("");
            }

            return f;
        }

        status_t PresetsWindow::show(tk::Widget *actor)
        {
            tk::PopupWindow *wnd = tk::widget_cast<tk::PopupWindow>(wWidget);
            if (wnd == NULL)
                return STATUS_BAD_STATE;

            if (actor != NULL)
            {
                ws::rectangle_t r;
                actor->get_screen_rectangle(&r);

                wnd->set_tether(presets_tether, sizeof(presets_tether)/sizeof(tk::tether_t));
                wnd->trigger_widget()->set(actor);
                wnd->trigger_area()->set(&r);
                wnd->show();
            }
            else
                wnd->show(wnd);

            wnd->grab_events(ws::GRAB_DROPDOWN);

            return STATUS_OK;
        }

        status_t PresetsWindow::show_export_settings_dialog()
        {
            tk::Display *dpy        = wWidget->display();
            tk::FileDialog *dlg     = wExport;

            if (dlg == NULL)
            {
                dlg = new tk::FileDialog(dpy);
                widgets()->add(dlg);
                wExport = dlg;

                dlg->init();
                dlg->mode()->set(tk::FDM_SAVE_FILE);
                dlg->title()->set("titles.export_settings");
                dlg->action_text()->set("actions.save");
                dlg->use_confirm()->set(true);
                dlg->confirm_message()->set("messages.file.confirm_overwrite");

                create_config_filters(dlg);

                // Add 'Relative paths' option if present
                tk::Box *wc = new tk::Box(dpy);
                widgets()->add(wc);
                wc->init();
                wc->orientation()->set_vertical();
                wc->allocation()->set_hfill(true);

                if (has_path_ports())
                {
                    tk::Box *op_rpath       = new tk::Box(dpy);
                    widgets()->add(op_rpath);
                    op_rpath->init();
                    op_rpath->orientation()->set_horizontal();
                    op_rpath->spacing()->set(4);

                    // Add switch button
                    tk::CheckBox *ck_rpath  = new tk::CheckBox(dpy);
                    widgets()->add(ck_rpath);
                    ck_rpath->init();
                    ck_rpath->slots()->bind(tk::SLOT_SUBMIT, slot_relative_path_changed, self());
                    wRelPaths               = ck_rpath;
                    op_rpath->add(ck_rpath);

                    // Add label
                    tk::Label *lbl_rpath     = new tk::Label(dpy);
                    widgets()->add(lbl_rpath);
                    lbl_rpath->init();

                    lbl_rpath->allocation()->set_hexpand(true);
                    lbl_rpath->allocation()->set_hfill(true);
                    lbl_rpath->text_layout()->set_halign(-1.0f);
                    lbl_rpath->text()->set("labels.relative_paths");
                    op_rpath->add(lbl_rpath);

                    // Add option to dialog
                    wc->add(op_rpath);
                }

                // Bind actions
                if (wc->items()->size() > 0)
                    dlg->options()->set(wc);
                dlg->slots()->bind(tk::SLOT_SUBMIT, slot_exec_export_settings_to_file, self());
                dlg->slots()->bind(tk::SLOT_SHOW, slot_fetch_path, self());
                dlg->slots()->bind(tk::SLOT_HIDE, slot_commit_path, self());
            }

            // Initialize and show the window
            if ((wRelPaths != NULL) && (pRelPaths != NULL))
            {
                bool checked = pRelPaths->value() >= 0.5f;
                wRelPaths->checked()->set(checked);
            }

            wWidget->hide();
            dlg->show(pPluginWindow->widget());
            return STATUS_OK;
        }

        // Slots

        status_t PresetsWindow::slot_fetch_path(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self == NULL)
                return STATUS_BAD_STATE;

            tk::FileDialog *dlg = tk::widget_cast<tk::FileDialog>(sender);
            if (dlg == NULL)
                return STATUS_OK;

            // Set-up path
            if (self->pPath != NULL)
            {
                dlg->path()->set_raw(self->pPath->buffer<char>());
            }
            // Set-up file type
            if (self->pFileType != NULL)
            {
                size_t filter = self->pFileType->value();
                if (filter < dlg->filter()->size())
                    dlg->selected_filter()->set(filter);
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_commit_path(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self == NULL)
                return STATUS_BAD_STATE;

            tk::FileDialog *dlg = tk::widget_cast<tk::FileDialog>(sender);
            if (dlg == NULL)
                return STATUS_OK;

            // Update file path
            if (self->pPath != NULL)
            {
                LSPString tmp_path;
                if (dlg->path()->format(&tmp_path) == STATUS_OK)
                {
                    const char *path = tmp_path.get_utf8();
                    if (path != NULL)
                    {
                        self->pPath->write(path, strlen(path));
                        self->pPath->notify_all(ui::PORT_USER_EDIT);
                    }
                }
            }
            // Update filter
            if (self->pFileType != NULL)
            {
                self->pFileType->set_value(dlg->selected_filter()->get());
                self->pFileType->notify_all(ui::PORT_USER_EDIT);
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_relative_path_changed(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self == NULL)
                return STATUS_OK;
            if (self->pRelPaths == NULL)
                return STATUS_OK;

            tk::CheckBox *ck_box    = tk::widget_cast<tk::CheckBox>(sender);
            if (ck_box == NULL)
                return STATUS_OK;

            const float value       = ck_box->checked()->get() ? 1.0f : 0.0f;
            self->pRelPaths->set_value(value);
            self->pRelPaths->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_exec_export_settings_to_file(tk::Widget *sender, void *ptr, void *data)
        {
            LSPString path;
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self == NULL)
                return STATUS_OK;
            if (self->wExport == NULL)
                return STATUS_OK;

            status_t res = self->wExport->selected_file()->format(&path);
            if (res == STATUS_OK)
            {
                bool relative = (self->pRelPaths != NULL) ? self->pRelPaths->value() >= 0.5f : false;
                self->pWrapper->export_settings(&path, relative);
            }

            return STATUS_OK;
        }

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

        status_t PresetsWindow::slot_preset_save_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_save_click");

//            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);

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

        status_t PresetsWindow::slot_submit_export_settings(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self != NULL)
                self->show_export_settings_dialog();

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
            if (self != NULL)
                self->sync_preset_selection(&self->vPresetsLists[PLT_FACTORY]);

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

