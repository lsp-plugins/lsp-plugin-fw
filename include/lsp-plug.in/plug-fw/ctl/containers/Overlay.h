/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 апр. 2025 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_OVERLAY_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_OVERLAY_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>


namespace lsp
{
    namespace ctl
    {
        /**
         * Overlay controller
         */
        class Overlay: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;          // Port that controls the visibility of the widget
                LSPString           sTriggerWID;    // Trigger widget identifier
                LSPString           sAreaWID;       // Area widget identifier
                float               fHOrigin;       // Horizontal origin
                float               fVOrigin;       // Vertial origin
                float               fHAlign;        // Horizontal alignment
                float               fVAlign;        // Vertial alignment

                ctl::Float          sTransparency;  // Transparency of the overlay
                ctl::Integer        sPriority;      // Drawing priority
                ctl::Boolean        sAutoClose;     // Automatically close flag
                ctl::Integer        sBorderRadius;  // Border radius
                ctl::Integer        sBorderSize;    // Border size
                ctl::Color          sBorderColor;   // Border color
                ctl::Padding        sIPadding;      // Internal padding

                ctl::Expression     sHOrigin;       // Horizontal relative offset of origin point from left-top position of trigger area
                ctl::Expression     sVOrigin;       // Vertical relative offset of origin point  from left-top position of trigger area
                ctl::Expression     sHAlign;        // Horizontal widget alignment relative to the origin point
                ctl::Expression     sVAlign;        // Vertical widget alignment relative to the origin point

                ctl::Expression     sLayoutHAlign;  // Internal layout horizontal alignment
                ctl::Expression     sLayoutVAlign;  // Internal layout vertical alignment
                ctl::Expression     sLayoutHScale;  // Internal layout horizontal scale
                ctl::Expression     sLayoutVScale;  // Internal layout vertical scale

            protected:
                static bool         update_float(float & value, ctl::Expression & expr);
                static bool         calc_position(ws::rectangle_t *rect, tk::Overlay *overlay, void *data);

                static status_t     slot_on_hide(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                update_layout_alignment();
                void                update_alignment();
                void                on_hide_overlay();
                bool                calc_position(ws::rectangle_t *rect, tk::Overlay *overlay);
                bool                get_area(ws::rectangle_t *rect, const LSPString *wid);

            public:
                explicit Overlay(ui::IWrapper *wrapper, tk::Overlay *widget);
                Overlay(const Overlay &) = delete;
                Overlay(Overlay &&) = delete;
                virtual ~Overlay() override;

                Overlay & operator = (const Overlay &) = delete;
                Overlay & operator = (Overlay &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual status_t    add(ui::UIContext *ctx, ctl::Widget *child) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_OVERLAY_H_ */
