/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 20 окт. 2021 г.
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

namespace lsp
{
    namespace ctl
    {
//        //---------------------------------------------------------------------
//        CTL_FACTORY_IMPL_START(Stream)
//            status_t res;
//
//            if (!name->equals_ascii("stream"))
//                return STATUS_NOT_FOUND;
//
//            tk::GraphMesh *w = new tk::GraphMesh(context->display());
//            if (w == NULL)
//                return STATUS_NO_MEM;
//            if ((res = context->widgets()->add(w)) != STATUS_OK)
//            {
//                delete w;
//                return res;
//            }
//
//            if ((res = w->init()) != STATUS_OK)
//                return res;
//
//            ctl::Stream *wc  = new ctl::Stream(context->wrapper(), w);
//            if (wc == NULL)
//                return STATUS_NO_MEM;
//
//            *ctl = wc;
//            return STATUS_OK;
//        CTL_FACTORY_IMPL_END(Stream)

        //-----------------------------------------------------------------
        const ctl_class_t Stream::metadata      = { "Stream", &Widget::metadata };

        Stream::Stream(ui::IWrapper *wrapper, tk::GraphMesh *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
        }

        Stream::~Stream()
        {
        }

        status_t Stream::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphMesh *gm   = tk::widget_cast<tk::GraphMesh>(wWidget);
            if (gm != NULL)
            {
                sWidth.init(pWrapper, gm->width());
                sSmooth.init(pWrapper, gm->smooth());
                sFill.init(pWrapper, gm->fill());
                sColor.init(pWrapper, gm->color());
                sFillColor.init(pWrapper, gm->fill_color());
            }

            return STATUS_OK;
        }

        void Stream::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphMesh *gm   = tk::widget_cast<tk::GraphMesh>(wWidget);
            if (gm != NULL)
            {
                bind_port(&pPort, "id", name, value);

                set_param(gm->origin(), "origin", name, value);
                set_param(gm->origin(), "center", name, value);
                set_param(gm->origin(), "o", name, value);

                set_param(gm->haxis(), "haxis", name, value);
                set_param(gm->haxis(), "xaxis", name, value);
                set_param(gm->haxis(), "basis", name, value);
                set_param(gm->haxis(), "ox", name, value);

                set_param(gm->vaxis(), "vaxis", name, value);
                set_param(gm->vaxis(), "yaxis", name, value);
                set_param(gm->vaxis(), "parallel", name, value);
                set_param(gm->vaxis(), "oy", name, value);

                sWidth.set("width", name, value);
                sSmooth.set("smooth", name, value);
                sFill.set("fill", name, value);
                sColor.set("color", name, value);
                sFillColor.set("fill.color", name, value);
                sFillColor.set("fcolor", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Stream::notify(ui::IPort *port)
        {
            Widget::notify(port);
            if ((port != NULL) && (port == pPort))
                commit_data();
        }

        void Stream::trigger_expr()
        {
        }

        void Stream::commit_data()
        {
        }


    } /* namespace ctl */
} /* namespace lsp */





