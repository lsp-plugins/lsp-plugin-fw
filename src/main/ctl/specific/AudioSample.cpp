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
            pMeshPort       = NULL;
            pPathPort       = NULL;
            pDialog         = NULL;
        }

        AudioSample::~AudioSample()
        {
            if (pDialog != NULL)
            {
                pDialog->destroy();
                delete pDialog;
            }
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

                sStatus.init(pWrapper, this);
                sHeadCut.init(pWrapper, this);
                sTailCut.init(pWrapper, this);
                sFadeIn.init(pWrapper, this);
                sFadeOut.init(pWrapper, this);
                sLength.init(pWrapper, this);

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

                // Bind slot
                as->slots()->bind(tk::SLOT_SUBMIT, slot_audio_sample_submit, this);
                as->active()->set(true);
            }

            return STATUS_OK;
        }

        void AudioSample::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::AudioSample *as = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as != NULL)
            {
                bind_port(&pPort, "id", name, value);
                bind_port(&pMeshPort, "mesh_id", name, value);
                bind_port(&pMeshPort, "mesh.id", name, value);
                bind_port(&pPathPort, "path.id", name, value);
                bind_port(&pPathPort, "path_id", name, value);

                set_expr(&sStatus, "status", name, value);
                set_expr(&sHeadCut, "head_cut", name, value);
                set_expr(&sHeadCut, "hcut", name, value);
                set_expr(&sTailCut, "tail_cut", name, value);
                set_expr(&sTailCut, "tcut", name, value);
                set_expr(&sFadeIn, "fade_in", name, value);
                set_expr(&sFadeIn, "fadein", name, value);
                set_expr(&sFadeOut, "fade_out", name, value);
                set_expr(&sFadeOut, "fadeout", name, value);
                set_expr(&sLength, "length", name, value);

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

        void AudioSample::end(ui::UIContext *ctx)
        {
            sync_status();
            sync_labels();
            sync_mesh();

            Widget::end(ctx);
        }

        void AudioSample::notify(ui::IPort *port)
        {
            Widget::notify(port);
            if (port == NULL)
                return;

            if (sStatus.depends(port))
                sync_status();

            if ((port == pMeshPort) ||
                (sFadeIn.depends(port)) ||
                (sFadeOut.depends(port)))
                sync_mesh();

            if ((sFadeIn.depends(port)) ||
                (sFadeOut.depends(port)) ||
                (sHeadCut.depends(port)) ||
                (sTailCut.depends(port)) ||
                (sLength.depends(port)))
                sync_labels();

//            LSPAudioFile *af    = widget_cast<LSPAudioFile>(pWidget);
//            if (af == NULL)
//                return;
//
//            if (sFormat.valid())
//                af->filter()->set_default(sFormat.evaluate());
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

        void AudioSample::sync_status()
        {
            tk::AudioSample *as     = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as == NULL)
                return;

            // Analyze status
            size_t status           = sStatus.evaluate_int(STATUS_UNSPECIFIED);

            if (status == STATUS_OK)
            {
                as->main_visibility()->set(false);
                return;
            }

            as->main_visibility()->set(true);

            if (status == STATUS_UNSPECIFIED)
                as->main_text()->set("labels.click_or_drag_to_load");
            else if (status == STATUS_LOADING)
                as->main_text()->set("statuses.loading");
            else
            {
                LSPString code;
                code.set_utf8("statuses.std.");
                code.append_utf8(get_status_lc_key(status_t(status)));

                as->main_visibility()->set(true);
                as->main_text()->set(&code);
            }
        }

        void AudioSample::sync_labels()
        {
        }

        void AudioSample::sync_mesh()
        {
            plug::mesh_t *mesh = (pMeshPort != NULL) ? pMeshPort->buffer<plug::mesh_t>() : NULL;
            if (mesh == NULL)
                return;

            tk::AudioSample *as     = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as == NULL)
                return;

            // Remove extra managed channels
            size_t count = as->channels()->size();
            while (count > mesh->nBuffers)
                as->channels()->remove(--count);

            // Add new managed channels
            while (count < mesh->nBuffers)
            {
                // Create audio channel
                tk::AudioChannel *ac = new tk::AudioChannel(wWidget->display());
                if (ac == NULL)
                    return;
                if (ac->init() != STATUS_OK)
                {
                    ac->destroy();
                    delete ac;
                    return;
                }

                // Inject style
                LSPString style;
                style.fmt_ascii("AudioSample::Channel%d", int(count % 8) + 1);
                inject_style(ac, style.get_ascii());

                // Add audio channel as managed and increment counter
                as->channels()->madd(ac);
                ++count;
            }

            // Synchronize mesh state
            float length   = sLength.evaluate_float();
            float fade_in  = sFadeIn.evaluate_float() / length;
            float fade_out = sFadeOut.evaluate_float() / length;

            for (size_t i=0; i<mesh->nBuffers; ++i)
            {
                tk::AudioChannel *ac = as->channels()->get(i);
                if (ac == NULL)
                    continue;

                // Update mesh
                ac->samples()->set(mesh->pvData[i], mesh->nItems);

                // Update fades
                ac->fade_in()->set(fade_in);
                ac->fade_out()->set(fade_out);
            }
        }

        void AudioSample::show_file_dialog()
        {
            if (pDialog == NULL)
            {
                pDialog = new tk::FileDialog(wWidget->display());
                if (pDialog == NULL)
                    return;
                status_t res = pDialog->init();
                if (res != STATUS_OK)
                {
                    pDialog->destroy();
                    delete pDialog;
                    pDialog = NULL;
                    return;
                }

                pDialog->title()->set("titles.load_audio_file");
                tk::FileMask *ffi;

                if ((ffi = pDialog->filter()->add()) != NULL)
                {
                    ffi->pattern()->set("*.wav");
                    ffi->title()->set("files.audio.wave");
                    ffi->extensions()->set_raw(".wav");
                }
                if ((ffi = pDialog->filter()->add()) != NULL)
                {
                    ffi->pattern()->set("*");
                    ffi->title()->set("files.all");
                    ffi->extensions()->set_raw("");
                }

                pDialog->selected_filter()->set(0);

                pDialog->action_text()->set("actions.load");
                pDialog->slots()->bind(tk::SLOT_SUBMIT, slot_dialog_submit, this);
                pDialog->slots()->bind(tk::SLOT_HIDE, slot_dialog_hide, this);
            }

            const char *path = (pPathPort != NULL) ? pPathPort->buffer<char>() : NULL;
            if (path != NULL)
                pDialog->path()->set_raw(path);

            pDialog->show(wWidget);
        }

        void AudioSample::update_path()
        {
            if ((pPathPort == NULL) || (pDialog == NULL))
                return;

            // Obtain the current path from dialog
            LSPString path;
            if (pDialog->path()->format(&path) != STATUS_OK)
                return;
            if (path.length() <= 0)
                return;

            // Write new path as UTF-8 string
            const char *u8path = path.get_utf8();
            pPathPort->write(u8path, strlen(u8path));
            pPathPort->notify_all();
        }

        void AudioSample::commit_file()
        {
            if ((pPort== NULL) || (pDialog == NULL))
                return;

            LSPString path;
            if (pDialog->selected_file()->format(&path) != STATUS_OK)
                return;

            // Write new path as UTF-8 string
            const char *u8path = path.get_utf8();
            pPort->write(u8path, strlen(u8path));
            pPort->notify_all();
        }

        status_t AudioSample::slot_audio_sample_submit(tk::Widget *sender, void *ptr, void *data)
        {
            AudioSample *as = static_cast<AudioSample *>(ptr);
            if (as != NULL)
                as->show_file_dialog();

            return STATUS_OK;
        }

        status_t AudioSample::slot_dialog_submit(tk::Widget *sender, void *ptr, void *data)
        {
            AudioSample *as = static_cast<AudioSample *>(ptr);
            if (as != NULL)
                as->commit_file();

            return STATUS_OK;
        }

        status_t AudioSample::slot_dialog_hide(tk::Widget *sender, void *ptr, void *data)
        {
            AudioSample *as = static_cast<AudioSample *>(ptr);
            if (as != NULL)
                as->update_path();

            return STATUS_OK;
        }

    } // namespace ctl
} // namespace lsp

