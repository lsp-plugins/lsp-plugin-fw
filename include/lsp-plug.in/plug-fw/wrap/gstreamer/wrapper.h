/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of llsp-plugin-fw
 * Created on: 25 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>
#include <lsp-plug.in/plug-fw/plug.h>

#include <lsp-plug.in/plug-fw/wrap/gstreamer/ports.h>

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

namespace lsp
{
    namespace gst
    {
        class Factory;

        /**
         * Wrapper for the plugin module
         */
        class Wrapper: public plug::IWrapper, public gst::IWrapper
        {
            protected:
                gst::Factory                       *pFactory;           // Plugin factory
                GstAudioFilter                     *pFilter;            // GStreamer audio filter class
                const meta::package_t              *pPackage;           // Package information
                ipc::IExecutor                     *pExecutor;          // Executor service
                ssize_t                             nLatency;           // Current latency value
                uint32_t                            nChannels;          // Number of channels
                uint32_t                            nFrameSize;         // Frame size
                uint32_t                            nSampleRate;        // Sample rate

                bool                                bUpdateSettings;    // Settings update flag
                bool                                bUpdateSampleRate;  // Flag that forces plugin to update sample rate
                bool                                bInterleaved;       // Interleaved audio layout

                lltl::parray<plug::IPort>           vAllPorts;          // All created ports
                lltl::parray<gst::AudioPort>        vAudioIn;           // All available audio inputs
                lltl::parray<gst::AudioPort>        vAudioOut;          // All available audio outputs
                lltl::parray<gst::MidiPort>         vMidiIn;            // All available MIDI inputs
                lltl::parray<gst::MidiPort>         vMidiOut;           // All available MIDI outputs
                lltl::parray<gst::ParameterPort>    vParameters;        // All available parameters
                lltl::parray<gst::MeterPort>        vMeters;            // All available meters
                lltl::parray<plug::IPort>           vPortMapping;       // All parameters visible to the host
                lltl::parray<meta::port_t>          vGenMetadata;       // Generated metadata
                lltl::parray<gst::AudioPort>        vSink;              // All available audio inputs
                lltl::parray<gst::AudioPort>        vSource;            // All available audio outputs

                core::KVTStorage                    sKVT;               // Key-value tree
                ipc::Mutex                          sKVTMutex;          // Key-value tree lock mutex
                core::SamplePlayer                 *pSamplePlayer;      // Sample player

            protected:
                plug::IPort                        *create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix);
                void                                do_destroy();
                void                                report_latency();
                GstClockTime                        gst_latency() const;

                static ssize_t                      compare_port_items(const meta::port_group_item_t *a, const meta::port_group_item_t *b);
                static gst::AudioPort              *find_port(lltl::parray<gst::AudioPort> & list, const char *id);
                static void                         make_audio_mapping(
                                                        lltl::parray<gst::AudioPort> & dst,
                                                        lltl::parray<gst::AudioPort> & list,
                                                        const meta::plugin_t *meta,
                                                        bool out);
                static void                         make_port_group_mapping(
                                                        lltl::parray<gst::AudioPort> & dst,
                                                        lltl::parray<gst::AudioPort> & list,
                                                        const meta::port_group_t *grp);
                static void                         make_port_mapping(
                                                        lltl::parray<gst::AudioPort> & dst,
                                                        lltl::parray<gst::AudioPort> & list,
                                                        bool out);
                static void                         clear_interleaved(float *dst, size_t stride, size_t count);

                void                                process_input_events();
                void                                process_output_events();

            public:
                explicit Wrapper(gst::Factory *factory, GstAudioFilter *filter, plug::Module *plugin, resource::ILoader *loader);
                virtual ~Wrapper() override;

                status_t                            init();
                void                                destroy();

            public: // plug::IWrapper
                virtual ipc::IExecutor             *executor() override;
                virtual const meta::package_t      *package() const override;
                virtual void                        request_settings_update() override;
                virtual meta::plugin_format_t       plugin_format() const override;
                virtual core::KVTStorage           *kvt_lock() override;
                virtual core::KVTStorage           *kvt_trylock() override;
                virtual bool                        kvt_release() override;

            public: // gst::IWrapper interface
                virtual void                        setup(const GstAudioInfo * info) override;
                virtual void                        change_state(GstStateChange transition) override;
                virtual void                        set_property(guint prop_id, const GValue *value, GParamSpec *pspec) override;
                virtual void                        get_property(guint prop_id, GValue *value, GParamSpec *pspec) override;
                virtual gboolean                    query(GstPad *pad, GstQuery *query) override;
                virtual void                        process(guint8 *out, const guint8 *in, size_t out_size, size_t in_size) override;
        };
    } /* namespace gst */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_WRAPPER_H_ */
