/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 нояб. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_TABCONTROL_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_TABCONTROL_H_

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
         * Tab control
         */
        class TabControl: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort                  *pPort;
                float                       fMin;
                float                       fMax;
                float                       fStep;

                ctl::Color                  sBorderColor;
                ctl::Color                  sHeadingColor;
                ctl::Color                  sHeadingSpacingColor;
                ctl::Color                  sHeadingGapColor;
                ctl::Integer                sBorderSize;
                ctl::Integer                sBorderRadius;
                ctl::Integer                sTabSpacing;
                ctl::Integer                sHeadingSpacing;
                ctl::Integer                sHeadingGap;
                ctl::Float                  sHeadingGapBrightness;
                ctl::Embedding              sEmbedding;
                ctl::Boolean                sTabJoint;
                ctl::Boolean                sHeadingFill;
                ctl::Boolean                sHeadingSpacingFill;
                ctl::Expression             sActive;

                lltl::parray<tk::Tab>       vTabs;

            protected:
                static status_t         slot_submit(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                    submit_value();
                void                    select_active_widget();
                tk::Tab                *create_new_tab(tk::Widget *child, tk::Registry *registry);

            public:
                explicit                TabControl(ui::IWrapper *wrapper, tk::TabControl *tc);
                virtual                ~TabControl() override;

                virtual status_t        init() override;

            public:
                virtual void            sync_metadata(ui::IPort *port) override;
                virtual void            set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual status_t        add(ui::UIContext *ctx, ctl::Widget *child) override;
                virtual void            end(ui::UIContext *ctx) override;
                virtual void            notify(ui::IPort *port, size_t flags) override;
        };

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_TABCONTROL_H_ */
