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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_SAMPLEPLAYER_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_SAMPLEPLAYER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/dsp-units/sampling/SamplePlayer.h>
#include <lsp-plug.in/ipc/ITask.h>
#include <lsp-plug.in/plug-fw/plug.h>


namespace lsp
{
    namespace core
    {
        /**
         * Sample player class for playing audio files on demand
         */
        class SamplePlayer
        {
            private:
                SamplePlayer & operator = (const SamplePlayer &);

            protected:
                class LoadTask: public ipc::ITask
                {
                    private:
                        SamplePlayer       *pCore;

                    public:
                        explicit LoadTask(SamplePlayer *core);
                        virtual ~LoadTask();

                    public:
                        virtual status_t        run();
                };

                class GCTask: public ipc::ITask
                {
                    private:
                        SamplePlayer       *pCore;

                    public:
                        explicit GCTask(SamplePlayer *core);
                        virtual ~GCTask();

                    public:
                        virtual status_t        run();
                };

            private:
                const meta::plugin_t   *pMetadata;
                plug::IWrapper         *pWrapper;
                LoadTask                sLoadTask;
                GCTask                  sGCTask;

                dspu::SamplePlayer      vPlayers[2];
                dspu::Playback          vPlaybacks[2];
                plug::IPort            *pOut[2];
                size_t                  nSampleRate;

                dspu::Sample           *pLoaded;                // Loaded sample
                dspu::Sample           *pGCList;                // Garbage collection

                wssize_t                nPlayPosition;          // Playback position
                wssize_t                nFileLength;            // Length of the file

                char                    sFileName[PATH_MAX];    // Actual file name
                char                    sReqFileName[PATH_MAX]; // Requested file name
                wsize_t                 nReqPosition;           // Requested playback position
                bool                    bReqRelease;            // Release request
                size_t                  nUpdateReq;             // Update request counter
                size_t                  nUpdateResp;            // Update response counter

            protected:
                static void destroy_sample(dspu::Sample * &sample);
                static void destroy_samples(dspu::Sample *gc_list);

                static plug::IPort *find_out_port(const char *id, plug::IPort **ports, size_t count);

            protected:
                void        connect_outputs(plug::IPort **ports, size_t count);
                status_t    load_sample();
                status_t    perform_gc();
                void        play_current_sample(wsize_t position);
                void        process_async_requests();
                void        process_gc_tasks();
                void        process_playback(size_t samples);

            public:
                explicit SamplePlayer(const meta::plugin_t *meta);
                ~SamplePlayer();

                void    init(plug::IWrapper *wrapper, plug::IPort **ports, size_t count);
                void    destroy();

            public:
                void    set_sample_rate(size_t sample_rate);
                void    process(size_t samples);
                void    play_sample(const char *file, wsize_t position, bool release);

                inline wssize_t position() const         { return nPlayPosition; }
                inline wssize_t sample_length() const    { return nFileLength;   }
        };

    } /* namespace core */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CORE_SAMPLEPLAYER_H_ */
