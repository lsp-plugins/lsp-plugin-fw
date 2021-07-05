/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 мая 2021 г.
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

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Mesh)
            status_t res;

            if (!name->equals_ascii("mesh"))
                return STATUS_NOT_FOUND;

            tk::GraphMesh *w = new tk::GraphMesh(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Mesh *wc  = new ctl::Mesh(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Mesh)

        //-----------------------------------------------------------------
        const ctl_class_t Mesh::metadata        = { "Mesh", &Widget::metadata };

        Mesh::Mesh(ui::IWrapper *wrapper, tk::GraphMesh *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
        }

        Mesh::~Mesh()
        {
        }

        status_t Mesh::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphMesh *gm   = tk::widget_cast<tk::GraphMesh>(wWidget);
            if (gm != NULL)
            {
                sWidth.init(pWrapper, gm->width());
                sSmooth.init(pWrapper, gm->smooth());
                sFill.init(pWrapper, gm->fill());
                sColor.init(pWrapper, gm->color());
                sFillColor.init(pWrapper, gm->fill_color());

                sXIndex.init(pWrapper, this);
                sYIndex.init(pWrapper, this);
                sSIndex.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void Mesh::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphMesh *gm   = tk::widget_cast<tk::GraphMesh>(wWidget);
            if (gm != NULL)
            {
                bind_port(&pPort, "id", name, value);

                set_param(gm->origin(), "origin", name, value);
                set_param(gm->origin(), "center", name, value);
                set_param(gm->origin(), "o", name, value);

                set_param(gm->haxis(), "haxis", name, value);
                set_param(gm->haxis(), "xaxis", name, value);
                set_param(gm->haxis(), "basis", name, value);
                set_param(gm->haxis(), "ox", name, value);

                set_param(gm->vaxis(), "vaxis", name, value);
                set_param(gm->vaxis(), "yaxis", name, value);
                set_param(gm->vaxis(), "parallel", name, value);
                set_param(gm->vaxis(), "oy", name, value);

                sWidth.set("width", name, value);
                sSmooth.set("smooth", name, value);
                sFill.set("fill", name, value);
                sColor.set("color", name, value);
                sFillColor.set("fill.color", name, value);
                sFillColor.set("fcolor", name, value);

                set_expr(&sXIndex, "x.index", name, value);
                set_expr(&sXIndex, "xi", name, value);
                set_expr(&sXIndex, "x", name, value);
                set_expr(&sYIndex, "y.index", name, value);
                set_expr(&sYIndex, "yi", name, value);
                set_expr(&sYIndex, "y", name, value);
                set_expr(&sSIndex, "strobe.index", name, value);
                set_expr(&sSIndex, "strobe", name, value);
                set_expr(&sSIndex, "si", name, value);
                set_expr(&sSIndex, "s", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Mesh::notify(ui::IPort *port)
        {
            Widget::notify(port);

            if (sXIndex.depends(port))
                rebuild_mesh();
            else if (sYIndex.depends(port))
                rebuild_mesh();
            else if (sSIndex.depends(port))
                rebuild_mesh();
            else if ((pPort == port) && (pPort != NULL))
                rebuild_mesh();
        }

        void Mesh::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
            rebuild_mesh();
        }

        void Mesh::rebuild_mesh()
        {
            tk::GraphMesh *gm   = tk::widget_cast<tk::GraphMesh>(wWidget);
            if (gm == NULL)
                return;

            tk::GraphMeshData *data = gm->data();
            plug::mesh_t *mesh  = (pPort != NULL) ? pPort->buffer<plug::mesh_t>() : NULL;
            if (mesh == NULL)
            {
                data->set_size(0);
                return;
            }

            ssize_t xi = -1, yi = -1, si = -1;

            if (sXIndex.valid())
                xi      = sXIndex.evaluate_int(0);
            if (sYIndex.valid())
                yi      = sYIndex.evaluate_int(0);
            if (sSIndex.valid())
                si      = sSIndex.evaluate_int(0);

            // Compute xi if not set
            if (xi < 0)
            {
                xi      = 0;
                if (xi == yi)
                    ++xi;
                if (xi == si)
                    ++xi;
            }

            // Compute yi if not set
            if (yi < 0)
            {
                yi      = 0;
                if (yi == xi)
                    ++yi;
                if (yi == si)
                    ++yi;
            }

            // Resize mesh data
            data->set_size(mesh->nItems);

            if ((xi >= 0) && (xi < ssize_t(mesh->nBuffers)))
                data->set_x(mesh->pvData[xi], mesh->nItems);
            if ((yi >= 0) && (xi < ssize_t(mesh->nBuffers)))
                data->set_y(mesh->pvData[yi], mesh->nItems);
// TODO
//            if ((si >= 0) && (si < mesh->nBuffers))
//                data->set_s(mesh->pvData[si], mesh->nItems);

        }

        void Mesh::schema_reloaded()
        {
            Widget::schema_reloaded();

            sColor.reload();
            sFillColor.reload();
        }
    }
}


