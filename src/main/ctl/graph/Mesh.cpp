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
#include <lsp-plug.in/plug-fw/meta/func.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Mesh)
            status_t res;
            bool stream = false;

            if (name->equals_ascii("mesh"))
                stream = false;
            else if (name->equals_ascii("stream"))
                stream = true;
            else
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

            ctl::Mesh *wc  = new ctl::Mesh(context->wrapper(), w, stream);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Mesh)

        //-----------------------------------------------------------------
        const ctl_class_t Mesh::metadata        = { "Mesh", &Widget::metadata };

        Mesh::Mesh(ui::IWrapper *wrapper, tk::GraphMesh *widget, bool stream): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;

            bStream         = stream;
            bStrobe         = false;
            nXIndex         = -1;
            nYIndex         = -1;
            nSIndex         = -1;
            nMaxDots        = -1;
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
                sStrobes.init(pWrapper, gm->strobes());
                sXAxis.init(pWrapper, gm->haxis());
                sYAxis.init(pWrapper, gm->vaxis());

                sColor.init(pWrapper, gm->color());
                sFillColor.init(pWrapper, gm->fill_color());

                sXIndex.init(pWrapper, this);
                sYIndex.init(pWrapper, this);
                sSIndex.init(pWrapper, this);
                sMaxDots.init(pWrapper, this);
                sStrobe.init(pWrapper, this);
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

                sWidth.set("width", name, value);
                sSmooth.set("smooth", name, value);
                sFill.set("fill", name, value);
                sStrobes.set("strobes", name, value);

                sXAxis.set("haxis", name, value);
                sXAxis.set("xaxis", name, value);
                sXAxis.set("basis", name, value);
                sXAxis.set("ox", name, value);

                sYAxis.set("vaxis", name, value);
                sYAxis.set("yaxis", name, value);
                sYAxis.set("parallel", name, value);
                sYAxis.set("oy", name, value);

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
                set_expr(&sSIndex, "s.index", name, value);
                set_expr(&sSIndex, "si", name, value);
                set_expr(&sSIndex, "s", name, value);
                set_expr(&sMaxDots, "dots.max", name, value);
                set_expr(&sStrobe, "strobe", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Mesh::trigger_expr()
        {
            nXIndex     = -1;
            nYIndex     = -1;
            nSIndex     = -1;

            if (sXIndex.valid())
                nXIndex     = sXIndex.evaluate_int(0);
            if (sYIndex.valid())
                nYIndex     = sYIndex.evaluate_int(0);
            if (sSIndex.valid())
                nSIndex     = sSIndex.evaluate_int(0);

            // Compute nXIndex if not set
            if (nXIndex < 0)
            {
                nXIndex = 0;
                while ((nXIndex == nYIndex) || (nXIndex == nSIndex))
                    ++nXIndex;
            }

            // Compute nYIndex if not set
            if (nYIndex < 0)
            {
                nYIndex = 0;
                while ((nYIndex == nXIndex) || (nYIndex == nSIndex))
                    ++nYIndex;
            }

            // Compute nSIndex if not set
            if (nSIndex < 0)
            {
                nSIndex = 0;
                while ((nSIndex == nXIndex) || (nSIndex == nYIndex))
                    ++nSIndex;
            }

            // Update maximum dots
            nMaxDots    = (sMaxDots.valid()) ? sMaxDots.evaluate_int(-1) : -1;

            // Update strobe usage
            bStrobe     = (sStrobe.valid()) ? sStrobe.evaluate_bool(false) : false;
        }

        void Mesh::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);

            if ((sXIndex.depends(port)) ||
                (sYIndex.depends(port)) ||
                (sSIndex.depends(port)) ||
                (sMaxDots.depends(port)) ||
                (sStrobe.depends(port)))
            {
                trigger_expr();
                commit_data();
            }
            else if ((pPort == port) && (pPort != NULL))
                commit_data();
        }

        ssize_t Mesh::get_strobe_block_size(const float *s, size_t size)
        {
            for (ssize_t i=size-1; i>=0; --i)
            {
                if (s[i] >= 0.5f)
                    return size - i;
            }
            return -1;
        }

        void Mesh::commit_data()
        {
            tk::GraphMesh *gm   = tk::widget_cast<tk::GraphMesh>(wWidget);
            if (gm == NULL)
                return;

            tk::GraphMeshData *data = gm->data();
            const meta::port_t *meta = (pPort != NULL) ? pPort->metadata() : NULL;

            if (bStream)
            {
                // Process port data as stream
                plug::stream_t *stream = (meta::is_stream_port(meta)) ? pPort->buffer<plug::stream_t>() : NULL;
                if (stream == NULL)
                {
                    data->set_size(0);
                    return;
                }

                // Fill mesh data
                bool valid = (nXIndex >= 0) && (nXIndex < ssize_t(stream->channels()));
                if (valid)
                    valid   = (nYIndex >= 0) && (nYIndex < ssize_t(stream->channels()));
                if ((valid) && (bStrobe))
                    valid   = (nSIndex >= 0) && (nSIndex < ssize_t(stream->channels()));

                if (valid)
                {
                    // Perform read from stream to mesh
                    size_t last     = stream->frame_id();
                    ssize_t length  = stream->get_length(last);
                    ssize_t dots    = (nMaxDots >= 0) ? lsp_min(length, nMaxDots) : length;
                    ssize_t off     = length - dots;

                    // Resize mesh data and set strobe flag
                    data->set_size(dots, bStrobe);

                /* TODO: this may be more correct behaviour since the frame is not full when the strobe signal is emitted

                    if (bStrobe)
                    {
                        float *s = data->s();
                        while (off > 0)
                        {
                            // Read the strobe signal
                            stream->read(nSIndex, s, off, dots);
                            ssize_t shift = get_strobe_block_size(s, dots);
                            if (shift >= 0)
                            {
                                off = lsp_max(0, ssize_t(off) - shift);
                                break;
                            }
                            else if (dots >= off)
                            {
                                off = 0;
                                break;
                            }
                            else
                                off -= dots;

                            // Read the stream again
                            stream->read(nSIndex, s, off, dots);
                        }

                        // Read the stream with last frame kicked off
                        stream->read(nSIndex, s, off, dots);
                    }
                */

                    stream->read(nXIndex, data->x(), off, dots);
                    stream->read(nYIndex, data->y(), off, dots);
                    if (bStrobe)
                        stream->read(nSIndex, data->s(), off, dots);
                }
                else
                    data->set_size(0);

                data->touch();
            }
            else
            {
                // Process port data as mesh
                plug::mesh_t *mesh  = (meta::is_mesh_port(meta)) ? pPort->buffer<plug::mesh_t>() : NULL;
                if (mesh == NULL)
                {
                    data->set_size(0);
                    return;
                }

                // Validate mesh data
                bool valid = (nXIndex >= 0) && (nXIndex < ssize_t(mesh->nBuffers));
                if (valid)
                    valid   = (nYIndex >= 0) && (nYIndex < ssize_t(mesh->nBuffers));
                if ((valid) && (bStrobe))
                    valid   = (nSIndex >= 0) && (nSIndex < ssize_t(mesh->nBuffers));

                // Fill mesh data
                if (valid)
                {
                    // Resize mesh data and set strobe flag
                    data->set_size(mesh->nItems, bStrobe);

                    data->set_x(mesh->pvData[nXIndex], mesh->nItems);
                    data->set_y(mesh->pvData[nYIndex], mesh->nItems);
                    if (bStrobe)
                        data->set_s(mesh->pvData[nSIndex], mesh->nItems);
                }
                else
                    data->set_size(0);

                data->touch();
            }
        }

        void Mesh::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
            trigger_expr();
        }

    } /* namespace ctl */
} /* namespace lsp */


