/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 19 июн. 2024 г.
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

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/core/AudioSend.h>

namespace lsp
{
    namespace core
    {
        AudioSend::AudioSend()
        {
        }

        AudioSend::~AudioSend()
        {
        }

        void AudioSend::set_name(const char *name)
        {
        }

        void AudioSend::set_channels(size_t channels)
        {
        }

        void AudioSend::set_params(const char *name, size_t channels)
        {
        }

        status_t AudioSend::begin(ssize_t block_size)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t AudioSend::write(size_t channel, const float *src, size_t samples)
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t AudioSend::end()
        {
            return STATUS_NOT_IMPLEMENTED;
        }

        bool AudioSend::update(dspu::Catalog * catalog)
        {
            return true;
        }

        bool AudioSend::apply(dspu::Catalog * catalog)
        {
            return true;
        }

    } /* namespace core */
} /* namespace lsp */


