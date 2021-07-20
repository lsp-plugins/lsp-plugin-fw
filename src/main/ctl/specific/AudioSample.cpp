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

#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(AudioSample)
            status_t res;

            if (!name->equals_ascii("asample"))
                return STATUS_NOT_FOUND;

            tk::AudioSample *w = new tk::AudioSample(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::AudioSample *wc  = new ctl::AudioSample(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(AudioSample)

        //-----------------------------------------------------------------
        static const char *label_names[] =
        {
            "fname",
            "duration",
            "fadein",
            "fadeout",
            "misc"
        };

        const ctl_class_t AudioSample::metadata        = { "AudioSample", &Widget::metadata };

        AudioSample::AudioSample(ui::IWrapper *wrapper, tk::AudioSample *widget): Widget(wrapper, widget)
        {
            pClass          = &metadata;

            pPort           = NULL;
        }

        AudioSample::~AudioSample()
        {
        }

        status_t AudioSample::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            tk::AudioSample *as = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as != NULL)
            {
                sWaveBorder.init(pWrapper, as->wave_border());
                sFadeInBorder.init(pWrapper, as->fade_in_border());
                sFadeOutBorder.init(pWrapper, as->fade_out_border());
                sLineWidth.init(pWrapper, as->line_width());
                sMainText.init(pWrapper, as->main_text());
                sLabelRadius.init(pWrapper, as->label_radius());
                sBorder.init(pWrapper, as->border_size());
                sBorderRadius.init(pWrapper, as->border_radius());
                sActive.init(pWrapper, as->active());
                sStereoGroups.init(pWrapper, as->stereo_groups());

                sBorderFlat.init(pWrapper, as->border_flat());
                sGlass.init(pWrapper, as->glass());

                sIPadding.init(pWrapper, as->ipadding());

                sColor.init(pWrapper, as->color());
                sBorderColor.init(pWrapper, as->border_color());
                sGlassColor.init(pWrapper, as->glass_color());
                sLineColor.init(pWrapper, as->line_color());
                sMainColor.init(pWrapper, as->main_color());
                sLabelBgColor.init(pWrapper, as->label_bg_color());

                for (size_t i=0; i<tk::AudioSample::LABELS; ++i)
                {
                    sLabelVisibility[i].init(pWrapper, as->label_visibility(i));
                    sLabelTextColor[i].init(pWrapper, as->label_color(i));
                }
            }

            return STATUS_OK;
        }

        void AudioSample::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::AudioSample *as = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sWaveBorder.set("wave.border", name, value);
                sWaveBorder.set("wborder", name, value);
                sFadeInBorder.set("fadein.border", name, value);
                sFadeOutBorder.set("fadeout.border", name, value);
                sLineWidth.set("line.width", name, value);
                sLabelRadius.set("label.radius", name, value);
                sBorder.set("border.size", name, value);
                sBorderRadius.set("border.radius", name, value);

                sMainText.set("text.main", name, value);

                sActive.set("active", name, value);
                sStereoGroups.set("stereo.groups", name, value);
                sStereoGroups.set("sgroups", name, value);
                sBorderFlat.set("border.flat", name, value);
                sGlass.set("glass", name, value);

                sIPadding.set("ipadding", name, value);

                set_constraints(as->constraints(), name, value);
                set_text_layout(as->main_text_layout(), "text.layout.main", name, value);
                set_text_layout(as->main_text_layout(), "tlayout.main", name, value);
                set_text_layout(as->main_text_layout(), "text.main", name, value);
                set_font(as->main_font(), "font.main", name, value);
                set_font(as->label_font(), "label.font", name, value);
                set_layout(as->label_layout(0), "", name, value);

                LSPString prefix;
                for (size_t i=0, n=lsp_min(size_t(LBL_COUNT), tk::AudioSample::LABELS); i<n; ++i)
                {
                    prefix.fmt_ascii("%s.visibility", label_names[i]);
                    sLabelVisibility[i].set(prefix.get_ascii(), name, value);
                    prefix.fmt_ascii("label.%d.visibility", int(i));
                    sLabelVisibility[i].set(prefix.get_ascii(), name, value);

                    prefix.fmt_ascii("%s.text.color", label_names[i]);
                    sLabelTextColor[i].set(prefix.get_ascii(), name, value);
                    prefix.fmt_ascii("%s.tcolor", label_names[i]);
                    sLabelTextColor[i].set(prefix.get_ascii(), name, value);
                    prefix.fmt_ascii("label.%d.text.color", int(i));
                    sLabelTextColor[i].set(prefix.get_ascii(), name, value);
                    prefix.fmt_ascii("label.%d.tcolor", int(i));
                    sLabelTextColor[i].set(prefix.get_ascii(), name, value);

                    prefix.fmt_ascii("%s", label_names[i]);
                    set_layout(as->label_layout(i), prefix.get_ascii(), name, value);
                    prefix.fmt_ascii("label.%d", int(i));
                    set_layout(as->label_layout(i), prefix.get_ascii(), name, value);

                    prefix.fmt_ascii("%s.text.layout", label_names[i]);
                    set_text_layout(as->label_text_layout(i), prefix.get_ascii(), name, value);
                    prefix.fmt_ascii("%s.tlayout", label_names[i]);
                    set_text_layout(as->label_text_layout(i), prefix.get_ascii(), name, value);
                    prefix.fmt_ascii("%d.text.layout", int(i));
                    set_text_layout(as->label_text_layout(i), prefix.get_ascii(), name, value);
                    prefix.fmt_ascii("%d.tlayout", int(i));
                    set_text_layout(as->label_text_layout(i), prefix.get_ascii(), name, value);
                }

                sLabelRadius.init(pWrapper, as->label_radius());
                sBorder.init(pWrapper, as->border_size());
                sBorderRadius.init(pWrapper, as->border_radius());

                sColor.set("color", name, value);
                sBorderColor.set("border.color", name, value);
                sGlassColor.set("glass.color", name, value);
                sLineColor.set("line.color", name, value);
                sMainColor.set("main.color", name, value);
                sLabelBgColor.set("label.bg.color", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void AudioSample::schema_reloaded()
        {
            Widget::schema_reloaded();

            sColor.reload();
            sBorderColor.reload();
            sGlassColor.reload();
            sLineColor.reload();
            sMainColor.reload();
            sLabelBgColor.reload();

            for (size_t i=0, n=lsp_min(size_t(LBL_COUNT), tk::AudioSample::LABELS); i<n; ++i)
            {
                sLabelTextColor[i].reload();
            }
        }

    } // namespace ctl
} // namespace lsp

