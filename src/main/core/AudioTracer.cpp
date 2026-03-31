/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 мар. 2026 г.
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

#include <lsp-plug.in/plug-fw/core/AudioTracer.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/stdlib/stdlib.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace core
    {
        AudioTracer::AudioTracer()
        {
            pId             = NULL;
            pInstance       = NULL;
            nFrameId        = 0;
        }

        AudioTracer::~AudioTracer()
        {
            stop_trace();
            sSample.destroy();
        }

        void AudioTracer::stop_trace()
        {
            if (pInstance == NULL)
                return;

            // Save frame if present
            save_frame();

            // Turn off tracing
            lsp_trace("Turning OFF data trace for instance=%p, object=%s", pInstance, pId);
            pInstance       = NULL;
            if (pId != NULL)
            {
                free(pId);
                pId             = NULL;
            }
        }

        status_t AudioTracer::make_full_path(io::Path &wpath, const char *format, const char *id, const void *instance)
        {
            status_t res;
            if ((res = system::get_temporary_dir(&wpath)) != STATUS_OK)
                return res;
            if ((res = update_status(res, wpath.append_child("lsp-plugins-dumps"))) != STATUS_OK)
                return res;
            if ((res = update_status(res, wpath.append_child(format))) != STATUS_OK)
                return res;

            char buf[32];
            snprintf(buf, sizeof(buf), "%p", instance);
            buf[sizeof(buf)-1] = '\0';
            if ((res = wpath.append_child(buf)) != STATUS_OK)
                return res;

            if ((res = update_status(res, wpath.append_child(id))) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        status_t AudioTracer::set_trace(const char *format, const char *id, const void *instance)
        {
            stop_trace();

            if ((format == NULL) || (id == NULL) || (instance == NULL))
                return STATUS_BAD_ARGUMENTS;

            io::Path wpath;
            status_t res;
            if ((res = make_full_path(wpath, format, id, instance)) != STATUS_OK)
            {
                lsp_warn("Failed to enable trace for instance=%p, object=%d", instance, id);
                return res;
            }

            if ((res = wpath.mkdir(true)) != STATUS_OK)
            {
                lsp_warn("Failed to create trace path for instance=%p, object=%s, path='%s': code=%d", instance, id, wpath.as_native(), int(res));
                return res;
            }

            if (!sSample.init(1, FRAME_LENTH, 0))
            {
                lsp_warn("Failed to initialize trace frame for instance=%p, object=%s", instance, id);
                return res;
            }
            sSample.set_sample_rate(SAMPLE_RATE);

            if ((pId = strdup(id)) == NULL)
            {
                lsp_warn("Failed to allocate memory for trace of instance=%p, object=%s", instance, id);
                return res;
            }

            pInstance       = instance;
            nFrameId        = 0;
            sPath.swap(&wpath);

            lsp_trace("Turning ON data trace for instance=%p, object=%s, location='%s'", pInstance, pId, sPath.as_native());

            return STATUS_OK;
        }

        status_t AudioTracer::save_frame()
        {
            // Is there something to save?
            if (sSample.length() <= 0)
                return STATUS_OK;

            // Clear frame at exit
            lsp_finally {
                sSample.set_length(0);
                ++nFrameId;
            };

            io::Path tmp;
            ssize_t n = tmp.fmt("%s/%08lx.wav", sPath.as_native(), long(nFrameId));
            if (n < 0)
            {
                lsp_warn("Failed to store frame for instance=%p, object=%d, location='%s': pathname init failed", long(nFrameId));
                return STATUS_NO_MEM;
            }

            const ssize_t saved = sSample.save(&tmp);
            if (saved < 0)
            {
                lsp_warn("Failed to store frame for instance=%p, object=%d, location='%s': code=%d",
                    pInstance, pId, tmp.as_native(), int(-saved));
                return -saved;
            }

            return STATUS_OK;
        }

        void AudioTracer::submit(const float *data, size_t count)
        {
            // Skip non-tracking events
            if (pInstance == NULL)
                return;

            for (size_t offset = 0; offset < count;)
            {
                // Can append data to the sample?
                const size_t to_do      = lsp_min(count - offset, sSample.max_length() - sSample.length());
                if (to_do > 0)
                {
                    float * const dst       = sSample.channel(0, sSample.length());
                    dsp::copy(dst, &data[offset], to_do);
                    sSample.set_length(sSample.length() + to_do);
                }
                offset                 += to_do;

                // Need to flush sample to disk?
                if (sSample.length() >= sSample.max_length())
                    save_frame();
            }
        }

    } /* namespace core */
} /* namespace lsp */


