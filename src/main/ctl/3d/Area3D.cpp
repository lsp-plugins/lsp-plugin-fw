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
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/r3d/iface/types.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Area3D)
            status_t res;

            if (!name->equals_ascii("area3d"))
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

            ctl::Area3D *wc  = new ctl::Area3D(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Area3D)

        //-----------------------------------------------------------------
        const ctl_class_t Area3D::metadata    = { "Viewer3D", &Widget::metadata };

        Area3D::Area3D(ui::IWrapper *wrapper, tk::Area3D *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPosX           = NULL;
            pPosY           = NULL;
            pPosZ           = NULL;
            pYaw            = NULL;
            pPitch          = NULL;
            pScaleX         = NULL;
            pScaleY         = NULL;
            pScaleZ         = NULL;
            pOrientation    = NULL;

            bViewChanged    = true;
            fFov            = 70.0f;

            dsp::init_point_xyz(&sPov, 0.0f, -6.0f, 0.0f);
            dsp::init_point_xyz(&sOldPov, 0.0f, -6.0f, 0.0f);
            dsp::init_vector_dxyz(&sScale, 1.0f, 1.0f, 1.0f);
            dsp::init_vector_dxyz(&sTop, 0.0f, 0.0f, -1.0f);
            dsp::init_vector_dxyz(&sXTop, 0.0f, 0.0f, -1.0f);
            dsp::init_vector_dxyz(&sDir, 0.0f, -1.0f, 0.0f);
            dsp::init_vector_dxyz(&sSide, -1.0f, 0.0f, 0.0f);

            sAngles.fYaw    = 0.0f;
            sAngles.fPitch  = 0.0f;
            sAngles.fRoll   = 0.0f;
            sOldAngles      = sAngles;

            nBMask          = 0;
            nMouseX         = 0;
            nMouseY         = 0;
        }

        Area3D::~Area3D()
        {
        }

        status_t Area3D::init()
        {
            status_t res = ctl::Widget::init();
            if (res != STATUS_OK)
                return res;

            tk::Area3D *a3d = tk::widget_cast<tk::Area3D>(wWidget);
            if (a3d != NULL)
            {
                vAxes[0].bind("axis.x.color", a3d->style());
                vAxes[1].bind("axis.y.color", a3d->style());
                vAxes[2].bind("axis.z.color", a3d->style());

                sBorderFlat.init(pWrapper, a3d->border_flat());
                sColor.init(pWrapper, a3d->color());
                sBorderColor.init(pWrapper, a3d->border_color());
                sGlassColor.init(pWrapper, a3d->glass_color());
                sAxes[0].init(pWrapper, &vAxes[0]);
                sAxes[1].init(pWrapper, &vAxes[1]);
                sAxes[2].init(pWrapper, &vAxes[2]);

                vAxes[0].set("area3d_x");
                vAxes[1].set("area3d_y");
                vAxes[2].set("area3d_z");

                a3d->slots()->bind(tk::SLOT_DRAW3D, slot_draw3d, this);
                a3d->slots()->bind(tk::SLOT_MOUSE_DOWN, slot_mouse_down, this);
                a3d->slots()->bind(tk::SLOT_MOUSE_UP, slot_mouse_up, this);
                a3d->slots()->bind(tk::SLOT_MOUSE_MOVE, slot_mouse_move, this);
            }

            return STATUS_OK;
        }

        void Area3D::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Area3D *a3d = tk::widget_cast<tk::Area3D>(wWidget);
            if (a3d != NULL)
            {
                // Bind ports
                bind_port(&pPosX, "pos.x.id", name, value);
                bind_port(&pPosY, "pos.y.id", name, value);
                bind_port(&pPosZ, "pos.z.id", name, value);
                bind_port(&pScaleX, "scale.x.id", name, value);
                bind_port(&pScaleY, "scale.y.id", name, value);
                bind_port(&pScaleZ, "scale.z.id", name, value);
                bind_port(&pYaw, "yaw.id", name, value);
                bind_port(&pPitch, "pitch.id", name, value);

                // Set-up static properties
                set_constraints(a3d->constraints(), name, value);
                set_param(a3d->border_size(), "border.size", name, value);
                set_param(a3d->border_size(), "bsize", name, value);
                set_param(a3d->border_radius(), "border.radius", name, value);
                set_param(a3d->border_radius(), "bradius", name, value);
                set_param(a3d->border_radius(), "brad", name, value);
                set_param(a3d->glass(), "glass", name, value);

                // Set-up dynamic properties
                sBorderFlat.set("border.flat", name, value);
                sBorderFlat.set("bflat", name, value);
                sColor.set("color", name, value);
                sBorderColor.set("border.color", name, value);
                sBorderColor.set("bcolor", name, value);
                sGlassColor.set("glass.color", name, value);
                sGlassColor.set("gcolor", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Area3D::property_changed(tk::Property *prop)
        {
            // Test Axes colors for changes
            for (size_t i=0; i<3; ++i)
            {
                if (vAxes[i].is(prop))
                    wWidget->query_draw();
            }
        }

        void Area3D::rotate_camera(ssize_t dx, ssize_t dy)
        {
            float dyaw      = get_adelta(pYaw, M_PI * 2e-3f);
            float dpitch    = get_adelta(pPitch, M_PI * 2e-3f);

            float yaw       = sOldAngles.fYaw - (dx * dyaw);
            float pitch     = sOldAngles.fPitch - (dy * dpitch);

            if (pPitch == NULL)
            {
                if (pitch >= (89.0f * M_PI / 360.0f))
                    pitch       = (89.0f * M_PI / 360.0f);
                else if (pitch <= (-89.0f * M_PI / 360.0f))
                    pitch       = (-89.0f * M_PI / 360.0f);
            }

            submit_angle_change(&sAngles.fYaw, yaw, pYaw);
            submit_angle_change(&sAngles.fPitch, pitch, pPitch);
        }

        void Area3D::move_camera(ssize_t dx, ssize_t dy, ssize_t dz)
        {
            dsp::point3d_t pov;
            float mdx       = dx * get_delta(pPosX, 0.01f) * 5.0f;
            float mdy       = dy * get_delta(pPosY, 0.01f) * 5.0f;
            float mdz       = dz * get_delta(pPosZ, 0.01f) * 5.0f;

            pov.x           = sOldPov.x + sSide.dx * mdx + sDir.dx * mdy + sXTop.dx * mdz;
            pov.y           = sOldPov.y + sSide.dy * mdx + sDir.dy * mdy + sXTop.dy * mdz;
            pov.z           = sOldPov.z + sSide.dz * mdx + sDir.dz * mdy + sXTop.dz * mdz;

            submit_pov_change(&sPov.x, pov.x, pPosX);
            submit_pov_change(&sPov.y, pov.y, pPosY);
            submit_pov_change(&sPov.z, pov.z, pPosZ);
        }

        float Area3D::get_delta(ui::IPort *p, float dfl)
        {
            const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
            if ((meta != NULL) && (meta->flags & meta::F_STEP))
                return meta->step;
            return dfl;
        }

        float Area3D::get_adelta(ui::IPort *p, float dfl)
        {
            const meta::port_t *meta = (p != NULL) ? p->metadata() : NULL;
            if ((meta != NULL) && (meta->flags & meta::F_STEP))
                return meta::is_degree_unit(meta->unit) ? meta->step * 5.0f * M_PI / 180.0f : meta->step;
            return dfl;
        }

        void Area3D::submit_pov_change(float *vold, float vnew, ui::IPort *port)
        {
            if (*vold == vnew)
                return;

            if (port != NULL)
            {
                port->set_value(vnew);
                port->notify_all();
            }
            else
            {
                *vold           = vnew;
                notify_view_changed();
            }
        }

        void Area3D::submit_angle_change(float *vold, float vnew, ui::IPort *port)
        {
            if (*vold == vnew)
                return;

            const meta::port_t *meta = (port != NULL) ? port->metadata() : NULL;
            if (meta != NULL)
            {
                if (meta::is_degree_unit(meta->unit))
                    vnew    = vnew * 180.0f / M_PI;
                port->set_value(vnew);
                port->notify_all();
            }
            else
            {
                *vold           = vnew;
                notify_view_changed();
            }
        }

        void Area3D::notify_view_changed()
        {
            // Notify children
            for (size_t i=0, n=vObjects.size(); i<n; ++i)
            {
                ctl::Object3D *obj = vObjects.uget(i);
                if (obj != NULL)
                    obj->query_draw();
            }

            query_view_change();
        }

        void Area3D::sync_pov_change(float *dst, ui::IPort *port, ui::IPort *psrc)
        {
            if ((psrc != port) || (port == NULL))
                return;
            *dst    = psrc->value();

            notify_view_changed();
        }

        void Area3D::sync_scale_change(float *dst, ui::IPort *port, ui::IPort *psrc)
        {
            if ((psrc != port) || (port == NULL))
                return;
            float v = psrc->value() * 0.01f;
            if (*dst == v)
                return;

            *dst            = v;
            query_view_change();
        }

        void Area3D::sync_angle_change(float *dst, ui::IPort *port, ui::IPort *psrc)
        {
            if ((psrc != port) || (port == NULL))
                return;
            const meta::port_t *meta = port->metadata();
            if (meta == NULL)
                return;

            float value = psrc->value();
            if (meta::is_degree_unit(meta->unit))
                value       = value * M_PI / 180.0f;
            *dst    = value;

            notify_view_changed();
        }

        void Area3D::setup_camera(ws::IR3DBackend *r3d)
        {
            // Apply the projection matrix
            {
                ssize_t vx, vy, vw, vh;

                // Get location of the area
                r3d->get_location(&vx, &vy, &vw, &vh);
                dsp::matrix3d_t projection;

                float aspect    = float(vw)/float(vh);
                float zNear     = 0.1f;
                float zFar      = 1000.0f;

                float fH        = tanf( fFov * M_PI / 360.0f) * zNear;
                float fW        = fH * aspect;
                dsp::init_matrix3d_frustum(&projection, -fW, fW, -fH, fH, zNear, zFar);

                r3d->set_matrix(r3d::MATRIX_PROJECTION, reinterpret_cast<r3d::mat4_t *>(&projection));
            }

            // Apply the view matrix
            {
                dsp::matrix3d_t view;

                // Compute rotation matrix
                dsp::matrix3d_t delta, tmp;
                dsp::init_matrix3d_rotate_z(&delta, sAngles.fYaw);
                dsp::init_matrix3d_rotate_x(&tmp, sAngles.fPitch);
                dsp::apply_matrix3d_mm1(&delta, &tmp);

                // Compute camera direction vector
                dsp::init_vector_dxyz(&sDir, 0.0f, -1.0f, 0.0f);
                dsp::init_vector_dxyz(&sSide, -1.0f, 0.0f, 0.0f);
                dsp::init_vector_dxyz(&sXTop, 0.0f, 0.0f, -1.0f);
                dsp::apply_matrix3d_mv1(&sDir, &delta);
                dsp::apply_matrix3d_mv1(&sSide, &delta);
                dsp::apply_matrix3d_mv1(&sXTop, &delta);

                // Initialize camera look
                dsp::init_matrix3d_lookat_p1v2(&view, &sPov, &sDir, &sTop);

                r3d->set_matrix(r3d::MATRIX_VIEW, reinterpret_cast<r3d::mat4_t *>(&view));
            }
        }

        void Area3D::setup_lighting(ws::IR3DBackend *r3d)
        {
            r3d::light_t light;

            light.type          = r3d::LIGHT_POINT;
            light.position.x    = 0;
            light.position.y    = 0;
            light.position.z    = 0;
            light.position.w    = 1.0;
            light.direction.dx  = -sDir.dx;
            light.direction.dy  = -sDir.dy;
            light.direction.dz  = -sDir.dz;
            light.direction.dw  = 0.0f;

            light.ambient.r     = 0.0f;
            light.ambient.g     = 0.0f;
            light.ambient.b     = 0.0f;
            light.ambient.a     = 1.0f;

            light.diffuse.r     = 1.0f;
            light.diffuse.g     = 1.0f;
            light.diffuse.b     = 1.0f;
            light.diffuse.a     = 1.0f;

            light.specular.r    = 1.0f;
            light.specular.g    = 1.0f;
            light.specular.b    = 1.0f;
            light.specular.a    = 1.0f;

            light.constant      = 1.0f;
            light.linear        = 0.0f;
            light.quadratic     = 0.0f;
            light.cutoff        = 180.0f;

            // Enable/disable lighting
            r3d->set_lights(&light, 1);
        }

        void Area3D::commit_view(ws::IR3DBackend *r3d)
        {
            // TODO
        }

        void Area3D::draw_supplementary(ws::IR3DBackend *r3d)
        {
            lltl::darray<r3d::buffer_t> buffers;

            // Prepare buffers
            for (size_t i=0, n=vObjects.size(); i<n; ++i)
            {
                ctl::Object3D *obj = vObjects.uget(i);
                if ((obj == NULL) || (!obj->visibility()->get()))
                    continue;

                obj->submit_foreground(&buffers);
            }

            // Draw data
            for (size_t i=0, n=buffers.size(); i<n; ++i)
                r3d->draw_primitives(buffers.uget(i));

            // Destroy buffers if destructors are set
            for (size_t i=0, n=buffers.size(); i<n; ++i)
            {
                r3d::buffer_t *buf = buffers.uget(i);
                if (buf->free != NULL)
                    buf->free(buf);
            }
        }

        void Area3D::draw_scene(ws::IR3DBackend *r3d)
        {
            // Check number of vertices in scene
            size_t nvertex      = vVertices.size();
            if (nvertex <= 0)
                return;

            vertex3d_t *vv      = vVertices.array();

            // Fill buffer
            r3d::buffer_t buf;
            r3d::init_buffer(&buf);

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

        status_t Area3D::render(ws::IR3DBackend *r3d)
        {
            // Configure camera and light
            setup_lighting(r3d);
            setup_camera(r3d);

            // Need to update vertex list for the scene?
            commit_view(r3d);

            // Perform draw
            draw_supplementary(r3d);
            draw_scene(r3d);

            return STATUS_OK;
        }

        status_t Area3D::add(ui::UIContext *ctx, ctl::Widget *child)
        {
            ctl::Object3D *obj = ctl::ctl_cast<ctl::Object3D>(child);
            if (obj == NULL)
                return STATUS_BAD_TYPE;

            if (!vObjects.add(obj))
                return STATUS_NO_MEM;
            obj->set_parent(this);

            return STATUS_OK;
        }

        void Area3D::notify(ui::IPort *port)
        {
            sync_pov_change(&sPov.x, pPosX, port);
            sync_pov_change(&sPov.y, pPosY, port);
            sync_pov_change(&sPov.z, pPosZ, port);
            sync_angle_change(&sAngles.fYaw, pYaw, port);
            sync_angle_change(&sAngles.fPitch, pPitch, port);
            sync_scale_change(&sScale.dx, pScaleX, port);
            sync_scale_change(&sScale.dy, pScaleY, port);
            sync_scale_change(&sScale.dz, pScaleZ, port);
        }

        status_t Area3D::slot_draw3d(tk::Widget *sender, void *ptr, void *data)
        {
            if ((ptr == NULL) || (data == NULL))
                return STATUS_BAD_ARGUMENTS;

            Area3D *_this     = static_cast<Area3D *>(ptr);
            return (_this != NULL) ? _this->render(static_cast<ws::IR3DBackend *>(data)) : STATUS_BAD_ARGUMENTS;
        }

        status_t Area3D::slot_mouse_down(tk::Widget *sender, void *ptr, void *data)
        {
            if ((ptr == NULL) || (data == NULL))
                return STATUS_BAD_ARGUMENTS;

            Area3D *_this     = static_cast<Area3D *>(ptr);
            ws::event_t *ev     = static_cast<ws::event_t *>(data);

            if (_this->nBMask == 0)
            {
                _this->nMouseX      = ev->nLeft;
                _this->nMouseY      = ev->nTop;
                _this->sOldAngles   = _this->sAngles;
                _this->sOldPov      = _this->sPov;
            }

            _this->nBMask |= (1 << ev->nCode);

            return STATUS_OK;
        }

        status_t Area3D::slot_mouse_up(tk::Widget *sender, void *ptr, void *data)
        {
            if ((ptr == NULL) || (data == NULL))
                return STATUS_BAD_ARGUMENTS;

            Area3D *_this     = static_cast<Area3D *>(ptr);
            ws::event_t *ev     = static_cast<ws::event_t *>(data);

            if (_this->nBMask == 0)
                return STATUS_OK;

            _this->nBMask &= ~(1 << ev->nCode);
            if (_this->nBMask == 0)
            {
                if (ev->nCode == ws::MCB_MIDDLE)
                    _this->rotate_camera(ev->nLeft - _this->nMouseX, ev->nTop - _this->nMouseY);
                else if (ev->nCode == ws::MCB_RIGHT)
                    _this->move_camera(ev->nLeft - _this->nMouseX, ev->nTop - _this->nMouseY, 0);
                else if (ev->nCode == ws::MCB_LEFT)
                    _this->move_camera(ev->nLeft - _this->nMouseX, 0, _this->nMouseY - ev->nTop);
            }

            return STATUS_OK;
        }

        status_t Area3D::slot_mouse_move(tk::Widget *sender, void *ptr, void *data)
        {
            if ((ptr == NULL) || (data == NULL))
                return STATUS_BAD_ARGUMENTS;

            Area3D *_this     = static_cast<Area3D *>(ptr);
            ws::event_t *ev     = static_cast<ws::event_t *>(data);

            if (_this->nBMask == (1 << ws::MCB_MIDDLE))
                _this->rotate_camera(ev->nLeft - _this->nMouseX, ev->nTop - _this->nMouseY);
            else if (_this->nBMask == (1 << ws::MCB_RIGHT))
                _this->move_camera(ev->nLeft - _this->nMouseX, ev->nTop - _this->nMouseY, 0);
            else if (_this->nBMask == (1 << ws::MCB_LEFT))
                _this->move_camera(ev->nLeft - _this->nMouseX, 0, _this->nMouseY - ev->nTop);

            return STATUS_OK;
        }

        void Area3D::query_draw()
        {
            if (wWidget != NULL)
                wWidget->query_draw();
        }

        void Area3D::query_view_change()
        {
            bViewChanged        = true;
            query_draw();
        }

    } /* namespace ctl */
} /* namespace lsp */


