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

                core::ICatalogFactory          *pFactory;           // Catalog factory
                core::Catalog                  *pCatalog;           // Catalog
                lltl::parray<send_t>            vSends;             // List of sends
                lltl::parray<return_t>          vReturns;           // List of returns
                size_t                          nSampleRate;        // Sample rate
                size_t                          nBufferSize;        // Buffer size

            private:
                static size_t   channels_count(const char *id, lltl::parray<plug::IPort> *ports);
                static void     bind_channels(plug::IPort **channels, const char *id, lltl::parray<plug::IPort> *ports);
                static void     scan_ports(lltl::parray<plug::IPort> *dst, meta::role_t role, plug::IPort **ports, size_t count);

                void            create_send(plug::IPort *p, lltl::parray<plug::IPort> *sends);
                void            create_return(plug::IPort *p, lltl::parray<plug::IPort> *returns);
                void            destroy_send(send_t *item);
                void            destroy_return(return_t *item);

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
                void            set_buffer_size(size_t size);
                void            set_sample_rate(size_t sample_rate);
                void            update_settings();

                void            begin(size_t samples);
                void            pre_process(size_t samples);
                void            post_process(size_t samples);
                void            end();
        };


    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_SHMCLIENT_H_ */
