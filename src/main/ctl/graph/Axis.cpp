/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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
            bLogSet     = false;
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
                sSmooth.init(pWrapper, ga->smooth());
                sMin.init(pWrapper, ga->min());
                sMax.init(pWrapper, ga->max());
                sZero.init(pWrapper, ga->zero());
                sDx.init(pWrapper, this);
                sDy.init(pWrapper, this);
                sAngle.init(pWrapper, this);
                sLength.init(pWrapper, this);
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

                if (set_param(ga->log_scale(), "log", name, value))
                    bLogSet     = true;
                if (set_param(ga->log_scale(), "logarithmic", name, value))
                    bLogSet     = true;

                sWidth.set("width", name, value);
                sColor.set("color", name, value);
                sSmooth.set("smooth", name, value);
                sMin.set("min", name, value);
                sMax.set("max", name, value);
                sZero.set("zero", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        float Axis::eval_expr(ctl::Expression *expr)
        {
            tk::GraphAxis *ga = tk::widget_cast<tk::GraphAxis>(wWidget);
            if (ga == NULL)
                return 0.0f;

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

            expr->params()->clear();
            expr->params()->set_int("_g_width", r.nWidth);
            expr->params()->set_int("_g_height", r.nHeight);
            expr->params()->set_int("_a_width", r.nLeft);
            expr->params()->set_int("_a_height", r.nTop);

            return expr->evaluate();
        }

        void Axis::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            tk::GraphAxis *ga = tk::widget_cast<tk::GraphAxis>(wWidget);
            if (ga != NULL)
            {
                if (sDx.depends(port))
                    ga->direction()->set_dx(eval_expr(&sDx));
                if (sDy.depends(port))
                    ga->direction()->set_dy(eval_expr(&sDy));
                if (sAngle.depends(port))
                    ga->direction()->set_angle(eval_expr(&sAngle) * M_PI);
                if (sLength.depends(port))
                    ga->length()->set(eval_expr(&sLength));
            }
        }

        void Axis::trigger_expr()
        {
            tk::GraphAxis *ga = tk::widget_cast<tk::GraphAxis>(wWidget);
            if (ga == NULL)
                return;

            if (sDx.valid())
                ga->direction()->set_dx(eval_expr(&sDx));
            if (sDy.valid())
                ga->direction()->set_dy(eval_expr(&sDy));
            if (sAngle.valid())
                ga->direction()->set_angle(eval_expr(&sAngle) * M_PI);
            if (sLength.valid())
                ga->length()->set(eval_expr(&sLength));
        }

        void Axis::end(ui::UIContext *ctx)
        {
            trigger_expr();

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
            if (!bLogSet)
                ga->log_scale()->set(meta::is_log_rule(mdata));
        }

        status_t Axis::slot_graph_resize(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Axis *_this    = static_cast<ctl::Axis *>(ptr);
            if (_this != NULL)
                _this->trigger_expr();
            return STATUS_OK;
        }

        void Axis::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);
            trigger_expr();
        }
    } /* namespace ctl */
} /* namespace lsp */


