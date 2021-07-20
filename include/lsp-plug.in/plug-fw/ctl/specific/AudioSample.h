/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                ui::IPort          *pPort;

                ctl::Integer        sWaveBorder;
                ctl::Integer        sFadeInBorder;
                ctl::Integer        sFadeOutBorder;
                ctl::Integer        sLineWidth;
                ctl::LCString       sMainText;
                ctl::Integer        sLabelRadius;
                ctl::Integer        sBorder;
                ctl::Integer        sBorderRadius;
                ctl::Boolean        sActive;
                ctl::Boolean        sStereoGroups;
                ctl::Boolean        sLabelVisibility[tk::AudioSample::LABELS];
                ctl::Boolean        sBorderFlat;
                ctl::Boolean        sGlass;

                ctl::Padding        sIPadding;

                ctl::Color          sColor;
                ctl::Color          sBorderColor;
                ctl::Color          sGlassColor;
                ctl::Color          sLineColor;
                ctl::Color          sMainColor;
                ctl::Color          sLabelTextColor[tk::AudioSample::LABELS];
                ctl::Color          sLabelBgColor;


            public:
                explicit AudioSample(ui::IWrapper *wrapper, tk::AudioSample *widget);
                virtual ~AudioSample();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        schema_reloaded();
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOSAMPLE_H_ */
