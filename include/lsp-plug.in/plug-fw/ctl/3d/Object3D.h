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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_3D_OBJECT3D_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_3D_OBJECT3D_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/3d/bsp/context.h>

namespace lsp
{
    namespace ctl
    {
        class Area3D;

        /**
         * ComboBox controller
         */
        class Object3D: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ctl::Area3D        *pParent;
                tk::Style           sStyle;

            public:
                Object3D(ui::IWrapper *wrapper);
                virtual ~Object3D();

                virtual status_t    init();

            public:
                inline void         set_parent(ctl::Area3D *area)       { pParent   = area; }

            public:
                /**
                 * Submit foreground object to the scene, the implementation should append data to the passed list
                 * @param dst list of drawing buffers to append data, drawing buffer fields should point to valid memory locations
                 * @return true if there was some data submitted
                 */
                virtual bool        submit_foreground(lltl::darray<r3d::buffer_t> *dst);

                /**
                 * Submit background object to the scene
                 * @param dst BSP context to append data
                 * @return true if there was some data submitted
                 */
                virtual bool        submit_background(dspu::bsp::context_t *dst);

                /**
                 * Query for redraw
                 */
                virtual void        query_redraw();
        };

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_3D_OBJECT3D_H_ */
