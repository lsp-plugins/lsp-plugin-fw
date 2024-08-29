/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-plugin-template
 * Created on: 26 июн. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_AUDIORETURN_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_AUDIORETURN_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/dsp-units/shared/AudioStream.h>
#include <lsp-plug.in/plug-fw/core/ICatalogClient.h>
#include <lsp-plug.in/lltl/state.h>

namespace lsp
{
    namespace core
    {
        /**
         * Audio return
         */
        class AudioReturn
        {
            private:
                enum conn_status_t
                {
                    ST_INACTIVE,        // Inactive, no I/O is performing
                    ST_UPDATING,        // The update is in progress
                    ST_ACTIVE,          // The send is active and available for transferring data
                    ST_STALLED,         // The transfer was stalled
                };

                typedef dspu::Catalog::Record   Record;

                typedef struct params_t
                {
                    char                sName[64];      // Name of the connection
                    bool                bFree;          // Free flag
                } params_t;

                typedef struct stream_t
                {
                    dspu::AudioStream  *pStream;        // Stream for writing
                    uint32_t            nStreamCounter; // Stream version counter
                    uint32_t            nStallCounter;  // Stalled counter
                    params_t            sParams;        // Current params applied to stream
                } stream_t;

            private:
                class Client: public ICatalogClient
                {
                    private:
                        AudioReturn        *pReturn;

                    public:
                        Client(AudioReturn *rtrn);
                        virtual ~Client() override;

                    public:
                        void                    apply_settings();

                    public: // core::ICatalogClient
                        virtual bool            update(dspu::Catalog * catalog) override;
                        virtual bool            apply(dspu::Catalog * catalog) override;
                        virtual void            keep_alive(dspu::Catalog *catalog) override;
                };


            private:
                Client                  sClient;        // Client
                lltl::state<stream_t>   sStream;        // Current stream for RT operations
                lltl::state<params_t>   sParams;        // Current state for RT operations
                Record                  sRecord;        // Catalog record
                params_t                sSetup;         // Setup parameters
                params_t                vState[4];      // Allocation list

                stream_t               *pStream;        // Current state used for RT I/O operations
                uatomic_t               enStatus;       // Actual connection status
                bool                    bProcessing;    // Processing mode

            private:
                static void             free_params(params_t *ptr);
                static stream_t        *create_stream(const Record *record, dspu::Catalog *catalog, const params_t * params);
                static void             free_stream(stream_t *ptr);

            private:
                bool                    update(dspu::Catalog *catalog);
                bool                    apply(dspu::Catalog *catalog);
                void                    keep_alive(dspu::Catalog *catalog);

            public:
                AudioReturn();
                AudioReturn(const AudioReturn &) = delete;
                AudioReturn(AudioReturn &&) = delete;
                ~AudioReturn();

                AudioReturn & operator = (const AudioReturn &) = delete;
                AudioReturn & operator = (AudioReturn &&) = delete;

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
                 * Connect to a publisher, RT safe
                 * @param name stream name
                 * @return true if operation wass successful
                 */
                bool                    connect(const char *name);

                /**
                 * Disconnect from a publisher
                 * @return true if operation wass successful
                 */
                bool                    disconnect();

                /**
                 * Check that return is active and available for data receive
                 * @return true if return is active and available for data receive
                 */
                bool                    active() const;

                /**
                 * Check that return is stalled (connected but not receiving data)
                 * @return true if return is stalled
                 */
                bool                    stalled() const;

                /**
                 * Get name of the stream, RT safe
                 * @return name of the stream or NULL if send is inactive
                 */
                const char             *name() const;

                /**
                 * Get number of channels of the publisher stream, RT safe
                 * @return number of channels or negative value if send is inactive
                 */
                ssize_t                 channels() const;

                /**
                 * Get length of the audio buffer in the publisher stream, RT safe
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
                 * Read contents of specific channel, RT safe
                 * Should be called between begin() and end() calls
                 *
                 * @param channel number of channel
                 * @param dst destination buffer to store data
                 * @param samples number of samples to read
                 * @return status of operation
                 */
                status_t                read(size_t channel, float *dst, size_t samples);

                /**
                 * Read sanitized contents (removed NaNs, Infs and denormals) of specific channel, RT safe
                 * Should be called between begin() and end() calls
                 *
                 * @param channel number of channel
                 * @param dst destination buffer to store data
                 * @param samples number of samples to read
                 * @return status of operation
                 */
                status_t                read_sanitized(size_t channel, float *dst, size_t samples);

                /**
                 * End I/O operations on the stream, RT safe
                 * @return status of operation
                 */
                status_t                end();
        };

    } /* namespace core */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_CORE_AUDIORETURN_H_ */
