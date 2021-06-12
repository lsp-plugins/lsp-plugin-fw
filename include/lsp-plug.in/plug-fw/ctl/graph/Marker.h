/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 16 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_MARKER_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_MARKER_H_

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
         * Graph widget that contains another graphic elements
         */
        class Marker: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;
                ctl::Expression     sMin;
                ctl::Expression     sMax;
                ctl::Expression     sValue;
                ctl::Expression     sOffset;
                ctl::Expression     sDx;
                ctl::Expression     sDy;
                ctl::Expression     sAngle;
                ctl::Boolean        sSmooth;
                ctl::Integer        sWidth;
                ctl::Integer        sHoverWidth;
                ctl::Boolean        sEditable;
                ctl::Integer        sLeftBorder;
                ctl::Integer        sRightBorder;
                ctl::Integer        sHoverLeftBorder;
                ctl::Integer        sHoverRightBorder;
                ctl::Color          sColor;
                ctl::Color          sHoverColor;
                ctl::Color          sLeftColor;
                ctl::Color          sRightColor;
                ctl::Color          sHoverLeftColor;
                ctl::Color          sHoverRightColor;

            protected:
                static status_t     slot_graph_resize(tk::Widget *sender, void *ptr, void *data);

            protected:
                float               eval_expr(ctl::Expression *expr);
                void                trigger_expr();

            public:
                explicit Marker(ui::IWrapper *wrapper, tk::GraphMarker *widget);
                virtual ~Marker();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);

                virtual void        notify(ui::IPort *port);

                virtual void        end(ui::UIContext *ctx);
        };
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_MARKER_H_ */
