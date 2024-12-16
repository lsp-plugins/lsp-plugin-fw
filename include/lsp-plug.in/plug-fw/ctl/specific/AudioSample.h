/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 20 июл. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOSAMPLE_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOSAMPLE_H_

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
         * Rack widget controller
         */
        class AudioSample: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                static const size_t CHANNEL_PERIOD  = 8;

            protected:
                enum labels_t
                {
                    LBL_FILE_NAME,
                    LBL_DURATION,
                    LBL_HEAD_CUT,
                    LBL_TAIL_CUT,
                    LBL_MISC,

                    LBL_COUNT
                };

            protected:
                class DataSink: public tk::TextDataSink
                {
                    private:
                        AudioSample    *pSample;

                    public:
                        explicit DataSink(AudioSample *sample);
                        virtual ~DataSink() override;

                    public:
                        virtual status_t    receive(const LSPString *text, const char *mime) override;
                        virtual status_t    error(status_t code) override;

                        void        unbind();
                };

                class DragInSink: public tk::URLSink
                {
                    protected:
                        AudioSample    *pSample;

                    public:
                        explicit DragInSink(AudioSample *sample);
                        virtual ~DragInSink() override;

                        void unbind();
                        virtual status_t    commit_url(const LSPString *url) override;
                };

            protected:
                ui::IPort          *pPort;
                ui::IPort          *pMeshPort;
                ui::IPort          *pPathPort;
                ui::IPort          *pFileTypePort;
                tk::FileDialog     *pDialog;
                ctl::Widget        *pFilePreview;
                tk::Menu           *pMenu;
                DataSink           *pDataSink;
                DragInSink         *pDragInSink;
                bool                bFullSample;
                bool                bLoadPreview;
                lltl::parray<file_format_t>     vFormats;
                lltl::parray<tk::MenuItem>      vMenuItems;
                lltl::pphash<char, ui::IPort>   vClipboardBind;

                LSPString           vChannelStyles[CHANNEL_PERIOD];

                ctl::Integer        sWaveBorder;
                ctl::Integer        sFadeInBorder;
                ctl::Integer        sFadeOutBorder;
                ctl::Integer        sStretchBorder;
                ctl::Integer        sLoopBorder;
                ctl::Integer        sPlayBorder;
                ctl::Integer        sLineWidth;
                ctl::LCString       sMainText;
                ctl::Integer        sLabelRadius;
                ctl::Integer        sBorder;
                ctl::Integer        sBorderRadius;
                ctl::Float          sMaxAmplitude;
                ctl::Boolean        sActive;
                ctl::Boolean        sStereoGroups;
                ctl::Boolean        sLabelVisibility[tk::AudioSample::LABELS];
                ctl::Boolean        sBorderFlat;
                ctl::Boolean        sGlass;

                ctl::Expression     sStatus;
                ctl::Expression     sHeadCut;
                ctl::Expression     sTailCut;
                ctl::Expression     sFadeIn;
                ctl::Expression     sFadeOut;
                ctl::Expression     sStretch;
                ctl::Expression     sStretchBegin;
                ctl::Expression     sStretchEnd;
                ctl::Expression     sLoop;
                ctl::Expression     sLoopBegin;
                ctl::Expression     sLoopEnd;
                ctl::Expression     sPlayPosition;
                ctl::Expression     sLength;
                ctl::Expression     sActualLength;

                ctl::Padding        sIPadding;

                ctl::Color          sColor;
                ctl::Color          sBorderColor;
                ctl::Color          sGlassColor;
                ctl::Color          sLineColor;
                ctl::Color          sMainColor;
                ctl::Color          sStretchColor;
                ctl::Color          sStretchBorderColor;
                ctl::Color          sLoopColor;
                ctl::Color          sLoopBorderColor;
                ctl::Color          sPlayColor;
                ctl::Color          sLabelTextColor[tk::AudioSample::LABELS];
                ctl::Color          sLabelBgColor;

            protected:
                static status_t     slot_audio_sample_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dialog_change(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dialog_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_dialog_hide(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_popup_cut_action(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_popup_copy_action(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_popup_paste_action(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_popup_clear_action(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_drag_request(tk::Widget *sender, void *ptr, void *data);

            protected:
                void                show_file_dialog();
                void                update_path();
                void                preview_file();
                void                commit_file();
                void                sync_status();
                void                sync_labels();
                void                sync_markers();
                void                sync_mesh();
                tk::Menu           *create_menu();
                tk::MenuItem       *create_menu_item(tk::Menu *menu);

            public:
                explicit AudioSample(ui::IWrapper *wrapper, tk::AudioSample *widget);
                AudioSample(const AudioSample &) = delete;
                AudioSample(AudioSample &&) = delete;
                virtual ~AudioSample() override;

                AudioSample & operator = (const AudioSample &) = delete;
                AudioSample & operator = (AudioSample &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
                virtual void        reloaded(const tk::StyleSheet *sheet) override;
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOSAMPLE_H_ */
