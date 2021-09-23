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
            if ((url == NULL) || (pButton->pFile == NULL))
                return STATUS_OK;

            LSPString decoded;
            status_t res = (url->starts_with_ascii("file://")) ?
                    url::decode(&decoded, url, 7) :
                    url::decode(&decoded, url);

            if (res != STATUS_OK)
                return res;

            lsp_trace("Set file path to %s", decoded.get_native());
            const char *path = decoded.get_utf8();

            pButton->pFile->write(path, strlen(path));
            pButton->pFile->notify_all();

            return STATUS_OK;
        }

        //-----------------------------------------------------------------
        const ctl_class_t FileButton::metadata          = { "FileButton", &Widget::metadata };

        FileButton::FileButton(ui::IWrapper *wrapper, tk::FileButton *widget, bool save):
            Widget(wrapper, widget)
        {
            bSave           = save;
            pFile           = NULL;
            pCommand        = NULL;
            pProgress       = NULL;
            pPath           = NULL;

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

                sColor.init(pWrapper, fb->color());
                sInvColor.init(pWrapper, fb->inv_color());
                sLineColor.init(pWrapper, fb->line_color());
                sInvLineColor.init(pWrapper, fb->inv_line_color());
                sTextColor.init(pWrapper, fb->text_color());
                sInvTextColor.init(pWrapper, fb->inv_text_color());

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
                bind_port(&pFile, "id", name, value);
                bind_port(&pCommand, "command_id", name, value);
                bind_port(&pCommand, "command.id", name, value);
                bind_port(&pProgress, "progress_id", name, value);
                bind_port(&pProgress, "progress.id", name, value);

                set_expr(&sProgress, "progress", name, value);
                set_expr(&sStatus, "status", name, value);

                sTextPadding.set("text.padding", name, value);
                sTextPadding.set("text.pad", name, value);
                sTextPadding.set("tpad", name, value);

                sColor.set("color", name, value);
                sInvColor.set("inv.color", name, value);
                sInvColor.set("icolor", name, value);
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
            }

            return Widget::set(ctx, name, value);
        }

        void FileButton::trigger_expr()
        {
            // TODO
        }

        void FileButton::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
            trigger_expr();
        }

        void FileButton::notify(ui::IPort *port)
        {
            Widget::notify(port);
        }

        void FileButton::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);

            trigger_expr();
        }

        void FileButton::on_submit()
        {
            lsp_trace("on_submit");
        }

        status_t FileButton::slot_submit(tk::Widget *sender, void *ptr, void *data)
        {
            FileButton *_this = static_cast<FileButton *>(ptr);
            if (_this != NULL)
                _this->on_submit();
            return STATUS_OK;
        }

        status_t FileButton::slot_drag_request(tk::Widget *sender, void *ptr, void *data)
        {
            // Get controller and display
            FileButton *_this   = static_cast<FileButton *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_ARGUMENTS;

            tk::Display *dpy    = (_this->wWidget != NULL) ? _this->wWidget->display() : NULL;
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
                dpy->accept_drag(_this->pDragInSink, ws::DRAG_COPY, true, &r);
                lsp_trace("Accepted drag");
            }
            else
            {
                dpy->reject_drag();
                lsp_trace("Rejected drag");
            }

            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


