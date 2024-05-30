/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_EXECUTOR_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_EXECUTOR_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/ipc/IExecutor.h>

namespace lsp
{
    namespace gst
    {
        /**
         * Executor service used for running LV2 offline tasks
         */
        class Executor: public ipc::IExecutor
        {
            private:
                ipc::IExecutor             *pExecutor;
                volatile atomic_t           nActiveTasks;

            public:
                Executor(ipc::IExecutor *executor);
                Executor(const Executor &) = delete;
                Executor(Executor &&) = delete;
                ~Executor();

                Executor & operator = (const Executor &) = delete;
                Executor & operator = (Executor &&) = delete;

            protected:
                virtual void task_finished(ipc::ITask *task) override;

            public:
                virtual bool submit(ipc::ITask *task) override;
                virtual void shutdown() override;
        };

    } /* namespace gst */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_EXECUTOR_H_ */
