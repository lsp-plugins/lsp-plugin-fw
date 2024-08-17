/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 17 авг. 2024 г.
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


#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_SHMLINK_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_SHMLINK_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        class ShmLink: public Widget
        {
            public:
                static const ctl_class_t metadata;

            private:
                static const tk::tether_t   popup_tether[];

            protected:
                class PopupWindow: public tk::PopupWindow
                {
                    private:
                        static const tk::w_class_t      metadata;

                    protected:
                        ShmLink    *pLink;
//                        tk::Box     sBox;
//                        tk::Edit    sSearch;
//                        tk::Button  sConnect;
//                        tk::ListBox sConnections;

                    public:
                        explicit PopupWindow(ShmLink *link, tk::Display *dpy);
                        virtual ~PopupWindow() override;

                        virtual status_t    init() override;
                        virtual void        destroy() override;
                };

            protected:
                ui::IPort          *pPort;

                ctl::Color          sColor;
                ctl::Color          sTextColor;
                ctl::Color          sBorderColor;
                ctl::Color          sHoverColor;
                ctl::Color          sTextHoverColor;
                ctl::Color          sBorderHoverColor;
                ctl::Color          sDownColor;
                ctl::Color          sTextDownColor;
                ctl::Color          sBorderDownColor;
                ctl::Color          sDownHoverColor;
                ctl::Color          sTextDownHoverColor;
                ctl::Color          sBorderDownHoverColor;
                ctl::Color          sHoleColor;

                ctl::Boolean        sEditable;
                ctl::Boolean        sHover;

                PopupWindow        *wPopup;

            protected:
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                do_destroy();
                void                sync_state();
                void                show_selector();
                PopupWindow        *create_popup_window();

            public:
                explicit ShmLink(ui::IWrapper *wrapper, tk::Button *widget);
                ShmLink(const ShmLink &) = delete;
                ShmLink(ShmLink &&) = delete;
                virtual ~ShmLink() override;

                ShmLink & operator = (const ShmLink &) = delete;
                ShmLink & operator = (ShmLink &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
        };

    } /* namespace ctl */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_SHMLINK_H_ */
