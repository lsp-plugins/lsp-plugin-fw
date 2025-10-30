/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
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

#include <lsp-plug.in/common/atomic.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/defs.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ui_timer.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>

namespace lsp
{
    namespace vst3
    {
#if defined(VST_USE_RUNLOOP_IFACE)
        class RunLoopTimer: public Steinberg::Linux::ITimerHandler
        {
            private:
                uatomic_t           nRefCounter;
                IUITimerHandler    *pHandler;

            public:
                RunLoopTimer(IUITimerHandler *handler)
                {
                    atomic_store(&nRefCounter, 1);
                    pHandler        = handler;
                }

                virtual ~RunLoopTimer()
                {
                    pHandler        = NULL;
                }

                RunLoopTimer(const RunLoopTimer &) = delete;
                RunLoopTimer(RunLoopTimer &&) = delete;

                RunLoopTimer & operator = (const RunLoopTimer &) = delete;
                RunLoopTimer & operator = (RunLoopTimer &&) = delete;

            public: // Steinberg::FUnknown
                virtual Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override
                {
                    // Cast to the requested interface
                    if (Steinberg::iidEqual(_iid, Steinberg::FUnknown::iid))
                        return cast_interface<Steinberg::FUnknown>(this, obj);
                    if (Steinberg::iidEqual(_iid, Steinberg::Linux::ITimerHandler::iid))
                        return cast_interface<Steinberg::Linux::ITimerHandler>(this, obj);

                    return no_interface(obj);
                }

                virtual Steinberg::uint32 PLUGIN_API addRef() override
                {
                    return atomic_add(&nRefCounter, 1) + 1;
                }

                virtual Steinberg::uint32 PLUGIN_API release() override
                {
                    atomic_t ref_count = atomic_add(&nRefCounter, -1) - 1;
                    if (ref_count == 0)
                        delete this;

                    return ref_count;
                }

            public: // Steinberg::Linux::ITimerHandler
                virtual void PLUGIN_API onTimer() override
                {
                    if (pHandler != NULL)
                        pHandler->on_timer();
                }
        };

        struct UITimer
        {
            Steinberg::Linux::IRunLoop *run_loop;
            RunLoopTimer *timer;
        };

        UITimer *create_timer(Steinberg::FUnknown *object, IUITimerHandler *handler, size_t interval)
        {
            // Acquire run loop
            Steinberg::Linux::IRunLoop * run_loop = safe_query_iface<Steinberg::Linux::IRunLoop>(object);
            if (run_loop == NULL)
                return NULL;
            lsp_finally { safe_release(run_loop); };

            // Create run loop timer
            RunLoopTimer *timer = new RunLoopTimer(handler);
            if (timer == NULL)
                return NULL;
            lsp_finally { safe_release(timer); };

            // Add timer to the run loop
            const Steinberg::tresult result = run_loop->registerTimer(timer, Steinberg::Linux::TimerInterval(interval));
            if (result != Steinberg::kResultOk)
                return NULL;

            // Now we can create UITimer structure and return it;
            UITimer *ui_timer   = new UITimer;
            if (ui_timer == NULL)
                return NULL;

            ui_timer->run_loop  = safe_acquire(run_loop);
            ui_timer->timer     = safe_acquire(timer);

            return ui_timer;
        }

        void destroy_timer(UITimer *timer)
        {
            if (timer == NULL)
                return;

            // First remove timer from the run loop
            if ((timer->run_loop != NULL) && (timer->timer != NULL))
                timer->run_loop->unregisterTimer(timer->timer);

            // Now release all related objects
            safe_release(timer->timer);
            safe_release(timer->run_loop);

            // Delete the timer structure
            delete timer;
        }

#elif defined(PLATFORM_WINDOWS)
#else
        struct UITimer
        {
        };

        UITimer *create_timer(Steinberg::FUnknown *object, IUITimerHandler *handler, size_t interval)
        {
            return NULL;
        }

        void destroy_timer(UITimer *timer)
        {
        }
#endif
    } /* namespace vst3 */
} /* namespace lsp */
