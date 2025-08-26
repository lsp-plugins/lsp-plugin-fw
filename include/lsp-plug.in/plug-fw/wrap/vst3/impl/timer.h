/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 2 февр. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_TIMER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_TIMER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <steinberg/vst3.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/data.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/sync.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/timer.h>

#ifdef VST_USE_RUNLOOP_IFACE

namespace lsp
{
    namespace vst3
    {
        PlatformTimer::PlatformTimer(IUISync *handler)
        {
            atomic_store(&nRefCounter, 1);
            pHandler        = handler;
        }

        PlatformTimer::~PlatformTimer()
        {
            pHandler        = NULL;
        }

        Steinberg::tresult PLUGIN_API PlatformTimer::queryInterface(const Steinberg::TUID _iid, void **obj)
        {
            // Cast to the requested interface
            if (Steinberg::iidEqual(_iid, Steinberg::FUnknown::iid))
                return cast_interface<Steinberg::FUnknown>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::Linux::ITimerHandler::iid))
                return cast_interface<Steinberg::Linux::ITimerHandler>(this, obj);

            return no_interface(obj);
        }

        Steinberg::uint32 PLUGIN_API PlatformTimer::addRef()
        {
            return atomic_add(&nRefCounter, 1) + 1;
        }

        Steinberg::uint32 PLUGIN_API PlatformTimer::release()
        {
            atomic_t ref_count = atomic_add(&nRefCounter, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        void PLUGIN_API PlatformTimer::onTimer()
        {
            if (pHandler != NULL)
                pHandler->sync_ui();
        };

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* VST_USE_RUNLOOP_IFACE */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_TIMER_H_ */
