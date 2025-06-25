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
#include <lsp-plug.in/io/InStringSequence.h>
#include <lsp-plug.in/io/OutStringSequence.h>
#include <lsp-plug.in/plug-fw/core/presets.h>
#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        //-----------------------------------------------------------------
        static const char *preset_lists_ids[] =
        {
            "all_presets_list",
            "factory_presets_list",
            "user_presets_list",
            "favourites_presets_list"
        };

        static const char *preset_management_buttons[] =
        {
            "btn_load",
            "btn_save",
            "btn_favourite",
            "btn_remove"
        };

        static const tk::tether_t presets_tether[] =
        {
            { tk::TF_BOTTOM | tk::TF_LEFT,      1.0f,  1.0f },
            { tk::TF_TOP | tk::TF_LEFT,         1.0f, -1.0f },
            { tk::TF_BOTTOM | tk::TF_RIGHT,    -1.0f,  1.0f },
            { tk::TF_TOP | tk::TF_RIGHT,       -1.0f, -1.0f },
        };

        //-----------------------------------------------------------------
        PresetsWindow::ConfigSink::ConfigSink(ui::IWrapper *wrapper)
        {
            pWrapper = wrapper;
        }

        void PresetsWindow::ConfigSink::unbind()
        {
            pWrapper        = NULL;
        }

        status_t PresetsWindow::ConfigSink::receive(const LSPString *text, const char *mime)
        {
            ui::IWrapper *wrapper = pWrapper;
            if (wrapper == NULL)
                return STATUS_NOT_BOUND;

            io::InStringSequence is(text);
            return wrapper->import_settings(&is, ui::IMPORT_FLAG_NONE);
        }

        //-----------------------------------------------------------------
        const ctl_class_t PresetsWindow::metadata = { "PresetsWindow", &Window::metadata };

        PresetsWindow::PresetsWindow(ui::IWrapper *src, tk::Window *widget, PluginWindow *pluginWindow):
            ctl::Window(src, widget)
        {
            pClass          = &metadata;

            pPluginWindow   = pluginWindow;
            wExport         = NULL;
            wImport         = NULL;
            wRelPaths       = NULL;
            wPresetPattern  = NULL;

            for (size_t i=0; i<ui::PRESET_TAB_TOTAL; ++i)
            {
                preset_list_t *plist = &vPresetsLists[i];
                plist->wList    = NULL;
            }
            for (size_t i=0; i<BTN_TOTAL; ++i)
                vButtons[i]     = NULL;

            pConfigSink     = NULL;

            pPath           = NULL;
            pFileType       = NULL;
            pRelPaths       = NULL;
        }

        PresetsWindow::~PresetsWindow()
        {
            do_destroy();
        }

        void PresetsWindow::do_destroy()
        {
            // Unbind self from wrapper
            if (pWrapper != NULL)
                pWrapper->remove_preset_listener(this);

            // Unbind configuration sink
            if (pConfigSink != NULL)
            {
                pConfigSink->unbind();
                pConfigSink->release();
                pConfigSink = NULL;
            }

            // Clear filters
            for (size_t i=0; i<ui::PRESET_TAB_TOTAL; ++i)
                destroy_preset_list(&vPresetsLists[i]);

            wExport         = NULL; // Will be destroyed by wrapper
        }

        void PresetsWindow::destroy_preset_list(preset_list_t *list)
        {
            if (list == NULL)
                return;

            list->vPresets.flush();

            for (size_t i=0, n=list->vItems.size(); i<n; ++i)
            {
                tk::ListBoxItem *item = list->vItems.uget(i);
                if (item != NULL)
                {
                    item->destroy();
                    delete item;
                }
            }
            list->vItems.flush();
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

            wPresetPattern  = widgets()->get<tk::Edit>("preset_filter");
            for (size_t i=0; i<BTN_TOTAL; ++i)
                vButtons[i]     = widgets()->get<tk::Button>(preset_management_buttons[i]);
            for (size_t i=0; i<ui::PRESET_TAB_TOTAL; ++i)
                vPresetsLists[i].wList  = widgets()->get<tk::ListBox>(preset_lists_ids[i]);

            if (vButtons[BTN_FAVOURITE] != NULL)
                vButtons[BTN_FAVOURITE]->mode()->set_toggle();

            bind_slot("preset_filter", tk::SLOT_CHANGE, slot_preset_filter_changed);
            bind_slot("btn_refresh", tk::SLOT_SUBMIT, slot_refresh_preset_list);
            bind_slot("btn_load", tk::SLOT_SUBMIT, slot_preset_load_submit);
            bind_slot("btn_save", tk::SLOT_SUBMIT, slot_preset_save_submit);
            bind_slot("btn_favourite", tk::SLOT_SUBMIT, slot_preset_favourite_submit);
            bind_slot("btn_reset", tk::SLOT_SUBMIT, slot_reset_settings);
            bind_slot("btn_remove", tk::SLOT_SUBMIT, slot_preset_remove_click);
            bind_slot("btn_import", tk::SLOT_SUBMIT, slot_submit_import_settings);
            bind_slot("btn_export", tk::SLOT_SUBMIT, slot_submit_export_settings);
            bind_slot("btn_copy", tk::SLOT_SUBMIT, slot_export_settings_to_clipboard);
            bind_slot("btn_paste", tk::SLOT_SUBMIT, slot_import_settings_from_clipboard);

            for (size_t i=0; i<ui::PRESET_TAB_TOTAL; ++i)
            {
                const char *list_id = preset_lists_ids[i];
                bind_slot(list_id, tk::SLOT_SUBMIT, slot_preset_select);
                bind_slot(list_id, tk::SLOT_MOUSE_DBL_CLICK, slot_preset_dblclick);
            }

            pWrapper->add_preset_listener(this);
            sync_preset_button_state();

            return STATUS_OK;
        }

        void PresetsWindow::destroy()
        {
            do_destroy();
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

        void PresetsWindow::make_preset_list(preset_list_t *list, const ui::preset_t *presets,
            size_t count, ui::preset_filter_t filter, bool indicate)
        {
            tk::ListBox *lb = list->wList;
            if (lb == NULL)
                return;

            status_t res;
            io::Path path;
            LSPString preset_name;
            LSPString preset_pattern;
            preset_list_t tmp;

            // Create temporary list
            tmp.wList       = NULL;
            lsp_finally {
                destroy_preset_list(&tmp);
            };

            tk::Display *dpy = wWidget->display();
            for (size_t i=0; i<count; ++i)
            {
                // Get preset and filter
                const ui::preset_t *p = &presets[i];
                if (!filter(p))
                    continue;

                // Create list box item
                tk::ListBoxItem *item = new tk::ListBoxItem(dpy);
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

                // Determine how to format the prest name
                const char *key = "labels.presets.name.normal";
                const bool is_dirty = p->flags & ui::PRESET_FLAG_DIRTY;
                if (indicate)
                {
                    if (p->flags & ui::PRESET_FLAG_USER)
                        key     = (is_dirty) ? "labels.presets.name.user_mod" : "labels.presets.name.user";
                    else
                        key     = (is_dirty) ? "labels.presets.name.factory_mod" : "labels.presets.name.factory";
                }
                else if (is_dirty)
                    key             = "labels.presets.name.mod";

                // Fill the item
                expr::Parameters params;
                params.set_string("name", &p->name);
                item->text()->set(key, &params);
                item->tag()->set(p->preset_id);

                // Add item to preset list
                if (!tmp.vPresets.add(const_cast<ui::preset_t *>(p)))
                    continue;

                // Add item to ListBox
                if (!tmp.vItems.add(item))
                {
                    tmp.vPresets.pop();
                    continue;
                }
                item    = NULL;
            }

            // Add new items to list
            lb->items()->clear();
            for (size_t i=0, n=tmp.vItems.size(); i<n; ++i)
            {
                tk::ListBoxItem *item = tmp.vItems.uget(i);
                if (item != NULL)
                    lb->add(item);
            }

            // Commit result
            tmp.vItems.swap(list->vItems);
            tmp.vPresets.swap(list->vPresets);
        }

        const ui::preset_t *PresetsWindow::current_preset()
        {
            const ui::preset_tab_t tab = pWrapper->preset_tab();
            preset_list_t *list = (tab < ui::PRESET_TAB_TOTAL) ? &vPresetsLists[tab] : NULL;

            tk::ListBoxItem *item = (list->wList != NULL) ? list->wList->selected()->any() : NULL;
            const size_t index = list->vItems.index_of(item);
            return list->vPresets.get(index);
        }

        void PresetsWindow::sync_preset_button_state()
        {
            const ui::preset_t *current = current_preset();
            sync_preset_button_state(current);
        }

        void PresetsWindow::sync_preset_button_state(const ui::preset_t *preset)
        {
            tk::Button *btn = vButtons[BTN_LOAD];
            if (btn != NULL)
            {
                const bool can_load = (preset != NULL);
                btn->editable()->set(can_load);
                btn->active()->set(can_load);
            }

            btn = vButtons[BTN_SAVE];
            if (btn != NULL)
            {
                const bool can_save = (preset != NULL) && (preset->flags & ui::PRESET_FLAG_USER);
                btn->editable()->set(can_save);
                btn->active()->set(can_save);
            }

            btn = vButtons[BTN_FAVOURITE];
            if (btn != NULL)
            {
                const bool can_toggle = preset != NULL;
                const bool is_favourite = (preset != NULL) && (preset->flags & ui::PRESET_FLAG_FAVOURITE);
                btn->editable()->set(can_toggle);
                btn->active()->set(can_toggle);
                btn->down()->set(is_favourite);
            }

            btn = vButtons[BTN_DELETE];
            if (btn != NULL)
            {
                const bool editable = (preset != NULL) && (preset->flags & ui::PRESET_FLAG_USER);
                btn->editable()->set(editable);
                btn->active()->set(editable);
            }
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
            if (pPluginWindow == NULL)
                return STATUS_BAD_STATE;

            tk::Display *dpy        = wWidget->display();
            tk::FileDialog *dlg     = wExport;

            if (dlg == NULL)
            {
                dlg = new tk::FileDialog(dpy);
                if (dlg == NULL)
                    return STATUS_NO_MEM;

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
                if (wc == NULL)
                    return STATUS_NO_MEM;

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

            dlg->show(pPluginWindow->widget());
            return STATUS_OK;
        }

        status_t PresetsWindow::show_import_settings_dialog()
        {
            if (pPluginWindow == NULL)
                return STATUS_BAD_STATE;

            tk::Display *dpy        = wWidget->display();
            tk::FileDialog *dlg     = wImport;

            if (dlg == NULL)
            {
                dlg             = new tk::FileDialog(dpy);
                if (dlg == NULL)
                    return STATUS_NO_MEM;

                widgets()->add(dlg);
                wImport      = dlg;

                dlg->init();
                dlg->mode()->set(tk::FDM_OPEN_FILE);
                dlg->title()->set("titles.import_settings");
                dlg->action_text()->set("actions.open");

                create_config_filters(dlg);

                dlg->slots()->bind(tk::SLOT_SUBMIT, slot_exec_import_settings_from_file, self());
                dlg->slots()->bind(tk::SLOT_SHOW, slot_fetch_path, self());
                dlg->slots()->bind(tk::SLOT_HIDE, slot_commit_path, self());
            }

            dlg->show(pPluginWindow->widget());
            return STATUS_OK;
        }

        status_t PresetsWindow::import_settings_from_clipboard()
        {
            tk::Display *dpy        = wWidget->display();

            // Create new sink
            ConfigSink *ds          = new ConfigSink(pWrapper);
            if (ds == NULL)
                return STATUS_NO_MEM;
            ds->acquire();

            // Release previously used
            ConfigSink *old         = pConfigSink;
            pConfigSink             = ds;

            if (old != NULL)
            {
                old->unbind();
                old->release();
            }

            // Request clipboard data
            return dpy->get_clipboard(ws::CBUF_CLIPBOARD, ds);
        }

        status_t PresetsWindow::export_settings_to_clipboard()
        {
            status_t res;
            LSPString buf;

            // Export settings to text buffer
            io::OutStringSequence sq(&buf);
            if ((res = pWrapper->export_settings(&sq)) != STATUS_OK)
                return STATUS_OK;
            sq.close();

            // Now 'buf' contains serialized configuration, put it to clipboard
            tk::TextDataSource *ds = new tk::TextDataSource();
            if (ds == NULL)
                return STATUS_NO_MEM;
            ds->acquire();
            lsp_finally { ds->release(); };

            res = ds->set_text(&buf);
            if (res == STATUS_OK)
                res = wWidget->display()->set_clipboard(ws::CBUF_CLIPBOARD, ds);

            return STATUS_OK;
        }

        status_t PresetsWindow::reset_settings()
        {
            pWrapper->reset_settings();
            return STATUS_OK;
        }

        void PresetsWindow::sync_preset_lists()
        {
            LSPString filter;
            if (wPresetPattern != NULL)
                wPresetPattern->text()->format(&filter);

            // Set selection for each list
            for (size_t i=0; i<ui::PRESET_TAB_TOTAL; ++i)
            {
                preset_list_t *list = &vPresetsLists[i];
                if (list->wList == NULL)
                    continue;

                // Update visibility of preset items
                tk::ListBoxItem *selected = NULL;
                for (size_t j=0, n=list->vPresets.size(); j<n; ++j)
                {
                    const ui::preset_t *preset = list->vPresets.get(j);
                    tk::ListBoxItem *item = list->vItems.get(j);
                    if ((preset == NULL) || (item == NULL))
                        continue;

                    const bool visible = preset->name.index_of_nocase(&filter) >= 0;
                    item->visibility()->set(visible);
                    if (preset->flags & ui::PRESET_FLAG_SELECTED)
                        selected = item;
                }

                // Make item being selected
                list->wList->selected()->clear();
                if (selected != NULL)
                    list->wList->selected()->add(selected);
            }
        }

        void PresetsWindow::preset_selected(const ui::preset_t *preset)
        {
            // Set selection for each list
            for (size_t i=0; i<ui::PRESET_TAB_TOTAL; ++i)
            {
                preset_list_t *list = &vPresetsLists[i];
                tk::ListBox *lbox   = list->wList;
                if (lbox == NULL)
                    continue;

                const ssize_t index = list->vPresets.index_of(preset);
                if (index < 0)
                {
                    lbox->selected()->clear();
                    continue;
                }

                // Synchronize state
                tk::ListBoxItem *selection = list->vItems.get(index);
                if ((selection == NULL) || (!selection->visibility()->get()))
                {
                    lbox->selected()->clear();
                    continue;
                }

                if (!lbox->selected()->contains(selection))
                {
                    lbox->selected()->clear();
                    lbox->selected()->add(selection);
                }
            }

            sync_preset_button_state(preset);
        }

        void PresetsWindow::presets_updated()
        {
            lsp_trace("presets updated");

            const ui::preset_t *presets = pWrapper->all_presets();
            const size_t count = pWrapper->num_presets();

            make_preset_list(
                &vPresetsLists[ui::PRESET_TAB_ALL],
                presets, count,
                ui::is_any_preset,
                true);
            make_preset_list(
                &vPresetsLists[ui::PRESET_TAB_FACTORY],
                presets, count,
                ui::is_factory_preset,
                false);
            make_preset_list(
                &vPresetsLists[ui::PRESET_TAB_USER],
                presets, count,
                ui::is_user_preset,
                false);
            make_preset_list(
                &vPresetsLists[ui::PRESET_TAB_FAVOURITES],
                presets, count,
                ui::is_favourite_preset,
                true);

            sync_preset_lists();
            sync_preset_button_state();
        }

        //-----------------------------------------------------------------
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
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self == NULL)
                return STATUS_OK;
            if (self->wExport == NULL)
                return STATUS_OK;

            LSPString path;
            status_t res = self->wExport->selected_file()->format(&path);
            if (res == STATUS_OK)
            {
                bool relative = (self->pRelPaths != NULL) ? self->pRelPaths->value() >= 0.5f : false;
                self->pWrapper->export_settings(&path, relative);
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_exec_import_settings_from_file(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self == NULL)
                return STATUS_OK;
            if (self->wImport == NULL)
                return STATUS_OK;

            LSPString path;
            status_t res = self->wImport->selected_file()->format(&path);
            if (res == STATUS_OK)
                self->pWrapper->import_settings(&path, ui::IMPORT_FLAG_NONE);

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

        status_t PresetsWindow::slot_preset_load_submit(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_load_click");

//            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);

            // TODO: Ask for name using prompt dialog
            // Validate the name
            // TODO: Save preset to the folder

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_remove_click(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self != NULL)
            {
                const ui::preset_t *current = self->current_preset();
                if (current != NULL)
                    self->pWrapper->remove_preset(current->preset_id);
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_save_submit(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_save_click");

//            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);

            // TODO: Ask for override confirmation
            // TODO: Update preset in the folder

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_favourite_submit(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self != NULL)
            {
                const ui::preset_t *current = self->current_preset();
                if (current != NULL)
                    self->pWrapper->mark_preset_favourite(
                        current->preset_id,
                        !(current->flags & ui::PRESET_FLAG_FAVOURITE));
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_submit_import_settings(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self != NULL)
            {
                if (self->wWidget != NULL)
                    self->wWidget->hide();
                self->show_import_settings_dialog();
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_submit_export_settings(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self != NULL)
            {
                if (self->wWidget != NULL)
                    self->wWidget->hide();
                self->show_export_settings_dialog();
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_export_settings_to_clipboard(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self != NULL)
            {
                if (self->wWidget != NULL)
                    self->wWidget->hide();
                self->export_settings_to_clipboard();
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_import_settings_from_clipboard(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self != NULL)
            {
                if (self->wWidget != NULL)
                    self->wWidget->hide();
                self->import_settings_from_clipboard();
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_refresh_preset_list(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self != NULL)
                self->pWrapper->scan_presets();

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_reset_settings(tk::Widget *sender, void *ptr, void *data)
        {
            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self != NULL)
            {
                if (self->wWidget != NULL)
                    self->wWidget->hide();
                self->reset_settings();
            }

            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_select(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_select");

            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            // Find the related list and select the preset
            for (size_t i=0; i<ui::PRESET_TAB_TOTAL; ++i)
            {
                preset_list_t *list = &self->vPresetsLists[i];
                if (list->wList == sender)
                {
                    // Select new preset
                    tk::ListBoxItem *item = (list->wList != NULL) ? list->wList->selected()->any() : NULL;
                    const size_t index = list->vItems.index_of(item);
                    const ui::preset_t *preset = list->vPresets.get(index);

                    self->pWrapper->select_active_preset((preset != NULL) ? preset->preset_id : -1);
                    break;
                }
            }

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

        status_t PresetsWindow::slot_preset_filter_changed(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("preset filter changed");

            PresetsWindow *self = static_cast<PresetsWindow *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            self->sync_preset_lists();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */

