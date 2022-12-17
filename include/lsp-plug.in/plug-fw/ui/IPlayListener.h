/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 дек. 2022 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IPLAYLISTENER_H_
#define LSP_PLUG_IN_PLUG_FW_UI_IPLAYLISTENER_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>

namespace lsp
{
    namespace ui
    {
        /**
         * Playback position listener
         */
        class IPlayListener
        {
            public:
                virtual ~IPlayListener();

            public:
                /**
                 * Obtain the actual playback position
                 * @param position the actual playback position in samples
                 * @param length the overall length of the file in samples
                 */
                virtual void play_position_update(wsize_t position, wsize_t length);
        };

    } /* namespace ui */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UI_IPLAYLISTENER_H_ */
