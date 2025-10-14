/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/plug-fw/ctl.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/meta/func.h>

#define AFOLDER_STYLE_ACTIVE            "AudioFolder::Active"
#define AFOLDER_STYLE_INACTIVE          "AudioFolder::Inactive"
#define AFOLDER_ITEM_ACTIVE             "AudioFolder::ListBoxItem::Active"

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(AudioFolder)
            status_t res;

            if (!name->equals_ascii("afolder"))
                return STATUS_NOT_FOUND;

            tk::ListBox *w = new tk::ListBox(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::AudioFolder *wc  = new ctl::AudioFolder(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(AudioFolder)

        //-----------------------------------------------------------------
        const ctl_class_t AudioFolder::metadata    = { "AudioFolder", &Widget::metadata };

        AudioFolder::AudioFolder(ui::IWrapper *wrapper, tk::ListBox *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            wActive         = NULL;

            bActive         = false;
            bAutoLoad       = false;
            bAutoPlay       = false;
        }

        AudioFolder::~AudioFolder()
        {
        }

        status_t AudioFolder::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::ListBox *lbox = tk::widget_cast<tk::ListBox>(wWidget);
            if (lbox != NULL)
            {
                sHScroll.init(pWrapper, lbox->hscroll_mode());
                sVScroll.init(pWrapper, lbox->vscroll_mode());
                sAutoLoad.init(pWrapper, this);
                sAutoPlay.init(pWrapper, this);

                // Bind slots
                lbox->slots()->bind(tk::SLOT_SUBMIT, slot_submit, this);
                lbox->slots()->bind(tk::SLOT_CHANGE, slot_change, this);

                sAutoLoad.parse(":" UI_FILELIST_NAVIGATION_AUTOLOAD_PORT);
                sAutoPlay.parse(":" UI_FILELIST_NAVIGATION_AUTOPLAY_PORT);
            }

            return STATUS_OK;
        }

        void AudioFolder::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::ListBox *lbox = tk::widget_cast<tk::ListBox>(wWidget);
            if (lbox != NULL)
            {
                bind_port(&pPort, "id", name, value);

                set_param(lbox->border_size(), "border.size", name, value);
                set_param(lbox->border_size(), "bsize", name, value);
                set_param(lbox->border_gap(), "border.gap", name, value);
                set_param(lbox->border_gap(), "bgap", name, value);
                set_param(lbox->border_radius(), "border.radius", name, value);
                set_param(lbox->border_radius(), "bradius", name, value);

                set_expr(&sAutoLoad, "autoload", name, value);
                set_expr(&sAutoPlay, "autoplay", name, value);

                sHScroll.set(name, "hscroll", value);
                sVScroll.set(name, "vscroll", value);

                set_font(lbox->font(), "font", name, value);
                set_constraints(lbox->constraints(), name, value);

                sDirController.set(name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void AudioFolder::end(ui::UIContext *ctx)
        {
            update_styles();
            sync_state();
            sync_auto_load();
            sync_auto_play();
            Widget::end(ctx);
        }

        bool AudioFolder::sync_list()
        {
            tk::ListBox *lbox = tk::widget_cast<tk::ListBox>(wWidget);
            if (lbox == NULL)
                return false;

            tk::WidgetList<tk::ListBoxItem> *items = lbox->items();
            lltl::parray<LSPString> *files = sDirController.files();

            if ((items == NULL) || (files == NULL))
                return false;

            // Fill list box with items
            items->clear();
            wActive     = NULL;

            for (lltl::iterator<LSPString> it = files->values(); it; ++it)
            {
                LSPString *name = it.get();
                if (name == NULL)
                    return false;

                // Create new item
                tk::ListBoxItem *item = new tk::ListBoxItem(lbox->display());
                if (item == NULL)
                    return false;
                if (item->init() != STATUS_OK)
                {
                    delete item;
                    return false;
                }
                if (items->madd(item) != STATUS_OK)
                {
                    delete item;
                    return false;
                }

                if (item->text()->set_raw(name) != STATUS_OK)
                    return false;
            }

            return true;
        }

        void AudioFolder::sync_auto_play()
        {
            bAutoPlay   = (sAutoPlay.valid()) ? sAutoPlay.evaluate() >= 0.5f : false;
        }

        void AudioFolder::sync_auto_load()
        {
            bAutoLoad   = (sAutoLoad.valid()) ? sAutoLoad.evaluate() >= 0.5f : false;
        }

        void AudioFolder::sync_state()
        {
            tk::ListBox *lbox = tk::widget_cast<tk::ListBox>(wWidget);
            if (lbox == NULL)
                return set_activity(false);

            // Check that port points to the valid data
            const meta::port_t *meta = (pPort != NULL) ? pPort->metadata() : NULL;
            if ((meta == NULL) || (!meta::is_path_port(meta)))
                return set_activity(false);

            // Obtain current file name
            const char *path = pPort->buffer<const char>();
            if ((path == NULL) || (strlen(path) == 0))
            {
                sDirController.set_current_file("");
                return set_activity(false);
            }

            // Apply changes to the controller
            const bool updated  = sDirController.set_current_file(path);
            if (!sDirController.valid())
                return set_activity(false);
            if (updated)
            {
                if (!sync_list())
                    return set_activity(false);
            }

            set_activity(true);

            // Deactivate all selected items
            if (wActive != NULL)
                revoke_style(wActive, AFOLDER_ITEM_ACTIVE);
            lbox->selected()->clear();

            // Update selected item
            const ssize_t file_index = sDirController.file_index();
            if (file_index < 0)
                return;

            tk::ListBoxItem *item = lbox->items()->get(file_index);
            if (item != NULL)
            {
                inject_style(item, AFOLDER_ITEM_ACTIVE);
                wActive = item;
                lbox->selected()->add(item);
                lbox->scroll_to(file_index);
            }
        }

        void AudioFolder::set_activity(bool active)
        {
            if (bActive == active)
                return;

            bActive     = active;
            if (!bActive)
            {
                tk::ListBox *lbox = tk::widget_cast<tk::ListBox>(wWidget);
                if (lbox != NULL)
                {
                    lbox->items()->clear();
                    wActive     = NULL;
                }
            }
            update_styles();
        }

        void AudioFolder::update_styles()
        {
            if (wWidget == NULL)
                return;

            revoke_style(wWidget, AFOLDER_STYLE_ACTIVE);
            revoke_style(wWidget, AFOLDER_STYLE_INACTIVE);
            inject_style(wWidget, (bActive) ? AFOLDER_STYLE_ACTIVE : AFOLDER_STYLE_INACTIVE);
        }

        void AudioFolder::apply_action()
        {
            if ((!bActive) || (pPort == NULL))
                return;

            tk::ListBox *lbox = tk::widget_cast<tk::ListBox>(wWidget);
            if (lbox == NULL)
            {
                set_activity(false);
                return;
            }

            tk::ListBoxItem *selected   = lbox->selected()->any();
            if (selected == NULL)
                return;

            // Ensure that file index has changed
            const ssize_t new_index = lbox->items()->index_of(selected);
            if (new_index == sDirController.file_index())
                return;

            io::Path file;
            if (file.set(sDirController.directory(), sDirController.files()->uget(new_index)) != STATUS_OK)
                return;
            const char *buf = file.as_utf8();
            if (buf == NULL)
                return;

            // Apply changes
            pWrapper->play_file(NULL, 0, false);
            if (bAutoPlay)
                pWrapper->play_file(buf, 0, true);

            if (bAutoLoad)
            {
                pPort->begin_edit();
                pPort->write(buf, strlen(buf));
                pPort->notify_all(ui::PORT_USER_EDIT);
                pPort->end_edit();
            }
        }

        void AudioFolder::notify(ui::IPort *port, size_t flags)
        {
            if ((pPort != NULL) && (port == pPort))
                sync_state();
            if (sAutoLoad.depends(port))
                sync_auto_load();
            if (sAutoPlay.depends(port))
                sync_auto_play();
        }

        status_t AudioFolder::slot_submit(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::AudioFolder *self      = static_cast<ctl::AudioFolder *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if ((self->bAutoLoad) || (self->bAutoPlay))
                self->apply_action();
            return STATUS_OK;
        }

        status_t AudioFolder::slot_change(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::AudioFolder *self      = static_cast<ctl::AudioFolder *>(ptr);
            if (self == NULL)
                return STATUS_OK;

            if ((self->bAutoLoad) || (self->bAutoPlay))
                self->apply_action();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */
