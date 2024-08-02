/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_KVTDISPATCHER_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_KVTDISPATCHER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/plug-fw/core/osc_buffer.h>
#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace core
    {
        class KVTDispatcher: public ipc::Thread
        {
            protected:
                core::osc_buffer_t *pRx;
                core::osc_buffer_t *pTx;
                KVTStorage         *pKVT;
                ipc::Mutex         *pKVTMutex;
                uint8_t            *pPacket;
                atomic_t            nClients;
                atomic_t            nTxRequest;

            protected:
                size_t              receive_changes();
                size_t              transmit_changes();

            public:
                explicit KVTDispatcher(KVTStorage *kvt, ipc::Mutex *mutex);
                KVTDispatcher(const KVTDispatcher &) = delete;
                KVTDispatcher(KVTDispatcher &&) = delete;
                virtual ~KVTDispatcher() override;

                KVTDispatcher & operator = (const KVTDispatcher &) = delete;
                KVTDispatcher & operator = (KVTDispatcher &) = delete;

            public:
                virtual status_t    run() override;

            public:
                ssize_t             iterate();

                status_t            submit(const void *data, size_t size);
                status_t            submit(const osc::packet_t *packet);

                status_t            fetch(void *data, size_t *size, size_t limit);
                status_t            fetch(osc::packet_t *packet, size_t limit);
                status_t            skip();

                void                connect_client();
                void                disconnect_client();

                inline size_t       rx_size() const { return pRx->size(); }
                inline size_t       tx_size() const { return pTx->size(); }

                static status_t     parse_message(KVTStorage *kvt, const void *data, size_t size, size_t flags);
                static status_t     parse_message(KVTStorage *kvt, const osc::packet_t *packet, size_t flags);

                static status_t     build_message(const char *param_name, const kvt_param_t *param, osc::packet_t *packet, size_t limit);
                static status_t     build_message(const char *param_name, const kvt_param_t *param, void *data, size_t *size, size_t limit);
        };
    } /* namespace core */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CORE_KVTDISPATCHER_H_ */
