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
#include <lsp-plug.in/lltl/state.h>

namespace lsp
{
    namespace core
    {
        /**
         * Audio send
         */
        class AudioSend
        {
            private:
                enum conn_status_t
                {
                    ST_INACTIVE,        // Inactive, no I/O is performing
                    ST_UPDATING,        // The update is in progress
                    ST_ACTIVE,          // The send is active and available for transferring data
                    ST_OVERRIDDEN,      // The send has been overridden by another one
                };

                typedef dspu::Catalog::Record   Record;

                typedef struct params_t
                {
                    uint32_t            nChannels;      // Number of channels
                    uint32_t            nLength;        // Buffer length
                    char                sName[64];      // Name of the connection
                    bool                bFree;          // Free flag
                } params_t;

                typedef struct stream_t
                {
                    dspu::AudioStream  *pStream;        // Stream for writing
                    params_t            sParams;        // Current params applied to stream
                } stream_t;

            private:
                class Client: public ICatalogClient
                {
                    private:
                        AudioSend              *pSend;

                    public:
                        Client(AudioSend *send);
                        virtual ~Client() override;

                    public:
                        void                    apply_settings();

                    public: // core::ICatalogClient
                        virtual bool            update(dspu::Catalog * catalog) override;
                        virtual bool            apply(dspu::Catalog * catalog) override;
                };


            private:
                Client                  sClient;        // Client
                lltl::state<stream_t>   sStream;        // Current stream for RT operations
                lltl::state<params_t>   sParams;        // Current state for RT operations
                Record                  sRecord;        // Catalog record
                params_t                vState[4];      // Allocation list

                stream_t               *pStream;        // Current state used for RT I/O operations
                uatomic_t               enStatus;       // Actual connection status
                bool                    bProcessing;    // Processing mode

            private:
                static void             free_params(params_t *ptr);
                static stream_t        *create_stream(Record *record, dspu::Catalog *catalog, const params_t * params);
                static void             free_stream(stream_t *ptr);

            private:
                bool                    update(dspu::Catalog *catalog);
                bool                    apply(dspu::Catalog *catalog);

            public:
                AudioSend();
                AudioSend(const AudioSend &) = delete;
                AudioSend(AudioSend &&) = delete;
                ~AudioSend();

                AudioSend & operator = (const AudioSend &) = delete;
                AudioSend & operator = (AudioSend &&) = delete;

            public:
                /**
                 * Connect to the catalog
                 * @param catalog catalog to connect to
                 * @return status of operation
                 */
                status_t                attach(Catalog *catalog);

                /**
                 * Disconnect from catalog
                 * @return status of operation
                 */
                status_t                detach();

                /**
                 * Check that client is connected to catalog
                 * @return true if client is connected to catalog
                 */
                bool                    attached() const;

            public:
                /**
                 * Publish new stream, RT safe
                 * @param name stream name
                 * @param channels number of channels
                 * @param length buffer length in audio frames
                 * @return true if operation wass successful
                 */
                bool                    publish(const char *name, size_t channels, size_t length);

                /**
                 * Revoke current published stream, RT safe
                 * @return true if operation wass successful
                 */
                bool                    revoke();

                /**
                 * Check that send is active and available for data transfer, RT safe
                 * @return true if send is active and available for data transfer
                 */
                bool                    active() const;

                /**
                 * Check that send has been overridden, RT safe
                 * @return true if send has been overridden by another send
                 */
                bool                    overridden() const;

                /**
                 * Get name of the stream, RT safe
                 * @return name of the stream or NULL if send is inactive
                 */
                const char             *name() const;

                /**
                 * Get number of channels, RT safe
                 * @return number of channels or negative value if send is inactive
                 */
                ssize_t                 channels() const;

                /**
                 * Get length of the audio buffer, RT safe
                 * @return length of the audio buffer in frames or negative value if send is inactive
                 */
                ssize_t                 length() const;

                /**
                 * Begin I/O operation on the stream, RT safe
                 * @param block_size the desired number of frames that will be written, zero value means infinite block size
                 * @return status of operation.
                 */
                status_t                begin(ssize_t block_size = 0);

                /**
                 * Write contents of the specific channel, RT safe
                 * Should be called between begin() and end() calls
                 *
                 * @param channel number of channel
                 * @param dst destination buffer to store data
                 * @param samples number of samples to read
                 * @return status of operation
                 */
                status_t                write(size_t channel, const float *src, size_t samples);

                /**
                 * Write sanitized contents (removed NaNs, Infs and denormals) of the specific channel, RT safe
                 * Should be called between begin() and end() calls
                 *
                 * @param channel number of channel
                 * @param dst destination buffer to store data
                 * @param samples number of samples to read
                 * @return status of operation
                 */
                status_t                write_sanitized(size_t channel, const float *src, size_t samples);

                /**
                 * End I/O operations on the stream, RT safe
                 * @return status of operation
                 */
                status_t                end();
        };

    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_AUDIOSEND_H_ */
