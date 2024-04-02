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

            refresh_presets();

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
            lltl::darray<resource::resource_t> presets;
            const meta::plugin_t *metadata = pWrapper->ui()->metadata();
            if (metadata == NULL)
                return STATUS_NOT_FOUND;

            if ((res = core::scan_presets(&presets, pWrapper->resources(), metadata->ui_presets)) != STATUS_OK)
                return res;
            if (presets.is_empty())
                return STATUS_NOT_FOUND;
            core::sort_presets(&presets, true);

            put_presets_to_list(&presets);

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
            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_delete_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_delete_click");
            return STATUS_OK;
        }

        status_t PresetsWindow::slot_preset_override_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_preset_override_click");
            return STATUS_OK;
        }

        status_t PresetsWindow::slot_import_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_import_click");
            return STATUS_OK;
        }

        status_t PresetsWindow::slot_export_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_export_click");
            return STATUS_OK;
        }

        status_t PresetsWindow::slot_state_copy_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_state_copy_click");
            return STATUS_OK;
        }

        status_t PresetsWindow::slot_state_paste_click(tk::Widget *sender, void *ptr, void *data)
        {
            lsp_trace("slot_state_paste_click");
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */

