/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 июл. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_MIDINOTE_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_MIDINOTE_H_

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
         * Midi note controller
         */
        class MidiNote: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                size_t                  nNote;
                size_t                  nDigits;
                ui::IPort              *pNote;
                ui::IPort              *pOctave;
                ui::IPort              *pValue;

                ctl::Color              sColor;
                ctl::Color              sTextColor;

                ctl::Padding            sIPadding;

//                PopupWindow    *pPopup;

            protected:
                void            do_destroy();
                void            commit_value(float value);
                bool            apply_value(const LSPString *value);
                void            apply_value(ssize_t value);

            public:
                explicit MidiNote(ui::IWrapper *wrapper, tk::Indicator *widget);
                virtual ~MidiNote();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        end(ui::UIContext *ctx);
                virtual void        notify(ui::IPort *port);
                virtual void        schema_reloaded();
        };
    } // namespace ctl
} // namespace lsp



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_MIDINOTE_H_ */
