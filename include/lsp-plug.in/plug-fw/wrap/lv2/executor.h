/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/ipc/IExecutor.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>

namespace lsp
{
    class LV2Executor: public ipc::IExecutor
    {
        private:
            static const uint32_t magic =
                    (uint32_t('L') << 24) |
                    (uint32_t('V') << 16) |
                    (uint32_t('2') << 8) |
                    (uint32_t('E') << 0);

            LV2_Worker_Schedule *sched;

            typedef struct task_descriptor_t
            {
                uint32_t        magic;
                ipc::ITask     *task;
            } task_descriptor_t;

        public:
            LV2Executor(LV2_Worker_Schedule *schedule)
            {
                sched       = schedule;
            }

            ~LV2Executor()
            {
                sched       = NULL;
            }

        public:
            virtual bool submit(ipc::ITask *task)
            {
                // Check state of task
                if (!task->idle())
                    return false;

                // Try to submit task
                task_descriptor_t descr = { magic, task };
                change_task_state(task, ipc::ITask::TS_SUBMITTED);
                if (sched->schedule_work(sched->handle, sizeof(task_descriptor_t), &descr) == LV2_WORKER_SUCCESS)
                    return true;

                // Failed to submit task, return status back
                change_task_state(task, ipc::ITask::TS_IDLE);
                return false;
            }

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
                run_task(descr->task);
            }
    };
}



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_EXECUTOR_H_ */
