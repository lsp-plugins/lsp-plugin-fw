/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 2 окт. 2021 г.
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
#include <private/ui/BuiltinStyle.h>

namespace lsp
{
    namespace ctl
    {
        namespace style
        {
            LSP_TK_STYLE_IMPL_BEGIN(Object3D, lsp::tk::Style)
                // Bind
                sVisibility.bind("visibility", this);

                // Configure
                sVisibility.set(true);
            LSP_TK_STYLE_IMPL_END

            LSP_UI_BUILTIN_STYLE(Object3D, "Object3D", "root");
        }

        //-----------------------------------------------------------------
        const ctl_class_t Object3D::metadata   = { "Object3D", &Widget::metadata };

        Object3D::Object3D(ui::IWrapper *wrapper):
            Widget(wrapper, NULL),
            sStyle(wrapper->display()->schema(), NULL, NULL),
            wVisibility(&sProperties)
        {
            pClass          = &metadata;

            pParent         = NULL;
        }

        Object3D::~Object3D()
        {
            pParent         = NULL;
        }

        status_t Object3D::init()
        {
            status_t res = Widget::init();
            if (res != STATUS_OK)
                return res;

            // Initialize style
            res = sStyle.init();
            if (res != STATUS_OK)
                return res;

            // Configure the style class
            const char *ws_class = pClass->name;
            tk::Style *sclass = pWrapper->display()->schema()->get(ws_class);
            if (sclass != NULL)
            {
                LSP_STATUS_ASSERT(sStyle.set_default_parents(ws_class));
                LSP_STATUS_ASSERT(sStyle.add_parent(sclass));
            }

            // Bind to style
            wVisibility.bind("visibility", &sStyle);

            // Bind controlles
            sVisibility.init(pWrapper, &wVisibility);

            return STATUS_OK;
        }

        void Object3D::property_changed(tk::Property *prop)
        {
            if (wVisibility.is(prop))
                query_draw_parent();
        }

        bool Object3D::submit_foreground(lltl::darray<r3d::buffer_t> *dst)
        {
            return false;
        }

        bool Object3D::submit_background(dspu::bsp::context_t *dst)
        {
            return false;
        }

        void Object3D::query_draw()
        {
            query_draw_parent();
        }

        void Object3D::query_draw_parent()
        {
            if (pParent != NULL)
                pParent->query_draw();
        }
    } // namespace ctl
} // namespace lsp


