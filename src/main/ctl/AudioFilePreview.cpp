/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 16 дек. 2022 г.
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

#include <private/ctl/AudioFilePreview.h>
#include <private/ui/xml/RootNode.h>
#include <private/ui/xml/Handler.h>

#include <lsp-plug.in/mm/InAudioFileStream.h>
#include <stdarg.h>

namespace lsp
{
    namespace ctl
    {
        //-----------------------------------------------------------------
        // AudioFilePreview
        const ctl_class_t AudioFilePreview::metadata = { "AudioFilePreview", &Widget::metadata };

        AudioFilePreview::AudioFilePreview(ui::IWrapper *src):
            ctl::Align(src, &sRoot),
            sRoot(src->display())
        {
            pClass          = &metadata;
        }

        AudioFilePreview::~AudioFilePreview()
        {
            do_destroy();
        }

        void AudioFilePreview::do_destroy()
        {
            vControllers.destroy();
            vWidgets.destroy();
        }

        status_t AudioFilePreview::init()
        {
            status_t res;

            LSP_STATUS_ASSERT(ctl::Align::init());
            LSP_STATUS_ASSERT(sRoot.init());

            // Create context
            ui::UIContext uctx(pWrapper, &vControllers, &vWidgets);
            LSP_STATUS_ASSERT(uctx.init());

            // Parse the XML document
            ui::xml::RootNode root(&uctx, "preview", this);
            ui::xml::Handler handler(pWrapper->resources());
            res = handler.parse_resource(LSP_BUILTIN_PREFIX "ui/audio_file_preview.xml", &root);
            if (res != STATUS_OK)
                lsp_warn("Error parsing resource: %s, error: %d", LSP_BUILTIN_PREFIX "ui/audio_file_preview.xml", int(res));

            return res;
        }

        void AudioFilePreview::destroy()
        {
            do_destroy();
            ctl::Align::destroy();
        }

        void AudioFilePreview::activate()
        {
            unselect_file();
        }

        void AudioFilePreview::deactivate()
        {
            unselect_file();
        }

        void AudioFilePreview::unselect_file()
        {
            set_localized("audio_channels", NULL);
            set_localized("sample_rate", NULL);
            set_localized("sample_format", NULL);
            set_localized("duration", NULL);

            // TODO: stop playback on the DSP backend
        }

        void AudioFilePreview::select_file(const char *file)
        {
            io::Path tmp;
            if ((file == NULL) || (tmp.set(file) != STATUS_OK))
            {
                unselect_file();
                return;
            }
            select_file(&tmp);
        }

        void AudioFilePreview::select_file(const LSPString *file)
        {
            io::Path tmp;
            if ((file == NULL) || (file->is_empty()) || (tmp.set(file) != STATUS_OK))
            {
                unselect_file();
                return;
            }
            select_file(&tmp);
        }

        void AudioFilePreview::set_raw(const char *id, const char *fmt...)
        {
            // Find widget
            tk::Label *lbl = vWidgets.get<tk::Label>(id);
            if (lbl == NULL)
                return;

            // Do we need to reset the value?
            if (fmt == NULL)
            {
                lbl->text()->set("labels.file_preview.n_a");
                return;
            }

            // Format the value
            va_list args;
            va_start(args, fmt);
            lsp_finally { va_end(args); };

            LSPString tmp;
            if (tmp.vfmt_utf8(fmt, args))
                lbl->text()->set_raw(&tmp);
            else
                lbl->text()->set("labels.file_preview.n_a");
        }

        void AudioFilePreview::set_localized(const char *id, const char *key, const expr::Parameters *params)
        {
            // Find widget
            tk::Label *lbl = vWidgets.get<tk::Label>(id);
            if (lbl == NULL)
                return;

            // Do we need to reset the value?
            if (key == NULL)
            {
                lbl->text()->set("labels.file_preview.n_a");
                return;
            }

            // Commit the new text value
            if (lbl->text()->set(key, params) != STATUS_OK)
                lbl->text()->set("labels.file_preview.n_a");
        }

        void AudioFilePreview::select_file(const io::Path *file)
        {
            status_t res;
            if ((file == NULL) || (file->is_empty()) || (!file->is_reg()))
            {
                unselect_file();
                return;
            }

            lsp_trace("select file: %s", file->as_native());

            // Obtain the information about audio stream
            mm::audio_stream_t info;
            {
                mm::InAudioFileStream ifs;
                if ((res = ifs.open(file)) != STATUS_OK)
                {
                    unselect_file();
                    return;
                }
                lsp_finally{ ifs.close(); };

                if ((res = ifs.info(&info)) != STATUS_OK)
                {
                    unselect_file();
                    return;
                }
            }

            // Determine the time parameters
            wssize_t time       = (info.frames * 1000) / info.srate;
            size_t msec         = time % 1000;
            time               /= 1000;
            ssize_t seconds     = time % 60;
            time               /= 60;
            ssize_t minutes     = time % 60;
            ssize_t hours       = time / 60;

            expr::Parameters tparams;
            tparams.set_int("frames", info.frames);
            tparams.set_int("msec", msec);
            tparams.set_int("sec", seconds);
            tparams.set_int("min", minutes);
            tparams.set_int("hour", hours);
            const char *lc_key =
                (hours != 0) ? "labels.file_preview.time_hms" :
                (minutes != 0) ? "labels.file_preview.time_ms" :
                "labels.file_preview.time_s";

            expr::Parameters srparams;
            srparams.set_int("value", info.srate);

            // Estimate the sample format
            LSPString sfmt_key;
            const char *sfmt = NULL;
            switch (mm::sformat_format(info.format))
            {
                case mm::SFMT_U8: sfmt = "u8"; break;
                case mm::SFMT_S8: sfmt = "s8"; break;
                case mm::SFMT_U16: sfmt = "u16"; break;
                case mm::SFMT_S16: sfmt = "s16"; break;
                case mm::SFMT_U24: sfmt = "u24"; break;
                case mm::SFMT_S24: sfmt = "s24"; break;
                case mm::SFMT_U32: sfmt = "u32"; break;
                case mm::SFMT_S32: sfmt = "s32"; break;
                case mm::SFMT_F32: sfmt = "f32"; break;
                case mm::SFMT_F64: sfmt = "f64"; break;
                default: sfmt = "unknown"; break;
            }
            sfmt_key.fmt_ascii("labels.file_preview.sample_format.%s", sfmt);

            set_raw("audio_channels", "%d", int(info.channels));
            set_localized("sample_rate", "labels.values.x_hz", &srparams);
            set_localized("sample_format", sfmt_key.get_utf8());
            set_localized("duration", lc_key, &tparams);

            // Check the auyo-play option.
            ui::IPort *p = pWrapper->port(UI_PREVIEW_AUTO_PLAY_PORT);
            if ((p != NULL) && (p->value() >= 0.5f))
            {
                // TODO: trigger DSP backend to play the file
            }
        }
    } /* namespace ctl */
} /* namespace lsp */


