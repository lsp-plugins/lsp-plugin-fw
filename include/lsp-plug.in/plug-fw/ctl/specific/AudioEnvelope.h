/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 июн. 2025 г.
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

#ifndef PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOENVELOPE_H_
#define PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOENVELOPE_H_

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
         * Audio envelope widget controller
         */
        class AudioEnvelope: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                typedef struct point_t
                {
                    ui::IPort          *pPort;
                    tk::Float          *pValue;
                    float               fOldValue;
                    float               fNewValue;
                } point_t;

                enum points_t
                {
                    P_ATTACK,
                    P_HOLD,
                    P_DECAY,
                    P_BREAK,
                    P_SLOPE,
                    P_SUSTAIN,
                    P_RELEASE,

                    P_TOTAL
                };

                enum role_t
                {
                    R_TIME,
                    R_CURVE,
                    R_LEVEL,

                    R_TOTAL
                };

            protected:
                point_t             vPoints[P_TOTAL][R_TOTAL];
                ui::IPort          *vTypes[P_TOTAL];

                ctl::Boolean        sHoldEnabled;
                ctl::Boolean        sBreakEnabled;
                ctl::Boolean        sQuadPoint;
                ctl::Boolean        sFill;
                ctl::Boolean        sWire;
                ctl::Boolean        sEditable;
                ctl::Integer        sLineWidth;
                ctl::Color          sLineColor;
                ctl::Color          sFillColor;
                ctl::Integer        sPointSize;
                ctl::Color          sPointColor;
                ctl::Color          sPointHoverColor;
                ctl::Color          sColor;
                ctl::Boolean        sBorderFlat;
                ctl::Boolean        sGlass;
                ctl::Color          sGlassColor;
                ctl::Padding        sIPadding;
                ctl::Color          sBorderColor;

                bool                bCommitting;
                bool                bSubmitting;

            protected:
                static float        get_normalized(ui::IPort *port);
                static void         set_normalized(ui::IPort *port, float value);
                static status_t     slot_change(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_begin_edit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_end_edit(tk::Widget *sender, void *ptr, void *data);
                static void         curve_function(float *y, const float *x, size_t count, const tk::AudioEnvelope *sender, void *data);

            protected:
                void                arrange_time_values();
                bool                sync_time_values(point_t *actor);
                void                commit_values();
                void                submit_ports();
                void                begin_edit();
                void                end_edit();
                size_t              get_function(points_t point);

            public:
                explicit AudioEnvelope(ui::IWrapper *wrapper, tk::AudioEnvelope *widget);
                AudioEnvelope(const AudioEnvelope &) = delete;
                AudioEnvelope(AudioEnvelope &&) = delete;
                virtual ~AudioEnvelope() override;

                AudioEnvelope & operator = (const AudioEnvelope &) = delete;
                AudioEnvelope & operator = (AudioEnvelope &&) = delete;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        end(ui::UIContext *ctx) override;
                virtual void        notify(ui::IPort *port, size_t flags) override;
        };

    } /* namespace ctl */
} /* namespace lsp */




#endif /* PLUG_IN_PLUG_FW_CTL_SPECIFIC_AUDIOENVELOPE_H_ */
