/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 авг. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_SHMCLIENT_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_SHMCLIENT_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/core/AudioReturn.h>
#include <lsp-plug.in/plug-fw/core/AudioSend.h>
#include <lsp-plug.in/plug-fw/core/Catalog.h>
#include <lsp-plug.in/plug-fw/core/ICatalogFactory.h>
#include <lsp-plug.in/plug-fw/core/ShmState.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace core
    {
        /**
         * Shared memory client, manages
         */
        class ShmClient
        {
            private:
                typedef struct send_t
                {
                    const char                 *sID;                // ID of the parameter
                    uint32_t                    nChannels;          // Number of channels
                    bool                        bPublish;           // Flag that forces to re-publish data
                    core::AudioSend            *pSend;              // Audio send
                    plug::IPort                *pName;              // Port that holds send name
                    char                        sLastName[MAX_SHM_SEGMENT_NAME_BYTES]; // Last name used by send
                    plug::IPort                *vChannels[];        // List of ports associated with channels
                } send_t;

                typedef struct return_t
                {
                    const char                 *sID;                // ID of the parameter
                    uint32_t                    nChannels;          // Number of channels
                    core::AudioReturn          *pReturn;            // Audio return
                    plug::IPort                *pName;              // Port that holds return name
                    plug::IPort                *vChannels[];        // List of ports associated with channels
                } return_t;

                class Listener: public ICatalogClient
                {
                    private:
                        ShmClient              *pClient;

                    public:
                        Listener(ShmClient *client);
                        virtual ~Listener() override;

                    public:
                        virtual bool            update(dspu::Catalog *catalog) override;
                };

            private:
                core::ICatalogFactory          *pFactory;           // Catalog factory
                core::Catalog                  *pCatalog;           // Catalog
                lltl::parray<send_t>            vSends;             // List of sends
                lltl::parray<return_t>          vReturns;           // List of returns
                lltl::state<ShmState>           sState;             // Shared memory state
                size_t                          nSampleRate;        // Sample rate
                size_t                          nBufferSize;        // Buffer size

            private:
                static size_t   channels_count(const char *id, lltl::parray<plug::IPort> *ports);
                static void     bind_channels(plug::IPort **channels, const char *id, lltl::parray<plug::IPort> *ports);
                static void     scan_ports(lltl::parray<plug::IPort> *dst, meta::role_t role, plug::IPort **ports, size_t count);
                static void     shm_state_deleter(ShmState *state);

                void            create_send(plug::IPort *p, lltl::parray<plug::IPort> *sends);
                void            create_return(plug::IPort *p, lltl::parray<plug::IPort> *returns);
                void            destroy_send(send_t *item);
                void            destroy_return(return_t *item);
                bool            update_catalog(dspu::Catalog *catalog);

            public:
                ShmClient();
                ShmClient(const ShmClient &) = delete;
                ShmClient(ShmClient &&) = delete;
                ShmClient & operator = (const ShmClient &) = delete;
                ShmClient & operator = (ShmClient &&) = delete;
                ~ShmClient();

                void            init(core::ICatalogFactory *factory, plug::IPort **ports, size_t count);
                void            destroy();

            public:
                /**
                 * Check that shared memory state has been updated
                 * @return true if shared memory state has been updated
                 */
                bool            state_updated();

                /**
                 * Get actual shared memory state and cleanup updated flag
                 * @return actual shared memory state
                 */
                const ShmState *state();

                /**
                 * Set overall I/O buffer size in samples
                 * @param size overall I/O buffer size in samples
                 */
                void            set_buffer_size(size_t size);

                /**
                 * Set current sample rate
                 * @param sample_rate current sample rate
                 */
                void            set_sample_rate(size_t sample_rate);

                /**
                 * Handle settings update
                 */
                void            update_settings();

                /**
                 * Start audo processing transaction
                 * @param samples number of samples per transaction
                 */
                void            begin(size_t samples);

                /**
                 * Pre-process data block: fetch all data from returns to associated memory buffers
                 * @param samples number of samples in data block
                 */
                void            pre_process(size_t samples);

                /**
                 * Post-process data block: write all data from associated memory buffers to sends
                 * @param samples number of samples in data block
                 */
                void            post_process(size_t samples);

                /**
                 * Finish the audio processing transaction
                 */
                void            end();
        };


    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_SHMCLIENT_H_ */
