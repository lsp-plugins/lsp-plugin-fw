/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

            private:
                static const tk::tether_t   popup_tether[];

            protected:
                class PopupWindow: public tk::PopupWindow
                {
                    private:
                        friend class ctl::MidiNote;

                    private:
                        static const tk::w_class_t      metadata;

                    protected:
                        MidiNote   *pLabel;
                        tk::Box     sBox;
                        tk::Edit    sValue;
                        tk::Label   sUnits;
                        tk::Button  sApply;
                        tk::Button  sCancel;

                    public:
                        explicit PopupWindow(MidiNote *label, tk::Display *dpy);
                        virtual ~PopupWindow() override;

                        virtual status_t    init() override;
                        virtual void        destroy() override;
                };

            protected:
                size_t                  nNote;
                size_t                  nDigits;
                ui::IPort              *pNote;
                ui::IPort              *pOctave;
                ui::IPort              *pValue;
                PopupWindow            *wPopup;

                ctl::Color              sColor;
                ctl::Color              sTextColor;
                ctl::Color              sInactiveColor;
                ctl::Color              sInactiveTextColor;

                ctl::Padding            sIPadding;

            protected:
                static status_t     slot_submit_value(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_change_value(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_cancel_value(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dbl_click(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_key_up(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_button(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_scroll(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                do_destroy();
                void                commit_value(float value);
                bool                apply_value(const LSPString *value);
                void                apply_value(ssize_t value);

            public:
                explicit MidiNote(ui::IWrapper *wrapper, tk::Indicator *widget);
                MidiNote(const MidiNote &) = delete;
                MidiNote(MidiNote &&) = delete;
                virtual ~MidiNote() override;
                MidiNote & operator = (const MidiNote &) = delete;
                MidiNote & operator = (MidiNote &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
        };
    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_MIDINOTE_H_ */
