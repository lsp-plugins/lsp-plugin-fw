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

namespace lsp
{
    namespace ctl
    {
        //-----------------------------------------------------------------
        const ctl_class_t Object3D::metadata   = { "Object3D", &Widget::metadata };

        Object3D::Object3D(ui::IWrapper *wrapper): Widget(wrapper, NULL)
        {
            pClass          = &metadata;
        }

        Object3D::~Object3D()
        {
        }

        bool Object3D::submit_foreground(ctl::Area3D *caller, lltl::darray<r3d::buffer_t> *buf)
        {
            return false;
        }

        bool Object3D::submit_background(ctl::Area3D *caller, dspu::bsp::context_t *ctx)
        {
            return false;
        }
    } // namespace ctl
} // namespace lsp


