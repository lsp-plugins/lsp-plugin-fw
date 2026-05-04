/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
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

                /**
                 * Initialize plugin module
                 *
                 * Caller thread: [user]
                 * @param wrapper plugin wrapper interface
                 * @param ports list of ports supplied by plugin wrapper
                 */
                virtual void                init(IWrapper *wrapper, IPort **ports);

                /**
                 * Destroy plugin module
                 * Caller thread: [any]
                 */
                virtual void                destroy();

            public:
                /**
                 * Get plugin's metadata. Realtime-safe.
                 * Caller thread: [any]
                 * @return plugin's metadata
                 */
                const meta::plugin_t       *metadata() const                { return pMetadata;         }

                /**
                 * Get the plugin's latency. The latency can be issued by some buffers that need to gather some
                 * audio data before starting the processing. For example, look-ahead compressors, FFT processors,
                 * linear-phase filters, etc. Delays should not report the latency because they
                 * Caller thread: [audio]
                 * @return plugin's latency in samples
                 */
                inline size_t               latency() const                 { return nLatency;          }

                /**
                 * Set the plugin's latency
                 * @param latency plugin;s latency
                 * Caller thread: [audio]
                 */
                inline void                 set_latency(ssize_t latency)    { nLatency = latency;       }

                /**
                 * Get the plugin's post-processing tail size (for example, reverb can report it's tail size.
                 * The plugin can have infinite tail, for example, when using some feed-back features
                 * (like feed-back delay).
                 * Caller thread: [audio]
                 * @return plugin's post-processing tail size in samples, 0 if no tail, negative for infinite tail.
                 */
                inline ssize_t              tail_size() const               { return nTailSize;         }

                /**
                 * Report the plugin's post-processing tail size in samples.
                 * Caller thread: [audio]
                 * @param tail post-processing tail size in samples, 0 if no tail, negative for infinite tail.
                 */
                inline void                 set_tail_size(ssize_t tail)     { nTailSize = tail;         }

                /**
                 * Set the plugin's actual sample rate. The plugin is safe for allocating some data or call some
                 * blocking functions.
                 * Caller thread: [user]
                 * @param sr sample rate.
                 */
                void                        set_sample_rate(uint32_t sr);

                /**
                 * Get current sample rate of the plugin
                 * Caller thread: [audio]
                 * @return current sample rate of the plugin
                 */
                inline uint32_t             sample_rate() const             { return fSampleRate;       }

                /**
                 * Called when the plugin is activated by the host
                 * Caller thread: [user]
                 */
                void                        activate();

                /**
                 * Called when the plugin is deactivated by the host
                 * Caller thread: [user]
                 */
                void                        deactivate();

                /**
                 * Set plugin's active/inactive state
                 * @param active active state flag
                 * Caller thread: [user]
                 */
                void                        set_active(bool active);

                /**
                 * Check current activity state of the plugin.
                 * @return true if plugin is active
                 * Caller thread: [user]
                 */
                inline bool                 active() const                  { return bActivated;        }

                /**
                 * Return the pointer to the wrapper which wraps the plugin into some plugin format and
                 * provides additional functions for interaction with the host and UI.
                 * @return pointer to the plugin wraper.
                 * Caller thread: [user]
                 */
                inline IWrapper            *wrapper()                       { return pWrapper;          }

                /**
                 * Called by the wrapper when the host starts at least one plugin's UI
                 * Caller thread: [audio]
                 */
                void                        activate_ui();

                /**
                 * Called when the host closes all of the plugin's UIs
                 * Caller thread: [audio]
                 */
                void                        deactivate_ui();

                /**
                 * Check that at least one plugin's UI is active
                 * Caller thread: [audio]
                 * @return true if plugin's UI is active
                 */
                inline bool                 ui_active() const               { return bUIActive;         }

            public:
                /**
                 * Update sample rate of data processing. The plugin is safe for allocating some data or call some
                 * blocking functions.
                 * Caller thread: [user]
                 *
                 * @param sr new sample rate
                 */
                virtual void                update_sample_rate(long sr);

                /**
                 * Handle plugin activation event. Afer activation the wrapper starts issuing process() and
                 * update_settings() calls.
                 * Caller thread: [user]
                 */
                virtual void                activated();

                /**
                 * Handle plugin deactivation event. Before the call the wrapper stops issuing process() and
                 * update_settings() calls.
                 * Caller thread: [user]
                 */
                virtual void                deactivated();

                /**
                 * Triggered change of one or more input ports, need to update plugin's setup.
                 * Caller thread: [audio]
                 */
                virtual void                update_settings();

                /**
                 * Report current time position for the plugin.
                 * Caller thread: [audio]
                 *
                 * @param pos current time position
                 * @return true if need to call for plugin setting update
                 */
                virtual bool                set_position(const position_t *pos);

                /**
                 * Process data. Called from realtime thread and can not issue any locks nor
                 * memory allocations nor system library calls except the calls that guarantee to
                 * be lock-free and of limited estimated execution time.
                 * Caller thread: [audio]
                 *
                 * @param samples number of samples to process
                 */
                virtual void                process(size_t samples);

                /**
                 * Draw inline display on canvas. This method is not called from the realtime thread
                 * and can issue synchronization locks and memory allocations.
                 * This feature will not work unless E_INLINE_DISPLAY extension is
                 * specified in plugin's metadata.
                 * Caller thread: [user]
                 *
                 * @param cv canvas for drawing plugin's inline display data
                 * @param width maximum canvas width
                 * @param height maximum canvas height
                 * @return status of operation
                 */
                virtual bool                inline_display(ICanvas *cv, size_t width, size_t height);

                /**
                 * Handle UI activation event. Issued when the wrapper detects at least one plugin's UI opened.
                 * Caller thread: [audio]
                 */
                virtual void                ui_activated();

                /**
                 * Handle UI deactivation event. Issued when the wrapper detects all of the plugin's UI closed.
                 * Caller thread: [audio]
                 */
                virtual void                ui_deactivated();

                /**
                 * Lock the KVT storage. Thread safe. Do not use this method in realtime thread. Use kvt_try_lock instead.
                 * Caller thread: [user]
                 * @return pointer to KVT storage or NULL
                 */
                virtual core::KVTStorage   *kvt_lock();

                /**
                 * Try to lock the KVT storage, thread safe
                 * Caller thread: [any]
                 * @return pointer to KVT storage or NULL if not locked/not supported
                 */
                virtual core::KVTStorage   *kvt_trylock();

                /**
                 * Release the KVT storage
                 * Caller thread: [any]
                 */
                virtual void                kvt_release();

                /**
                 * Callback before the state of the plugin becomes saved.
                 * Caller thread: [user]
                 */
                virtual void                before_state_save();

                /**
                 * Callback for case when plugin's state has been just saved
                 * Caller thread: [user]
                 */
                virtual void                state_saved();

                /**
                 * Callback before the state of the plugin becomes saved. The plugin can store to
                 * KVT some internal state that can be used after state has been loaded.
                 * Caller thread: [user]
                 */
                virtual void                before_state_load();

                /**
                 * Callback for case when plugin's state has been just loaded
                 * Caller thread: [user]
                 */
                virtual void                state_loaded();

                /**
                 * Dump plugin state. This is a debug call which breaks RT safety.
                 * Caller thread: [audio]
                 * @param v state dumper
                 */
                virtual void                dump(dspu::IStateDumper *v) const;
        };

    } /* namespace plug */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_BACK_MODULE_H_ */
