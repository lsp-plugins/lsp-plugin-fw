/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/fmt/url.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(FileButton)
            status_t res;
            bool save = false;

            if (name->equals_ascii("save"))
                save    = true;
            else if (name->equals_ascii("load"))
                save    = false;
            else
                return STATUS_NOT_FOUND;

            tk::FileButton *w = new tk::FileButton(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::FileButton *wc  = new ctl::FileButton(context->wrapper(), w, save);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(FileButton)

        //-----------------------------------------------------------------
        static const char *save_keys[] =
        {
            "statuses.save.save",
            "statuses.save.saving",
            "statuses.save.saved",
            "statuses.save.error",
            NULL
        };

        static const char *load_keys[] =
        {
            "statuses.load.load",
            "statuses.load.loading",
            "statuses.load.loaded",
            "statuses.load.error",
            NULL
        };

        static const char *styles[] =
        {
            "FileButton::Select",
            "FileButton::Progress",
            "FileButton::Success",
            "FileButton::Error",
            NULL
        };

        //-----------------------------------------------------------------
        FileButton::DragInSink::DragInSink(FileButton *button)
        {
            pButton         = button;
        }

        FileButton::DragInSink::~DragInSink()
        {
            unbind();
        }

        void FileButton::DragInSink::unbind()
        {
            if (pButton != NULL)
            {
                if (pButton->pDragInSink == this)
                    pButton->pDragInSink    = NULL;
                pButton = NULL;
            }
        }

        status_t FileButton::DragInSink::commit_url(const LSPString *url)
        {
            if ((url == NULL) || (pButton->pPort == NULL))
                return STATUS_OK;

            LSPString decoded;
            status_t res = (url->starts_with_ascii("file://")) ?
                    url::decode(&decoded, url, 7) :
                    url::decode(&decoded, url);

            if (res != STATUS_OK)
                return res;

            lsp_trace("Set file path to %s", decoded.get_native());
            const char *path = decoded.get_utf8();

            pButton->pPort->write(path, strlen(path));
            pButton->pPort->notify_all(ui::PORT_USER_EDIT);

            return STATUS_OK;
        }

        //-----------------------------------------------------------------
        const ctl_class_t FileButton::metadata          = { "FileButton", &Widget::metadata };

        FileButton::FileButton(ui::IWrapper *wrapper, tk::FileButton *widget, bool save):
            Widget(wrapper, widget)
        {
            pClass          = &metadata;

            nStatus         = FB_SELECT_FILE;
            bSave           = save;
            pPort           = NULL;
            pCommand        = NULL;
            pProgress       = NULL;
            pPathPort       = NULL;
            pFileTypePort   = NULL;

            pDragInSink     = NULL;
            pDialog         = NULL;
        }

        FileButton::~FileButton()
        {
            // Destroy sink
            DragInSink *sink = pDragInSink;
            if (sink != NULL)
            {
                sink->unbind();
                sink->release();
                sink   = NULL;
            }

            // Destroy dialog
            if (pDialog != NULL)
            {
                pDialog->destroy();
                delete pDialog;
                pDialog     = NULL;
            }
        }

        status_t FileButton::init()
        {
            status_t res = Widget::init();
            if (res != STATUS_OK)
                return res;

            // Initialize sink
            pDragInSink = new DragInSink(this);
            if (pDragInSink == NULL)
                return STATUS_NO_MEM;
            pDragInSink->acquire();

            tk::FileButton *fb = tk::widget_cast<tk::FileButton>(wWidget);
            if (fb != NULL)
            {
                sStatus.init(pWrapper, this);
                sProgress.init(pWrapper, this);
                sTextPadding.init(pWrapper, fb->text_padding());
                sGradient.init(pWrapper, fb->gradient());
                sBorderSize.init(pWrapper, fb->border_size());
                sBorderPressedSize.init(pWrapper, fb->border_pressed_size());

                sColor.init(pWrapper, fb->color());
                sInvColor.init(pWrapper, fb->inv_color());
                sBorderColor.init(pWrapper, fb->border_color());
                sInvBorderColor.init(pWrapper, fb->inv_border_color());
                sLineColor.init(pWrapper, fb->line_color());
                sInvLineColor.init(pWrapper, fb->inv_line_color());
                sTextColor.init(pWrapper, fb->text_color());
                sInvTextColor.init(pWrapper, fb->inv_text_color());

                // By default use 'all' file formats
                parse_file_formats(&vFormats, "all");

                // Fill the estimation list
                tk::StringList *sl = fb->text_list();
                sl->clear();
                for (const char * const *list = (bSave) ? save_keys : load_keys; *list != NULL; ++list)
                    sl->append()->set(*list);

                // Bind slots
                fb->slots()->bind(tk::SLOT_SUBMIT, slot_submit, this);
                fb->slots()->bind(tk::SLOT_DRAG_REQUEST, slot_drag_request, this);
            }

            return STATUS_OK;
        }

        void FileButton::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::FileButton *fb = tk::widget_cast<tk::FileButton>(wWidget);
            if (fb != NULL)
            {
                bind_port(&pPort, "id", name, value);
                bind_port(&pCommand, "command_id", name, value);
                bind_port(&pCommand, "command.id", name, value);
                bind_port(&pProgress, "progress_id", name, value);
                bind_port(&pProgress, "progress.id", name, value);
                bind_port(&pPathPort, "path.id", name, value);
                bind_port(&pPathPort, "path_id", name, value);
                bind_port(&pFileTypePort, "ftype.id", name, value);
                bind_port(&pFileTypePort, "ftype_id", name, value);

                set_expr(&sProgress, "progress", name, value);
                set_expr(&sStatus, "status", name, value);

                sTextPadding.set("text.padding", name, value);
                sTextPadding.set("text.pad", name, value);
                sTextPadding.set("tpad", name, value);
                sGradient.set("gradient", name, value);
                sBorderSize.set("border.size", name, value);
                sBorderSize.set("bsize", name, value);
                sBorderPressedSize.set("border.pressed.size", name, value);
                sBorderPressedSize.set("bpsize", name, value);

                sColor.set("color", name, value);
                sInvColor.set("inv.color", name, value);
                sInvColor.set("icolor", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sInvBorderColor.set("border.inv.color", name, value);
                sInvBorderColor.set("ibcolor", name, value);
                sLineColor.set("line.color", name, value);
                sLineColor.set("lcolor", name, value);
                sInvLineColor.set("line.inv.color", name, value);
                sInvLineColor.set("ilcolor", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sInvTextColor.set("text.inv.color", name, value);
                sInvTextColor.set("itcolor", name, value);

                set_constraints(fb->constraints(), name, value);
                set_text_layout(fb->text_layout(), "text.layout", name, value);
                set_text_layout(fb->text_layout(), "tlayout", name, value);
                set_font(fb->font(), "font", name, value);

                // Parse file formats
                if ((!strcmp(name, "format")) || (!strcmp(name, "formats")) || (!strcmp(name, "fmt")))
                    parse_file_formats(&vFormats, value);
            }

            return Widget::set(ctx, name, value);
        }

        void FileButton::update_state()
        {
            // Compute the state
            tk::FileButton *fb = tk::widget_cast<tk::FileButton>(wWidget);
            if (fb == NULL)
                return;

            size_t state = sStatus.evaluate_int(STATUS_UNKNOWN_ERR);
            switch (state)
            {
                case STATUS_UNSPECIFIED:    state = FB_SELECT_FILE; break;
                case STATUS_IN_PROCESS:     state = FB_PROGRESS;    break;
                case STATUS_LOADING:        state = FB_PROGRESS;    break;
                case STATUS_OK:             state = FB_SUCCESS;     break;
                default:                    state = FB_ERROR;       break;
            }
            const char * const *keys = (bSave) ? save_keys : load_keys;

            // Discard styles
            for (const char * const *s = styles; *s != NULL; ++s)
                revoke_style(fb, *s);

            float progress = fb->value()->min();
            if (state == FB_PROGRESS)
            {
                if (sProgress.valid())
                    progress = sProgress.evaluate_float(fb->value()->min());
                else if (pProgress != NULL)
                    progress = pProgress->value();
            }
            else if ((state == FB_SUCCESS) || (state == FB_ERROR))
            {
                if (pCommand != NULL)
                    pCommand->set_value(0.0f);
            }

            // Update state of widget
            inject_style(fb, styles[state]);
            fb->text()->set(keys[state]);
            fb->value()->set(progress);
        }

        void FileButton::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            tk::FileButton *fb = tk::widget_cast<tk::FileButton>(wWidget);
            if (fb != NULL)
            {
                fb->value()->set_range(0.0f, 1.0f);
                if (pProgress != NULL)
                {
                    const meta::port_t *meta = pProgress->metadata();
                    if (meta != NULL)
                    {
                        if (meta->flags & meta::F_LOWER)
                            fb->value()->set_min(meta->min);
                        if (meta->flags & meta::F_UPPER)
                            fb->value()->set_max(meta->max);
                    }
                }
            }

            update_state();
        }

        void FileButton::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);
            bool update = false;
            if (port != NULL)
            {
                if (pProgress == port)
                    update      = true;
                if (sProgress.depends(port))
                    update      = true;
                if (sStatus.depends(port))
                    update      = true;
            }

            if (update)
                update_state();
        }

        void FileButton::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);
            update_state();
        }

        void FileButton::show_file_dialog()
        {
            if (pDialog == NULL)
            {
                pDialog = new tk::FileDialog(wWidget->display());
                if (pDialog == NULL)
                    return;
                status_t res = pDialog->init();
                if (res != STATUS_OK)
                {
                    pDialog->destroy();
                    delete pDialog;
                    pDialog = NULL;
                    return;
                }

                // Configure file dialog
                if (bSave)
                {
                    pDialog->title()->set("titles.save_to_file");
                    pDialog->mode()->set(tk::FDM_SAVE_FILE);
                    pDialog->action_text()->set("actions.save");
                    pDialog->use_confirm()->set(true);
                    pDialog->confirm_message()->set("messages.file.confirm_overwrite");
                }
                else
                {
                    pDialog->title()->set("titles.load_from_file");
                    pDialog->mode()->set(tk::FDM_OPEN_FILE);
                    pDialog->action_text()->set("actions.open");
                }

                // Add all listed formats
                tk::FileMask *ffi;
                for (size_t i=0, n=vFormats.size(); i<n; ++i)
                {
                    file_format_t *f = vFormats.uget(i);
                    if ((ffi = pDialog->filter()->add()) != NULL)
                    {
                        ffi->pattern()->set(f->filter, f->flags);
                        ffi->title()->set(f->title);
                        ffi->extensions()->set_raw(f->extension);
                    }
                }

                pDialog->selected_filter()->set(0);

                pDialog->slots()->bind(tk::SLOT_SUBMIT, slot_dialog_submit, this);
                pDialog->slots()->bind(tk::SLOT_HIDE, slot_dialog_hide, this);
            }

            // Initialize the current path
            const char *path = (pPathPort != NULL) ? pPathPort->buffer<char>() : NULL;
            if (path != NULL)
                pDialog->path()->set_raw(path);

            // Initialize the current selected file
            if (pFileTypePort != NULL)
            {
                size_t ftype    = pFileTypePort->value();
                if (ftype < pDialog->filter()->size())
                    pDialog->selected_filter()->set(ftype);
            }

            // Show the dialog
            pDialog->show(wWidget);
        }

        void FileButton::update_path()
        {
            if (pDialog == NULL)
                return;

            // Write new path as UTF-8 string
            if (pPathPort != NULL)
            {
                // Obtain the current path from dialog
                LSPString path;
                status_t res = pDialog->path()->format(&path);
                if ((res == STATUS_OK) && (path.length() > 0))
                {
                    const char *u8path = path.get_utf8();
                    if (u8path == NULL)
                        u8path          = "";
                    pPathPort->write(u8path, strlen(u8path));
                    pPathPort->notify_all(ui::PORT_USER_EDIT);
                }
            }

            // Write file type as integer
            if (pFileTypePort != NULL)
            {
                pFileTypePort->set_value(pDialog->selected_filter()->get());
                pFileTypePort->notify_all(ui::PORT_USER_EDIT);
            }
        }

        void FileButton::commit_file()
        {
            if (pDialog == NULL)
                return;

            LSPString path;
            if (pDialog->selected_file()->format(&path) != STATUS_OK)
                return;

            // Write new path as UTF-8 string
            if (pPort != NULL)
            {
                const char *u8path = path.get_utf8();
                pPort->write(u8path, strlen(u8path));
                pPort->notify_all(ui::PORT_USER_EDIT);
            }
            // Trigger file save
            if (pCommand != NULL)
            {
                pCommand->set_value(1.0f);
                pCommand->notify_all(ui::PORT_USER_EDIT);
            }
        }

        status_t FileButton::slot_submit(tk::Widget *sender, void *ptr, void *data)
        {
            FileButton *_this = static_cast<FileButton *>(ptr);
            if (_this != NULL)
                _this->show_file_dialog();
            return STATUS_OK;
        }

        status_t FileButton::slot_drag_request(tk::Widget *sender, void *ptr, void *data)
        {
            // Get controller and display
            FileButton *_this   = static_cast<FileButton *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_ARGUMENTS;

            if (_this->wWidget == NULL)
                return STATUS_BAD_STATE;

            tk::Display *dpy    = _this->wWidget->display();
            if (dpy == NULL)
                return STATUS_BAD_STATE;

            // Disable drag-in for the 'save' widget
            if (_this->bSave)
            {
                dpy->reject_drag();
                lsp_trace("Rejected drag");
                return STATUS_OK;
            }

            // Process the drag request
            ws::rectangle_t r;
            _this->wWidget->get_rectangle(&r);

            const char * const *ctype = dpy->get_drag_mime_types();
            ssize_t idx = _this->pDragInSink->select_mime_type(ctype);
            if (idx >= 0)
            {
                dpy->accept_drag(_this->pDragInSink, ws::DRAG_COPY, &r);
                lsp_trace("Accepted drag");
            }
            else
            {
                dpy->reject_drag();
                lsp_trace("Rejected drag");
            }

            return STATUS_OK;
        }

        status_t FileButton::slot_dialog_submit(tk::Widget *sender, void *ptr, void *data)
        {
            // Get controller and display
            FileButton *_this   = static_cast<FileButton *>(ptr);
            if (_this != NULL)
                _this->commit_file();

            return STATUS_OK;
        }

        status_t FileButton::slot_dialog_hide(tk::Widget *sender, void *ptr, void *data)
        {
            // Get controller and display
            FileButton *_this   = static_cast<FileButton *>(ptr);
            if (_this != NULL)
                _this->update_path();
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


