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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <private/ui/BuiltinStyle.h>

namespace lsp
{
    namespace ctl
    {
        //-----------------------------------------------------------------
        static dsp::axis_orientation_t get_orientation(ssize_t value)
        {
            #define C(X) case dsp::X: return dsp::X;

            switch (value)
            {
                C(AO3D_POS_X_FWD_POS_Y_UP)
                C(AO3D_POS_X_FWD_POS_Z_UP)
                C(AO3D_POS_X_FWD_NEG_Y_UP)
                C(AO3D_POS_X_FWD_NEG_Z_UP)
                C(AO3D_NEG_X_FWD_POS_Y_UP)
                C(AO3D_NEG_X_FWD_POS_Z_UP)
                C(AO3D_NEG_X_FWD_NEG_Y_UP)
                C(AO3D_NEG_X_FWD_NEG_Z_UP)

                C(AO3D_POS_Y_FWD_POS_X_UP)
                C(AO3D_POS_Y_FWD_POS_Z_UP)
                C(AO3D_POS_Y_FWD_NEG_X_UP)
                C(AO3D_POS_Y_FWD_NEG_Z_UP)
                C(AO3D_NEG_Y_FWD_POS_X_UP)
                C(AO3D_NEG_Y_FWD_POS_Z_UP)
                C(AO3D_NEG_Y_FWD_NEG_X_UP)
                C(AO3D_NEG_Y_FWD_NEG_Z_UP)

                C(AO3D_POS_Z_FWD_POS_X_UP)
                C(AO3D_POS_Z_FWD_POS_Y_UP)
                C(AO3D_POS_Z_FWD_NEG_X_UP)
                C(AO3D_POS_Z_FWD_NEG_Y_UP)
                C(AO3D_NEG_Z_FWD_POS_X_UP)
                C(AO3D_NEG_Z_FWD_POS_Y_UP)
                C(AO3D_NEG_Z_FWD_NEG_X_UP)
                C(AO3D_NEG_Z_FWD_NEG_Y_UP)

                default: break;
            }

            return dsp::AO3D_POS_X_FWD_POS_Z_UP;

            #undef C
        }

        template <class T>
            static bool kvt_fetch(core::KVTStorage *kvt, const char *base, const char *branch, T *value, T dfl)
            {
                char name[0x100]; // Should be enough;
                size_t len = ::strlen(base) + ::strlen(branch) + 2;
                if (len >= sizeof(name))
                    return false;

                char *tail = ::stpcpy(name, base);
                *(tail++)  = '/';
                stpcpy(tail, branch);

                return kvt->get_dfl(name, value, dfl);
            }

        //-----------------------------------------------------------------
        namespace style
        {
            LSP_TK_STYLE_IMPL_BEGIN(Model3D, Object3D)
                // Bind
                sOrientation.bind("orientation", this);
                sTransparency.bind("transparency", this);
                sPosX.bind("position.x", this);
                sPosY.bind("position.y", this);
                sPosZ.bind("position.z", this);
                sYaw.bind("rotation.yaw", this);
                sPitch.bind("rotation.pitch", this);
                sRoll.bind("rotation.roll", this);
                sScaleX.bind("scale.x", this);
                sScaleY.bind("scale.y", this);
                sScaleZ.bind("scale.z", this);
                sColor.bind("color", this);

                // Configure
                sOrientation.set(0);
                sTransparency.set(0.75f);
                sPosX.set(0.0f);
                sPosY.set(0.0f);
                sPosZ.set(0.0f);
                sYaw.set(0.0f);
                sPitch.set(0.0f);
                sRoll.set(0.0f);
                sScaleX.set(1.0f);
                sScaleY.set(1.0f);
                sScaleZ.set(1.0f);
                sColor.set("#ff0000");
            LSP_TK_STYLE_IMPL_END

            LSP_UI_BUILTIN_STYLE(Model3D, "Model3D", "root");
        }

        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Model3D)
            if (!name->equals_ascii("model3d"))
                return STATUS_NOT_FOUND;

            ctl::Model3D *wc    = new ctl::Model3D(context->wrapper());
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Model3D)

        //-----------------------------------------------------------------
        const ctl_class_t Model3D::metadata     = { "Model3D", &Object3D::metadata };

        Model3D::Model3D(ui::IWrapper *wrapper):
            Object3D(wrapper),
            sOrientation(&sProperties),
            sTransparency(&sProperties),
            sPosX(&sProperties),
            sPosY(&sProperties),
            sPosZ(&sProperties),
            sYaw(&sProperties),
            sPitch(&sProperties),
            sRoll(&sProperties),
            sScaleX(&sProperties),
            sScaleY(&sProperties),
            sScaleZ(&sProperties),
            sColor(&sProperties)
        {
            pFile           = NULL;

            dsp::init_matrix3d_identity(&matOrientation);

            pClass          = &metadata;
        }

        Model3D::~Model3D()
        {
        }

        status_t Model3D::init()
        {
            status_t res = Object3D::init();
            if (res != STATUS_OK)
                return res;

            // Properties
            sOrientation.bind("orientation", &sStyle);
            sTransparency.bind("transparency", &sStyle);
            sPosX.bind("position.x", &sStyle);
            sPosY.bind("position.y", &sStyle);
            sPosZ.bind("position.z", &sStyle);
            sYaw.bind("rotation.yaw", &sStyle);
            sPitch.bind("rotation.pitch", &sStyle);
            sRoll.bind("rotation.roll", &sStyle);
            sScaleX.bind("scale.x", &sStyle);
            sScaleY.bind("scale.y", &sStyle);
            sScaleZ.bind("scale.z", &sStyle);
            sColor.bind("color", &sStyle);

            // Controllers
            cOrientation.init(pWrapper, &sOrientation);
            cTransparency.init(pWrapper, &sTransparency);
            cPosX.init(pWrapper, &sPosX);
            cPosY.init(pWrapper, &sPosY);
            cPosZ.init(pWrapper, &sPosZ);
            cYaw.init(pWrapper, &sYaw);
            cPitch.init(pWrapper, &sPitch);
            cRoll.init(pWrapper, &sRoll);
            cScaleX.init(pWrapper, &sScaleX);
            cScaleY.init(pWrapper, &sScaleY);
            cScaleZ.init(pWrapper, &sScaleZ);
            cColor.init(pWrapper, &sColor);
            cTempColor.init(pWrapper, &sTempColor);

            sStatus.init(pWrapper, this);

            return STATUS_OK;
        }

        void Model3D::destroy()
        {
            sScene.destroy();
        }

        void Model3D::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            bind_port(&pFile, "id", name, value);

            cOrientation.set("orientation", name, value);
            cOrientation.set("o", name, value);
            cTransparency.set("transparency", name, value);
            cTransparency.set("transp", name, value);

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

            if ((!strcmp("kvt.root", name) || (!strcmp("kvt_root", name))))
            {
                sKvtRoot.set_utf8(value);
                if (!sKvtRoot.ends_with('/'))
                    sKvtRoot.append('/');
            }

            set_expr(&sStatus, "status", name, value);

            return Object3D::set(ctx, name, value);
        }

        void Model3D::notify(ui::IPort *port, size_t flags)
        {
            Object3D::notify(port, flags);
            if (port == NULL)
                return;

            if ((pFile == port) || (sStatus.depends(port)))
                update_model_file();
        }

        void Model3D::end(ui::UIContext *ctx)
        {
            if (!sKvtRoot.is_empty())
                pWrapper->kvt_subscribe(this);
        }

        void Model3D::update_model_file()
        {
            // Clear scene state
            sScene.clear();

            // Mark that view has changed and query for redraw
            query_mesh_change();

            // Load scene only if status is not defined or valid
            ssize_t status = (sStatus.valid()) ? sStatus.evaluate_int(STATUS_UNKNOWN_ERR) : STATUS_UNKNOWN_ERR;
            if (status != STATUS_OK)
                return;

            const char *spath   = pFile->buffer<char>();
            if (spath == NULL)
                return;

            // Load file from resources
            lsp_trace("Loading scene from %s", spath);
            io::IInStream *is = pWrapper->resources()->read_stream(spath);
            if (is == NULL)
                return;

            // Try to load
            status_t res = sScene.load(is);
            if (res != STATUS_OK)
                sScene.clear();
            is->close();
            delete is;
        }

        void Model3D::property_changed(tk::Property *prop)
        {
            Object3D::property_changed(prop);

            if (sOrientation.is(prop))
            {
                dsp::init_matrix3d_orientation(&matOrientation, get_orientation(sOrientation.get()));
                query_mesh_change();
            }
            if (sTransparency.is(prop))
                query_mesh_change();
            if (sPosX.is(prop))
                query_mesh_change();
            if (sPosY.is(prop))
                query_mesh_change();
            if (sPosZ.is(prop))
                query_mesh_change();
            if (sYaw.is(prop))
                query_mesh_change();
            if (sPitch.is(prop))
                query_mesh_change();
            if (sRoll.is(prop))
                query_mesh_change();
            if (sScaleX.is(prop))
                query_mesh_change();
            if (sScaleY.is(prop))
                query_mesh_change();
            if (sScaleZ.is(prop))
                query_mesh_change();
        }

        void Model3D::read_object_properties(
            core::KVTStorage *kvt,
            const char *base,
            dsp::matrix3d_t *m,
            float *hue,
            bool *visible
        )
        {
            dsp::matrix3d_t tmp;
            float cx = 0.0f, cy = 0.0f, cz = 0.0f;
            float dx = 0.0f, dy = 0.0f, dz = 0.0f;
            float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
            float sx = 1.0f, sy = 1.0f, sz = 1.0f;
            *hue = 0.0f;
            float enabled = 0.0f;

            // Fetch data from KVT
            kvt_fetch(kvt, base, "enabled", &enabled, 1.0f);
            kvt_fetch(kvt, base, "center/x", &cx, 0.0f);
            kvt_fetch(kvt, base, "center/y", &cy, 0.0f);
            kvt_fetch(kvt, base, "center/z", &cz, 0.0f);
            kvt_fetch(kvt, base, "position/x", &dx, 0.0f);
            kvt_fetch(kvt, base, "position/y", &dy, 0.0f);
            kvt_fetch(kvt, base, "position/z", &dz, 0.0f);
            kvt_fetch(kvt, base, "rotation/yaw", &yaw, 0.0f);
            kvt_fetch(kvt, base, "rotation/pitch", &pitch, 0.0f);
            kvt_fetch(kvt, base, "rotation/roll", &roll, 0.0f);
            kvt_fetch(kvt, base, "scale/x", &sx, 1.0f);
            kvt_fetch(kvt, base, "scale/y", &sy, 1.0f);
            kvt_fetch(kvt, base, "scale/z", &sz, 1.0f);
            kvt_fetch(kvt, base, "color/hue", hue, 0.0f);

            *visible        = (enabled >= 0.5f);

            // Compute the matrix
            // Translation
            dsp::init_matrix3d_translate(m, dx + cx, dy + cy, dz + cz);

            // Rotation
            dsp::init_matrix3d_rotate_z(&tmp, yaw * M_PI / 180.0f);
            dsp::apply_matrix3d_mm1(m, &tmp);
            dsp::init_matrix3d_rotate_y(&tmp, pitch * M_PI / 180.0f);
            dsp::apply_matrix3d_mm1(m, &tmp);
            dsp::init_matrix3d_rotate_x(&tmp, roll * M_PI / 180.0f);
            dsp::apply_matrix3d_mm1(m, &tmp);

            // Scale
            dsp::init_matrix3d_scale(&tmp, sx * 0.01f, sy * 0.01f, sz * 0.01f);
            dsp::apply_matrix3d_mm1(m, &tmp);

            // Move center to (0, 0, 0) point
            dsp::init_matrix3d_translate(&tmp, -cx, -cy, -cz);
            dsp::apply_matrix3d_mm1(m, &tmp);
        }

        bool Model3D::submit_background(dspu::bsp::context_t *dst)
        {
            if (!wVisibility.get())
                return false;

            // Init color
            bool added = false;
            float opacity = lsp_limit(1.0f - sTransparency.get(), 0.0f, 1.0f);

            // Init world matrix for model
            dsp::matrix3d_t m, om, world;

            dsp::init_matrix3d_translate(&world, sPosX.get(), sPosY.get(), sPosZ.get());

            dsp::init_matrix3d_rotate_z(&m, sYaw.get() * M_PI / 180.0f);
            dsp::apply_matrix3d_mm1(&world, &m);
            dsp::init_matrix3d_rotate_y(&m, sPitch.get() * M_PI / 180.0f);
            dsp::apply_matrix3d_mm1(&world, &m);
            dsp::init_matrix3d_rotate_x(&m, sRoll.get() * M_PI / 180.0f);
            dsp::apply_matrix3d_mm1(&world, &m);

            dsp::init_matrix3d_scale(&m, sScaleX.get(), sScaleY.get(), sScaleZ.get());
            dsp::apply_matrix3d_mm1(&world, &m);

            // Add all visible objects to BSP context
            for (size_t i=0, n=sScene.num_objects(); i<n; ++i)
            {
                // Check object visibility
                dspu::Object3D *o = sScene.object(i);
                if (o == NULL)
                    continue;

                cTempColor.set(cColor.value());
                cTempColor.set_hue(float(i) / float(n));

                // Apply changes
                om = *(o->matrix());
                if (!sKvtRoot.is_empty())
                {
                    core::KVTStorage *kvt = pWrapper->kvt_lock();
                    if (kvt)
                    {
                        LSPString base;
                        bool res = base.set(&sKvtRoot);
                        if (res)
                            res = base.fmt_append_ascii("%d", int(i));

                        if (res)
                        {
                            bool visible = false;
                            float hue = 0.0f;
                            read_object_properties(kvt, base.get_utf8(), &om, &hue, &visible);
                            o->set_visible(visible);
                            cTempColor.set_hue(hue);
                        }

                        pWrapper->kvt_release();
                    }
                }

                if (!o->is_visible())
                    continue;

                dsp::color3d_t c    = cTempColor.color3d();
                c.a                 = 1.0f - (1.0f - c.a) * opacity;

                dsp::apply_matrix3d_mm2(&m, &world, &om);
                dsp::apply_matrix3d_mm1(&m, &matOrientation);
                if ((dst->add_object(o, &m, &c)) == STATUS_OK)
                    added       = true;
            }

            return added;
        }

        bool Model3D::changed(core::KVTStorage *kvt, const char *id, const core::kvt_param_t *value)
        {
            if (!match(id))
                return false;

            query_mesh_change();
            return true;
        }

        bool Model3D::match(const char *id)
        {
            if (sKvtRoot.is_empty())
                return false;

            const char *prefix = sKvtRoot.get_utf8();
            size_t len = strlen(prefix);

            return strncmp(id, prefix, len) == 0;
        }

        void Model3D::query_mesh_change()
        {
            if (pParent != NULL)
                pParent->query_view_change();
        }

    } /* namespace ctl */
} /* namespace lsp */


