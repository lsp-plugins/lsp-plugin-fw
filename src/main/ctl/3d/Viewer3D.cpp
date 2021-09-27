/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 сент. 2021 г.
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
#include <lsp-plug.in/r3d/iface/types.h>

namespace lsp
{
    namespace ctl
    {
        static const point3d_t axis_lines[] =
        {
            // X axis (red)
            { {     0.0f,   0.0f,   0.0f,   1.0f }, {   1.0f,   0.0f,   0.0f,   1.0f } },
            { {     0.25f,  0.0f,   0.0f,   1.0f }, {   1.0f,   0.0f,   0.0f,   1.0f } },
            // Y axis (green)
            { {     0.0f,   0.0f,   0.0f,   1.0f }, {   0.0f,   1.0f,   0.0f,   1.0f } },
            { {     0.0f,   0.25f,  0.0f,   1.0f }, {   0.0f,   1.0f,   0.0f,   1.0f } },
            // Z axis (blue)
            { {     0.0f,   0.0f,   0.0f,   1.0f }, {   0.0f,   0.0f,   1.0f,   1.0f } },
            { {     0.0f,   0.0f,   0.25f,  1.0f }, {   0.0f,   0.0f,   1.0f,   1.0f } }
        };

        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Viewer3D)
            status_t res;

            if (!name->equals_ascii("viewer3d"))
                return STATUS_NOT_FOUND;

            tk::Area3D *w = new tk::Area3D(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Viewer3D *wc  = new ctl::Viewer3D(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Viewer3D)

        //-----------------------------------------------------------------
        const ctl_class_t Viewer3D::metadata    = { "Viewer3D", &Widget::metadata };

        Viewer3D::Viewer3D(ui::IWrapper *wrapper, tk::Area3D *widget): Widget(wrapper, widget)
        {
        }

        Viewer3D::~Viewer3D()
        {
        }

        status_t Viewer3D::init()
        {
            status_t res = ctl::Widget::init();
            if (res != STATUS_OK)
                return res;

            tk::Area3D *a3d = tk::widget_cast<tk::Area3D>(wWidget);
            if (a3d != NULL)
            {
                // TODO


                a3d->slots()->bind(tk::SLOT_DRAW3D, slot_draw3d, this);
                a3d->slots()->bind(tk::SLOT_RESIZE, slot_resize, this);
                a3d->slots()->bind(tk::SLOT_MOUSE_DOWN, slot_mouse_down, this);
                a3d->slots()->bind(tk::SLOT_MOUSE_UP, slot_mouse_up, this);
                a3d->slots()->bind(tk::SLOT_MOUSE_MOVE, slot_mouse_move, this);
            }

            return STATUS_OK;
        }

        void Viewer3D::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Area3D *a3d = tk::widget_cast<tk::Area3D>(wWidget);
            if (a3d != NULL)
            {
                // TODO
            }

            return Widget::set(ctx, name, value);
        }

        void Viewer3D::end(ui::UIContext *ctx)
        {


            Widget::end(ctx);
        }

        void Viewer3D::commit_view(ws::IR3DBackend *r3d)
        {
        }

        status_t Viewer3D::render(ws::IR3DBackend *r3d)
        {
            // Need to update vertex list for the scene?
            commit_view(r3d);

            // Set Light parameters
//            r3d::light_t light;
//
//            light.type          = r3d::LIGHT_POINT;
//            light.position      = sPov;
//            light.direction.dx  = -sDir.dx;
//            light.direction.dy  = -sDir.dy;
//            light.direction.dz  = -sDir.dz;
//            light.direction.dw  = 0.0f;
//
//            light.ambient.r     = 0.0f;
//            light.ambient.g     = 0.0f;
//            light.ambient.b     = 0.0f;
//            light.ambient.a     = 1.0f;
//
//            light.diffuse.r     = 1.0f;
//            light.diffuse.g     = 1.0f;
//            light.diffuse.b     = 1.0f;
//            light.diffuse.a     = 1.0f;
//
//            light.specular.r    = 1.0f;
//            light.specular.g    = 1.0f;
//            light.specular.b    = 1.0f;
//            light.specular.a    = 1.0f;
//
//            light.constant      = 1.0f;
//            light.linear        = 0.0f;
//            light.quadratic     = 0.0f;
//            light.cutoff        = 180.0f;

            // Enable/disable lighting
//            r3d->set_lights(&light, 1);

            r3d::buffer_t buf;
            init_buffer(&buf);

            // Draw axes
            buf.type            = r3d::PRIMITIVE_LINES;
            buf.width           = 2.0f;
            buf.count           = sizeof(axis_lines) / (sizeof(point3d_t) * 2);
            buf.flags           = 0;

            buf.vertex.data     = &axis_lines[0].p;
            buf.vertex.stride   = sizeof(point3d_t);
            buf.vertex.index    = NULL;
            buf.normal.data     = NULL;
            buf.normal.stride   = sizeof(point3d_t);
            buf.normal.index    = NULL;
            buf.color.data      = &axis_lines[0].c;
            buf.color.stride    = sizeof(point3d_t);
            buf.color.index     = NULL;

            // Draw call
            r3d->draw_primitives(&buf);

//            // Render supplementary objects
//            for (size_t i=0, n=area->num_objects3d(); i<n; ++i)
//            {
//                LSPObject3D *obj = area->object3d(i);
//                if ((obj != NULL) && (obj->visible()))
//                    obj->render(r3d);
//            }
//
            // Draw scene primitives
            size_t nvertex      = vVertices.size();
            if (nvertex > 0)
            {
                vertex3d_t *vv      = vVertices.array();
                init_buffer(&buf);

                // Fill buffer
                buf.type            = r3d::PRIMITIVE_TRIANGLES;
                buf.width           = 1.0f;
                buf.count           = nvertex / 3;
                buf.flags           = r3d::BUFFER_BLENDING | r3d::BUFFER_LIGHTING;

                buf.vertex.data     = &vv->p;
                buf.vertex.stride   = sizeof(vertex3d_t);
                buf.vertex.index    = NULL;
                buf.normal.data     = &vv->n;
                buf.normal.stride   = sizeof(vertex3d_t);
                buf.normal.index    = NULL;
                buf.color.data      = &vv->c;
                buf.color.stride    = sizeof(vertex3d_t);
                buf.color.index     = NULL;

                // Draw call
                r3d->draw_primitives(&buf);
            }

            return STATUS_OK;
        }

        status_t Viewer3D::slot_draw3d(tk::Widget *sender, void *ptr, void *data)
        {
            if ((ptr == NULL) || (data == NULL))
                return STATUS_BAD_ARGUMENTS;

            Viewer3D *_this     = static_cast<Viewer3D *>(ptr);
            return (_this != NULL) ? _this->render(static_cast<ws::IR3DBackend *>(data)) : STATUS_BAD_ARGUMENTS;
        }

        status_t Viewer3D::slot_resize(tk::Widget *sender, void *ptr, void *data)
        {
            // TODO
            return STATUS_OK;
        }

        status_t Viewer3D::slot_mouse_down(tk::Widget *sender, void *ptr, void *data)
        {
            // TODO
            return STATUS_OK;
        }

        status_t Viewer3D::slot_mouse_up(tk::Widget *sender, void *ptr, void *data)
        {
            // TODO
            return STATUS_OK;
        }

        status_t Viewer3D::slot_mouse_move(tk::Widget *sender, void *ptr, void *data)
        {
            // TODO
            return STATUS_OK;
        }

    } /* namespace ctl */
} /* namespace lsp */


