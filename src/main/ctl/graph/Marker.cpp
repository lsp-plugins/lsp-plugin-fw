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

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/meta/func.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Marker)
            status_t res;

            if (!name->equals_ascii("marker"))
                return STATUS_NOT_FOUND;

            tk::GraphMarker *w = new tk::GraphMarker(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Marker *wc  = new ctl::Marker(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Marker)

        //-----------------------------------------------------------------
        const ctl_class_t Marker::metadata        = { "Marker", &Widget::metadata };

        Marker::Marker(ui::IWrapper *wrapper, tk::GraphMarker *widget): Widget(wrapper, widget)
        {
            pClass      = &metadata;

            pPort       = NULL;
        }

        Marker::~Marker()
        {
        }

        status_t Marker::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphMarker *gm = tk::widget_cast<tk::GraphMarker>(wWidget);
            if (gm != NULL)
            {
                sMin.init(pWrapper, this);
                sMax.init(pWrapper, this);
                sValue.init(pWrapper, this);
                sOffset.init(pWrapper, this);
                sDx.init(pWrapper, this);
                sDy.init(pWrapper, this);
                sAngle.init(pWrapper, this);

                sSmooth.init(pWrapper, gm->smooth());
                sWidth.init(pWrapper, gm->width());
                sHoverWidth.init(pWrapper, gm->hover_width());
                sEditable.init(pWrapper, gm->editable());
                sLeftBorder.init(pWrapper, gm->left_border());
                sRightBorder.init(pWrapper, gm->right_border());
                sHoverLeftBorder.init(pWrapper, gm->hover_left_border());
                sHoverRightBorder.init(pWrapper, gm->hover_right_border());

                sColor.init(pWrapper, gm->color());
                sHoverColor.init(pWrapper, gm->hover_color());
                sLeftColor.init(pWrapper, gm->border_left_color());
                sRightColor.init(pWrapper, gm->border_right_color());
                sHoverLeftColor.init(pWrapper, gm->hover_border_left_color());
                sHoverRightColor.init(pWrapper, gm->hover_border_right_color());

                gm->slots()->bind(tk::SLOT_RESIZE_PARENT, slot_graph_resize, this);
            }

            return STATUS_OK;
        }

        void Marker::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphMarker *gm = tk::widget_cast<tk::GraphMarker>(wWidget);
            if (gm != NULL)
            {
                bind_port(&pPort, "id", name, value);

                set_param(gm->basis(), "basis", name, value);
                set_param(gm->parallel(), "parallel", name, value);
                set_param(gm->origin(), "origin", name, value);
                set_param(gm->origin(), "center", name, value);

                set_expr(&sMin, "min", name, value);
                set_expr(&sMax, "max", name, value);
                set_expr(&sValue, "value", name, value);
                set_expr(&sDx, "dx", name, value);
                set_expr(&sDy, "dy", name, value);
                set_expr(&sAngle, "angle", name, value);
                set_expr(&sOffset, "offset", name, value);

                sSmooth.set("smooth", name, value);
                sWidth.set("width", name, value);
                sHoverWidth.set("hwidth", name, value);
                sEditable.set("editable", name, value);
                sLeftBorder.set("lborder", name, value);
                sLeftBorder.set("left_border", name, value);
                sRightBorder.set("rborder", name, value);
                sRightBorder.set("right_border", name, value);

                sHoverLeftBorder.set("hlborder", name, value);
                sHoverLeftBorder.set("hover_left_border", name, value);
                sHoverRightBorder.set("hrborder", name, value);
                sHoverRightBorder.set("hover_right_border", name, value);

                sColor.set("color", name, value);
                sHoverColor.set("hcolor", name, value);
                sHoverColor.set("hover_color", name, value);
                sLeftColor.set("lcolor", name, value);
                sLeftColor.set("left_color", name, value);
                sRightColor.set("rcolor", name, value);
                sRightColor.set("right_color", name, value);
                sHoverLeftColor.set("hlcolor", name, value);
                sHoverLeftColor.set("hover_left_color", name, value);
                sHoverRightColor.set("hrcolor", name, value);
                sHoverRightColor.set("hover_right_color", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        float Marker::eval_expr(ctl::Expression *expr)
        {
            tk::GraphMarker *gm = tk::widget_cast<tk::GraphMarker>(wWidget);
            if (gm == NULL)
                return 0.0f;

            tk::Graph *g = gm->graph();
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

        void Marker::notify(ui::IPort *port)
        {
            Widget::notify(port);

            tk::GraphMarker *gm = tk::widget_cast<tk::GraphMarker>(wWidget);
            if (gm != NULL)
            {
                if ((pPort == port) && (pPort != NULL))
                    gm->value()->set(pPort->value());

                if (sMin.depends(port))
                    gm->value()->set_min(eval_expr(&sMin));
                if (sMax.depends(port))
                    gm->value()->set_min(eval_expr(&sMax));
                if (sValue.depends(port))
                    gm->value()->set(eval_expr(&sValue));
                if (sOffset.depends(port))
                    gm->offset()->set(eval_expr(&sOffset));
                if (sDx.depends(port))
                    gm->direction()->set_dx(eval_expr(&sDx));
                if (sDy.depends(port))
                    gm->direction()->set_dy(eval_expr(&sDy));
                if (sAngle.depends(port))
                    gm->direction()->set_angle(eval_expr(&sAngle) * M_PI);
            }
        }

        void Marker::trigger_expr()
        {
            tk::GraphMarker *gm = tk::widget_cast<tk::GraphMarker>(wWidget);
            if (gm == NULL)
                return;

            if (sMin.valid())
                gm->value()->set_min(eval_expr(&sMin));
            if (sMax.valid())
                gm->value()->set_min(eval_expr(&sMax));
            if (sValue.valid())
                gm->value()->set(eval_expr(&sValue));
            if (sOffset.valid())
                gm->offset()->set(eval_expr(&sOffset));
            if (sDx.valid())
                gm->direction()->set_dx(eval_expr(&sDx));
            if (sDy.valid())
                gm->direction()->set_dy(eval_expr(&sDy));
            if (sAngle.valid())
                gm->direction()->set_angle(eval_expr(&sAngle) * M_PI);
        }

        void Marker::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);

            trigger_expr();

            tk::GraphMarker *gm = tk::widget_cast<tk::GraphMarker>(wWidget);
            if (gm == NULL)
                return;

            const meta::port_t *mdata = (pPort != NULL) ? pPort->metadata() : NULL;
            if (mdata == NULL)
                return;

            if (!sMin.valid())
                gm->value()->set_min(mdata->min);
            if (!sMax.valid())
                gm->value()->set_max(mdata->max);
            if (!sValue.valid())
                gm->value()->set_max(mdata->start);
        }

        status_t Marker::slot_graph_resize(tk::Widget *sender, void *ptr, void *data)
        {
            ctl::Marker *_this    = static_cast<ctl::Marker *>(ptr);
            if (_this != NULL)
                _this->trigger_expr();
            return STATUS_OK;
        }

        void Marker::schema_reloaded()
        {
            Widget::schema_reloaded();

            sColor.reload();
            sHoverColor.reload();
            sLeftColor.reload();
            sRightColor.reload();
            sHoverLeftColor.reload();
            sHoverRightColor.reload();
        }
    }
}





