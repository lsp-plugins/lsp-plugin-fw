/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 6 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_LABEL_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_LABEL_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        enum label_type_t
        {
            CTL_LABEL_TEXT,
            CTL_LABEL_VALUE,
            CTL_STATUS_CODE
        };

        /**
         * Text label or some value
         */
        class Label: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                class PopupWindow: public tk::PopupWindow
                {
                    private:
                        friend class ctl::Label;

                    protected:
                        Label      *pLabel;
                        tk::Box     sBox;
                        tk::Edit    sValue;
                        tk::Label   sUnits;
                        tk::Button  sApply;
                        tk::Button  sCancel;

                    public:
                        explicit PopupWindow(Label *label, tk::Display *dpy);
                        virtual ~PopupWindow();

                        virtual status_t    init();
                        virtual void        destroy();
                };

            protected:
                label_type_t        enType;

                ctl::Color          sColor;
                ctl::Color          sHoverColor;

                ctl::LCString       sText;
                ui::IPort          *pPort;
                float               fValue;
                bool                bDetailed;
                bool                bSameLine;
                bool                bReadOnly;
                ssize_t             nUnits;
                ssize_t             nPrecision;
                PopupWindow        *wPopup;

            protected:
                static status_t     slot_submit_value(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_change_value(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_cancel_value(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dbl_click(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_key_up(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_button(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                commit_value();
                bool                apply_value(const LSPString *value);

            private:
                void                do_destroy();

            public:
                explicit Label(ui::IWrapper *wrapper, tk::Label *widget, label_type_t type);
                virtual ~Label();

                virtual status_t    init();
                virtual void        destroy();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        notify(ui::IPort *port);
                virtual void        end(ui::UIContext *ctx);
        };
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_LABEL_H_ */
