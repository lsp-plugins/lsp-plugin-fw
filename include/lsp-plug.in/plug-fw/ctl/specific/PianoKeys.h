/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 сент. 2025 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_PIANOKEYS_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_PIANOKEYS_H_

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
         * Piano keyboard controller
         */
        class PianoKeys: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                ui::IPort          *pSelectionStart;
                ui::IPort          *pSelectionEnd;

                ctl::Padding        sBorder;
                ctl::Integer        sSplitSize;
                ctl::Integer        sMinNote;
                ctl::Integer        sMaxNote;
                ctl::Integer        sAngle;
                ctl::Float          sKeyAspect;
                ctl::Boolean        sNatural;
                ctl::Boolean        sEditable;
                ctl::Boolean        sSelectable;
                ctl::Boolean        sClearSelection;

            protected:
                static status_t     slot_submit_key(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_change_selection(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                do_destroy();

            public:
                explicit PianoKeys(ui::IWrapper *wrapper, tk::PianoKeys *widget);
                PianoKeys(const PianoKeys &) = delete;
                PianoKeys(PianoKeys &&) = delete;
                virtual ~PianoKeys() override;
                PianoKeys & operator = (const PianoKeys &) = delete;
                PianoKeys & operator = (PianoKeys &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        end(ui::UIContext *ctx) override;
        };
    } /* namespace ctl */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_PIANOKEYS_H_ */
