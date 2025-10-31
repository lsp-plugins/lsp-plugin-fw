/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 окт. 2025 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_UI_TIMER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_UI_TIMER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        struct AppTimer;

        class IAppTimerHandler
        {
            public:
                virtual ~IAppTimerHandler() = default;

            public:
                virtual void on_timer() = 0;
        };

        /**
         * Create periodic application timer that executes in the main UI thread, mostly in run loop.
         * If it was not possible to create run loop timer, the thread that emulates timer is created.
         *
         * @param object object that can provide IRunLoop interface
         * @param handler timer handler
         * @param interval timer repeat interval in milliseconds
         * @return pointer to the timer or NULL on error.
         */
        AppTimer *create_app_timer(Steinberg::FUnknown *object, IAppTimerHandler *handler, size_t interval);

        /**
         * Destroy periodic application timer timer
         * @param timer timer to destoy, may be NULL
         */
        void destroy_app_timer(AppTimer *timer);
    } /* namespace vst3 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_UI_TIMER_H_ */
