/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 июн. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_LINESEGMENT_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_LINESEGMENT_H_

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
         * Line segment on the graph widget
         */
        class LineSegment: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                enum dot_flags_t
                {
                    DF_MIN          = 1 << 0,
                    DF_MAX          = 1 << 1,
                    DF_STEP         = 1 << 2,
                    DF_ASTEP        = 1 << 3,
                    DF_DSTEP        = 1 << 4,
                    DF_LOG          = 1 << 5,
                    DF_LOG_SET      = 1 << 6,
                    DF_AXIS         = 1 << 7
                };

                typedef struct param_t
                {
                    size_t              nFlags;
                    float               fMin;
                    float               fMax;
                    float               fDefault;
                    float               fStep;
                    float               fAStep;
                    float               fDStep;

                    ui::IPort          *pPort;
                    ctl::Expression     sExpr;
                    ctl::Boolean        sEditable;
                    tk::RangeFloat     *pValue;
                    tk::StepFloat      *pStep;
                } param_t;

            protected:
                param_t             sX;
                param_t             sY;
                param_t             sZ;

                ctl::Boolean        sSmooth;
                ctl::Integer        sWidth;
                ctl::Integer        sHoverWidth;
                ctl::Integer        sLeftBorder;
                ctl::Integer        sRightBorder;
                ctl::Integer        sHoverLeftBorder;
                ctl::Integer        sHoverRightBorder;
                ctl::Expression     sBeginX;
                ctl::Expression     sBeginY;

                ctl::Color          sColor;
                ctl::Color          sHoverColor;
                ctl::Color          sLeftColor;
                ctl::Color          sRightColor;
                ctl::Color          sHoverLeftColor;
                ctl::Color          sHoverRightColor;

            protected:
                static void         init_param(param_t *p, tk::RangeFloat *value, tk::StepFloat *step);
                void                set_segment_param(param_t *p, const char *prefix, const char *name, const char *value);
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dbl_click(tk::Widget *sender, void *ptr, void *data);
                void                submit_values();
                void                submit_default_values();
                void                configure_param(param_t *p, bool axis);
                void                submit_value(param_t *p, float value);
                void                commit_value(param_t *p, ui::IPort *port, bool force);

            public:
                explicit LineSegment(ui::IWrapper *wrapper, tk::GraphLineSegment *widget);
                virtual ~LineSegment() override;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_LINESEGMENT_H_ */
