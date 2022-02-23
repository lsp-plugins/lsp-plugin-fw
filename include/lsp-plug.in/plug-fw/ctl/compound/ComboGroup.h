/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 19 июл. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_COMPOUND_COMBOGROUP_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_COMPOUND_COMBOGROUP_H_

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
         * Combo group
         */
        class ComboGroup: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort              *pPort;
                float                   fMin;
                float                   fMax;
                float                   fStep;
                ssize_t                 nActive;

                ctl::Color              sColor;
                ctl::Color              sTextColor;
                ctl::Color              sSpinColor;
                ctl::LCString           sEmptyText;
                ctl::Padding            sTextPadding;
                ctl::Expression         sActive;
                ctl::Embedding          sEmbed;

            protected:
                static status_t         slot_combo_submit(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                    sync_metadata(ui::IPort *port);
                void                    submit_value();
                void                    select_active_widget();

            public:
                explicit                ComboGroup(ui::IWrapper *wrapper, tk::ComboGroup *cgroup);
                virtual                ~ComboGroup();

                virtual status_t        init();

            public:
                virtual void            set(ui::UIContext *ctx, const char *name, const char *value);
                virtual status_t        add(ui::UIContext *ctx, ctl::Widget *child);
                virtual void            end(ui::UIContext *ctx);
                virtual void            notify(ui::IPort *port);
        };

    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_CONTAINERS_COMBOGROUP_H_ */
