/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 июл. 2021 г.
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
        CTL_FACTORY_IMPL_START(FBuffer)
            status_t res;

            if (!name->equals_ascii("fbuffer"))
                return STATUS_NOT_FOUND;

            tk::GraphFrameBuffer *w = new tk::GraphFrameBuffer(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::FBuffer *wc  = new ctl::FBuffer(context->wrapper(), w);
            if (wc == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(FBuffer)

        //-----------------------------------------------------------------
        const ctl_class_t FBuffer::metadata       = { "FBuffer", &Widget::metadata };

        FBuffer::FBuffer(ui::IWrapper *wrapper, tk::GraphFrameBuffer *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
            nRowID          = 0;
        }

        FBuffer::~FBuffer()
        {
        }

        status_t FBuffer::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::GraphFrameBuffer *fb    = tk::widget_cast<tk::GraphFrameBuffer>(wWidget);
            if (fb != NULL)
            {
                sColor.init(pWrapper, fb->color());
                sTransparency.init(pWrapper, fb->transparency());
                sHPos.init(pWrapper, fb->hpos());
                sVPos.init(pWrapper, fb->vpos());
                sHScale.init(pWrapper, fb->hscale());
                sVScale.init(pWrapper, fb->vscale());
                sMode.init(pWrapper, this);
            }

            return STATUS_OK;
        }

        void FBuffer::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::GraphFrameBuffer *fb   = tk::widget_cast<tk::GraphFrameBuffer>(wWidget);
            if (fb != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sColor.set("color", name, value);

                sTransparency.set("transparency", name, value);
                sTransparency.set("transp", name, value);
                sHPos.set("hpos", name, value);
                sHPos.set("x", name, value);
                sVPos.set("vpos", name, value);
                sVPos.set("y", name, value);
                sHScale.set("hscale", name, value);
                sHScale.set("width", name, value);
                sVScale.set("vscale", name, value);
                sVScale.set("height", name, value);

                set_expr(&sMode, "mode", name, value);

                set_param(fb->angle(), "angle", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void FBuffer::notify(ui::IPort *port)
        {
            Widget::notify(port);

            tk::GraphFrameBuffer *fb   = tk::widget_cast<tk::GraphFrameBuffer>(wWidget);
            if ((fb != NULL) && (port != NULL))
            {
                if (sMode.depends(port))
                    fb->function()->set_index(sMode.evaluate_int());

                // Deploy new value for framebuffer
                const meta::port_t *mdata = pPort->metadata();
                if (meta::is_framebuffer_port(mdata))
                {
                    plug::frame_buffer_t *data  = pPort->buffer<plug::frame_buffer_t>();
                    if (data != NULL)
                    {
                        // Set the proper size of the buffer
                        fb->data()->set_size(data->rows(), data->cols());

                        // Append the desired number of rows
                        size_t rowid                = data->next_rowid();
                        size_t delta                = rowid - nRowID;
                        if (delta > fb->data()->rows())
                            nRowID                      = rowid - fb->data()->rows();

                        while (nRowID != rowid)
                        {
                            float *row = data->get_row(nRowID++);
                            if (row != NULL)
                                fb->data()->set_row(nRowID, row);
                        }
                    }
                }
            }
        }

        void FBuffer::end(ui::UIContext *ctx)
        {
            tk::GraphFrameBuffer *fb   = tk::widget_cast<tk::GraphFrameBuffer>(wWidget);
            if (fb != NULL)
            {
                if (sMode.valid())
                    fb->function()->set_index(sMode.evaluate_int());
            }
        }

        void FBuffer::schema_reloaded()
        {
            Widget::schema_reloaded();

            sColor.reload();
        }

    }
}





