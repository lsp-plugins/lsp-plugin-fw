/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_BACK_MODULE_H_
#define LSP_PLUG_IN_PLUG_FW_BACK_MODULE_H_

#ifndef LSP_PLUG_IN_PLUG_FW_PLUG_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/plug.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_PLUG_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/plug/IPort.h>
#include <lsp-plug.in/plug-fw/plug/ICanvas.h>
#include <lsp-plug.in/plug-fw/plug/data.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp-units/iface/IStateDumper.h>

namespace lsp
{
    namespace plug
    {
        class IWrapper;

        /**
         * Main plugin class
         */
        class Module
        {
            protected:
                const meta::plugin_t       *pMetadata;
                IWrapper                   *pWrapper;

                long                        fSampleRate;
                ssize_t                     nLatency;
                bool                        bActivated;
                bool                        bUIActive;

            public:
                explicit Module(const meta::plugin_t *meta);
                Module(const Module &) = delete;
                Module(Module &&) = delete;
                virtual ~Module();

                Module &operator = (const Module &) = delete;
                Module &operator = (Module &&) = delete;

                /** Initialize plugin module
                 *
                 * @param wrapper plugin wrapper interface
                 * @param ports list of ports supplied by plugin wrapper
                 */
                virtual void                init(IWrapper *wrapper, IPort **ports);

                /** Destroy plugin module
                 *
                 */
                virtual void                destroy();

            public:
                const meta::plugin_t       *metadata() const                { return pMetadata;         }
                inline ssize_t              latency() const                 { return nLatency;          }
                inline void                 set_latency(ssize_t latency)    { nLatency = latency;       }

                void                        set_sample_rate(long sr);

                inline long                 get_sample_rate() const         { return fSampleRate;       }
                inline bool                 active() const                  { return bActivated;        }
                inline bool                 ui_active() const               { return bUIActive;         }

                inline IWrapper            *wrapper()                       { return pWrapper;          }

                void                        activate_ui();
                void                        deactivate_ui();
                void                        activate();
                void                        deactivate();

            public:
                /** Update sample rate of data processing
                 *
                 * @param sr new sample rate
                 */
                virtual void                update_sample_rate(long sr);

                /** Triggered plugin activation
                 *
                 */
                virtual void                activated();

                /** Triggered UI activation
                 *
                 */
                virtual void                ui_activated();

                /** Triggered input port change, need to update configuration
                 *
                 */
                virtual void                update_settings();

                /** Report current time position for plugin
                 *
                 * @param pos current time position
                 * @return true if need to call for plugin setting update
                 */
                virtual bool                set_position(const position_t *pos);

                /** Process data
                 *
                 * @param samples number of samples to process
                 */
                virtual void                process(size_t samples);

                /** Draw inline display on canvas
                 * This feature will not work unless E_INLINE_DISPLAY extension is
                 * specified in plugin's metadata
                 *
                 * @param cv canvas
                 * @param width maximum canvas width
                 * @param height maximum canvas height
                 * @return status of operation
                 */
                virtual bool                inline_display(ICanvas *cv, size_t width, size_t height);

                /** Triggered UI deactivation
                 *
                 */
                virtual void                ui_deactivated();

                /** Triggered plugin deactivation
                 *
                 */
                virtual void                deactivated();

                /**
                 * Lock the KVT storage
                 * @return pointer to KVT storage or NULL
                 */
                virtual core::KVTStorage   *kvt_lock();

                /**
                 * Try to lock the KVT storage
                 * @return pointer to KVT storage or NULL if not locked/not supported
                 */
                virtual core::KVTStorage   *kvt_trylock();

                /**
                 * Release the KVT storage
                 */
                virtual void                kvt_release();

                /** Callback for case when plugin's state has been saved
                 *
                 */
                virtual void                state_saved();

                /** Callback for case when plugin's state has been loaded
                 *
                 */
                virtual void                state_loaded();

                /**
                 * Dump plugin state
                 * @param v state dumper
                 */
                virtual void                dump(dspu::IStateDumper *v) const;
        };

    } /* namespace plug */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_BACK_MODULE_H_ */
