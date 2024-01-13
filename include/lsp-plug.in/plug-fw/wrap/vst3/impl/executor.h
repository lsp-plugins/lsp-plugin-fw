/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 14 янв. 2024 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_EXECUTOR_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_EXECUTOR_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/ipc/Thread.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/executor.h>

namespace lsp
{
    namespace vst3
    {
        Executor::Executor(ipc::IExecutor *executor)
        {
            pExecutor       = executor;
            nActiveTasks    = 0;
        }

        Executor::~Executor()
        {
            pExecutor       = NULL;
        }

        void Executor::task_finished(ipc::ITask *task)
        {
            set_executor(task, NULL);
            atomic_add(&nActiveTasks, -1);
        }

        bool Executor::submit(ipc::ITask *task)
        {
            if (pExecutor == NULL)
                return false;

            // Try to submit task
            atomic_add(&nActiveTasks, 1);
            set_executor(task, this);
            if (pExecutor->submit(task))
                return true;

            // Failed to submit task, release resources
            set_executor(task, NULL);
            atomic_add(&nActiveTasks, -1);

            return true;
        }

        void Executor::shutdown()
        {
            // We need to wait until all offline tasks have been executed
            while (nActiveTasks > 0)
                ipc::Thread::sleep(10);
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_EXECUTOR_H_ */
