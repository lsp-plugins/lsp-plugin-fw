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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_TIMER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_TIMER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <steinberg/vst3.h>

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/data.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/sync.h>

#ifdef VST_USE_RUNLOOP_IFACE

namespace lsp
{
    namespace vst3
    {
        class PlatformTimer: public Steinberg::Linux::ITimerHandler
        {
            private:
                uatomic_t                               nRefCounter;
                IUISync                                *pHandler;

            public:
                PlatformTimer(IUISync *handler);
                virtual ~PlatformTimer();
                PlatformTimer(const PlatformTimer &) = delete;
                PlatformTimer(PlatformTimer &&) = delete;

                PlatformTimer & operator = (const PlatformTimer &) = delete;
                PlatformTimer & operator = (PlatformTimer &&) = delete;

            public: // Steinberg::FUnknown
                virtual Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;
                virtual Steinberg::uint32 PLUGIN_API addRef() override;
                virtual Steinberg::uint32 PLUGIN_API release() override;

            public: // Steinberg::Linux::ITimerHandler
                virtual void PLUGIN_API onTimer() override;
        };

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* VST_USE_RUNLOOP_IFACE */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_TIMER_H_ */
