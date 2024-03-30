/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 июн. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_AUDIOSEND_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_AUDIOSEND_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/dsp-units/shared/AudioStream.h>
#include <lsp-plug.in/plug-fw/core/ICatalogClient.h>

namespace lsp
{
    namespace core
    {
        /**
         * Audio send
         */
        class AudioSend: public ICatalogClient
        {
            private:
                enum state_t
                {
                    ST_INACTIVE,        // Disconnected from catalog
                    ST_APPLY,           // Pending for changes
                    ST_ACTIVE           // Active
                };

                typedef struct params_t
                {
                    uint32_t        nChannels;
                    char            sName[64];
                } params_t;

            private:
                dspu::AudioStream      *vStreams[2];    // Active and pending
                uint32_t                nVersion;       // Stream version
                uint32_t                nState;         // Current state
                params_t                sParams[2];     // Current parameters and pending ones

            public:
                AudioSend();
                AudioSend(const AudioSend &) = delete;
                AudioSend(AudioSend &&) = delete;
                virtual ~AudioSend() override;

                AudioSend & operator = (const AudioSend &) = delete;
                AudioSend & operator = (AudioSend &&) = delete;

            public:
                /**
                 * Set name of the stream, RT safe
                 * @param name name of the stream
                 */
                void                    set_name(const char *name);

                /**
                 * Set number of output channels of the stream, RT safe
                 * @param channels number of channels of the stream
                 */
                void                    set_channels(size_t channels);

                /**
                 * Set stream params, RT safe
                 * @param name stream name
                 * @param channels number of channels
                 */
                void                    set_params(const char *name, size_t channels);

                /**
                 * Begin I/O operation on the stream
                 * @param block_size the desired block size that will be read or written, zero value means infinite block size
                 * @return status of operation
                 */
                status_t                begin(ssize_t block_size = 0);

                /**
                 * Write contents of the specific channel
                 * Should be called between begin() and end() calls
                 *
                 * @param channel number of channel
                 * @param dst destination buffer to store data
                 * @param samples number of samples to read
                 * @return status of operation
                 */
                status_t                write(size_t channel, const float *src, size_t samples);

                /**
                 * End I/O operations on the stream
                 * @return status of operation
                 */
                status_t                end();

            public: // core::ICatalogClient
                /**
                 * Notify client about changes in the catalog and force the client to
                 * update configuration.
                 * @param catalog the catalog that can be modified
                 */
                virtual bool            update(dspu::Catalog * catalog) override;

                /**
                 * Apply changes introduced by the client to the catalog.
                 * @param catalog the catalog that can be modified
                 */
                virtual bool            apply(dspu::Catalog * catalog) override;
        };

    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_AUDIOSEND_H_ */
