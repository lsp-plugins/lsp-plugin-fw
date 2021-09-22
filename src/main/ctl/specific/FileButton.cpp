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
        const ctl_class_t FileButton::metadata          = { "FileButton", &Widget::metadata };

        FileButton::FileButton(ui::IWrapper *wrapper, tk::FileButton *widget, bool save):
            Widget(wrapper, widget)
        {
            bSave       = save;
            pFile       = NULL;
            pCommand    = NULL;
            pProgress   = NULL;
            pPath       = NULL;
        }

        FileButton::~FileButton()
        {
        }

        status_t FileButton::init()
        {
            status_t res = Widget::init();
            if (res != STATUS_OK)
                return res;

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

    } /* namespace ctl */
} /* namespace lsp */


