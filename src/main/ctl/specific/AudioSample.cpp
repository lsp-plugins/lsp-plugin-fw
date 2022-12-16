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
#include <lsp-plug.in/fmt/config/Serializer.h>
#include <lsp-plug.in/fmt/config/PullParser.h>
#include <lsp-plug.in/fmt/url.h>
#include <lsp-plug.in/io/InStringSequence.h>
#include <lsp-plug.in/expr/Tokenizer.h>

#include <private/ctl/AudioFilePreview.h>

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
        AudioSample::DataSink::DataSink(AudioSample *sample)
        {
            pSample         = sample;
        }

        AudioSample::DataSink::~DataSink()
        {
            unbind();
        }

        status_t AudioSample::DataSink::receive(const LSPString *text, const char *mime)
        {
            if (pSample == NULL)
                return STATUS_OK;

            // Apply configuration
            config::PullParser p;
            if (p.wrap(text) != STATUS_OK)
                return STATUS_OK;

            config::param_t param;
            while (p.next(&param) == STATUS_OK)
            {
                if ((param.name.equals_ascii("file")) && (param.is_string()) && (pSample->pPort != NULL))
                {
                    pSample->pPort->write(param.v.str, strlen(param.v.str));
                    pSample->pPort->notify_all();
                }
                else if (param.is_numeric())
                {
                    const char *pname = param.name.get_utf8();
                    ui::IPort *port = pSample->vClipboardBind.get(pname);
                    if (port != NULL)
                    {
                        port->set_value(param.to_f32());
                        port->notify_all();
                    }
                }
            }

            return STATUS_OK;
        }

        status_t AudioSample::DataSink::error(status_t code)
        {
            unbind();
            return STATUS_OK;
        }

        void AudioSample::DataSink::unbind()
        {
            if (pSample != NULL)
            {
                if (pSample->pDataSink == this)
                    pSample->pDataSink  = NULL;
                pSample = NULL;
            }
        }

        //-----------------------------------------------------------------
        AudioSample::DragInSink::DragInSink(AudioSample *sample)
        {
            pSample     = sample;
        }

        AudioSample::DragInSink::~DragInSink()
        {
            unbind();
        }

        void AudioSample::DragInSink::unbind()
        {
            if (pSample != NULL)
            {
                if (pSample->pDragInSink == this)
                    pSample->pDragInSink    = NULL;
                pSample = NULL;
            }
        }

        status_t AudioSample::DragInSink::commit_url(const LSPString *url)
        {
            if ((url == NULL) || (pSample->pPort == NULL))
                return STATUS_OK;

            LSPString decoded;
            status_t res = (url->starts_with_ascii("file://")) ?
                    url::decode(&decoded, url, 7) :
                    url::decode(&decoded, url);

            if (res != STATUS_OK)
                return res;

            lsp_trace("Set file path to %s", decoded.get_native());
            const char *path = decoded.get_utf8();

            pSample->pPort->write(path, strlen(path));
            pSample->pPort->notify_all();

            return STATUS_OK;
        }

        //-----------------------------------------------------------------
        static const char *label_names[] =
        {
            "fname",
            "duration",
            "head",
            "tail",
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
            pFilePreview    = NULL;
            pMenu           = NULL;
            pDataSink       = NULL;
            pDragInSink     = NULL;
            bFullSample     = false;
            bLoadPreview    = false;
        }

        AudioSample::~AudioSample()
        {
            tk::AudioSample *as     = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as != NULL)
                as->channels()->flush();

            // Destroy sink
            DragInSink *sink = pDragInSink;
            if (sink != NULL)
            {
                sink->unbind();
                sink->release();
                sink   = NULL;
            }

            // Destroy dialog
            if (pDialog != NULL)
            {
                pDialog->destroy();
                delete pDialog;
                pDialog     = NULL;
            }

            // Destroy menu items
            for (size_t i=0, n=vMenuItems.size(); i<n; ++i)
            {
                tk::MenuItem *mi = vMenuItems.uget(i);
                if (mi != NULL)
                {
                    mi->destroy();
                    delete mi;
                }
            }
            vMenuItems.flush();

            // Destroy menu
            if (pMenu != NULL)
            {
                pMenu->destroy();
                delete pMenu;
                pMenu       = NULL;
            }

            vClipboardBind.flush();
        }

        status_t AudioSample::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            pDragInSink = new DragInSink(this);
            if (pDragInSink == NULL)
                return STATUS_NO_MEM;
            pDragInSink->acquire();

            tk::AudioSample *as = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as != NULL)
            {
                sWaveBorder.init(pWrapper, as->wave_border());
                sFadeInBorder.init(pWrapper, as->fade_in_border());
                sFadeOutBorder.init(pWrapper, as->fade_out_border());
                sStretchBorder.init(pWrapper, as->stretch_border());
                sLoopBorder.init(pWrapper, as->loop_border());
                sPlayBorder.init(pWrapper, as->play_border());
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
                sStretch.init(pWrapper, this);
                sStretchBegin.init(pWrapper, this);
                sStretchEnd.init(pWrapper, this);
                sLoop.init(pWrapper, this);
                sLoopBegin.init(pWrapper, this);
                sLoopEnd.init(pWrapper, this);
                sPlayPosition.init(pWrapper, this);
                sLength.init(pWrapper, this);
                sActualLength.init(pWrapper, this);

                sColor.init(pWrapper, as->color());
                sBorderColor.init(pWrapper, as->border_color());
                sGlassColor.init(pWrapper, as->glass_color());
                sLineColor.init(pWrapper, as->line_color());
                sMainColor.init(pWrapper, as->main_color());
                sStretchColor.init(pWrapper, as->stretch_color());
                sStretchBorderColor.init(pWrapper, as->stretch_border_color());
                sLoopColor.init(pWrapper, as->loop_color());
                sLoopBorderColor.init(pWrapper, as->loop_border_color());
                sPlayColor.init(pWrapper, as->play_color());
                sLabelBgColor.init(pWrapper, as->label_bg_color());

                for (size_t i=0; i<tk::AudioSample::LABELS; ++i)
                {
                    sLabelVisibility[i].init(pWrapper, as->label_visibility(i));
                    sLabelTextColor[i].init(pWrapper, as->label_color(i));
                }

                // By default use 'wav' and 'all' file formats
                parse_file_formats(&vFormats, "wav,all");

                // Bind slot
                as->slots()->bind(tk::SLOT_SUBMIT, slot_audio_sample_submit, this);
                as->slots()->bind(tk::SLOT_DRAG_REQUEST, slot_drag_request, this);
                as->active()->set(true);

                // Create menu item
                as->popup()->set(create_menu());

                // Init labels
                for (size_t i=0, n=lsp_min(size_t(LBL_COUNT), tk::AudioSample::LABELS); i<n; ++i)
                {
                    LSPString key;
                    key.fmt_ascii("labels.asample.%s", label_names[i]);
                    as->label(i)->set(&key);
                }
            }

            return STATUS_OK;
        }

        void AudioSample::destroy()
        {
            // Destroy the file preview
            if (pFilePreview != NULL)
            {
                pFilePreview->destroy();
                delete pFilePreview;
                pFilePreview = NULL;
            }

            ctl::Widget::destroy();
        }

        tk::MenuItem *AudioSample::create_menu_item(tk::Menu *menu)
        {
            tk::MenuItem *mi = new tk::MenuItem(wWidget->display());
            if (mi == NULL)
                return NULL;
            if (mi->init() != STATUS_OK)
            {
                mi->destroy();
                delete mi;
                return NULL;
            }
            if (!vMenuItems.add(mi))
            {
                mi->destroy();
                delete mi;
                return NULL;
            }

            return (menu->add(mi) == STATUS_OK) ? mi : NULL;
        }

        tk::Menu *AudioSample::create_menu()
        {
            // Initialize menu
            pMenu = new tk::Menu(wWidget->display());
            if (pMenu == NULL)
                return NULL;
            if (pMenu->init() != STATUS_OK)
            {
                pMenu->destroy();
                delete pMenu;
                return pMenu = NULL;
            }

            // Fill items
            tk::MenuItem *mi;
            if ((mi = create_menu_item(pMenu)) == NULL)
                return pMenu;
            mi->text()->set("actions.edit.cut");
            mi->slots()->bind(tk::SLOT_SUBMIT, slot_popup_cut_action, this);

            if ((mi = create_menu_item(pMenu)) == NULL)
                return pMenu;
            mi->text()->set("actions.edit.copy");
            mi->slots()->bind(tk::SLOT_SUBMIT, slot_popup_copy_action, this);

            if ((mi = create_menu_item(pMenu)) == NULL)
                return pMenu;
            mi->text()->set("actions.edit.paste");
            mi->slots()->bind(tk::SLOT_SUBMIT, slot_popup_paste_action, this);

            if ((mi = create_menu_item(pMenu)) == NULL)
                return pMenu;
            mi->text()->set("actions.edit.clear");
            mi->slots()->bind(tk::SLOT_SUBMIT, slot_popup_clear_action, this);

            return pMenu;
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
                set_expr(&sFadeIn, "fade.in", name, value);
                set_expr(&sFadeOut, "fade_out", name, value);
                set_expr(&sFadeOut, "fadeout", name, value);
                set_expr(&sFadeOut, "fade.out", name, value);
                set_expr(&sStretch, "stretch.enable", name, value);
                set_expr(&sStretch, "stretch.enabled", name, value);
                set_expr(&sStretchBegin, "stretch.begin", name, value);
                set_expr(&sStretchEnd, "stretch.end", name, value);
                set_expr(&sLoop, "loop.enable", name, value);
                set_expr(&sLoop, "loop.enabled", name, value);
                set_expr(&sLoopBegin, "loop.begin", name, value);
                set_expr(&sLoopEnd, "loop.end", name, value);
                set_expr(&sPlayPosition, "play.position", name, value);
                set_expr(&sLength, "length", name, value);
                set_expr(&sActualLength, "length.actual", name, value);

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

                set_value(&bFullSample, "sample.full", name, value);
                set_value(&bLoadPreview, "load.preview", name, value);
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
                sStretchColor.set("stretch.color", name, value);
                sStretchBorderColor.set("stretch.border.color", name, value);
                sLoopColor.set("loop.color", name, value);
                sLoopBorderColor.set("loop.border.color", name, value);

                // Parse file formats
                if ((!strcmp(name, "format")) || (!strcmp(name, "formats")) || (!strcmp(name, "fmt")))
                    parse_file_formats(&vFormats, value);

                // Process clipboard bindings
                const char *bind = match_prefix("clipboard", name);
                if ((bind != NULL) && (strlen(bind) > 0))
                {
                    ui::IPort *port = pWrapper->port(value);
                    if (port != NULL)
                        vClipboardBind.create(bind, port);
                }
            }

            return Widget::set(ctx, name, value);
        }

        void AudioSample::end(ui::UIContext *ctx)
        {
            sync_status();
            sync_mesh();
            sync_labels();
            sync_markers();

            Widget::end(ctx);
        }

        void AudioSample::notify(ui::IPort *port)
        {
            Widget::notify(port);
            if (port == NULL)
                return;

            if (sStatus.depends(port))
                sync_status();

            if (port == pMeshPort)
                sync_mesh();

            if ((port == pMeshPort) ||
                (port == pPort) ||
                (sFadeIn.depends(port)) ||
                (sFadeOut.depends(port)) ||
                (sStretch.depends(port)) ||
                (sStretchBegin.depends(port)) ||
                (sStretchEnd.depends(port)) ||
                (sLoop.depends(port)) ||
                (sLoopBegin.depends(port)) ||
                (sLoopEnd.depends(port)) ||
                (sPlayPosition.depends(port)) ||
                (sHeadCut.depends(port)) ||
                (sTailCut.depends(port)) ||
                (sLength.depends(port)) ||
                (sActualLength.depends(port)))
            {
                sync_labels();
                sync_markers();
            }
        }

        void AudioSample::reloaded(const tk::StyleSheet *sheet)
        {
            Widget::reloaded(sheet);

            sync_status();
            sync_mesh();
            sync_labels();
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

            revoke_style(as, "AudioSample::ok");
            revoke_style(as, "AudioSample::info");
            revoke_style(as, "AudioSample::error");

            if (status == STATUS_UNSPECIFIED)
            {
                inject_style(as, "AudioSample::ok");
                as->main_text()->set("labels.click_or_drag_to_load");
            }
            else if (status == STATUS_LOADING)
            {
                inject_style(as, "AudioSample::info");
                as->main_text()->set("statuses.loading");
            }
            else
            {
                LSPString code;
                code.set_utf8("statuses.std.");
                code.append_utf8(get_status_lc_key(status_t(status)));

                inject_style(as, "AudioSample::error");
                as->main_visibility()->set(true);
                as->main_text()->set(&code);
            }
        }

        void AudioSample::sync_labels()
        {
            tk::AudioSample *as     = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as == NULL)
                return;

            io::Path fpath;
            if (pPort != NULL)
            {
                const char *path = pPort->buffer<char>();
                if (path == NULL)
                    path    = "";
                fpath.set(path);
            }

            // Set different parameters for string
            for (size_t i=0, n=lsp_min(size_t(LBL_COUNT), tk::AudioSample::LABELS); i<n; ++i)
            {
                tk::String *dst = as->label(i);
                expr::Parameters *p = dst->params();

                float length    = sLength.evaluate_float();
                float head_cut  = sHeadCut.evaluate_float();
                float tail_cut  = sTailCut.evaluate_float();
                float length_cut= lsp_max(0.0f, length - head_cut - tail_cut);
                float actual_len= sActualLength.evaluate_float(length_cut);
                float fade_in   = sFadeIn.evaluate_float();
                float fade_out  = sFadeOut.evaluate_float();
                float s_begin   = sStretchBegin.evaluate_float();
                float s_end     = sStretchEnd.evaluate_float();
                float l_begin   = sLoopBegin.evaluate_float();
                float l_end     = sLoopEnd.evaluate_float();
                float pp        = sPlayPosition.evaluate_float();

                p->set_float("length", length);
                p->set_float("head_cut", head_cut);
                p->set_float("tail_cut", tail_cut);
                p->set_float("length_cut", actual_len);
                p->set_float("fade_in", fade_in);
                p->set_float("fade_out", fade_out);
                p->set_float("stretch_begin", s_begin);
                p->set_float("stretch_end", s_end);
                p->set_float("loop_begin", l_begin);
                p->set_float("loop_end", l_end);
                p->set_float("play_position", pp);

                LSPString tmp;
                p->set_string("file", fpath.as_string());
                fpath.get_last(&tmp);
                p->set_string("file_name", &tmp);
                fpath.get_parent(&tmp);
                p->set_string("file_dir", &tmp);
                fpath.get_ext(&tmp);
                p->set_string("file_ext", &tmp);
                fpath.get_noext(&tmp);
                p->set_string("file_noext", &tmp);
            }
        }

        void AudioSample::sync_markers()
        {
            plug::mesh_t *mesh = (pMeshPort != NULL) ? pMeshPort->buffer<plug::mesh_t>() : NULL;
            if (mesh == NULL)
                return;

            tk::AudioSample *as     = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as == NULL)
                return;

            size_t channels     = (mesh->nBuffers & 1) ? mesh->nBuffers + 1 : mesh->nBuffers;

            // Synchronize mesh state
            size_t samples  = mesh->nItems;
            float length = 0.0f;
            float head_cut = 0.0f, tail_cut = 0.0f;
            float fade_in = 0.0f, fade_out = 0.0f;
            float s_begin = -1.0f, s_end = -1.0f;
            float l_begin = -1.0f, l_end = -1.0f;
            float pp = sPlayPosition.evaluate_float(-1.0f);
            bool s_on = sStretch.evaluate_bool(false);
            bool l_on = sLoop.evaluate_bool(false);

            if (bFullSample)
            {
                length      = sLength.evaluate_float();
                float kl    = samples / length;

                fade_in     = kl * sFadeIn.evaluate_float();
                fade_out    = kl * sFadeOut.evaluate_float();
                head_cut    = kl * sHeadCut.evaluate_float(0.0f);
                tail_cut    = kl * sTailCut.evaluate_float(0.0f);
                s_begin     = ((s_on) && (length > 0)) ? kl * sStretchBegin.evaluate_float(-1.0f) : -1.0f;
                s_end       = ((s_on) && (length > 0)) ? kl * sStretchEnd.evaluate_float(-1.0f)   : -1.0f;
                l_begin     = ((l_on) && (length > 0)) ? kl * sLoopBegin.evaluate_float(-1.0f)    : -1.0f;
                l_end       = ((l_on) && (length > 0)) ? kl * sLoopEnd.evaluate_float(-1.0f)      : -1.0f;
                pp          = ((pp >= 0) && (length > 0)) ? kl * pp : -1.0f;
            }
            else
            {
                length      = sLength.evaluate_float() - sHeadCut.evaluate_float() - sTailCut.evaluate_float();
                float kl    = samples / length;

                fade_in     = (length > 0) ? kl * sFadeIn.evaluate_float() : 0.0f;
                fade_out    = (length > 0) ? kl * sFadeOut.evaluate_float() : 0.0f;
                s_begin     = ((s_on) && (length > 0)) ? kl * sStretchBegin.evaluate_float(-1.0f) : -1.0f;
                s_end       = ((s_on) && (length > 0)) ? kl * sStretchEnd.evaluate_float(-1.0f)   : -1.0f;
                l_begin     = ((l_on) && (length > 0)) ? kl * sLoopBegin.evaluate_float(-1.0f) : -1.0f;
                l_end       = ((l_on) && (length > 0)) ? kl * sLoopEnd.evaluate_float(-1.0f)   : -1.0f;
                pp          = ((pp >= 0) && (length > 0)) ? kl * pp : -1.0f;
            }

            // Configure the values
            if (s_begin >= 0.0f)
                s_begin     = lsp_limit(s_begin, 0.0f, length);
            if (s_end >= 0.0f)
                s_end       = lsp_limit(s_end, 0.0f, length);
            if (l_begin >= 0.0f)
                l_begin     = lsp_limit(l_begin, 0.0f, length);
            if (l_end >= 0.0f)
                l_end       = lsp_limit(l_end, 0.0f, length);
            if (s_end < s_begin)
                lsp::swap(s_begin, s_end);
            if (l_end < l_begin)
                lsp::swap(l_begin, l_end);

//            lsp_trace("head_cut=%f, tail_cut=%f, pp = %f", head_cut, tail_cut, pp);

            for (size_t i=0; i<channels; ++i)
            {
                tk::AudioChannel *ac = as->channels()->get(i);
                if (ac == NULL)
                    continue;

                // Update fades and markers
                ac->fade_in()->set(fade_in);
                ac->fade_out()->set(fade_out);
                ac->stretch_begin()->set(s_begin);
                ac->stretch_end()->set(s_end);
                ac->loop_begin()->set(l_begin);
                ac->loop_end()->set(l_end);
                ac->head_cut()->set(head_cut);
                ac->tail_cut()->set(tail_cut);
                ac->play_position()->set(pp);
//                lsp_trace("actual play position: %d", int(ac->play_position()->get()));
            }
        }

        void AudioSample::sync_mesh()
        {
            plug::mesh_t *mesh = (pMeshPort != NULL) ? pMeshPort->buffer<plug::mesh_t>() : NULL;
            if (mesh == NULL)
                return;

            tk::AudioSample *as     = tk::widget_cast<tk::AudioSample>(wWidget);
            if (as == NULL)
                return;

            // Recreate channels
            as->channels()->clear();
            size_t channels = (mesh->nBuffers & 1) ? mesh->nBuffers + 1 : mesh->nBuffers;
            size_t samples  = mesh->nItems;

            // Add new managed channels
            for (size_t i=0; i < channels; ++i)
            {
                size_t src_idx = lsp_min(i, mesh->nBuffers-1);

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

                ac->samples()->set(mesh->pvData[src_idx], samples);

                // Inject style
                LSPString style;
                style.fmt_ascii("AudioSample::Channel%d", int(src_idx % 8) + 1);
                inject_style(ac, style.get_ascii());

                // Add audio channel as managed and increment counter
                as->channels()->madd(ac);
            }
        }

        void AudioSample::show_file_dialog()
        {
            if (pDialog == NULL)
            {
                tk::FileDialog *dlg = new tk::FileDialog(wWidget->display());
                if (dlg == NULL)
                    return;
                lsp_finally {
                    if (dlg != NULL)
                    {
                        dlg->destroy();
                        delete dlg;
                    }
                };
                status_t res = dlg->init();
                if (res != STATUS_OK)
                    return;

                dlg->title()->set("titles.load_audio_file");
                dlg->mode()->set(tk::FDM_OPEN_FILE);
                tk::FileMask *ffi;

                // Add all listed formats
                for (size_t i=0, n=vFormats.size(); i<n; ++i)
                {
                    file_format_t *f = vFormats.uget(i);
                    if ((ffi = dlg->filter()->add()) != NULL)
                    {
                        ffi->pattern()->set(f->filter, f->flags);
                        ffi->title()->set(f->title);
                        ffi->extensions()->set_raw(f->extension);
                    }
                }

                dlg->selected_filter()->set(0);

                dlg->action_text()->set("actions.load");
                dlg->slots()->bind(tk::SLOT_SUBMIT, slot_dialog_submit, this);
                dlg->slots()->bind(tk::SLOT_HIDE, slot_dialog_hide, this);

                // Commit the change
                lsp::swap(pDialog, dlg);
            }

            if ((bLoadPreview) && (pFilePreview == NULL))
            {
                ctl::Widget *pw = new AudioFilePreview(pWrapper);
                if (pw == NULL)
                    return;
                lsp_finally {
                    if (pw != NULL)
                    {
                        pw->destroy();
                        delete pw;
                    }
                };

                status_t res = pw->init();
                if (res != STATUS_OK)
                    return;

                lsp::swap(pFilePreview, pw);
            }

            const char *path = (pPathPort != NULL) ? pPathPort->buffer<char>() : NULL;
            if (path != NULL)
                pDialog->path()->set_raw(path);

            pDialog->preview()->set((pFilePreview != NULL) ? pFilePreview->widget() : NULL);
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
            AudioSample *_this = static_cast<AudioSample *>(ptr);
            if (_this != NULL)
                _this->show_file_dialog();

            return STATUS_OK;
        }

        status_t AudioSample::slot_dialog_submit(tk::Widget *sender, void *ptr, void *data)
        {
            AudioSample *_this = static_cast<AudioSample *>(ptr);
            if (_this != NULL)
                _this->commit_file();

            return STATUS_OK;
        }

        status_t AudioSample::slot_dialog_hide(tk::Widget *sender, void *ptr, void *data)
        {
            AudioSample *_this = static_cast<AudioSample *>(ptr);
            if (_this != NULL)
                _this->update_path();

            return STATUS_OK;
        }

        status_t AudioSample::slot_popup_cut_action(tk::Widget *sender, void *ptr, void *data)
        {
            LSP_STATUS_ASSERT(slot_popup_copy_action(sender, ptr, data));
            return slot_popup_clear_action(sender, ptr, data);
        }

        status_t AudioSample::slot_popup_copy_action(tk::Widget *sender, void *ptr, void *data)
        {
            AudioSample *_this      = static_cast<AudioSample *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_ARGUMENTS;
            tk::AudioSample *as     = tk::widget_cast<tk::AudioSample>(_this->wWidget);
            if (as == NULL)
                return STATUS_BAD_STATE;

            status_t res;
            LSPString str;
            config::Serializer s;
            if ((res = s.wrap(&str)) == STATUS_OK)
            {
                if (_this->pPort != NULL)
                {
                    const char *value = _this->pPort->buffer<char>();
                    s.write_string("file", value, config::SF_QUOTED);
                }

                lltl::parray<char> keys;
                lltl::parray<ui::IPort> values;
                _this->vClipboardBind.items(&keys, &values);
                for (size_t i=0, n=keys.size(); i<n; ++i)
                {
                    const char *key = keys.uget(i);
                    ui::IPort *value = values.uget(i);
                    if ((key == NULL) || (value == NULL))
                        continue;

                    s.write_f32(key, value->value(), config::SF_NONE);
                }

                lsp_trace("Serialized config: \n%s", str.get_native());

                // Copy data to clipboard
                tk::TextDataSource *ds = new tk::TextDataSource();
                if (ds == NULL)
                    return STATUS_NO_MEM;
                ds->acquire();

                if ((res = ds->set_text(&str)) == STATUS_OK)
                    as->display()->set_clipboard(ws::CBUF_CLIPBOARD, ds);
                ds->release();
            }

            return res;
        }

        status_t AudioSample::slot_popup_paste_action(tk::Widget *sender, void *ptr, void *data)
        {
            AudioSample *_this  = static_cast<AudioSample *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_ARGUMENTS;
            tk::AudioSample *as = tk::widget_cast<tk::AudioSample>(_this->wWidget);
            if (as == NULL)
                return STATUS_BAD_STATE;

            // Fetch data from clipboard
            DataSink *ds = new DataSink(_this);
            if (ds == NULL)
                return STATUS_NO_MEM;
            if (_this->pDataSink != NULL)
                _this->pDataSink->unbind();
            _this->pDataSink = ds;

            ds->acquire();
            status_t res = as->display()->get_clipboard(ws::CBUF_CLIPBOARD, ds);
            ds->release();
            return res;
        }

        status_t AudioSample::slot_popup_clear_action(tk::Widget *sender, void *ptr, void *data)
        {
            AudioSample *_this  = static_cast<AudioSample *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_ARGUMENTS;
            if (_this->pPort != NULL)
            {
                // Write new path as UTF-8 string
                _this->pPort->write("", 0);
                _this->pPort->notify_all();
            }

            return STATUS_OK;
        }

        status_t AudioSample::slot_drag_request(tk::Widget *sender, void *ptr, void *data)
        {
            AudioSample *_this  = static_cast<AudioSample *>(ptr);
            if (_this == NULL)
                return STATUS_BAD_ARGUMENTS;

            tk::Display *dpy    = (_this->wWidget != NULL) ? _this->wWidget->display() : NULL;
            if (dpy == NULL)
                return STATUS_BAD_STATE;

            ws::rectangle_t r;
            _this->wWidget->get_rectangle(&r);

            const char * const *ctype = dpy->get_drag_mime_types();
            ssize_t idx = _this->pDragInSink->select_mime_type(ctype);
            if (idx >= 0)
            {
                dpy->accept_drag(_this->pDragInSink, ws::DRAG_COPY, &r);
                lsp_trace("Accepted drag");
            }
            else
            {
                dpy->reject_drag();
                lsp_trace("Rejected drag");
            }

            return STATUS_OK;
        }

    } // namespace ctl
} // namespace lsp

