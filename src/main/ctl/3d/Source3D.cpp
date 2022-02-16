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
#include <lsp-plug.in/dsp-units/3d/raytrace.h>

#include <private/ui/BuiltinStyle.h>


namespace lsp
{
    namespace ctl
    {
        static const r3d::mat4_t &to_mat4(const dsp::matrix3d_t *matrix)
        {
            return *reinterpret_cast<const r3d::mat4_t *>(matrix);
        }

        //---------------------------------------------------------------------
        static dspu::rt_audio_source_t decode_source_type(ssize_t type)
        {
            switch (type)
            {
                case 1:     return dspu::RT_AS_TETRA;
                case 2:     return dspu::RT_AS_OCTA;
                case 3:     return dspu::RT_AS_BOX;
                case 4:     return dspu::RT_AS_ICO;
                case 5:     return dspu::RT_AS_CYLINDER;
                case 6:     return dspu::RT_AS_CONE;
                case 7:     return dspu::RT_AS_OCTASPHERE;
                case 8:     return dspu::RT_AS_ICOSPHERE;
                case 9:     return dspu::RT_AS_FSPOT;
                case 10:    return dspu::RT_AS_CSPOT;
                case 11:    return dspu::RT_AS_SSPOT;
                default:    break;
            }
            return dspu::RT_AS_TRIANGLE;
        }

        //---------------------------------------------------------------------
        namespace style
        {
            LSP_TK_STYLE_IMPL_BEGIN(Source3D, Mesh3D)
                // Bind
                sType.bind("type", this);
                sSize.bind("size", this);
                sCurvature.bind("curvature", this);
                sHeight.bind("height", this);
                sAngle.bind("angle", this);
                sRayLength.bind("ray.length", this);
                sRayWidth.bind("ray.width", this);

                // Configure
                sType.set(0);
                sSize.set(1.0f);
                sCurvature.set(0.0f);
                sHeight.set(1.0f);
                sAngle.set(0.0f);
                sRayLength.set(0.25f);
                sRayWidth.set(1.0f);
            LSP_TK_STYLE_IMPL_END

            LSP_UI_BUILTIN_STYLE(Source3D, "Source3D", "root");
        }

        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Source3D)
            if (!name->equals_ascii("source3d"))
                return STATUS_NOT_FOUND;

            ctl::Source3D *wc   = new ctl::Source3D(context->wrapper());
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Source3D)

        //-----------------------------------------------------------------
        const ctl_class_t Source3D::metadata   = { "Source3D", &Mesh3D::metadata };

        Source3D::Source3D(ui::IWrapper *wrapper):
            Mesh3D(wrapper),
            sType(&sProperties),
            sSize(&sProperties),
            sCurvature(&sProperties),
            sHeight(&sProperties),
            sAngle(&sProperties),
            sRayLength(&sProperties),
            sRayWidth(&sProperties)
        {
            pClass      = &metadata;

            r3d::init_buffer(&sShape);
            r3d::init_buffer(&sRays);
        }

        Source3D::~Source3D()
        {
        }

        status_t Source3D::init()
        {
            status_t res = Mesh3D::init();
            if (res != STATUS_OK)
                return res;

            // Bind to style
            sType.bind("type", &sStyle);
            sSize.bind("size", &sStyle);
            sCurvature.bind("curvature", &sStyle);
            sHeight.bind("height", &sStyle);
            sAngle.bind("angle", &sStyle);
            sRayLength.bind("ray.length", &sStyle);
            sRayWidth.bind("ray.width", &sStyle);

            // Bind to controllers
            cType.init(pWrapper, &sType);
            cSize.init(pWrapper, &sSize);
            cCurvature.init(pWrapper, &sCurvature);
            cHeight.init(pWrapper, &sHeight);
            cAngle.init(pWrapper, &sAngle);
            cRayLength.init(pWrapper, &sRayLength);
            cRayWidth.init(pWrapper, &sRayWidth);

            return STATUS_OK;
        }

        void Source3D::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            cType.set("type", name, value);
            cSize.set("size", name, value);
            cCurvature.set("curvature", name, value);
            cHeight.set("height", name, value);
            cAngle.set("angle", name, value);
            cRayLength.set("ray.length", name, value);
            cRayLength.set("rlength", name, value);
            cRayWidth.set("ray.width", name, value);
            cRayWidth.set("rwidth", name, value);

            return Mesh3D::set(ctx, name, value);
        }

        void Source3D::property_changed(tk::Property *prop)
        {
            Mesh3D::property_changed(prop);

            if (sType.is(prop))
                query_data_change();
            if (sSize.is(prop))
                query_data_change();
            if (sCurvature.is(prop))
                query_data_change();
            if (sHeight.is(prop))
                query_data_change();
            if (sAngle.is(prop))
                query_data_change();
            if (sRayLength.is(prop))
                query_data_change();
            if (sRayWidth.is(prop))
                query_data_change();
        }

        status_t Source3D::compute_source_settings(dspu::rt_source_settings_t *settings)
        {
            dspu::room_source_config_t config;

            // Configure the source
            dsp::init_point_xyz(&config.sPos, sPosX.get(), sPosY.get(), sPosZ.get());
            config.fYaw         = sYaw.get();
            config.fPitch       = sPitch.get();
            config.fRoll        = sRoll.get();
            config.enType       = decode_source_type(sType.get());
            config.fSize        = sSize.get();
            config.fHeight      = sHeight.get();
            config.fAngle       = sAngle.get();
            config.fCurvature   = sCurvature.get();
            config.fAmplitude   = 1.0f;

            return dspu::rt_configure_source(settings, &config);
        }

        void Source3D::process_transform_change()
        {
            Mesh3D::process_transform_change();

            // Configure source settings
            dspu::rt_source_settings_t settings;
            if (compute_source_settings(&settings) != STATUS_OK)
                return;

            // Update mesh properties
            sShape.model    = to_mat4(&settings.pos);
            sRays.model     = to_mat4(&settings.pos);
        }

        void Source3D::process_data_change(lltl::parray<r3d::buffer_t> *dst)
        {
            Mesh3D::process_data_change(dst);

            // Clear state
            vVertices.clear();
            vNormals.clear();
            vLines.clear();

            // Configure source settings
            dspu::rt_source_settings_t settings;
            if (compute_source_settings(&settings) != STATUS_OK)
                return;

            // Generate source mesh depending on current configuration
            lltl::darray<dspu::rt::group_t> groups;
            status_t res    = dspu::rt_gen_source_mesh(groups, &settings);
            if (res != STATUS_OK)
                return;

            // Create mesh
            create_mesh(groups);

            // Initialize shape buffer
            r3d::init_buffer(&sShape);

            sShape.model            = to_mat4(&settings.pos);
            sShape.type             = r3d::PRIMITIVE_TRIANGLES;
            sShape.flags            = r3d::BUFFER_LIGHTING;
            sShape.width            = 0.0f;
            sShape.count            = groups.size();

            sShape.vertex.data      = array_cast<r3d::dot4_t>(vVertices.array());
            sShape.vertex.stride    = sizeof(dsp::point3d_t);
            sShape.normal.data      = array_cast<r3d::vec4_t>(vNormals.array());
            sShape.normal.stride    = sizeof(dsp::vector3d_t);
            sShape.color.dfl        = cColor.r3d_color();

            dst->add(&sShape);

            // Initialize ray buffer
            r3d::init_buffer(&sRays);

            sRays.model             = to_mat4(&settings.pos);
            sRays.type              = r3d::PRIMITIVE_LINES;
            sRays.flags             = 0;
            sRays.width             = sRayWidth.get();
            sRays.count             = groups.size() * 3;

            sRays.vertex.data       = array_cast<r3d::dot4_t>(vLines.array());
            sRays.vertex.stride     = sizeof(dsp::point3d_t);
            sRays.color.dfl         = cLineColor.r3d_color();

            dst->add(&sRays);
        }

        void Source3D::create_mesh(const lltl::darray<dspu::rt::group_t> &groups)
        {
            size_t  nt          = groups.size();
            dsp::point3d_t *dp  = vVertices.append_n(nt * 3); // 1 triangle x 3 vertices
            if (dp == NULL)
                return;
            dsp::vector3d_t *dn = vNormals.append_n(nt * 3); // 1 vertex = 1 normal
            if (dn == NULL)
                return;
            dsp::point3d_t *dl  = vLines.append_n(nt * 6); // 1 triangle x 3 lines x 2 vertices
            if (dp == NULL)
                return;

            const dspu::rt::group_t *grp = groups.array();
            float ray_length    = sRayLength.get();

            // Generate the final data
            dsp::vector3d_t vn[3];
            for (size_t i=0; i<nt; ++i, ++grp, dp += 3, dn += 3, dl += 6)
            {
                // Compute triangle vertices
                dp[0]   = grp->p[0];
                dp[1]   = grp->p[1];
                dp[2]   = grp->p[2];

                // Compute triangle normals
                dsp::calc_normal3d_pv(&dn[0], dp);
                dn[1]   = dn[0];
                dn[2]   = dn[0];

                // Compute rays
                dl[0]   = dp[0];
                dl[2]   = dp[1];
                dl[4]   = dp[2];

                dsp::init_vector_p2(&vn[0], &grp->s, &dp[0]);
                dsp::init_vector_p2(&vn[1], &grp->s, &dp[1]);
                dsp::init_vector_p2(&vn[2], &grp->s, &dp[2]);

                dsp::normalize_vector(&vn[0]);
                dsp::normalize_vector(&vn[1]);
                dsp::normalize_vector(&vn[2]);

                dsp::add_vector_pvk2(&dl[1], &dp[0], &vn[0], ray_length);
                dsp::add_vector_pvk2(&dl[3], &dp[1], &vn[1], ray_length);
                dsp::add_vector_pvk2(&dl[5], &dp[2], &vn[2], ray_length);
            }
        }

    } /* namespace ctl */
} /* namespace lsp */


