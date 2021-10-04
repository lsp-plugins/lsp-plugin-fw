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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_3D_SOURCE3D_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_3D_SOURCE3D_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/ctl/3d/Mesh3D.h>

namespace lsp
{
    namespace ctl
    {
        namespace style
        {
            LSP_TK_STYLE_DEF_BEGIN(Source3D, Mesh3D)
                tk::prop::Integer           sType;          // Mode
                tk::prop::Float             sSize;          // Size
                tk::prop::Float             sCurvature;     // Curvature
                tk::prop::Float             sHeight;        // Height
                tk::prop::Float             sAngle;         // Angle
            LSP_TK_STYLE_DEF_END
        }

        /**
         * ComboBox controller
         */
        class Source3D: public Mesh3D
        {
            public:
                static const ctl_class_t metadata;

            protected:
                tk::prop::Integer           sType;          // Mode
                tk::prop::Float             sSize;          // Size
                tk::prop::Float             sCurvature;     // Curvature
                tk::prop::Float             sHeight;        // Height
                tk::prop::Float             sAngle;         // Angle

                ctl::Integer                cMode;
                ctl::Float                  cSize;
                ctl::Float                  cCurvature;
                ctl::Float                  cHeight;
                ctl::Float                  cAngle;

                lltl::darray<dsp::point3d_t>    vVertices;  // Triangle vertices
                lltl::darray<dsp::vector3d_t>   vNormals;   // Normals
                lltl::darray<dsp::point3d_t>    vLines;     // Lines

            protected:
                static void         free_buffer(r3d::buffer_t *buf);

            protected:
                virtual void        process_data_change();

            public:
                explicit Source3D(ui::IWrapper *wrapper);
                virtual ~Source3D();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);

                virtual void        property_changed(tk::Property *prop);
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_3D_SOURCE3D_H_ */
