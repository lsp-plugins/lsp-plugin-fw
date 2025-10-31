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
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/app_timer.h>

#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/defs.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>

#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#include <winuser.h>
#endif /* PLATFORM_WINDOWS */

namespace lsp
{
    namespace vst3
    {
        struct AppTimer
        {
            ipc::Thread *thread;
        };

        class TimerThread: public ipc::Thread
        {
            private:
                IAppTimerHandler   *pHandler;
                size_t              nInterval;

            public:
                TimerThread(IAppTimerHandler *handler, size_t interval)
                {
                    pHandler            = handler;
                    nInterval           = interval;
                }

            public:
                virtual status_t run() override
                {
                    lsp_trace("start thread timer loop this=%p", this);

                    while (!ipc::Thread::is_cancelled())
                    {
                        // Measure the start time
                        const system::time_millis_t time = system::get_time_millis();

                        // Execute timer handler
                        pHandler->on_timer();

                        // Wait for a while
                        const system::time_millis_t consumed = system::get_time_millis() - time;
                        if (consumed < nInterval)
                            ipc::Thread::sleep(nInterval - consumed);
                    }

                    lsp_trace("leave thread timer loop this=%p", this);

                    return ipc::Thread::run();
                }
        };

        AppTimer *create_thread_app_timer(AppTimer *app_timer, IAppTimerHandler *handler, size_t interval)
        {
            // Delete structure if not released
            lsp_finally {
                if (app_timer != NULL)
                    delete app_timer;
            };

            // Create timer thread
            ipc::Thread *thread = new TimerThread(handler, interval);
            if (thread == NULL)
                return NULL;
            lsp_finally {
                if (thread != NULL)
                    delete thread;
            };

            // Start timer thread
            if (thread->start() != STATUS_OK)
                return NULL;

            // Fill structure fields
            app_timer->thread   = release_ptr(thread);

            lsp_trace("Created native thread application timer ptr=%p, thread=%p",
                app_timer, app_timer->thread);

            // Return the result
            return release_ptr(app_timer);
        }

        void destroy_thread_app_timer(AppTimer *app_timer)
        {
            // Stop the timer thread
            ipc::Thread *thread     = release_ptr(app_timer->thread);
            if (thread != NULL)
            {
                lsp_trace("Terminating native thread application timer");
                thread->cancel();
                thread->join();
                delete thread;
                lsp_trace("Native thread application timer terminated");
            }
        }

#if defined(PLATFORM_WINDOWS)
        static constexpr const HWND INVALID_HWND       = HWND(nullptr);
        static constexpr const UINT_PTR IDT_TIMER      = 1;

        struct WinAppTimer: public AppTimer
        {
            IAppTimerHandler   *handler;        // Timer handler
            HWND                window;         // Window for messaging
            UINT_PTR            timer_id;       // Timer identifier
        };

        static LRESULT CALLBACK timer_window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            if (uMsg == WM_CREATE)
            {
                CREATESTRUCTW *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
                WinAppTimer *data = reinterpret_cast<WinAppTimer *>(create->lpCreateParams);
                if (data != NULL)
                    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
            }

            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        static void CALLBACK timer_proc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
        {
            // Obtain the pointer to AppTimer
            WinAppTimer *app_timer = reinterpret_cast<WinAppTimer *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if ((app_timer != NULL) && (app_timer->handler != NULL))
                app_timer->handler->on_timer();
        }

        AppTimer *create_app_timer(Steinberg::FUnknown *object, IAppTimerHandler *handler, size_t interval)
        {
            WinAppTimer *app_timer = new WinAppTimer;
            if (app_timer == NULL)
                return NULL;

            app_timer->thread       = NULL;
            app_timer->handler      = handler;
            app_timer->window       = INVALID_HWND;
            app_timer->timer_id     = UINT_PTR(0);

            // Create window for processing WM_TIMER events
            HWND wnd                = CreateWindowExW(
                0,                      // dwExStyle
                L"Message",             // lpClassName
                L"VST3 Timer",          // lpWindowName
                WS_DISABLED | WS_CHILDWINDOW, // dwStyle
                0,                      // X
                0,                      // Y
                32,                     // nWidth
                32,                     // nHeight
                HWND_MESSAGE,           // hWndParent
                NULL,                   // hMenu,
                GetModuleHandleW(NULL), // hInstance
                app_timer); // lpCreateParam

            if (wnd == NULL)
            {
                lsp_trace("Failed to create window, code=0x%x", GetLastError());
                return create_thread_app_timer(app_timer, handler, interval);
            }
            lsp_finally {
                if (wnd != NULL)
                    DestroyWindow(wnd);
            };

            // Set user data
            SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app_timer));
            SetWindowLongPtrW(wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(timer_window_proc));

            // Now Set up timer
            UINT_PTR timer              = SetTimer(wnd, IDT_TIMER, interval, timer_proc);
            if (timer == 0)
            {
                lsp_trace("Failed to create timer, code=0x%x", GetLastError());
                return create_thread_app_timer(app_timer, handler, interval);
            }

            // Fill data structure and return
            app_timer->window       = wnd;
            app_timer->timer_id     = timer;

            wnd                     = INVALID_HWND;

            lsp_trace("Created WinAPI application timer ptr=%p, thread=%p",
                app_timer, app_timer->thread);

            return release_ptr(app_timer);
        }

        void destroy_app_timer(AppTimer *timer)
        {
            if (timer == NULL)
                return;

            WinAppTimer *app_timer = static_cast<WinAppTimer *>(timer);

            // Kill timer if present
            if (app_timer->window != INVALID_HWND)
            {
                if (app_timer->timer_id > 0)
                    KillTimer(app_timer->window, app_timer->timer_id);
                DestroyWindow(app_timer->window);
            }

            // Stop the timer thread
            destroy_thread_app_timer(timer);

            lsp_trace("Destroyed application timer ptr=%p", timer);
        }

#else

    #ifdef VST_USE_RUNLOOP_IFACE
        class RunLoopTimer: public Steinberg::Linux::ITimerHandler
        {
            private:
                uatomic_t           nRefCounter;
                IAppTimerHandler   *pHandler;

            public:
                RunLoopTimer(IAppTimerHandler *handler)
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
    #endif /* VST_USE_RUNLOOP_IFACE */

        struct PosixAppTimer: public AppTimer
        {
        #ifdef VST_USE_RUNLOOP_IFACE
            Steinberg::Linux::IRunLoop *run_loop;
            RunLoopTimer           *timer;
        #endif /* VST_USE_RUNLOOP_IFACE */
        };

        AppTimer *create_app_timer(Steinberg::FUnknown *object, IAppTimerHandler *handler, size_t interval)
        {
            // Create AppTimer structure
            PosixAppTimer *app_timer     = new PosixAppTimer;
            if (app_timer == NULL)
                return NULL;

            app_timer->thread       = NULL;

        #ifdef VST_USE_RUNLOOP_IFACE
            app_timer->run_loop     = NULL;
            app_timer->timer        = NULL;

            // Try to use IRunLoop first
            Steinberg::Linux::IRunLoop * run_loop = safe_query_iface<Steinberg::Linux::IRunLoop>(object);
            lsp_finally { safe_release(run_loop); };

            if (run_loop == NULL)
                return create_thread_app_timer(release_ptr(app_timer), handler, interval);

            lsp_finally {
                if (app_timer != NULL)
                    delete app_timer;
            };

            // Create run loop timer
            RunLoopTimer *timer = new RunLoopTimer(handler);
            if (timer == NULL)
                return NULL;
            lsp_finally { safe_release(timer); };

            // Add timer to the run loop
            const Steinberg::tresult result = run_loop->registerTimer(timer, Steinberg::Linux::TimerInterval(interval));
            if (result != Steinberg::kResultOk)
                return NULL;

            // Fill structure fields
            app_timer->run_loop = safe_acquire(run_loop);
            app_timer->timer    = safe_acquire(timer);

            lsp_trace("Created IRunLoop application timer ptr=%p, timer=%p for run_loop=%p",
                app_timer, app_timer->timer, app_timer->run_loop);

            return release_ptr(app_timer);
        #endif /* VST_USE_RUNLOOP_IFACE */

            return create_thread_app_timer(app_timer, handler, interval);
        }

        void destroy_app_timer(AppTimer *timer)
        {
            if (timer == NULL)
                return;

            PosixAppTimer *app_timer = static_cast<PosixAppTimer *>(timer);

            // First remove timer from the run loop
            if ((app_timer->run_loop != NULL) && (app_timer->timer != NULL))
                app_timer->run_loop->unregisterTimer(app_timer->timer);

            // Stop the timer thread
            destroy_thread_app_timer(timer);

            // Now release all related objects
            safe_release(app_timer->timer);
            safe_release(app_timer->run_loop);

            // Delete the timer structure
            delete timer;

            lsp_trace("Destroyed application timer ptr=%p", timer);
        }

#endif /* PLATFORM */

    } /* namespace vst3 */
} /* namespace lsp */
