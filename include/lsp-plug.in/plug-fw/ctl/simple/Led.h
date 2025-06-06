/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 мая 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_LED_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_LED_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * Led widget controller
         */
        class Led: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ctl::Color          sColor;
                ctl::Color          sLightColor;
                ctl::Color          sBorderColor;
                ctl::Color          sLightBorderColor;

                ctl::Color          sInactiveColor;
                ctl::Color          sInactiveLightColor;
                ctl::Color          sInactiveBorderColor;
                ctl::Color          sInactiveLightBorderColor;

                ctl::Color          sHoleColor;

                ctl::Expression     sLight;
                ui::IPort          *pPort;

                float               fValue;
                float               fKey;
                bool                bInvert;

            protected:
                void                update_value();

            public:
                explicit Led(ui::IWrapper *wrapper, tk::Led *widget);
                virtual ~Led() override;

                virtual status_t    init() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;
        };

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SIMPLE_LED_H_ */
