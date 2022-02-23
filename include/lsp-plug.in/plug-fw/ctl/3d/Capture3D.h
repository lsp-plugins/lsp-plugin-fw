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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_3D_CAPTURE3D_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_3D_CAPTURE3D_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/ctl/3d/Mesh3D.h>
#include <lsp-plug.in/dsp-units/3d/rt/types.h>

namespace lsp
{
    namespace ctl
    {
        namespace style
        {
            LSP_TK_STYLE_DEF_BEGIN(Capture3D, Mesh3D)
                tk::prop::Integer           sType;          // Mode
                tk::prop::Float             sSize;          // Size
                tk::prop::Float             sAngle;         // Angle
                tk::prop::Float             sDistance;      // Distance
                tk::prop::Float             sArrowLength;   // Arrow size
                tk::prop::Float             sArrowWidth;    // Arrow width
            LSP_TK_STYLE_DEF_END
        }

        /**
         * ComboBox controller
         */
        class Capture3D: public Mesh3D
        {
            public:
                static const ctl_class_t metadata;

            protected:
                tk::prop::Integer           sType;          // Mode
                tk::prop::Float             sSize;          // Size
                tk::prop::Float             sAngle;         // Angle
                tk::prop::Float             sDistance;      // Distance
                tk::prop::Float             sArrowLength;   // Arrow size
                tk::prop::Float             sArrowWidth;    // Arrow width

                ctl::Integer                cType;
                ctl::Float                  cSize;
                ctl::Float                  cAngle;
                ctl::Float                  cDistance;
                ctl::Float                  cArrowLength;
                ctl::Float                  cArrowWidth;

                lltl::darray<dsp::point3d_t>    vVertices;  // Triangle vertices
                lltl::darray<dsp::vector3d_t>   vNormals;   // Normals
                lltl::darray<dsp::point3d_t>    vLines;     // Lines
                r3d::buffer_t               sShapes[2];
                r3d::buffer_t               sLines[2];

            protected:
                static void         free_buffer(r3d::buffer_t *buf);
                void                create_mesh(const lltl::darray<dsp::raw_triangle_t> &mesh);
                status_t            compute_capture_settings(size_t *num_settings, dspu::rt_capture_settings_t *settings);

            protected:
                virtual void        process_data_change(lltl::parray<r3d::buffer_t> *dst);
                virtual void        process_transform_change();

            public:
                explicit Capture3D(ui::IWrapper *wrapper);
                virtual ~Capture3D();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);

                virtual void        property_changed(tk::Property *prop);
        };

    } /* namespace ctl */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_3D_CAPTURE3D_H_ */
