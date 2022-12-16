/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
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

#ifndef PRIVATE_CTL_AUDIOFILEPREVIEW_H_
#define PRIVATE_CTL_AUDIOFILEPREVIEW_H_

#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        /** Audio file preview controller.
         * This class can be used as a controller around functions responsible for audio file preview and playback.
         */
        class AudioFilePreview: public ctl::Align
        {
            public:
                static const ctl_class_t metadata;

            protected:
                tk::Registry        vWidgets;
                ctl::Registry       vControllers;
                tk::Align           sRoot;              // Root widget

            protected:
                void                do_destroy();

            public:
                explicit AudioFilePreview(ui::IWrapper *src);
                virtual ~AudioFilePreview() override;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                // TODO: public interface for file manipulation
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* PRIVATE_CTL_AUDIOFILEPREVIEW_H_ */
