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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/meta/func.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Axis)
            status_t res;

            if (!name->equals_ascii("axis"))
                return STATUS_NOT_FOUND;

            tk::GraphAxis *w = new tk::GraphAxis(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Axis *wc  = new ctl::Axis(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Axis)

        //-----------------------------------------------------------------
        const ctl_class_t Axis::metadata        = { "Axis", &Widget::metadata };

        Axis::Axis(ui::IWrapper *wrapper, tk::GraphAxis *widget): Widget(wrapper, widget)
        {
            pClass      = &metadata;

            pPort       = NULL;
        }

        Axis::~Axis()
        {
        }

        status_t Axis::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphAxis *ga = tk::widget_cast<tk::GraphAxis>(wWidget);
            if (ga != NULL)
            {
                sSmooth.init(pWrapper, ga->smooth(), &sVariables);
                sMin.init(pWrapper, ga->min(), &sVariables);
                sMax.init(pWrapper, ga->max(), &sVariables);
                sZero.init(pWrapper, ga->zero(), &sVariables);
                sLogScale.init(pWrapper, ga->log_scale(), &sVariables);
                sDx.init(pWrapper, this, &sVariables);
                sDy.init(pWrapper, this, &sVariables);
                sAngle.init(pWrapper, this, &sVariables);
                sLength.init(pWrapper, this, &sVariables);
                sWidth.init(pWrapper, ga->width());
                sColor.init(pWrapper, ga->color());

                ga->slots()->bind(tk::SLOT_RESIZE_PARENT, slot_graph_resize, this);
            }

            return STATUS_OK;
        }

        void Axis::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphAxis *ga = tk::widget_cast<tk::GraphAxis>(wWidget);
            if (ga != NULL)
            {
                bind_port(&pPort, "id", name, value);

                set_expr(&sDx, "dx", name, value);
                set_expr(&sDy, "dy", name, value);
                set_expr(&sAngle, "angle", name, value);
                set_expr(&sLength, "length", name, value);

                set_param(ga->origin(), "origin", name, value);
                set_param(ga->origin(), "center", name, value);
                set_param(ga->origin(), "o", name, value);

                set_param(ga->priority(), "priority", name, value);
                set_param(ga->priority_group(), "priority_group", name, value);
                set_param(ga->priority_group(), "pgroup", name, value);

                sLogScale.set("log", name, value);
                sLogScale.set("logarithmic", name, value);
                sWidth.set("width", name, value);
                sColor.set("color", name, value);
                sSmooth.set("smooth", name, value);
                sMin.set("min", name, value);
                sMax.set("max", name, value);
                sZero.set("zero", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Axis::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            tk::GraphAxis *ga = tk::widget_cast<tk::GraphAxis>(wWidget);
            if (ga != NULL)
            {
                if (sDx.depends(port))
                    ga->direction()->set_dx(sDx.evaluate_float());
                if (sDy.depends(port))
                    ga->direction()->set_dy(sDy.evaluate_float());
                if (sAngle.depends(port))
                    ga->direction()->set_angle(sAngle.evaluate_float() * M_PI);
                if (sLength.depends(port))
                    ga->length()->set(sLength.evaluate_float());
            }
        }

        void Axis::trigger_expr()
        {
            sSmooth.apply_changes();
            sMin.apply_changes();
            sMax.apply_changes();
            sZero.apply_changes();
            sLogScale.apply_changes();

            tk::GraphAxis *ga = tk::widget_cast<tk::GraphAxis>(wWidget);
            if (ga != NULL)
            {
                if (sDx.valid())
                    ga->direction()->set_dx(sDx.evaluate_float());
                if (sDy.valid())
                    ga->direction()->set_dy(sDy.evaluate_float());
                if (sAngle.valid())
                    ga->direction()->set_angle(sAngle.evaluate_float() * M_PI);
                if (sLength.valid())
                    ga->length()->set(sLength.evaluate_float());
            }
        }

        void Axis::end(ui::UIContext *ctx)
        {
            on_graph_resize();

            tk::GraphAxis *ga = tk::widget_cast<tk::GraphAxis>(wWidget);
            if (ga == NULL)
                return;

            const meta::port_t *mdata = (pPort != NULL) ? pPort->metadata() : NULL;
            if (mdata == NULL)
                return;

            if (!sMin.valid())
                ga->min()->set(mdata->min);
            if (!sMax.valid())
                ga->max()->set(mdata->max);
            if (!sLogScale.valid())
                ga->log_scale()->set(meta::is_log_rule(mdata));
        }

        void Axis::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);
            trigger_expr();
        }

        void Axis::on_graph_resize()
        {
            tk::GraphAxis *ga = tk::widget_cast<tk::GraphAxis>(wWidget);
            if (ga == NULL)
                return;

            tk::Graph *g = ga->graph();
            ws::rectangle_t r;
            r.nLeft     = 0;
            r.nTop      = 0;
            r.nWidth    = 0;
            r.nHeight   = 0;

            if (g != NULL)
            {
                g->get_rectangle(&r);
                r.nLeft = g->canvas_width();
                r.nTop  = g->canvas_height();
            }

            sVariables.set_int("_g_width", r.nWidth);
            sVariables.set_int("_g_height", r.nHeight);
            sVariables.set_int("_a_width", r.nLeft);
            sVariables.set_int("_a_height", r.nTop);

            trigger_expr();
        }

        status_t Axis::slot_graph_resize(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Axis *self     = static_cast<ctl::Axis *>(ptr);
            if (self != NULL)
                self->on_graph_resize();

            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


