/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 20 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_EXECUTOR_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_EXECUTOR_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/ipc/IExecutor.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>

namespace lsp
{
    namespace lv2
    {
        /**
         * Executor service used for running LV2 offline tasks
         */
        class Executor: public ipc::IExecutor
        {
            private:
                static const uint32_t magic =
                        (uint32_t('L') << 24) |
                        (uint32_t('V') << 16) |
                        (uint32_t('2') << 8) |
                        (uint32_t('E') << 0);

                typedef struct task_descriptor_t
                {
                    uint32_t        magic;
                    ipc::ITask     *task;
                } task_descriptor_t;

            private:
                LV2_Worker_Schedule        *sched;      // Schedule interface
                volatile atomic_t           queued;     // Number of queued tasks

            public:
                Executor(LV2_Worker_Schedule *schedule)
                {
                    sched       = schedule;
                    atomic_init(queued);
                }
                Executor(const Executor &) = delete;
                Executor(Executor &&) = delete;

                ~Executor()
                {
                    sched       = NULL;
                }

                Executor & operator = (const Executor &) = delete;
                Executor & operator = (Executor &&) = delete;

            public:
                virtual bool submit(ipc::ITask *task) override
                {
                    // Check state of task
                    if (!task->idle())
                        return false;

                    // Try to submit task
                    task_descriptor_t descr = { magic, task };
                    change_task_state(task, ipc::ITask::TS_SUBMITTED);
                    if (sched->schedule_work(sched->handle, sizeof(task_descriptor_t), &descr) == LV2_WORKER_SUCCESS)
                    {
                        atomic_add(&queued, 1);
                        return true;
                    }

                    // Failed to submit task, return status back
                    change_task_state(task, ipc::ITask::TS_IDLE);
                    return false;
                }

                virtual void shutdown() override
                {
                    // We need to wait until all offline tasks have been executed
                    while (queued > 0)
                        ipc::Thread::sleep(10);
                }

            public:
                inline void run_job(
                    LV2_Worker_Respond_Handle   handle,
                    LV2_Worker_Respond_Function respond,
                    uint32_t                    size,
                    const void*                 data
                )
                {
                    // Validate structure
                    if (size != sizeof(task_descriptor_t))
                        return;
                    const task_descriptor_t *descr = reinterpret_cast<const task_descriptor_t *>(data);
                    if (descr->magic != magic)
                        return;

                    // Run task
                    lsp_finally { atomic_add(&queued, -1); };
                    run_task(descr->task);
                }
        };

    } /* namespace lv2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_EXECUTOR_H_ */
