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

#include <lsp-plug.in/common/types.h>

#if defined(PLATFORM_MACOS)

#include <Foundation/Foundation.h>

namespace lsp
{
    namespace vst3
    {
        struct UITimer
        {
            NSTimer *timer;
        };

        UITimer *create_timer(Steinberg::FUnknown *object, IUITimerHandler *handler, size_t interval)
        {
			UITimer * ui_timer;
			
			@autoreleasepool {
				// Get run loop
			    NSRunLoop *runner = [NSRunLoop currentRunLoop];
			    if (runner == NULL)
			    	return NULL;

				// Create timer handler
			    IUITimerHandler * __block handler_ptr = handler;
			    NSTimer *timer = [NSTimer
			        timerWithTimeInterval: interval * 0.001
			        repeats:YES
			        block:^(NSTimer *timer) {
			            ++runs;
			            printf("Timer event runs=%d\n", runs);
			        }];
		        if (timer == NULL)
		        	return NULL;
    
    			// Attach timer to the run loop
    			[runner addTimer: timer forMode: NSDefaultRunLoopMode];

				// Create and fill structure
				ui_timer = new UITimer;
				if (ui_timer == NULL)
					return NULL;
				
				// Store timer
				ui_timer.timer = timer;
			}
			
			return ui_timer;
        }

        void destroy_timer(UITimer *timer)
        {
            if (timer == NULL)
                return;

            @autoreleasepool {
				// Invalidate timer and remove from run loop
				if (timer->timer != NULL)
				{
					[timer->timer invalidate];
					timer->timer = NULL;
				}

				// Destroy timer structure
				delete timer;
			}
        }

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* PLATFORM_MACOS */
