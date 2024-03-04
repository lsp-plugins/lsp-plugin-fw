/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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

                uint32_t                    fSampleRate;
                uint32_t                    nLatency;
                int32_t                     nTailSize;

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
                /**
                 * Get plugin's metadata
                 * @return plugin's metadata
                 */
                const meta::plugin_t       *metadata() const                { return pMetadata;         }

                /**
                 * Get the plugin's latency. The latency can be issued by some buffers that need to gather some
                 * audio data before starting the processing. For example, look-ahead compressors, FFT processors,
                 * linear-phase filters, etc. Delays should not report the latency because they
                 * @return plugin's latency in samples
                 */
                inline size_t               latency() const                 { return nLatency;          }
                inline void                 set_latency(ssize_t latency)    { nLatency = latency;       }

                /**
                 * Get the plugin's post-processing tail size (for example, reverb can report it's tail size.
                 * The plugin can have infinite tail, for example, when using some feed-back features
                 * (like feed-back delay).
                 * @return plugin's post-processing tail size in samples, 0 if no tail, negative for infinite tail.
                 */
                inline ssize_t              tail_size() const               { return nTailSize;         }

                /**
                 * Report the plugin's post-processing tail size in samples.
                 * @param tail post-processing tail size in samples, 0 if no tail, negative for infinite tail.
                 */
                inline void                 set_tail_size(ssize_t tail)     { nTailSize = tail;         }

                /**
                 * Set the plugin's actual sample rate. The plugin is safe for allocating some data or call some
                 * blocking functions.
                 * @param sr sample rate.
                 */
                void                        set_sample_rate(uint32_t sr);

                /**
                 * Get current sample rate of the plugin
                 * @deprecated use sample_rate() instead
                 * @return current sample rate of the plugin
                 */
                inline uint32_t             get_sample_rate() const         { return fSampleRate;       }

                /**
                 * Get current sample rate of the plugin
                 * @deprecated use sample_rate() instead
                 * @return current sample rate of the plugin
                 */
                inline uint32_t             sample_rate() const             { return fSampleRate;       }

                /**
                 * Called when the plugin is activated by the host
                 */
                void                        activate();

                /**
                 * Called when the plugin is deactivated by the host
                 */
                void                        deactivate();

                /**
                 * Check current activity state of the plugin.
                 * @return true if plugin is active
                 */
                inline bool                 active() const                  { return bActivated;        }

                /**
                 * Return the pointer to the wrapper which wraps the plugin into some plugin format and
                 * provides additional functions for interaction with the host and UI.
                 * @return pointer to the plugin wraper.
                 */
                inline IWrapper            *wrapper()                       { return pWrapper;          }

                /**
                 * Called when the host starts the plugin's UI
                 */
                void                        activate_ui();

                /**
                 * Called when the host closes the plugin's UI
                 */
                void                        deactivate_ui();

                /**
                 * Check that plugin's UI is active
                 * @return true if plugin's UI is active
                 */
                inline bool                 ui_active() const               { return bUIActive;         }

            public:
                /** Update sample rate of data processing. The plugin is safe for allocating some data or call some
                 * blocking functions.
                 *
                 * @param sr new sample rate
                 */
                virtual void                update_sample_rate(long sr);

                /**
                 * Handle plugin activation event. Consider this method is called from realtime thread.
                 */
                virtual void                activated();

                /**
                 * Handle plugin deactivation event. Consider this method is called from realtime thread.
                 */
                virtual void                deactivated();

                /**
                 * Triggered input port change, need to update configuration
                 */
                virtual void                update_settings();

                /**
                 * Report current time position for plugin
                 *
                 * @param pos current time position
                 * @return true if need to call for plugin setting update
                 */
                virtual bool                set_position(const position_t *pos);

                /**
                 * Process data. Called from realtime thread and can not issue any locks nor
                 * memory allocations nor system library calls except the calls that guarantee to
                 * be lock-free and of limited estimated execution time.
                 *
                 * @param samples number of samples to process
                 */
                virtual void                process(size_t samples);

                /**
                 * Draw inline display on canvas. This method is not called from the realtime thread
                 * and can issue synchronization locks and memory allocations.
                 * This feature will not work unless E_INLINE_DISPLAY extension is
                 * specified in plugin's metadata.
                 *
                 * @param cv canvas for drawing plugin's inline display data
                 * @param width maximum canvas width
                 * @param height maximum canvas height
                 * @return status of operation
                 */
                virtual bool                inline_display(ICanvas *cv, size_t width, size_t height);

                /**
                 * Handle UI activation event. Consider this method is called from realtime thread.
                 */
                virtual void                ui_activated();

                /**
                 * Handle UI deactivation event. Consider this method is called from realtime thread.
                 */
                virtual void                ui_deactivated();

                /**
                 * Lock the KVT storage. Do not use this method in realtime thread. Use kvt_try_lock instead.
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

                /**
                 * Callback for case when plugin's state has been saved
                 */
                virtual void                state_saved();

                /**
                 * Callback for case when plugin's state has been loaded
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
