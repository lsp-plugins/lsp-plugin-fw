/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 сент. 2025 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_GRAPHEMBED_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_GRAPHEMBED_H_

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
         * Graph widget that allows to place another widget on graph
         */
        class Embed: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ctl::Integer            sXAxis;
                ctl::Integer            sYAxis;
                ctl::Float              sHStartValue;           // Horizontal axis start value
                ctl::Float              sVStartValue;           // Vertical axis start value
                ctl::Float              sHEndValue;             // Horizontal axis end value
                ctl::Float              sVEndValue;             // Vertical axis end value
                ctl::Layout             sLayout;                // Layout
                ctl::Float              sTransparency;          // Transparency

            public:
                explicit Embed(ui::IWrapper *wrapper, tk::GraphEmbed *widget);
                Embed(const Embed &) = delete;
                Embed(Embed &&) = delete;
                virtual ~Embed() override;

                Embed & operator = (const Embed &) = delete;
                Embed & operator = (Embed &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual status_t    add(ui::UIContext *ctx, ctl::Widget *child) override;
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_GRAPHEMBED_H_ */
