/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 окт. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_3D_MODEL3D_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_3D_MODEL3D_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/ctl/3d/Object3D.h>
#include <lsp-plug.in/dsp-units/3d/Scene3D.h>

namespace lsp
{
    namespace ctl
    {
        namespace style
        {
            LSP_TK_STYLE_DEF_BEGIN(Model3D, Object3D)
                tk::prop::Float             sPosX;          // X position
                tk::prop::Float             sPosY;          // Y position
                tk::prop::Float             sPosZ;          // Z position
                tk::prop::Float             sYaw;           // Yaw angle (degrees)
                tk::prop::Float             sPitch;         // Pitch angle (degrees)
                tk::prop::Float             sRoll;          // Roll angle (degrees)
                tk::prop::Float             sScaleX;        // Scaling by X axis
                tk::prop::Float             sScaleY;        // Scaling by Y axis
                tk::prop::Float             sScaleZ;        // Scaling by Z axis
                tk::prop::Integer           sOrientation;   // Orientation
                tk::prop::Float             sTransparency;  // Transparency
            LSP_TK_STYLE_DEF_END
        }

        /**
         * ComboBox controller
         */
        class Model3D: public Object3D, public ui::IKVTListener
        {
            public:
                static const ctl_class_t metadata;

            private:
                ui::IPort          *pFile;          // Location of the model file
                ui::IPort          *pStatus;        // Status of loading of the model file

                dsp::matrix3d_t     matOrientation; // Orientation matrix

                dspu::Scene3D       sScene;
                LSPString           sKvtRoot;

                tk::prop::Integer   sOrientation;   // Orientation
                tk::prop::Float     sTransparency;  // Transparency
                tk::prop::Float     sPosX;          // X position
                tk::prop::Float     sPosY;          // Y position
                tk::prop::Float     sPosZ;          // Z position
                tk::prop::Float     sYaw;           // Yaw angle (degrees)
                tk::prop::Float     sPitch;         // Pitch angle (degrees)
                tk::prop::Float     sRoll;          // Roll angle (degrees)
                tk::prop::Float     sScaleX;        // Scaling by X axis
                tk::prop::Float     sScaleY;        // Scaling by Y axis
                tk::prop::Float     sScaleZ;        // Scaling by Z axis

                ctl::Integer        cOrientation;   // Orientation controller
                ctl::Float          cTransparency;  // Transparency
                ctl::Float          cPosX;
                ctl::Float          cPosY;
                ctl::Float          cPosZ;
                ctl::Float          cYaw;
                ctl::Float          cPitch;
                ctl::Float          cRoll;
                ctl::Float          cScaleX;
                ctl::Float          cScaleY;
                ctl::Float          cScaleZ;

            protected:
                void                update_model_file();
                void                read_object_properties(
                                        core::KVTStorage *kvt,
                                        const char *base,
                                        dsp::matrix3d_t *m,
                                        float *hue,
                                        bool *visible
                                    );

            public:
                explicit Model3D(ui::IWrapper *wrapper);
                virtual ~Model3D();

                virtual status_t    init();
                virtual void        destroy();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        notify(ui::IPort *port);

                virtual void        property_changed(tk::Property *prop);
                virtual bool        submit_background(dspu::bsp::context_t *dst);

                virtual bool        changed(core::KVTStorage *kvt, const char *id, const core::kvt_param_t *value);
                virtual bool        match(const char *id);

            public:
                void                query_mesh_change();
        };

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_3D_MODEL3D_H_ */
