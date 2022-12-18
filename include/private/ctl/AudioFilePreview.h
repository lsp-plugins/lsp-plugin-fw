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
        class AudioFilePreview: public ctl::Align, public ui::IPlayListener
        {
            public:
                static const ctl_class_t metadata;

            protected:
                tk::Registry        vWidgets;
                ctl::Registry       vControllers;
                tk::Align           sRoot;              // Root widget
                io::Path            sFile;              // File for the playback
                wsize_t             nPlayPosition;      // Play position
                wsize_t             nFileLength;        // File length
                bool                bPlaying;           // Playing flag

            protected:
                void                do_destroy();
                void                set_raw(const char *id, const char *fmt...);
                void                set_localized(const char *id, const char *key, const expr::Parameters *params = NULL);
                void                update_play_button(bool playing);
                void                on_play_pause_submitted();
                void                on_stop_submitted();
                void                on_play_position_changed();
                tk::handler_id_t    bind_slot(const char *id, tk::slot_t slot, tk::event_handler_t handler);

            protected:
                static status_t     slot_play_pause_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_stop_submit(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_play_position_change(tk::Widget *sender, void *ptr, void *data);

            public:
                explicit AudioFilePreview(ui::IWrapper *src);
                virtual ~AudioFilePreview() override;

                virtual status_t    init() override;
                virtual void        destroy() override;

            public:
                virtual void        play_position_update(wssize_t position, wssize_t length) override;

            public:
                /**
                 * Do the actions before the file dialog becomes visible
                 */
                void                activate();

                /**
                 * Do the actions after the file dialog becomes invisible
                 */
                void                deactivate();

                /**
                 * Select the specific file for playback
                 * @param file file to select
                 */
                void                select_file(const char *file);
                void                select_file(const LSPString *file);
                void                select_file(const io::Path *file);

                /**
                 * Unselect currently selected file
                 */
                void                unselect_file();
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* PRIVATE_CTL_AUDIOFILEPREVIEW_H_ */
