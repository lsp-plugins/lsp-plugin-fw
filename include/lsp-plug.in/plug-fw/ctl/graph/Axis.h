/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                ctl::Expression     sDx;
                ctl::Expression     sDy;
                ctl::Expression     sAngle;
                ctl::Integer        sWidth;
                ctl::Float          sLength;
                ctl::Color          sColor;
                bool                bLogSet;

            protected:
                float               eval_expr(ctl::Expression *expr);

            public:
                explicit Axis(ui::IWrapper *wrapper, tk::GraphAxis *widget);
                virtual ~Axis();

                virtual status_t    init();

            public:
                virtual void        set(const char *name, const char *value);

                virtual void        notify(ui::IPort *port);

                virtual void        end();
        };
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_AXIS_H_ */
