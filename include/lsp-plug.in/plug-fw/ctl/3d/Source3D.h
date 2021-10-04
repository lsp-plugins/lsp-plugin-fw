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
            LSP_TK_STYLE_DEF_BEGIN(Source3D, lsp::tk::Style)
                tk::prop::Color             sColor;         // X, Y, Z colors
                tk::prop::Color             sNormalColor;   // Normal color
                tk::prop::Integer           sMode;          // Mode
                tk::prop::Float             sPosX;          // X position
                tk::prop::Float             sPosY;          // Y position
                tk::prop::Float             sPosZ;          // Z position
                tk::prop::Float             sYaw;           // Yaw angle
                tk::prop::Float             sPitch;         // Pitch angle
                tk::prop::Float             sRoll;          // Roll angle
                tk::prop::Float             sSize;          // Size
                tk::prop::Float             sCurvature;     // Curvature
                tk::prop::Float             sHeight;        // Height
                tk::prop::Float             sAngle;         // Angle
            LSP_TK_STYLE_DEF_END
        }

        /**
         * ComboBox controller
         */
        class Source3D: public Object3D
        {
            public:
                static const ctl_class_t metadata;

            protected:
                bool                        bRebuildMesh;

                tk::prop::Color             sColor;         // X, Y, Z colors
                tk::prop::Color             sNormalColor;   // Normal color
                tk::prop::Integer           sMode;          // Mode
                tk::prop::Float             sPosX;          // X position
                tk::prop::Float             sPosY;          // Y position
                tk::prop::Float             sPosZ;          // Z position
                tk::prop::Float             sYaw;           // Yaw angle
                tk::prop::Float             sPitch;         // Pitch angle
                tk::prop::Float             sRoll;          // Roll angle
                tk::prop::Float             sSize;          // Size
                tk::prop::Float             sCurvature;     // Curvature
                tk::prop::Float             sHeight;        // Height
                tk::prop::Float             sAngle;         // Angle

                ctl::Color                  cColor;
                ctl::Color                  cNormalColor;
                ctl::Integer                cMode;
                ctl::Float                  cPosX;
                ctl::Float                  cPosY;
                ctl::Float                  cPosZ;
                ctl::Float                  cYaw;
                ctl::Float                  cPitch;
                ctl::Float                  cRoll;
                ctl::Float                  cSize;
                ctl::Float                  cCurvature;
                ctl::Float                  cHeight;
                ctl::Float                  cAngle;

                lltl::darray<vertex3d_t>    vTriangles;
                lltl::darray<point3d_t>     vLines;

            public:
                explicit Source3D(ui::IWrapper *wrapper);
                virtual ~Source3D();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);

                virtual void        property_changed(tk::Property *prop);

                virtual bool        submit_foreground(lltl::darray<r3d::buffer_t> *dst);
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_3D_SOURCE3D_H_ */
