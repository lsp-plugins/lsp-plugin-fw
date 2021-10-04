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

                // Configure
                sType.set(0);
                sSize.set(1.0f);
                sCurvature.set(0.0f);
                sHeight.set(1.0f);
                sAngle.set(0.0f);
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
            sAngle(&sProperties)
        {
            pClass      = &metadata;
        }

        Source3D::~Source3D()
        {
        }

        void Source3D::free_buffer(r3d::buffer_t *buf)
        {
            if (buf == NULL)
                return;

            if (buf->vertex.data != NULL)
            {
                free(buf->vertex.data);
                buf->vertex.data = NULL;
            }
            if (buf->normal.data != NULL)
            {
                free(buf->normal.data);
                buf->normal.data = NULL;
            }
            free(buf);
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

            // Bind to controllers
            cMode.init(pWrapper, &sType);
            cSize.init(pWrapper, &sSize);
            cCurvature.init(pWrapper, &sCurvature);
            cHeight.init(pWrapper, &sHeight);
            cAngle.init(pWrapper, &sAngle);

            return STATUS_OK;
        }

        void Source3D::set(ui::UIContext *ctx, const char *name, const char *value)
        {
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
        }

        void Source3D::process_data_change()
        {
            dspu::room_source_config_t config;
            dspu::rt_source_settings_t settings;

            // Clear state
            clear();
            vVertices.clear();
            vNormals.clear();
            vLines.clear();

            // Get point of view
            const dsp::point3d_t *pov = (pParent != NULL) ? pParent->point_of_view() : NULL;
            if (pov == NULL)
                return;

            // Configure the source
            config.sPos         = *pov;
            config.fYaw         = sYaw.get();
            config.fPitch       = sPitch.get();
            config.fRoll        = sRoll.get();
            config.enType       = decode_source_type(sType.get());
            config.fSize        = sSize.get();
            config.fHeight      = sHeight.get();
            config.fAngle       = sAngle.get();
            config.fCurvature   = sCurvature.get();
            config.fAmplitude   = 1.0f;

            if (dspu::rt_configure_source(&settings, &config) != STATUS_OK)
                return;

            // Generate source mesh depending on current configuration
            lltl::darray<dspu::rt::group_t> groups;
            status_t res    = dspu::rt_gen_source_mesh(groups, &settings);
            if (res != STATUS_OK)
                return;

//            size_t  nt          = groups.size();


        }

    } /* namespace ctl */
} /* namespace lsp */


