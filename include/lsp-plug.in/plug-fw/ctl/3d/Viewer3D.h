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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_3D_VIEWER3D_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_3D_VIEWER3D_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>
#include <lsp-plug.in/dsp/dsp.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * ComboBox controller
         */
        class Viewer3D: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                static status_t     slot_draw3d(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_down(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_up(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_move(tk::Widget *sender, void *ptr, void *data);

            protected:
                typedef struct pov_angles_t
                {
                    float                   fYaw;
                    float                   fPitch;
                    float                   fRoll;
                } pov_angles_t;

            protected:
                lltl::darray<vertex3d_t>    vVertices;  // Vertices of the scene

                // Camera control
                ui::IPort          *pPosX;
                ui::IPort          *pPosY;
                ui::IPort          *pPosZ;
                ui::IPort          *pYaw;
                ui::IPort          *pPitch;
                ui::IPort          *pScaleX;
                ui::IPort          *pScaleY;
                ui::IPort          *pScaleZ;
                ui::IPort          *pOrientation;

                // Camera position
                bool                bViewChanged;   // View has changed
                float               fFov;           // Field of view
                dsp::point3d_t      sPov;           // Point-of-view for the camera
                dsp::point3d_t      sOldPov;        // Old point of view
                dsp::vector3d_t     sScale;         // Scene scaling
                dsp::vector3d_t     sTop;           // Top-of-view for the camera
                dsp::vector3d_t     sXTop;          // Updated top-of-view for the camera
                dsp::vector3d_t     sDir;           // Direction-of-view for the camera
                dsp::vector3d_t     sSide;          // Side-of-view for the camera
                pov_angles_t        sAngles;        // Yaw, pitch, roll
                pov_angles_t        sOldAngles;     // Old angles

                // Mouse events
                size_t              nBMask;         // Button mask
                ssize_t             nMouseX;        // Mouse X position
                ssize_t             nMouseY;        // Mouse Y position

            protected:
                static float        get_delta(ui::IPort *p, float dfl);
                static float        get_adelta(ui::IPort *p, float dfl);
                void                submit_pov_change(float *vold, float vnew, ui::IPort *port);
                void                submit_angle_change(float *vold, float vnew, ui::IPort *port);
                void                sync_pov_change(float *dst, ui::IPort *port, ui::IPort *psrc);
                void                sync_scale_change(float *dst, ui::IPort *port, ui::IPort *psrc);
                void                sync_angle_change(float *dst, ui::IPort *port, ui::IPort *psrc);

            protected:
                status_t            render(ws::IR3DBackend *r3d);
                void                commit_view(ws::IR3DBackend *r3d);

                void                update_frustum();
                void                rotate_camera(ssize_t dx, ssize_t dy);
                void                move_camera(ssize_t dx, ssize_t dy, ssize_t dz);

            public:
                explicit Viewer3D(ui::IWrapper *wrapper, tk::Area3D *widget);
                virtual ~Viewer3D();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        notify(ui::IPort *port);
                virtual void        end(ui::UIContext *ctx);
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_3D_VIEWER3D_H_ */
