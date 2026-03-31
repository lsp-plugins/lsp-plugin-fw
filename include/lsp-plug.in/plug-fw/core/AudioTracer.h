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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_AUDIOTRACER_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_AUDIOTRACER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/dsp-units/sampling/Sample.h>
#include <lsp-plug.in/io/Path.h>

namespace lsp
{
    namespace core
    {
        /**
         * Simple audio tracer object that allows to trace audio streams into separate audio files
         * stored in some folder.
         */
        class AudioTracer
        {
            public:
                static constexpr size_t SAMPLE_RATE         = 48000;
                static constexpr size_t FRAME_LENTH         = SAMPLE_RATE * 5;

            private:
                dspu::Sample    sSample;
                io::Path        sPath;
                char           *pId;
                const void     *pInstance;
                size_t          nFrameId;

            public:
                AudioTracer();
                AudioTracer(const AudioTracer &) = delete;
                AudioTracer(AudioTracer &&) = delete;
                ~AudioTracer();

                AudioTracer & operator = (const AudioTracer &) = delete;
                AudioTracer & operator = (AudioTracer &&) = delete;

            private:
                void            stop_trace();
                status_t        make_full_path(io::Path &wpath, const char *format, const char *id, const void *instance);
                status_t        save_frame();

            public:
                /**
                 * Enable/disable trace
                 * @param format plugin format, NULL to disable
                 * @param id object identifier, NULL to disable
                 * @param instance associated instance pointer, NULL to disable
                 */
                status_t        set_trace(const char *format, const char *id, const void *instance);

                /**
                 * Submit audio samples to the trace
                 * @param data data to submit
                 * @param count number of samples to submit
                 */
                void            submit(const float *data, size_t count);
        };

    } /* namespace core */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CORE_AUDIOTRACER_H_ */
