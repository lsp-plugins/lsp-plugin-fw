/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 19 дек. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_DRYWETLINK_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_DRYWETLINK_H_

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
         * Dry/Wet link controller
         */
        class DryWetLink: public Button
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pDry;
                ui::IPort          *pWet;

            protected:
                void                sync_value(ui::IPort *dst, ui::IPort *src);
                static float        get_gain(ui::IPort *port);
                static void         set_gain(ui::IPort *port, float gain);

            public:
                explicit DryWetLink(ui::IWrapper *wrapper, tk::Button *widget);
                DryWetLink(const DryWetLink &) = delete;
                DryWetLink(DryWetLink &&) = delete;
                virtual ~DryWetLink() override;

                DryWetLink & operator = (const DryWetLink &) = delete;
                DryWetLink & operator = (DryWetLink &&) = delete;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
        };

    } /* namespace ctl */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_DRYWETLINK_H_ */
