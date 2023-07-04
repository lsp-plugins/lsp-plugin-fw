/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 3 окт. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_3D_ORIGIN3D_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_3D_ORIGIN3D_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        namespace style
        {
            LSP_TK_STYLE_DEF_BEGIN(Origin3D, Object3D)
                tk::prop::Float             sWidth;     // Width
                tk::prop::Float             sLength[3]; // X, Y, Z length
                tk::prop::Color             sColor[3];  // X, Y, Z colors
            LSP_TK_STYLE_DEF_END
        }

        /**
         * ComboBox controller
         */
        class Origin3D: public Object3D
        {
            public:
                static const ctl_class_t metadata;

            protected:
                tk::prop::Float             sWidth;     // Width
                tk::prop::Float             sLength[3]; // X, Y, Z length
                tk::prop::Color             sColor[3];  // X, Y, Z colors

                ctl::Float                  cWidth;
                ctl::Float                  cLength[3];
                ctl::Color                  cColor[3];

                r3d::dot4_t                 vAxisLines[6];
                r3d::color_t                vAxisColors[6];

            public:
                explicit Origin3D(ui::IWrapper *wrapper);
                virtual ~Origin3D() override;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        property_changed(tk::Property *prop) override;
                virtual bool        submit_foreground(lltl::darray<r3d::buffer_t> *dst) override;
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_3D_ORIGIN3D_H_ */
