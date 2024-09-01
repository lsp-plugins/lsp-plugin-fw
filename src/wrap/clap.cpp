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
#include <lsp-plug.in/plug-fw/wrap/clap/factory.h>
#include <lsp-plug.in/plug-fw/wrap/clap/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/clap/impl/factory.h>
#include <lsp-plug.in/plug-fw/wrap/clap/impl/wrapper.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>

#ifdef WITH_UI_FEATURE
    #include <lsp-plug.in/plug-fw/wrap/clap/ui_wrapper.h>
    #include <lsp-plug.in/plug-fw/wrap/clap/impl/ui_wrapper.h>
#endif /* WITH_UI_FEATURE */

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
        struct factory_t: public clap_plugin_factory_t
        {
            clap::Factory *factory;
        };

        static factory_t *plugin_factory;
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

    #ifdef WITH_UI_FEATURE
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
    #endif /* WITH_UI_FEATURE */

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

        #ifdef WITH_UI_FEATURE
            if ((!strcmp(id, CLAP_EXT_GUI)) && (w->ui_provided()))
                return &ui_extension;
        #endif /* WITH_UI_FEATURE */

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
        static uint32_t get_plugin_count(const clap_plugin_factory_t *factory)
        {
            const factory_t *f = static_cast<const factory_t *>(factory);
            return (f->factory != NULL) ? f->factory->descriptors_count() : 0;
        }

        static const clap_plugin_descriptor_t *get_plugin_descriptor(
            const clap_plugin_factory_t *factory,
            uint32_t index)
        {
            const factory_t *f = static_cast<const factory_t *>(factory);
            return (f->factory != NULL) ? f->factory->descriptor(index) : 0;
        }

        static const clap_plugin_t *create_plugin(
            const clap_plugin_factory_t *factory,
            const clap_host_t *host,
            const char *plugin_id)
        {
            if (!clap_version_is_compatible(host->clap_version))
                return NULL;

            const factory_t *f = static_cast<const factory_t *>(factory);
            if (f->factory == NULL)
                return NULL;

            // Find the corresponding CLAP plugin descriptor
            const clap_plugin_descriptor_t *descriptor = NULL;
            for (size_t i=0, n=f->factory->descriptors_count(); i<n; ++i)
            {
                const clap_plugin_descriptor_t *d   = f->factory->descriptor(i);
                if ((d != NULL) && (!strcmp(d->id, plugin_id)))
                {
                    descriptor = d;
                    break;
                }
            }
            if (descriptor == NULL)
            {
                lsp_warn("Invalid CLAP plugin identifier: %s", plugin_id);
                return NULL;
            }

            // Create plugin wrapper
            clap::Wrapper *wrapper  = f->factory->instantiate(host, descriptor);
            if (wrapper == NULL)
            {
                lsp_error("Failed instantiation of CLAP plugin: %s", plugin_id);
                return NULL;
            }
            lsp_finally {
                if (wrapper != NULL)
                {
                    wrapper->destroy();
                    delete wrapper;
                }
            };


            // Allocate the plugin handle and fill it
            clap_plugin_t *h   = static_cast<clap_plugin_t *>(malloc(sizeof(clap_plugin_t)));
            if (h == NULL)
                return NULL;
            bzero(h, sizeof(*h));

            // Fill-in the plugin data and return
            h->desc             = descriptor;
            h->plugin_data      = release_ptr(wrapper);
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

            return h;
        }

        //---------------------------------------------------------------------
        // Library-related stuff
        static void destroy_factory(factory_t * &factory)
        {
            if (factory != NULL)
            {
                if (factory->factory != NULL)
                {
                    factory->factory->release();
                    factory->factory     = NULL;
                }
                free(factory);
                factory     = NULL;
            }
        }

        static bool init_library(const char *plugin_path)
        {
        #ifndef LSP_IDE_DEBUG
            IF_DEBUG( lsp::debug::redirect(CLAP_LOG_FILE); );
        #endif /* LSP_IDE_DEBUG */

            // Check that data already has been initialized
            if (library.initialized())
                return true;

            dsp::init();

            factory_t *factory = static_cast<factory_t *>(malloc(sizeof(factory_t)));
            if (factory == NULL)
                return false;

            lsp_finally {
                destroy_factory(factory);
            };

            factory->get_plugin_count       = get_plugin_count;
            factory->get_plugin_descriptor  = get_plugin_descriptor;
            factory->create_plugin          = create_plugin;
            factory->factory                = new Factory();

            if (factory->factory == NULL)
                return false;

            lsp_trace("Created plugin factory %p wrapped by interface %p", factory->factory, factory);

            // Commit the generated objects to the global variables
            lsp_singletone_init(library) {
                lsp::swap(plugin_factory, factory);
            };

            return true;
        }

        static void destroy_library(void)
        {
            lsp_trace("Destroying plugin factory interface %p", plugin_factory);
            destroy_factory(plugin_factory);
        }

        const void *get_factory(const char *factory_id)
        {
            if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
            {
                lsp_trace("Returning plugin factory interface %p", plugin_factory);
                return plugin_factory;
            }
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

