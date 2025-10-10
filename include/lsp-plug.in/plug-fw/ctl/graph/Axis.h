/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_AXIS_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_AXIS_H_

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
        class Axis: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;
                ctl::Boolean        sSmooth;
                ctl::Float          sMin;
                ctl::Float          sMax;
                ctl::Float          sZero;
                ctl::Boolean        sLogScale;
                ctl::Expression     sDx;
                ctl::Expression     sDy;
                ctl::Expression     sAngle;
                ctl::Expression     sLength;
                ctl::Integer        sWidth;
                ctl::Color          sColor;

                expr::Variables     sVariables;

            protected:
                static status_t     slot_graph_resize(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                trigger_expr();
                void                on_graph_resize();

            public:
                explicit Axis(ui::IWrapper *wrapper, tk::GraphAxis *widget);
                Axis(const Axis &) = delete;
                Axis(Axis &&) = delete;
                virtual ~Axis() override;

                Axis & operator = (const Axis &) = delete;
                Axis & operator = (Axis &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_AXIS_H_ */
