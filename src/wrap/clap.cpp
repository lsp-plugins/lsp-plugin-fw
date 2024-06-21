/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 дек. 2022 г.
 *
 * lsp-plugin-fw is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugin-fw is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more des.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugin-fw. If not, see <https://www.gnu.org/licenses/>.
 */

#include <clap/clap.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/singletone.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/clap/debug.h>
#include <lsp-plug.in/plug-fw/wrap/clap/impl/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/clap/impl/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/clap/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/clap/wrapper.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>


#define CLAP_LOG_FILE           "lsp-clap.log"

namespace lsp
{
    namespace clap
    {
        //---------------------------------------------------------------------
        // List of plugin descriptors. The list is generated at first demand because
        // of undetermined order of initialization of static objects which may
        // cause undefined behaviour when reading the list of plugin factories
        // which can be not fully initialized.
        static lltl::darray<clap_plugin_descriptor_t> descriptors;
        static meta::package_t *package_manifest = NULL;
        static lsp::singletone_t library;

        //---------------------------------------------------------------------
        // Audio ports extension
        static uint32_t audio_ports_count(const clap_plugin_t *plugin, bool is_input)
        {
            lsp_trace("plugin=%p, is_input=%d", plugin, int(is_input));

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->audio_ports_count(is_input);
        }

        static bool audio_ports_get(
            const clap_plugin_t *plugin,
            uint32_t index,
            bool is_input,
            clap_audio_port_info_t *info)
        {
            lsp_trace("plugin=%p, index=%d, is_input=%d, info=%p", plugin, index, int(is_input), info);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->audio_port_info(info, index, is_input) == STATUS_OK;
        }

        static const clap_plugin_audio_ports_t audio_ports_extension =
        {
            .count = audio_ports_count,
            .get = audio_ports_get,
        };

        //---------------------------------------------------------------------
        // Note ports extension
        static uint32_t note_ports_count(const clap_plugin_t *plugin, bool is_input)
        {
            lsp_trace("plugin=%p, is_input=%d", plugin, int(is_input));

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->note_ports_count(is_input);
        }

        static bool note_ports_get(
            const clap_plugin_t *plugin,
            uint32_t index,
            bool is_input,
            clap_note_port_info_t *info)
        {
            lsp_trace("plugin=%p, index=%d, is_input=%d, info=%p", plugin, index, int(is_input), info);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->note_port_info(info, index, is_input) == STATUS_OK;
        }

        static const clap_plugin_note_ports_t note_ports_extension =
        {
            .count = note_ports_count,
            .get = note_ports_get,
        };

        //---------------------------------------------------------------------
        // Parameters extension
        // Returns the number of parameters.
        // [main-thread]
        uint32_t CLAP_ABI params_count(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->params_count();
        }

        bool CLAP_ABI get_param_info(
            const clap_plugin_t *plugin,
            uint32_t param_index,
            clap_param_info_t *param_info)
        {
            lsp_trace("plugin=%p, param_index=%d, param_info=%p", plugin, int(param_index), param_info);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->param_info(param_info, param_index) == STATUS_OK;
        }

        bool CLAP_ABI get_param_value(const clap_plugin_t *plugin, clap_id param_id, double *value)
        {
            lsp_trace("plugin=%p, param_id=0x%08x, value=%p", plugin, int(param_id), value);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->get_param_value(value, param_id) == STATUS_OK;
        }

        bool CLAP_ABI format_param_value(
           const clap_plugin_t *plugin,
           clap_id param_id,
           double value,
           char *display,
           uint32_t size)
        {
            lsp_trace("plugin=%p, param_id=0x%08x, value=%f, display=%p, size=%d",
                plugin, int(param_id), value, display, int(size));

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->format_param_value(display, size, param_id, value) == STATUS_OK;
        }

        bool CLAP_ABI parse_param_value(
            const clap_plugin_t *plugin,
            clap_id param_id,
            const char *display,
            double *value)
        {
            lsp_trace("plugin=%p, param_id=0x%08x, display=%s, value=%p",
                plugin, int(param_id), display, value);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->parse_param_value(value, param_id, display) == STATUS_OK;
        }

        void CLAP_ABI flush_param_events(
            const clap_plugin_t *plugin,
            const clap_input_events_t *in,
            const clap_output_events_t *out)
        {
            lsp_trace("plugin=%p, in=%p, out=%p", plugin, in, out);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            w->flush_param_events(in, out);
        }

        static const clap_plugin_params_t plugin_params_extension =
        {
            .count          = params_count,
            .get_info       = get_param_info,
            .get_value      = get_param_value,
            .value_to_text  = format_param_value,
            .text_to_value  = parse_param_value,
            .flush          = flush_param_events,
        };

        //---------------------------------------------------------------------
        // Latency extension
        uint32_t CLAP_ABI get_latency(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->latency();
        }

        static const clap_plugin_latency_t latency_extension =
        {
            .get = get_latency,
        };

        //---------------------------------------------------------------------
        // State extension
        bool CLAP_ABI save_state(const clap_plugin_t *plugin, const clap_ostream_t *stream)
        {
            lsp_trace("plugin=%p, stream=%p", plugin, stream);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);

//        #ifdef LSP_TRACE
//            debug_ostream_t dbg(stream);
//
//            status_t res = w->save_state(&dbg);
//            if (res == STATUS_OK)
//            {
//                lsp_dumpb("Output state dump", dbg.data(), dbg.size());
//                res     = dbg.flush();
//            }
//
//            return res == STATUS_OK;
//        #else
            return w->save_state(stream) == STATUS_OK;
//        #endif /* LSP_TRACE */
        }

        bool CLAP_ABI load_state(const clap_plugin_t *plugin, const clap_istream_t *stream)
        {
            lsp_trace("plugin=%p, stream=%p", plugin, stream);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);

//        #ifdef LSP_TRACE
//            debug_istream_t dbg(stream);
//            status_t res = dbg.fill();
//            if (res == STATUS_OK)
//            {
//                lsp_dumpb("Input state dump", dbg.data(), dbg.size());
//                res     = w->load_state(&dbg);
//            }
//            return res == STATUS_OK;
//        #else
            return w->load_state(stream) == STATUS_OK;
//        #endif /* LSP_TRACE */
        }

        const clap_plugin_state_t state_extension =
        {
            .save = save_state,
            .load = load_state,
        };

        //---------------------------------------------------------------------
        // Tail extension
        uint32_t CLAP_ABI get_tail(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->tail_size();
        }

        const clap_plugin_tail tail_extension =
        {
            .get = get_tail
        };

        //---------------------------------------------------------------------
        // UI extension
        bool CLAP_ABI ui_is_api_supported(const clap_plugin_t *plugin, const char *api, bool is_floating)
        {
            lsp_trace("plugin = %p, api=%s, is_floating=%d", plugin, api, int(is_floating));
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            if (!w->ui_provided())
                return false;

        #if defined(PLATFORM_WINDOWS)
            if (!strcmp(api, CLAP_WINDOW_API_WIN32))
                return true;
        #elif defined(PLATFORM_MACOSX)
            if (!strcmp(api, CLAP_WINDOW_API_COCOA))
                return true;
        #else
            if (!strcmp(api, CLAP_WINDOW_API_X11))
                return true;
        #endif

            return false;
        }

        bool CLAP_ABI ui_get_preferred_api(const clap_plugin_t *plugin, const char **api, bool *is_floating)
        {
            lsp_trace("plugin = %p, api=%p, is_floating=%p", plugin, api, is_floating);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            if (!w->ui_provided())
                return false;

        #if defined(PLATFORM_WINDOWS)
            *api        = CLAP_WINDOW_API_WIN32;
        #elif defined(PLATFORM_MACOSX)
            *api        = CLAP_WINDOW_API_COCOA;
        #else
            *api        = CLAP_WINDOW_API_X11;
        #endif
            *is_floating= false;

            return true;
        }

        bool CLAP_ABI ui_create(const clap_plugin_t *plugin, const char *api, bool is_floating)
        {
            lsp_trace("plugin = %p, api=%s, is_floating=%d", plugin, api, int(is_floating));
            if (!ui_is_api_supported(plugin, api, is_floating))
                return false;

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->create_ui();
            return uw != NULL;
        }

        void CLAP_ABI ui_destroy(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin = %p", plugin);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            w->destroy_ui();
        }

        bool CLAP_ABI ui_set_scale(const clap_plugin_t *plugin, double scale)
        {
            lsp_trace("plugin = %p, scale=%f", plugin, scale);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            return (uw != NULL) ? uw->set_scale(scale) : false;
        }

        bool CLAP_ABI ui_get_size(const clap_plugin_t *plugin, uint32_t *width, uint32_t *height)
        {
            lsp_trace("plugin = %p, width=%p, height=%p", plugin, width, height);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            return (uw != NULL) ? uw->get_size(width, height) : false;
        }

        bool CLAP_ABI ui_can_resize(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin = %p", plugin);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            return (uw != NULL) ? uw->can_resize() : false;
        }

        bool CLAP_ABI ui_get_resize_hints(const clap_plugin_t *plugin, clap_gui_resize_hints_t *hints)
        {
            lsp_trace("plugin = %p, hints=%p", plugin, hints);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            return (uw != NULL) ? uw->get_resize_hints(hints) : false;
        }

        bool CLAP_ABI ui_adjust_size(const clap_plugin_t *plugin, uint32_t *width, uint32_t *height)
        {
            lsp_trace("plugin = %p, width=%p, height=%p", plugin, width, height);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            return (uw != NULL) ? uw->adjust_size(width, height) : false;
        }

        bool CLAP_ABI ui_set_size(const clap_plugin_t *plugin, uint32_t width, uint32_t height)
        {
            lsp_trace("plugin = %p, width=%d, height=%d", plugin, int(width), int(height));
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            return (uw != NULL) ? uw->set_size(width, height) : false;
        }

        bool CLAP_ABI ui_set_parent(const clap_plugin_t *plugin, const clap_window_t *window)
        {
            lsp_trace("plugin = %p, window=%p data=%p", plugin, window);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();

            lsp_trace("ui_wrapper=%p", uw);
            bool result = (uw != NULL) ? uw->set_parent(window) : false;

            lsp_trace("plugin = %p, window=%p result=%s", plugin, window, (result) ? "true" : "false");

            return result;
        }

        bool CLAP_ABI ui_set_transient(const clap_plugin_t *plugin, const clap_window_t *window)
        {
            lsp_trace("plugin = %p, window=%p", plugin, window);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            return (uw != NULL) ? uw->set_transient(window) : false;
        }

        void CLAP_ABI ui_suggest_title(const clap_plugin_t *plugin, const char *title)
        {
            lsp_trace("plugin = %p, title=%s", plugin, title);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            if (uw != NULL)
                uw->suggest_title(title);
        }

        bool CLAP_ABI ui_show(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin = %p", plugin);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            return (uw != NULL) ? uw->show() : false;
        }

        bool CLAP_ABI ui_hide(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin = %p", plugin);
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            UIWrapper *uw = w->ui_wrapper();
            return (uw != NULL) ? uw->hide() : false;
        }

        const clap_plugin_gui_t ui_extension =
        {
            .is_api_supported = ui_is_api_supported,
            .get_preferred_api = ui_get_preferred_api,
            .create = ui_create,
            .destroy = ui_destroy,
            .set_scale = ui_set_scale,
            .get_size = ui_get_size,
            .can_resize = ui_can_resize,
            .get_resize_hints = ui_get_resize_hints,
            .adjust_size = ui_adjust_size,
            .set_size = ui_set_size,
            .set_parent = ui_set_parent,
            .set_transient = ui_set_transient,
            .suggest_title = ui_suggest_title,
            .show = ui_show,
            .hide = ui_hide
        };

        //---------------------------------------------------------------------
        // Plugin instance related stuff
        bool CLAP_ABI init(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->init() == STATUS_OK;
        }

        void CLAP_ABI destroy(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            // Destroy the wrapper
            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            if (w != NULL)
            {
                w->destroy();
                delete w;
            }

            // Free the plugin data structure as required by CLAP specification
            free(const_cast<clap_plugin_t *>(plugin));
        }

        bool CLAP_ABI activate(
            const clap_plugin_t *plugin,
            double sample_rate,
            uint32_t min_frames_count,
            uint32_t max_frames_count)
        {
            lsp_trace("plugin=%p, sample_rate=%f, min_frames_count=%d, max_frames_count=%d",
                plugin, sample_rate, int(min_frames_count), int(max_frames_count));

            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            status_t res = w->activate(sample_rate, min_frames_count, max_frames_count);

            lsp_trace("wrapper=%p, activate result = %d", w, int(res));

            return res == STATUS_OK;
        }

        void CLAP_ABI deactivate(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            w->deactivate();
        }

        bool CLAP_ABI start_processing(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->start_processing() == STATUS_OK;
        }

        void CLAP_ABI stop_processing(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            w->stop_processing();
        }

        void CLAP_ABI reset(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            w->reset();
        }

        clap_process_status CLAP_ABI process(
            const struct clap_plugin *plugin,
            const clap_process_t *process)
        {
            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

//            lsp_trace("plugin=%p, process=%p", plugin, process);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            return w->process(process);
        }

        const void * CLAP_ABI get_extension(const clap_plugin_t *plugin, const char *id)
        {
            lsp_trace("plugin=%p, id=%s", plugin, id);

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);

            if (!strcmp(id, CLAP_EXT_LATENCY))
                return &latency_extension;
            if (!strcmp(id, CLAP_EXT_AUDIO_PORTS))
                return &audio_ports_extension;
            if (!strcmp(id, CLAP_EXT_STATE))
                return &state_extension;
            if (!strcmp(id, CLAP_EXT_PARAMS))
                return &plugin_params_extension;
            if ((!strcmp(id, CLAP_EXT_NOTE_PORTS)) && (w->has_note_ports()))
                return &note_ports_extension;
            if (!strcmp(id, CLAP_EXT_TAIL))
                return &tail_extension;
            if ((!strcmp(id, CLAP_EXT_GUI)) && (w->ui_provided()))
                return &ui_extension;

            return w->get_extension(id);
        }

        void CLAP_ABI on_main_thread(const clap_plugin_t *plugin)
        {
            lsp_trace("plugin=%p", plugin);

            dsp::context_t ctx;
            dsp::start(&ctx);
            lsp_finally { dsp::finish(&ctx); };

            Wrapper *w = static_cast<Wrapper *>(plugin->plugin_data);
            w->on_main_thread();
        }

        //---------------------------------------------------------------------
        // Factory-related stuff
        static meta::package_t *load_manifest(resource::ILoader *loader)
        {
            status_t res;
            if (loader == NULL)
                return NULL;

            io::IInStream *is = loader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
            if (is == NULL)
                return NULL;
            lsp_finally {
                is->close();
                delete is;
            };

            meta::package_t *manifest = NULL;
            if ((res = meta::load_manifest(&manifest, is)) != STATUS_OK)
            {
                lsp_warn("Error loading manifest file, error=%d", int(res));
                return NULL;
            }

            return manifest;
        }

        static ssize_t cmp_descriptors(const clap_plugin_descriptor_t *d1, const clap_plugin_descriptor_t *d2)
        {
            return strcmp(d1->id, d2->id);
        }

        static const char * const *make_feature_list(const meta::plugin_t *meta)
        {
            // Estimate the overall number of CLAP features
            size_t count = 0;
            for (const int *f=meta->clap_features; *f >= 0; ++f)
                if (*f < meta::CF_TOTAL)
                    ++count;

            // Allocate the array of the corresponding size;
            const char **list = static_cast<const char **>(malloc(sizeof(const char *) * (count + 1)));
            if (list == NULL)
                return NULL;

            // Fill the list with CLAP features
            count = 0;
            for (const int *f=meta->clap_features; *f >= 0; ++f)
            {
                const char *cf = NULL;
                switch (*f)
                {
                    case meta::CF_INSTRUMENT:       cf = CLAP_PLUGIN_FEATURE_INSTRUMENT; break;
                    case meta::CF_AUDIO_EFFECT:     cf = CLAP_PLUGIN_FEATURE_AUDIO_EFFECT; break;
                    case meta::CF_NOTE_EFFECT:      cf = CLAP_PLUGIN_FEATURE_NOTE_EFFECT; break;
                    case meta::CF_ANALYZER:         cf = CLAP_PLUGIN_FEATURE_ANALYZER; break;
                    case meta::CF_SYNTHESIZER:      cf = CLAP_PLUGIN_FEATURE_SYNTHESIZER; break;
                    case meta::CF_SAMPLER:          cf = CLAP_PLUGIN_FEATURE_SAMPLER; break;
                    case meta::CF_DRUM:             cf = CLAP_PLUGIN_FEATURE_DRUM; break;
                    case meta::CF_DRUM_MACHINE:     cf = CLAP_PLUGIN_FEATURE_DRUM_MACHINE; break;
                    case meta::CF_FILTER:           cf = CLAP_PLUGIN_FEATURE_FILTER; break;
                    case meta::CF_PHASER:           cf = CLAP_PLUGIN_FEATURE_PHASER; break;
                    case meta::CF_EQUALIZER:        cf = CLAP_PLUGIN_FEATURE_EQUALIZER; break;
                    case meta::CF_DEESSER:          cf = CLAP_PLUGIN_FEATURE_DEESSER; break;
                    case meta::CF_PHASE_VOCODER:    cf = CLAP_PLUGIN_FEATURE_PHASE_VOCODER; break;
                    case meta::CF_GRANULAR:         cf = CLAP_PLUGIN_FEATURE_GRANULAR; break;
                    case meta::CF_FREQUENCY_SHIFTER:cf = CLAP_PLUGIN_FEATURE_FREQUENCY_SHIFTER; break;
                    case meta::CF_PITCH_SHIFTER:    cf = CLAP_PLUGIN_FEATURE_PITCH_SHIFTER; break;
                    case meta::CF_DISTORTION:       cf = CLAP_PLUGIN_FEATURE_DISTORTION; break;
                    case meta::CF_TRANSIENT_SHAPER: cf = CLAP_PLUGIN_FEATURE_TRANSIENT_SHAPER; break;
                    case meta::CF_COMPRESSOR:       cf = CLAP_PLUGIN_FEATURE_COMPRESSOR; break;
                    case meta::CF_LIMITER:          cf = CLAP_PLUGIN_FEATURE_LIMITER; break;
                    case meta::CF_FLANGER:          cf = CLAP_PLUGIN_FEATURE_FLANGER; break;
                    case meta::CF_CHORUS:           cf = CLAP_PLUGIN_FEATURE_CHORUS; break;
                    case meta::CF_DELAY:            cf = CLAP_PLUGIN_FEATURE_DELAY; break;
                    case meta::CF_REVERB:           cf = CLAP_PLUGIN_FEATURE_REVERB; break;
                    case meta::CF_TREMOLO:          cf = CLAP_PLUGIN_FEATURE_TREMOLO; break;
                    case meta::CF_GLITCH:           cf = CLAP_PLUGIN_FEATURE_GLITCH; break;
                    case meta::CF_UTILITY:          cf = CLAP_PLUGIN_FEATURE_UTILITY; break;
                    case meta::CF_PITCH_CORRECTION: cf = CLAP_PLUGIN_FEATURE_PITCH_CORRECTION; break;
                    case meta::CF_RESTORATION:      cf = CLAP_PLUGIN_FEATURE_RESTORATION; break;
                    case meta::CF_MULTI_EFFECTS:    cf = CLAP_PLUGIN_FEATURE_MULTI_EFFECTS; break;
                    case meta::CF_MIXING:           cf = CLAP_PLUGIN_FEATURE_MIXING; break;
                    case meta::CF_MASTERING:        cf = CLAP_PLUGIN_FEATURE_MASTERING; break;
                    case meta::CF_MONO:             cf = CLAP_PLUGIN_FEATURE_MONO; break;
                    case meta::CF_STEREO:           cf = CLAP_PLUGIN_FEATURE_STEREO; break;
                    case meta::CF_SURROUND:         cf = CLAP_PLUGIN_FEATURE_SURROUND; break;
                    case meta::CF_AMBISONIC:        cf = CLAP_PLUGIN_FEATURE_AMBISONIC; break;
                    default:
                        break;
                }
                if (cf != NULL)
                    list[count++] = cf;
            }

            // Add NULL terminator
            list[count]     = NULL;

            return list;
        }

        static void gen_descriptors()
        {
            // Check that data already has been initialized
            if (library.initialized())
                return;

            // Obtain the resource loader
            lsp_trace("Obtaining resource loader...");
            resource::ILoader *loader = core::create_resource_loader();
            lsp_finally {
                if (loader != NULL)
                    delete loader;
            };

            // Obtain the manifest
            lsp_trace("Obtaining manifest...");
            meta::package_t *manifest = load_manifest(loader);
            lsp_finally {
                if (manifest != NULL)
                    meta::free_manifest(manifest);
            };
            if (manifest == NULL)
                lsp_trace("No manifest file found");

            // Generate descriptors
            lltl::darray<clap_plugin_descriptor_t> result;
            lsp_finally { result.flush(); };

            lsp_trace("generating descriptors...");

            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Skip plugins not compatible with CLAP
                    const meta::plugin_t *meta = f->enumerate(i);
                    if ((meta == NULL) || (meta->uids.clap == NULL))
                        break;

                    // Allocate new descriptor
                    clap_plugin_descriptor_t *d = result.add();
                    if (d == NULL)
                    {
                        lsp_warn("Error allocating CLAP descriptor for plugin %s", meta->uids.clap);
                        continue;
                    }

                    // Initialize descriptor
                    char *tmp;
                    bzero(d, sizeof(*d));

                    d->clap_version     = CLAP_VERSION;
                    d->id               = meta->uids.clap;
                    d->name             = meta->description;
                    d->vendor           = NULL;
                    d->url              = NULL;
                    d->manual_url       = NULL;
                    d->support_url      = NULL;
                    d->version          = NULL;
                    d->description      = (meta->bundle != NULL) ? meta->bundle->description : NULL;
                    d->features         = make_feature_list(meta);

                    if (asprintf(&tmp, "%d.%d.%d",
                        int(LSP_MODULE_VERSION_MAJOR(meta->version)),
                        int(LSP_MODULE_VERSION_MINOR(meta->version)),
                        int(LSP_MODULE_VERSION_MICRO(meta->version))) >= 0)
                        d->version          = tmp;

                    if (manifest != NULL)
                    {
                        if (asprintf(&tmp, "%s CLAP", manifest->brand) >= 0)
                            d->vendor           = tmp;
                        d->url              = manifest->site;
                        if (asprintf(&tmp, "%s/doc/%s/html/plugins/%s.html", manifest->site, "lsp-plugins", meta->uid) >= 0)
                            d->manual_url       = tmp;
                    }
                }
            }

            // Sort descriptors
            result.qsort(cmp_descriptors);

        #ifdef LSP_TRACE
            lsp_trace("generated %d descriptors:", int(result.size()));
            for (size_t i=0, n=result.size(); i<n; ++i)
            {
                clap_plugin_descriptor_t *d = result.uget(i);
                lsp_trace("[%4d] %p: %s", int(i), d, d->id);
            }
        #endif /* LSP_TRACE */

            // Commit the generated objects to the global variables
            lsp_singletone_init(library) {
                descriptors.swap(result);
                lsp::swap(package_manifest, manifest);
            };
        }

        static void drop_descriptor(clap_plugin_descriptor_t *d)
        {
            if (d == NULL)
                return;

            if (d->version != NULL)
                free(const_cast<char *>(d->version));
            if (d->vendor != NULL)
                free(const_cast<char *>(d->vendor));
            if (d->manual_url != NULL)
                free(const_cast<char *>(d->manual_url));
            if (d->features != NULL)
                free(const_cast<char **>(d->features));

            d->version          = NULL;
            d->vendor           = NULL;
            d->manual_url       = NULL;
        }

        static void drop_descriptors()
        {
            lsp_trace("dropping %d descriptors", int(descriptors.size()));
            for (size_t i=0, n=descriptors.size(); i<n; ++i)
                drop_descriptor(descriptors.uget(i));
            descriptors.flush();
        }

        static uint32_t get_plugin_count(const clap_plugin_factory_t *factory)
        {
            return descriptors.size();
        }

        static const clap_plugin_descriptor_t *get_plugin_descriptor(
            const clap_plugin_factory_t *factory,
            uint32_t index)
        {
            return descriptors.get(index);
        }

        static const clap_plugin_t *create_plugin(
            const clap_plugin_factory_t *factory,
            const clap_host_t *host,
            const char *plugin_id)
        {
            if (!clap_version_is_compatible(host->clap_version))
                return NULL;

            // Find the corresponding CLAP plugin descriptor
            const clap_plugin_descriptor_t *descriptor = NULL;
            for (size_t i=0, n=descriptors.size(); i<n; ++i)
            {
                const clap_plugin_descriptor_t *d   = descriptors.uget(i);
                if ((d != NULL) && (!strcmp(d->id, plugin_id)))
                {
                    descriptor = d;
                    break;
                }
            }
            if (descriptor == NULL)
            {
                lsp_warn("Invalid CLAP descriptor: %s", plugin_id);
                return NULL;
            }

            // Find the plugin by it's identifier in the list of plugin factories
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Find the corresponding CLAP plugin
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    if (!strcmp(meta->uids.clap, plugin_id))
                    {
                        // Create module
                        plug::Module *plugin = f->create(meta);
                        if (plugin == NULL)
                        {
                            lsp_warn("Failed instantiation of CLAP plugin %s", plugin_id);
                            return NULL;
                        }
                        lsp_finally {
                            if (plugin != NULL)
                                delete plugin;
                        };

                        // Create the resource loader
                        resource::ILoader *loader = core::create_resource_loader();
                        lsp_finally {
                            if (loader != NULL)
                                delete loader;
                        };
                        lsp_trace("created resource loader %p for plugin %p", loader, plugin);

                        // Create the wrapper
                        Wrapper *wrapper    = new Wrapper(plugin, package_manifest, loader, host);
                        if (wrapper == NULL)
                        {
                            lsp_warn("Error creating wrapper");
                            return NULL;
                        }
                        loader              = NULL;

                        lsp_finally {
                            if (wrapper != NULL)
                            {
                                wrapper->destroy();
                                delete wrapper;
                            }
                        };
                        plugin              = NULL;

                        // Allocate the plugin handle and fill it
                        clap_plugin_t *h   = static_cast<clap_plugin_t *>(malloc(sizeof(clap_plugin_t)));
                        if (h == NULL)
                            return NULL;
                        bzero(h, sizeof(*h));

                        // Fill-in the plugin data and return
                        h->desc             = descriptor;
                        h->plugin_data      = wrapper;
                        h->init             = init;
                        h->destroy          = destroy;
                        h->activate         = activate;
                        h->deactivate       = deactivate;
                        h->start_processing = start_processing;
                        h->stop_processing  = stop_processing;
                        h->reset            = reset;
                        h->process          = process;
                        h->get_extension    = get_extension;
                        h->on_main_thread   = on_main_thread;

                        // Prevent wrapper of being deleted
                        wrapper             = NULL;

                        return h;
                    }
                }
            }

            lsp_warn("Invalid CLAP plugin identifier: %s", plugin_id);
            return NULL;
        }


        static const clap_plugin_factory_t plugin_factory =
        {
           .get_plugin_count        = get_plugin_count,
           .get_plugin_descriptor   = get_plugin_descriptor,
           .create_plugin           = create_plugin,
        };

        //---------------------------------------------------------------------
        // Library-related stuff
        static bool init_library(const char *plugin_path)
        {
        #ifndef LSP_IDE_DEBUG
            IF_DEBUG( lsp::debug::redirect(CLAP_LOG_FILE); );
        #endif /* LSP_IDE_DEBUG */

            dsp::init();
            gen_descriptors();
            return true;
        }

        static void destroy_library(void)
        {
            drop_descriptors();
            if (package_manifest != NULL)
            {
                meta::free_manifest(package_manifest);
                package_manifest = NULL;
            }
        }

        const void *get_factory(const char *factory_id)
        {
            if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
                return &plugin_factory;
            return NULL;
        }

    } /* namespace clap */
} /* namespace lsp */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern CLAP_EXPORT
    const clap_plugin_entry_t clap_entry =
    {
        .clap_version   = CLAP_VERSION_INIT,
        .init           = lsp::clap::init_library,
        .deinit         = lsp::clap::destroy_library,
        .get_factory    = lsp::clap::get_factory
    };

#ifdef __cplusplus
}
#endif /* __cplusplus */

