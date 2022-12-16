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

    } /* namespace ctl */
} /* namespace lsp */


