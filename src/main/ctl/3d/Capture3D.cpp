/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 6 окт. 2021 г.
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
#include <lsp-plug.in/dsp-units/3d/raytrace.h>

#include <private/ui/BuiltinStyle.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        #define V3(x, y, z) { x, y, z, 1.0f }

        static const dsp::point3d_t capture_arrow_vertices[] =
        {
            V3(0.0f,    0.0f,   0.0f),
            V3(1.0f,    0.0f,   0.0f),
            V3(0.74f,   0.2f,   0.0f),
            V3(0.74f,  -0.2f,   0.0f),
            V3(0.74f,   0.0f,   0.2f),
            V3(0.74f,   0.0f,  -0.2f)
        };

        static const uint32_t capture_arrow_indices[] =
        {
            0, 1,
            1, 2,
            1, 3,
            1, 4,
            1, 5
        };

        #undef V3

        static dspu::rt_capture_config_t decode_config(float value)
        {
            switch (ssize_t(value))
            {
                case 1:     return dspu::RT_CC_XY;
                case 2:     return dspu::RT_CC_AB;
                case 3:     return dspu::RT_CC_ORTF;
                case 4:     return dspu::RT_CC_MS;
                default:    break;
            }
            return dspu::RT_CC_MONO;
        }

        //---------------------------------------------------------------------
        namespace style
        {
            LSP_TK_STYLE_IMPL_BEGIN(Capture3D, Mesh3D)
                // Bind
                sType.bind("type", this);
                sSize.bind("size", this);
                sAngle.bind("angle", this);
                sDistance.bind("angle", this);

                sArrowLength.bind("arrow.length", this);
                sArrowWidth.bind("arrow.width", this);

                // Configure
                sType.set(0);
                sSize.set(0.0f);
                sAngle.set(0.0f);
                sDistance.set(1.0f);
                sArrowLength.set(0.3f);
                sArrowWidth.set(2.0f);
            LSP_TK_STYLE_IMPL_END

            LSP_UI_BUILTIN_STYLE(Capture3D, "Capture3D", "root");
        }

        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Capture3D)
            if (!name->equals_ascii("capture3d"))
                return STATUS_NOT_FOUND;

            ctl::Capture3D *wc   = new ctl::Capture3D(context->wrapper());
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Capture3D)

        //-----------------------------------------------------------------
        const ctl_class_t Capture3D::metadata   = { "Capture3D", &Mesh3D::metadata };

        Capture3D::Capture3D(ui::IWrapper *wrapper):
            Mesh3D(wrapper),
            sType(&sProperties),
            sSize(&sProperties),
            sAngle(&sProperties),
            sDistance(&sProperties),
            sArrowLength(&sProperties),
            sArrowWidth(&sProperties)
        {
            pClass      = &metadata;

            r3d::init_buffer(&sShapes[0]);
            r3d::init_buffer(&sShapes[1]);
            r3d::init_buffer(&sLines[0]);
            r3d::init_buffer(&sLines[1]);
        }

        Capture3D::~Capture3D()
        {
        }

        status_t Capture3D::init()
        {
            status_t res = Mesh3D::init();
            if (res != STATUS_OK)
                return res;

            // Bind to style
            sType.bind("type", &sStyle);
            sSize.bind("size", &sStyle);
            sAngle.bind("angle", &sStyle);
            sDistance.bind("distance", &sStyle);
            sArrowLength.bind("arrow.length", &sStyle);
            sArrowWidth.bind("arrow.width", &sStyle);

            // Bind to controllers
            cType.init(pWrapper, &sType);
            cSize.init(pWrapper, &sSize);
            cAngle.init(pWrapper, &sAngle);
            cDistance.init(pWrapper, &sDistance);
            cArrowLength.init(pWrapper, &sArrowLength);
            cArrowWidth.init(pWrapper, &sArrowWidth);

            return STATUS_OK;
        }

        void Capture3D::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            cType.set("type", name, value);
            cSize.set("size", name, value);
            cAngle.set("angle", name, value);
            cDistance.set("distance", name, value);
            cDistance.set("dist", name, value);

            cArrowLength.set("arrow.length", name, value);
            cArrowLength.set("alength", name, value);
            cArrowWidth.set("arrow.width", name, value);
            cArrowWidth.set("awidth", name, value);

            return Mesh3D::set(ctx, name, value);
        }

        void Capture3D::property_changed(tk::Property *prop)
        {
            Mesh3D::property_changed(prop);

            if (sType.is(prop))
                query_data_change();
            if (sSize.is(prop))
                query_data_change();
            if (sAngle.is(prop))
                query_data_change();
            if (sDistance.is(prop))
                query_data_change();
            if (sArrowLength.is(prop))
                query_data_change();
            if (sArrowWidth.is(prop))
                query_data_change();
        }

        status_t Capture3D::compute_capture_settings(size_t *num_settings, dspu::rt_capture_settings_t *settings)
        {
            dspu::room_capture_config_t config;

            // Configure the capture
            dsp::init_point_xyz(&config.sPos, sPosX.get(), sPosY.get(), sPosZ.get());
            config.fYaw         = sYaw.get();
            config.fPitch       = sPitch.get();
            config.fRoll        = sRoll.get();
            config.fCapsule     = sSize.get() * 0.5f;
            config.sConfig      = decode_config(sType.get());
            config.fAngle       = sAngle.get();
            config.fDistance    = sDistance.get();
            config.enDirection  = dspu::RT_AC_OMNI;
            config.enSide       = dspu::RT_AC_OMNI;

            return dspu::rt_configure_capture(num_settings, settings, &config);
        }

        void Capture3D::process_transform_change()
        {
            Mesh3D::process_transform_change();

            // Configure source settings
            dspu::rt_capture_settings_t settings[2];
            size_t num_settings = 0;
            if (compute_capture_settings(&num_settings, settings) != STATUS_OK)
                return;

            // Update mesh properties
            for (size_t i=0; i<num_settings; ++i)
            {
                sShapes[i].model    = *reinterpret_cast<r3d::mat4_t *>(&settings[i].pos);
                sLines[i].model     = *reinterpret_cast<r3d::mat4_t *>(&settings[i].pos);
            }
        }

        void Capture3D::process_data_change(lltl::parray<r3d::buffer_t> *dst)
        {
            Mesh3D::process_data_change(dst);

            // Clear state
            vVertices.clear();
            vNormals.clear();
            vLines.clear();

            // Configure the source
            dspu::rt_capture_settings_t settings[2];
            size_t num_settings = 0;
            size_t v_offsets[2], n_offsets[2], l_offsets[2], counts[2];
            if (compute_capture_settings(&num_settings, settings) != STATUS_OK)
                return;

            r3d::init_buffer(&sShapes[0]);
            r3d::init_buffer(&sShapes[1]);
            r3d::init_buffer(&sLines[0]);
            r3d::init_buffer(&sLines[1]);

            if (num_settings <= 0)
                return;

            // Generate source mesh depending on current configuration
            lltl::darray<dsp::raw_triangle_t> mesh;
            for (size_t i=0; i<num_settings; ++i)
            {
                v_offsets[i]    = vVertices.size();
                n_offsets[i]    = vNormals.size();
                l_offsets[i]    = vLines.size();

                mesh.clear();
                status_t res    = dspu::rt_gen_capture_mesh(mesh, &settings[i]);
                if (res != STATUS_OK)
                    return;

                counts[i]       = mesh.size();
                create_mesh(mesh);
            }

            // Initialize shape buffer
            for (size_t i=0; i<num_settings; ++i)
            {
                r3d::buffer_t *buf          = &sShapes[i];
                const dsp::point3d_t *vv    = vVertices.array();
                const dsp::vector3d_t *vn   = vNormals.array();

                buf->model                  = *reinterpret_cast<r3d::mat4_t *>(&settings[i].pos);
                buf->type                   = r3d::PRIMITIVE_TRIANGLES;
                buf->flags                  = r3d::BUFFER_LIGHTING;
                buf->width                  = 0.0f;
                buf->count                  = counts[i];

                buf->vertex.data            = array_cast<r3d::dot4_t>(&vv[v_offsets[i]]);
                buf->vertex.stride          = sizeof(dsp::point3d_t);
                buf->normal.data            = array_cast<r3d::vec4_t>(&vn[n_offsets[i]]);
                buf->normal.stride          = sizeof(dsp::vector3d_t);
                buf->color.dfl              = cColor.r3d_color();

                dst->add(buf);
            }

            // Initialize arrow buffers
            for (size_t i=0; i<num_settings; ++i)
            {
                r3d::buffer_t *buf          = &sLines[i];
                const dsp::point3d_t *vl    = vLines.array();

                buf->model                  = *reinterpret_cast<r3d::mat4_t *>(&settings[i].pos);
                buf->type                   = r3d::PRIMITIVE_LINES;
                buf->flags                  = 0;
                buf->width                  = sArrowWidth.get();
                buf->count                  = sizeof(capture_arrow_indices) / (sizeof(capture_arrow_indices[0]) * 2);
                buf->vertex.data            = array_cast<r3d::dot4_t>(&vl[l_offsets[i]]);
                buf->vertex.stride          = sizeof(dsp::point3d_t);
                buf->vertex.index           = array_cast<uint32_t>(capture_arrow_indices);
                buf->color.dfl              = cLineColor.r3d_color();

                dst->add(buf);
            }
        }

        void Capture3D::create_mesh(const lltl::darray<dsp::raw_triangle_t> &mesh)
        {
            size_t nt           = mesh.size();
            size_t nl           = sizeof(capture_arrow_vertices) / sizeof(capture_arrow_vertices[0]);
            dsp::point3d_t *dp  = vVertices.append_n(nt * 3); // 1 triangle x 3 vertices
            if (dp == NULL)
                return;
            dsp::vector3d_t *dn = vNormals.append_n(nt * 3); // 1 vertex = 1 normal
            if (dn == NULL)
                return;
            dsp::point3d_t *dl  = vLines.append_n(nl);
            if (dl == NULL)
                return;

            const dsp::raw_triangle_t *t = mesh.array();

            // Generate the surface
            for (size_t i=0; i<nt; ++i, ++t, dp += 3, dn += 3)
            {
                // Compute triangle vertices
                dp[0]   = t->v[0];
                dp[1]   = t->v[1];
                dp[2]   = t->v[2];

                // Compute triangle normals
                dsp::calc_normal3d_pv(&dn[0], dp);
                dn[1]   = dn[0];
                dn[2]   = dn[0];
            }

            // Generate arrow
            float arrow_size        = sArrowLength.get();
            const dsp::point3d_t *sl= capture_arrow_vertices;
            for (size_t i=0; i<nl; ++i, ++dl, ++sl)
            {
                dl->x   = sl->x * arrow_size;
                dl->y   = sl->y * arrow_size;
                dl->z   = sl->z * arrow_size;
                dl->w   = sl->w;
            }
        }

    } /* namespace ctl */
} /* namespace lsp */





