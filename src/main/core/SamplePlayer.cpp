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

#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace core
    {
        //-------------------------------------------------------------------------
        SamplePlayer::LoadTask::LoadTask(SamplePlayer *core)
        {
            pCore       = core;
        }

        SamplePlayer::LoadTask::~LoadTask()
        {
            pCore       = NULL;
        }

        status_t SamplePlayer::LoadTask::run()
        {
            return pCore->load_sample();
        };

        //-------------------------------------------------------------------------
        SamplePlayer::GCTask::GCTask(SamplePlayer *core)
        {
            pCore       = core;
        }

        SamplePlayer::GCTask::~GCTask()
        {
            pCore       = NULL;
        }

        status_t SamplePlayer::GCTask::run()
        {
            return pCore->perform_gc();
        };

        //-------------------------------------------------------------------------
        SamplePlayer::SamplePlayer(const meta::plugin_t *plugin):
            sLoadTask(this),
            sGCTask(this)
        {
            pMetadata       = plugin;
            pWrapper        = NULL;

            pOut[0]         = NULL;
            pOut[1]         = NULL;
            nSampleRate     = 0;

            pLoaded         = NULL;
            pGCList         = NULL;

            nPlayPosition   = 0;
            nFileLength     = 0;

            sFileName[0]    = '\0';
            nReqPosition    = 0;
            bReqRelease     = false;
            nUpdateReq      = 0;
            nUpdateResp     = 0;
        }

        SamplePlayer::~SamplePlayer()
        {
            destroy();
        }

        void SamplePlayer::destroy_sample(dspu::Sample * &sample)
        {
            if (sample == NULL)
                return;

            // Destroy the sample
            sample->destroy();
            delete sample;
            lsp_trace("Destroyed sample %p", sample);
            sample  = NULL;
        }

        plug::IPort *SamplePlayer::find_out_port(const char *id, plug::IPort **ports, size_t count)
        {
            for (size_t i=0; i<count; ++i)
            {
                const meta::port_t *pmeta = ports[i]->metadata();
                if (!meta::is_audio_out_port(pmeta))
                    continue;
                if (!strcmp(pmeta->id, id))
                    return ports[i];
            }
            return NULL;
        }

        void SamplePlayer::destroy()
        {
            // Destroy playbacks
            for (size_t i=0; i<2; ++i)
                vPlaybacks[i].clear();

            // Perform garbage collection for each channel
            for (size_t i=0; i<2; ++i)
            {
                dspu::Sample *gc_list = vPlayers[0].destroy(false);
                destroy_samples(gc_list);
                pOut[i]     = NULL;
            }

            // Perform pending gabrage collection
            perform_gc();
        }

        void SamplePlayer::destroy_samples(dspu::Sample *gc_list)
        {
            // Iterate over the list and destroy each sample in the list
            while (gc_list != NULL)
            {
                dspu::Sample *next = gc_list->gc_next();
                destroy_sample(gc_list);
                gc_list = next;
            }
        }

        void SamplePlayer::connect_outputs(plug::IPort **ports, size_t count)
        {
            // No ports connected
            pOut[0]     = NULL;
            pOut[1]     = NULL;

            // Try to connect outputs using port groups
            for (const meta::port_group_t *pg = pMetadata->port_groups; (pg != NULL) && (pg->id != NULL); ++pg)
            {
                if (pg->flags != (meta::PGF_OUT | meta::PGF_MAIN))
                    continue;

                if (pg->type == meta::GRP_MONO)
                {
                    for (const meta::port_group_item_t *pgi = pg->items; (pgi != NULL) && (pgi->id != NULL); ++pgi)
                    {
                        if (pgi->role == meta::PGR_CENTER)
                            pOut[0]     = find_out_port(pgi->id, ports, count);
                    }
                    return;
                }
                else // Consider the port group has at least two outputs
                {
                    for (const meta::port_group_item_t *pgi = pg->items; (pgi != NULL) && (pgi->id != NULL); ++pgi)
                    {
                        if (pgi->role == meta::PGR_LEFT)
                            pOut[0]     = find_out_port(pgi->id, ports, count);
                        else if (pgi->role == meta::PGR_RIGHT)
                            pOut[1]     = find_out_port(pgi->id, ports, count);
                    }
                    return;
                }
            }

            // No port groups found, try to connect as much as possible
            size_t connected = 0;
            for (size_t i=0; i<count; ++i)
            {
                if (!meta::is_audio_out_port(ports[i]->metadata()))
                    continue;
                pOut[connected++]   = ports[i];
                if (connected >= 2)
                    break;
            }
        }

        void SamplePlayer::init(plug::IWrapper *wrapper, plug::IPort **ports, size_t count)
        {
            // Remember the wrapper
            pWrapper = wrapper;

            // Connect output ports
            connect_outputs(ports, count);

            // Initialize sample player
            for (size_t i=0; i<2; ++i)
                vPlayers[i].init(1, 1);
        }

        status_t SamplePlayer::load_sample()
        {
            // Load sample
            lsp_trace("file = %s", sFileName);

            // Destroy previously loaded sample
            destroy_sample(pLoaded);

            // Load audio file
            dspu::Sample *source    = new dspu::Sample();
            if (source == NULL)
                return STATUS_NO_MEM;
            lsp_trace("Allocated sample %p", source);
            lsp_finally { destroy_sample(source); };

            // Load sample
            status_t res = source->load_ext(sFileName);
            if (res != STATUS_OK)
            {
                lsp_trace("load failed: status=%d (%s)", res, get_status(res));
                return res;
            }

            // Resample
            if ((res = source->resample(nSampleRate)) != STATUS_OK)
            {
                lsp_trace("resample failed: status=%d (%s)", res, get_status(res));
                return res;
            }

            // Commit the result
            lsp_trace("file successfully loaded: %s", sFileName);
            lsp::swap(pLoaded, source);

            return STATUS_OK;
        }

        status_t SamplePlayer::perform_gc()
        {
            dspu::Sample *gc_list = lsp::atomic_swap(&pGCList, NULL);
            lsp_trace("gc_list = %p", gc_list);
            destroy_samples(gc_list);
            return STATUS_OK;
        }

        void SamplePlayer::set_sample_rate(size_t sample_rate)
        {
            nSampleRate     = sample_rate;
        }

        void SamplePlayer::play_sample(const char *file, wsize_t position, bool release)
        {
            // Update file name
            if (file == NULL)
                file            = "";

            // Form the async request
            strncpy(sReqFileName, file, PATH_MAX);
            sReqFileName[PATH_MAX-1] = '\0';

            play_sample(position, release);
        }

        void SamplePlayer::play_sample(wsize_t position, bool release)
        {
            nReqPosition    = position;
            bReqRelease     = release;
            ++nUpdateReq;
        }

        void SamplePlayer::process_async_requests()
        {
            if ((sLoadTask.idle()) && (nUpdateReq != nUpdateResp))
            {
                // Requested cancel of the playback?
                if (strlen(sReqFileName) == 0)
                {
                    // Cancel active playbacks and unbind samples if needed
                    for (size_t i=0; i<2; ++i)
                    {
                        vPlaybacks[i].cancel();
                        if (bReqRelease)
                            vPlayers[i].unbind(0);
                    }

                    nUpdateResp     = nUpdateReq;
                    sFileName[0]    = '\0';
                    return;
                }

                // File name matches, need to update position?
                if (strcmp(sReqFileName, sFileName) == 0)
                {
                    // Cancel current playbacks
                    for (size_t i=0; i<2; ++i)
                        vPlaybacks[i].cancel();

                    nUpdateResp     = nUpdateReq;
                    play_current_sample(nReqPosition);
                    return;
                }

                // We need to load file first before doing the rest stuff
                strcpy(sFileName, sReqFileName);
                if (pWrapper->executor()->submit(&sLoadTask))
                {
                    nUpdateResp     = nUpdateReq;
                }
            }
            else if (sLoadTask.completed())
            {
                // Some payload data received?
                if ((sLoadTask.successful()) && (nUpdateReq == nUpdateResp))
                {
                    // Bind new sample to players
                    for (size_t i=0; i<2; ++i)
                        vPlayers[i].bind(0, pLoaded);

                    // Launch the playback
                    pLoaded     = NULL;
                    play_current_sample(nReqPosition);
                }

                // Reset the loading task to idle state
                sLoadTask.reset();
            }
        }

        void SamplePlayer::process_gc_tasks()
        {
            if (sGCTask.completed())
                sGCTask.reset();

            if (sGCTask.idle())
            {
                // Obtain the list of samples for destroy
                if (pGCList == NULL)
                {
                    for (size_t i=0; i<2; ++i)
                        if ((pGCList = vPlayers[i].gc()) != NULL)
                            break;
                }

                // Submit garbage collection task if needed
                if (pGCList != NULL)
                    pWrapper->executor()->submit(&sGCTask);
            }
        }

        void SamplePlayer::play_current_sample(wsize_t position)
        {
            // Cancel current playbacks
            for (size_t i=0; i<2; ++i)
                vPlaybacks[i].cancel();

            // Compute the number of output channels
            size_t out_channels = 0;
            for (size_t i=0; i<2; ++i)
                if (pOut[i] != NULL)
                    ++out_channels;
            if (out_channels == 0)
                return;

            // Obtain current sample
            dspu::Sample *s = vPlayers[0].get(0);
            if (s == NULL)
                return;

            // Compute the gain of the playback
            size_t sample_channels = lsp_min(s->channels(), 2u);
            if (sample_channels == 0)
                return;

            // Launch the new playbacks
            dspu::PlaySettings ps;
            ps.set_start(position);

            if (out_channels == 1)
            {
                if (sample_channels == 1)
                {
                    // 1 channel -> 1 channel
                    ps.set_channel(0, 0);
                    vPlaybacks[0] = vPlayers[0].play(&ps);
                }
                else // sample_channels == 2
                {
                    // 2 channels -> 1 channel
                    ps.set_volume(0.5f); // Stereo is mixing to mono, reduce the gain
                    for (size_t i=0; i<2; ++i)
                    {
                        ps.set_channel(0, i);
                        vPlaybacks[i] = vPlayers[i].play(&ps);
                    }
                }
            }
            else // out_channels == 2
            {
                // 1/2 channels -> 2 channels
                for (size_t i=0; i<sample_channels; ++i)
                {
                    ps.set_channel(0, i % sample_channels);
                    vPlaybacks[i] = vPlayers[i].play(&ps);
                }
            }
        }

        void SamplePlayer::process_playback(size_t samples)
        {
            if (pOut[0] != NULL)
            {
                // Obtain data buffers
                float *buf[2];
                buf[0]  = pOut[0]->buffer<float>();
                buf[1]  = (pOut[1] != NULL) ? pOut[1]->buffer<float>() : buf[0];

                for (size_t i=0; i<2; ++i)
                    vPlayers[i].process(buf[i], buf[i], samples);
            }

            // Update state
            nPlayPosition   = vPlaybacks[0].position();
            nFileLength     = vPlaybacks[0].sample_length();
        }

        void SamplePlayer::process(size_t samples)
        {
            process_async_requests();
            process_gc_tasks();
            process_playback(samples);
        }

    } /* namespace core */
} /* namespace lsp */


