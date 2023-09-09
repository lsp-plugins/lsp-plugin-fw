/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 окт. 2021 г.
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
#include <private/ui/BuiltinStyle.h>

namespace lsp
{
    namespace ctl
    {
        //-----------------------------------------------------------------
        namespace style
        {
            LSP_TK_STYLE_IMPL_BEGIN(Origin3D, Object3D)
                // Bind
                sWidth.bind("width", this);
                sColor[0].bind("x.color", this);
                sColor[1].bind("y.color", this);
                sColor[2].bind("z.color", this);
                sLength[0].bind("x.length", this);
                sLength[1].bind("y.length", this);
                sLength[2].bind("z.length", this);

                // Configure
                sWidth.set(2.0f);
                sColor[0].set("#ff0000");
                sColor[1].set("#00ff00");
                sColor[2].set("#0000ff");
                sLength[0].set(0.25f);
                sLength[1].set(0.25f);
                sLength[2].set(0.25f);
            LSP_TK_STYLE_IMPL_END

            LSP_UI_BUILTIN_STYLE(Origin3D, "Origin3D", "root");
        }

        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Origin3D)
            if (!name->equals_ascii("origin3d"))
                return STATUS_NOT_FOUND;

            ctl::Origin3D *wc   = new ctl::Origin3D(context->wrapper());
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Origin3D)

        //-----------------------------------------------------------------
        const ctl_class_t Origin3D::metadata   = { "Origin3D", &Object3D::metadata };

        Origin3D::Origin3D(ui::IWrapper *wrapper):
            Object3D(wrapper),
            sWidth(&sProperties)
        {
            pClass          = &metadata;

            // Set listener
            for (size_t i=0; i<3; ++i)
            {
                sColor[i].listener(&sProperties);
                sLength[i].listener(&sProperties);
            }

            for (size_t i=0; i<6; ++i)
            {
                r3d::dot4_t *dot    = &vAxisLines[i];
                dot->x              = 0.0f;
                dot->y              = 0.0f;
                dot->z              = 0.0f;
                dot->w              = 1.0f;
            }

            for (size_t i=0; i<6; ++i)
            {
                r3d::color_t *col   = &vAxisColors[i];
                col->r              = 0.0f;
                col->g              = 0.0f;
                col->b              = 0.0f;
                col->a              = 0.0f;
            }
        }

        Origin3D::~Origin3D()
        {
            pParent         = NULL;
        }

        status_t Origin3D::init()
        {
            status_t res = Object3D::init();
            if (res != STATUS_OK)
                return res;

            // Initialize styles
            sWidth.bind("width", &sStyle);
            sColor[0].bind("x.color", &sStyle);
            sColor[1].bind("y.color", &sStyle);
            sColor[2].bind("z.color", &sStyle);
            sLength[0].bind("x.length", &sStyle);
            sLength[1].bind("y.length", &sStyle);
            sLength[2].bind("z.length", &sStyle);

            // Initialize controllers
            cWidth.init(pWrapper, &sWidth);
            cColor[0].init(pWrapper, &sColor[0]);
            cColor[1].init(pWrapper, &sColor[1]);
            cColor[2].init(pWrapper, &sColor[2]);
            cLength[0].init(pWrapper, &sLength[0]);
            cLength[1].init(pWrapper, &sLength[1]);
            cLength[2].init(pWrapper, &sLength[2]);

            return STATUS_OK;
        }

        void Origin3D::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            // Set-up settings
            cWidth.set("width", name, value);
            cColor[0].set("x.color", name, value);
            cColor[1].set("y.color", name, value);
            cColor[2].set("z.color", name, value);
            cLength[0].set("x.length", name, value);
            cLength[1].set("y.length", name, value);
            cLength[2].set("z.length", name, value);

            return Widget::set(ctx, name, value);
        }

        void Origin3D::property_changed(tk::Property *prop)
        {
            if (sWidth.is(prop))
                query_draw();
            for (size_t i=0; i<3; ++i)
            {
                if (sColor[i].is(prop))
                    query_draw();
                if (sLength[i].is(prop))
                    query_draw();
            }
        }

        bool Origin3D::submit_foreground(lltl::darray<r3d::buffer_t> *dst)
        {
            r3d::buffer_t *buf = dst->add();
            if (buf == NULL)
                return false;

            r3d::init_buffer(buf);

            // Initialize colors
            for (size_t i=0; i<3; ++i)
            {
                r3d::color_t *c = &vAxisColors[i << 1];
                sColor[i].get_rgba(c->r, c->g, c->b, c->a);
                c[1]            = c[0];
            }

            // Initialize lines
            for (size_t i=0; i<6; ++i)
            {
                vAxisLines[i].x     = 0.0f;
                vAxisLines[i].y     = 0.0f;
                vAxisLines[i].z     = 0.0f;
                vAxisLines[i].w     = 1.0f;
            }
            vAxisLines[1].x     = sLength[0].get();
            vAxisLines[3].y     = sLength[1].get();
            vAxisLines[5].z     = sLength[2].get();

            // Draw axes
            buf->type           = r3d::PRIMITIVE_LINES;
            buf->width          = sWidth.get();
            buf->count          = 3; // 3 lines
            buf->flags          = r3d::BUFFER_BLENDING;

            buf->vertex.data    = vAxisLines;
            buf->vertex.stride  = sizeof(r3d::dot4_t);
            buf->vertex.index   = NULL;
            buf->color.data     = vAxisColors;
            buf->color.stride   = sizeof(r3d::color_t);
            buf->color.index    = NULL;

            return true;
        }

    } /* namespace ctl */
} /* namespace lsp */


