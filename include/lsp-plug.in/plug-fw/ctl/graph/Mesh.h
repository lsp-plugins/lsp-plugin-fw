/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_MESH_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_MESH_H_

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
        class Mesh: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pPort;
                ctl::Integer        sWidth;
                ctl::Boolean        sSmooth;
                ctl::Boolean        sFill;
                ctl::Integer        sStrobes;
                ctl::Integer        sXAxis;
                ctl::Integer        sYAxis;

                ctl::Color          sColor;
                ctl::Color          sFillColor;

                ctl::Expression     sXIndex;
                ctl::Expression     sYIndex;
                ctl::Expression     sSIndex;
                ctl::Expression     sMaxDots;
                ctl::Expression     sStrobe;

                bool                bStream;
                bool                bStrobe;
                ssize_t             nXIndex;
                ssize_t             nYIndex;
                ssize_t             nSIndex;
                ssize_t             nMaxDots;

            protected:
                void                trigger_expr();
                void                commit_data();
                static ssize_t      get_strobe_block_size(const float *s, size_t size);

            public:
                explicit Mesh(ui::IWrapper *wrapper, tk::GraphMesh *widget, bool stream);
                virtual ~Mesh() override;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_MESH_H_ */
