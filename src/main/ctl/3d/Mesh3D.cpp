/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 окт. 2021 г.
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
            LSP_TK_STYLE_IMPL_BEGIN(Mesh3D, Object3D)
                // Bind
                sColor.bind("color", this);
                sLineColor.bind("line.color", this);
                sPointColor.bind("point.color", this);
                sPosX.bind("position.x", this);
                sPosY.bind("position.y", this);
                sPosZ.bind("position.z", this);
                sYaw.bind("rotation.yaw", this);
                sPitch.bind("rotation.pitch", this);
                sRoll.bind("rotation.roll", this);
                sScaleX.bind("scale.x", this);
                sScaleY.bind("scale.y", this);
                sScaleZ.bind("scale.z", this);

                // Configure
                sColor.set("#cccccc");
                sLineColor.set("#cccccc");
                sPointColor.set("#cccccc");
                sPosX.set(0.0f);
                sPosY.set(0.0f);
                sPosZ.set(0.0f);
                sYaw.set(0.0f);
                sPitch.set(0.0f);
                sRoll.set(0.0f);
                sScaleX.set(1.0f);
                sScaleY.set(1.0f);
                sScaleZ.set(1.0f);
            LSP_TK_STYLE_IMPL_END

            LSP_UI_BUILTIN_STYLE(Mesh3D, "Mesh3D", "root");
        }

        //-----------------------------------------------------------------
        const ctl_class_t Mesh3D::metadata   = { "Mesh3D", &Object3D::metadata };

        Mesh3D::Mesh3D(ui::IWrapper *wrapper):
            Object3D(wrapper),
            sColor(&sProperties),
            sLineColor(&sProperties),
            sPointColor(&sProperties),
            sPosX(&sProperties),
            sPosY(&sProperties),
            sPosZ(&sProperties),
            sYaw(&sProperties),
            sPitch(&sProperties),
            sRoll(&sProperties),
            sScaleX(&sProperties),
            sScaleY(&sProperties),
            sScaleZ(&sProperties)
        {
            pClass          = &metadata;

            dsp::init_matrix3d_identity(&sMatrix);

            bDataChanged    = false;
            bPovChanged     = false;
        }

        Mesh3D::~Mesh3D()
        {
            pParent         = NULL;
            clear();
        }

        status_t Mesh3D::init()
        {
            status_t res = Object3D::init();
            if (res != STATUS_OK)
                return res;

            // Initialize styles
            sColor.bind("color", &sStyle);
            sLineColor.bind("line.color", &sStyle);
            sPointColor.bind("point.color", &sStyle);
            sPosX.bind("position.x", &sStyle);
            sPosY.bind("position.y", &sStyle);
            sPosZ.bind("position.z", &sStyle);
            sYaw.bind("rotation.yaw", &sStyle);
            sPitch.bind("rotation.pitch", &sStyle);
            sRoll.bind("rotation.roll", &sStyle);
            sScaleX.bind("scale.x", &sStyle);
            sScaleY.bind("scale.y", &sStyle);
            sScaleZ.bind("scale.z", &sStyle);

            // Initialize controllers
            cColor.init(pWrapper, &sColor);
            cLineColor.init(pWrapper, &sLineColor);
            cPointColor.init(pWrapper, &sPointColor);
            cPosX.init(pWrapper, &sPosX);
            cPosY.init(pWrapper, &sPosY);
            cPosZ.init(pWrapper, &sPosZ);
            cYaw.init(pWrapper, &sYaw);
            cPitch.init(pWrapper, &sPitch);
            cRoll.init(pWrapper, &sRoll);
            cScaleX.init(pWrapper, &sScaleX);
            cScaleY.init(pWrapper, &sScaleY);
            cScaleZ.init(pWrapper, &sScaleZ);

            return STATUS_OK;
        }

        void Mesh3D::destroy()
        {
            clear();
        }

        void Mesh3D::clear()
        {
            for (size_t i=0, n=vBuffers.size(); i<n; ++i)
            {
                r3d::buffer_t *buf = vBuffers.uget(i);
                if (buf->free != NULL)
                    buf->free(buf);
            }
            vBuffers.flush();
        }

        status_t Mesh3D::append(r3d::buffer_t *buffer)
        {
            return (vBuffers.add(buffer)) ? STATUS_OK : STATUS_NO_MEM;
        }

        void Mesh3D::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            // Set-up settings
            cColor.set("color", name, value);
            cLineColor.set("line.color", name, value);
            cLineColor.set("lcolor", name, value);
            cPointColor.set("point.color", name, value);
            cPointColor.set("pcolor", name, value);

            cPosX.set("x", name, value);
            cPosY.set("y", name, value);
            cPosZ.set("z", name, value);

            cYaw.set("yaw", name, value);
            cPitch.set("pitch", name, value);
            cRoll.set("roll", name, value);

            cScaleX.set("sx", name, value);
            cScaleX.set("scale.x", name, value);
            cScaleY.set("sy", name, value);
            cScaleY.set("scale.y", name, value);
            cScaleZ.set("sz", name, value);
            cScaleZ.set("scale.z", name, value);

            return Widget::set(ctx, name, value);
        }

        void Mesh3D::property_changed(tk::Property *prop)
        {
            Object3D::property_changed(prop);

            if (sColor.is(prop))
                query_draw_parent();
            if (sLineColor.is(prop))
                query_draw_parent();
            if (sPointColor.is(prop))
                query_draw_parent();
            if (sPosX.is(prop))
                query_draw();
            if (sPosY.is(prop))
                query_draw();
            if (sPosZ.is(prop))
                query_draw();
            if (sYaw.is(prop))
                query_draw();
            if (sPitch.is(prop))
                query_draw();
            if (sRoll.is(prop))
                query_draw();
            if (sScaleX.is(prop))
                query_draw();
            if (sScaleY.is(prop))
                query_draw();
            if (sScaleZ.is(prop))
                query_draw();
        }

        void Mesh3D::query_draw()
        {
            // Mark mesh for rebuild
            bPovChanged     = true;
            Object3D::query_draw();
        }

        void Mesh3D::query_data_change()
        {
            bDataChanged    = true;
            query_draw();
        }

        bool Mesh3D::submit_foreground(lltl::darray<r3d::buffer_t> *dst)
        {
            if (bDataChanged)
            {
                // TODO: recompute matrix
                vBuffers.clear();

                process_data_change(&vBuffers);
                bDataChanged = false;
            }

            size_t count = vBuffers.size();
            if (count <= 0)
                return false;

            if (bPovChanged)
            {
                process_position_change();
                bPovChanged = false;
            }

            // Add buffers
            r3d::buffer_t *v = dst->add_n(count);
            if (v == NULL)
                return false;

            // Copy all buffer data
            for (size_t i=0; i<count; ++i)
            {
                r3d::buffer_t *src = vBuffers.uget(i);

                // Init buffer to avoid mismatch of structure size
                init_buffer(&v[i]);

                // Copy buffer excluding user data and free
                v[i]        = *src;
                v[i].user   = NULL;
                v[i].free   = NULL;
            }

            return true;
        }

        void Mesh3D::reorder_triangles(const dsp::point3d_t *pov, r3d::buffer_t *buf)
        {
            if (pov == NULL)
                return;
            if ((buf->vertex.data == NULL) || (buf->vertex.index != NULL))
                return;
            if (buf->normal.index != NULL)
                return;

            uint8_t *bv     = array_cast<uint8_t>(buf->vertex.data);
            uint8_t *bn     = array_cast<uint8_t>(buf->normal.data);
            const dsp::matrix3d_t *m = reinterpret_cast<dsp::matrix3d_t *>(&buf->model);

            size_t bstride  = (buf->vertex.stride > 0) ? buf->vertex.stride : sizeof(r3d::dot4_t);
            size_t nstride  = (bn == NULL) ? 0 :
                              (buf->normal.stride > 0) ? buf->normal.stride : sizeof(r3d::vec4_t);

            dsp::point3d_t *p0, *p1, *p2, t[3];
            dsp::vector3d_t *n0, *n1, *n2, pl;

            for (size_t i=0; i<buf->count; ++i)
            {
                // Get pointers to vertices and normals;
                p0  = array_cast<dsp::point3d_t>(bv);
                bv += bstride;
                p1  = array_cast<dsp::point3d_t>(bv);
                bv += bstride;
                p2  = array_cast<dsp::point3d_t>(bv);
                bv += bstride;

                n0  = array_cast<dsp::vector3d_t>(bn);
                bn += nstride;
                n1  = array_cast<dsp::vector3d_t>(bn);
                bn += nstride;
                n2  = array_cast<dsp::vector3d_t>(bn);
                bn += nstride;

                // Apply transformation to points
                dsp::apply_matrix3d_mp2(&t[0], p0, m);
                dsp::apply_matrix3d_mp2(&t[1], p1, m);
                dsp::apply_matrix3d_mp2(&t[2], p2, m);

                // Compute plane equation and location of POV to the plane
                dsp::calc_plane_pv(&pl, t);
                float d         = pov->x*pl.dx + pov->y*pl.dy + pov->z*pl.dz + pov->w * pl.dw;

                // Need to flip order of vertices and normals?
                if (d < -DSP_3D_TOLERANCE)
                {
                    lsp::swap(*p1, *p2);
                    if (bn != NULL)
                    {
                        lsp::swap(*n1, *n2);
                        dsp::flip_vector_v1(n0);
                        dsp::flip_vector_v1(n1);
                        dsp::flip_vector_v1(n2);
                    }
                }
            }
        }

        void Mesh3D::process_position_change()
        {
            const dsp::point3d_t *pov  = (pParent != NULL) ? pParent->point_of_view() : NULL;

            // Update mesh properties
            for (size_t i=0, n=vBuffers.size(); i<n; ++i)
            {
                r3d::buffer_t *buf = vBuffers.uget(i);

                buf->model  = *reinterpret_cast<r3d::mat4_t *>(&sMatrix);

                switch (buf->type)
                {
                    case r3d::PRIMITIVE_TRIANGLES:
                        reorder_triangles(pov, buf);
                        buf->color.dfl = cColor.r3d_color();
                        break;
                    case r3d::PRIMITIVE_WIREFRAME_TRIANGLES:
                        buf->color.dfl = cColor.r3d_color();
                        break;
                    case r3d::PRIMITIVE_LINES:
                        buf->color.dfl = cLineColor.r3d_color();
                        break;
                    case r3d::PRIMITIVE_POINTS:
                        buf->color.dfl = cPointColor.r3d_color();
                        break;
                    default:
                        break;
                }
            }
        }

        void Mesh3D::process_data_change(lltl::parray<r3d::buffer_t> *dst)
        {
        }

    } // namespace ctl
} // namespace lsp



