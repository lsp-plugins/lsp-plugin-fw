/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 авг. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_DOT_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_DOT_H_

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
        class Dot: public Widget
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
                    tk::Boolean        *pEditable;
                    tk::StepFloat      *pStep;
                } param_t;

            protected:
                param_t             sX;
                param_t             sY;
                param_t             sZ;
                bool                bEditing;

                ctl::Integer        sSize;
                ctl::Integer        sHoverSize;
                ctl::Integer        sBorderSize;
                ctl::Integer        sHoverBorderSize;
                ctl::Integer        sGap;
                ctl::Integer        sHoverGap;

                ctl::Color          sColor;
                ctl::Color          sHoverColor;
                ctl::Color          sBorderColor;
                ctl::Color          sHoverBorderColor;
                ctl::Color          sGapColor;
                ctl::Color          sHoverGapColor;

            protected:
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_begin_edit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_end_edit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dbl_click(tk::Widget *sender, void *ptr, void *data);

            protected:
                static void         init_param(param_t *p, tk::RangeFloat *value, tk::StepFloat *step, tk::Boolean *editable);
                void                set_dot_param(param_t *p, const char *prefix, const char *name, const char *value);
                void                submit_values();
                void                submit_default_values();
                void                configure_param(param_t *p, bool axis);
                void                submit_value(param_t *p, float value);
                void                commit_value(param_t *p, ui::IPort *port, bool force);
                void                begin_edit();
                void                end_edit();

            public:
                explicit Dot(ui::IWrapper *wrapper, tk::GraphDot *widget);
                Dot(const Dot &) = delete;
                Dot(Dot &&) = delete;
                virtual ~Dot() override;

                Dot & operator = (const Dot &) = delete;
                Dot & operator = (Dot &&) = delete;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_GRAPH_DOT_H_ */
