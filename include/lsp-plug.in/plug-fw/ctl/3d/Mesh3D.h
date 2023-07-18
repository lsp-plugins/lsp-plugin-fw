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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_3D_MESH3D_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_3D_MESH3D_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/ctl/3d/Object3D.h>

namespace lsp
{
    namespace ctl
    {
        namespace style
        {
            LSP_TK_STYLE_DEF_BEGIN(Mesh3D, Object3D)
                tk::prop::Color             sColor;         // Default color for triangles
                tk::prop::Color             sLineColor;     // Default color for lines
                tk::prop::Color             sPointColor;    // Default color for points
                tk::prop::Float             sPosX;          // X position
                tk::prop::Float             sPosY;          // Y position
                tk::prop::Float             sPosZ;          // Z position
                tk::prop::Float             sYaw;           // Yaw angle (degrees)
                tk::prop::Float             sPitch;         // Pitch angle (degrees)
                tk::prop::Float             sRoll;          // Roll angle (degrees)
                tk::prop::Float             sScaleX;        // Scaling by X axis
                tk::prop::Float             sScaleY;        // Scaling by Y axis
                tk::prop::Float             sScaleZ;        // Scaling by Z axis
            LSP_TK_STYLE_DEF_END
        }

        /**
         * ComboBox controller
         */
        class Mesh3D: public Object3D
        {
            public:
                static const ctl_class_t metadata;

            protected:
                enum flags_t
                {
                    F_DATA_CHANGED          = 1 << 0,
                    F_VIEW_CHANGED          = 1 << 1,
                    F_TRANSFORM_CHANGED     = 1 << 2,
                    F_COLOR_CHANGED         = 1 << 3,
                };

            protected:
                size_t                      nFlags;         // Change flags

                tk::prop::Color             sColor;         // Default color for triangles
                tk::prop::Color             sLineColor;     // Default color for lines
                tk::prop::Color             sPointColor;    // Default color for points
                tk::prop::Float             sPosX;          // X position
                tk::prop::Float             sPosY;          // Y position
                tk::prop::Float             sPosZ;          // Z position
                tk::prop::Float             sYaw;           // Yaw angle (degrees)
                tk::prop::Float             sPitch;         // Pitch angle (degrees)
                tk::prop::Float             sRoll;          // Roll angle (degrees)
                tk::prop::Float             sScaleX;        // Scaling by X axis
                tk::prop::Float             sScaleY;        // Scaling by Y axis
                tk::prop::Float             sScaleZ;        // Scaling by Z axis

                ctl::Color                  cColor;
                ctl::Color                  cLineColor;
                ctl::Color                  cPointColor;
                ctl::Float                  cPosX;
                ctl::Float                  cPosY;
                ctl::Float                  cPosZ;
                ctl::Float                  cYaw;
                ctl::Float                  cPitch;
                ctl::Float                  cRoll;
                ctl::Float                  cScaleX;
                ctl::Float                  cScaleY;
                ctl::Float                  cScaleZ;

                lltl::parray<r3d::buffer_t> vBuffers;

            protected:
                virtual void        process_view_change(const dsp::point3d_t *pov);
                virtual void        process_color_change();
                virtual void        process_transform_change();
                virtual void        process_data_change(lltl::parray<r3d::buffer_t> *dst);
                virtual void        reorder_triangles(const dsp::point3d_t *pov, r3d::buffer_t *buf);

            public:
                explicit Mesh3D(ui::IWrapper *wrapper);
                virtual ~Mesh3D() override;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        property_changed(tk::Property *prop) override;
                virtual bool        submit_foreground(lltl::darray<r3d::buffer_t> *dst) override;
                virtual void        query_draw() override;

            public:
                virtual void        query_data_change();
                virtual void        query_transform_change();
                virtual void        query_color_change();
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_3D_MESH3D_H_ */
