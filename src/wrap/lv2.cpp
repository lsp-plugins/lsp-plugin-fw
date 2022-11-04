/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 нояб. 2021 г.
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

#include <lsp-plug.in/common/static.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/types.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/impl/wrapper.h>

#define LV2_LOG_FILE            "lsp-lv2.log"

namespace lsp
{
    namespace lv2
    {
        //---------------------------------------------------------------------
        // List of LV2 descriptors. The list is generated at first demand because
        // of undetermined order of initialization of static objects which may
        // cause undefined behaviour when reading the list of plugin factories
        // which can be not fully initialized.
        static lltl::darray<LV2_Descriptor> descriptors;
        static ipc::Mutex descriptors_mutex;
        static volatile bool descriptors_initialized = false;

        //---------------------------------------------------------------------
        void activate(LV2_Handle instance)
        {
            lsp_trace("instance = %p", instance);
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);
            w->activate();
        }

        void cleanup(LV2_Handle instance)
        {
            lsp_trace("instance = %p", instance);
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);
            w->destroy();
            delete w;
        }

        void connect_port(
            LV2_Handle instance,
            uint32_t   port,
            void      *data_location)
        {
            // lsp_trace("instance = %p, port = %d, data_location=%p", instance, int(port), data_location);
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);
            w->connect(port, data_location);
        }

        void deactivate(LV2_Handle instance)
        {
            lsp_trace("instance = %p", instance);
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);
            w->deactivate();
        }

        LV2_Handle instantiate(
            const LV2_Descriptor          *descriptor,
            double                         sample_rate,
            const char *                   bundle_path,
            const LV2_Feature *const *     features)
        {
            lsp_trace("%p: instantiate descriptor->URI=%s", descriptor, descriptor->URI);

            // Check sample rate
            if (sample_rate > MAX_SAMPLE_RATE)
            {
                lsp_error("Unsupported sample rate: %f, maximum supported sample rate is %ld", float(sample_rate), long(MAX_SAMPLE_RATE));
                return NULL;
            }

            // Initialize DSP
            dsp::init();

            // Lookup plugin identifier among all registered plugin factories
            plug::Module *plugin = NULL;
            const meta::plugin_t *meta = NULL;

            for (plug::Factory *f = plug::Factory::root(); (plugin == NULL) && (f != NULL); f = f->next())
            {
                for (size_t i=0; plugin == NULL; ++i)
                {
                    // Enumerate next element
                    if ((meta = f->enumerate(i)) == NULL)
                        break;
                    if ((meta->uid == NULL) ||
                        (meta->lv2_uri == NULL))
                        continue;

                    // Check plugin identifier
                    if (!::strcmp(meta->lv2_uri, descriptor->URI))
                    {
                        // Instantiate the plugin and return
                        if ((plugin = f->create(meta)) == NULL)
                        {
                            lsp_error("Plugin instantiation error: %s", meta->lv2_uri);
                            return NULL;
                        }
                        else
                        {
                            lsp_trace("%p: Instantiated plugin with URI=%s", descriptor, meta->lv2_uri);
                        }
                    }
                }
            }

            // No plugin has been found?
            if (plugin == NULL)
            {
                lsp_error("Unknown plugin identifier: %s\n", descriptor->URI);
                return NULL;
            }

            lsp_trace("%p: descriptor_uri=%s, uri=%s, sample_rate=%f", descriptor, descriptor->URI, meta->lv2_uri, sample_rate);

            // Create resource loader
            resource::ILoader *loader = core::create_resource_loader();
            if (loader != NULL)
            {
                // Create LV2 extension handler
                lv2::Extensions *ext = new lv2::Extensions(features,
                        meta->lv2_uri, LSP_LV2_TYPES_URI, LSP_LV2_KVT_URI,
                        NULL, NULL);
                if (ext != NULL)
                {
                    // Create LV2 plugin wrapper
                    lv2::Wrapper *wrapper  = new lv2::Wrapper(plugin, loader, ext);
                    if (wrapper != NULL)
                    {
                        // Initialize LV2 plugin wrapper
                        status_t res = wrapper->init(sample_rate);
                        if (res != STATUS_OK)
                        {
                            lsp_error("Error initializing plugin wrapper, code: %d", int(res));

                            wrapper->destroy(); // The ext, loader and plugin will be destroyed here
                            delete wrapper;
                            wrapper = NULL;
                        }

                        return reinterpret_cast<LV2_Handle>(wrapper);
                    }
                    else
                        lsp_error("Error allocating plugin wrapper");
                    delete ext;
                }
                else
                    fprintf(stderr, "No resource loader available");
                delete loader;
            }
            else
                fprintf(stderr, "No resource loader available");
            delete plugin;

            return static_cast<LV2_Handle>(NULL);
        }

        void run(LV2_Handle instance, uint32_t sample_count)
        {
            // lsp_trace("instance = %p, sample_count=%d", instance, int(sample_count));

            dsp::context_t ctx;
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);

            // Call the plugin for processing
            dsp::start(&ctx);
            w->run(sample_count);
            dsp::finish(&ctx);
        }

        LV2_State_Status save_state(
            LV2_Handle                 instance,
            LV2_State_Store_Function   store,
            LV2_State_Handle           handle,
            uint32_t                   flags,
            const LV2_Feature *const * features)
        {
            lsp_trace("instance = %p", instance);
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);
            w->save_state(store, handle, flags, features);

            return LV2_STATE_SUCCESS;
        }

        LV2_State_Status restore_state(
            LV2_Handle                  instance,
            LV2_State_Retrieve_Function retrieve,
            LV2_State_Handle            handle,
            uint32_t                    flags,
            const LV2_Feature *const *  features)
        {
            lsp_trace("instance = %p", instance);
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);
            w->restore_state(retrieve, handle, flags, features);

            return LV2_STATE_SUCCESS;
        }

        LV2_Inline_Display_Image_Surface *render_inline_display(
                                           LV2_Handle instance,
                                           uint32_t w, uint32_t h)
        {
            // lsp_trace("instance = %p", instance);

            dsp::context_t ctx;
            LV2_Inline_Display_Image_Surface *result;
            lv2::Wrapper *wrapper = reinterpret_cast<lv2::Wrapper *>(instance);

            dsp::start(&ctx);
    //        lsp_trace("call wrapper for rendering w=%d, h=%d", int(w), int(h));
            result              = wrapper->render_inline_display(w, h);
            dsp::finish(&ctx);

            return result;
        }

        LV2_Worker_Status job_run(
            LV2_Handle                  instance,
            LV2_Worker_Respond_Function respond,
            LV2_Worker_Respond_Handle   handle,
            uint32_t                    size,
            const void*                 data)
        {
            // lsp_trace("instance = %p", instance);
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);
            w->job_run(handle, respond, size, data);
            return LV2_WORKER_SUCCESS;
        }

        LV2_Worker_Status job_response(
            LV2_Handle  instance,
            uint32_t    size,
            const void* body)
        {
            // lsp_trace("instance = %p", instance);
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);
            w->job_response(size, body);
            return LV2_WORKER_SUCCESS;
        }

        LV2_Worker_Status job_end(LV2_Handle instance)
        {
            // lsp_trace("instance = %p", instance);
            lv2::Wrapper *w = reinterpret_cast<lv2::Wrapper *>(instance);
            w->job_end();
            return LV2_WORKER_SUCCESS;
        }

        static const LV2_State_Interface state_interface =
        {
            save_state,
            restore_state
        };

        static const LV2_Inline_Display_Interface inline_display_interface =
        {
            render_inline_display
        };

        static const LV2_Worker_Interface worker_interface =
        {
            job_run,
            job_response,
            job_end
        };

        const void *extension_data(const char * uri)
        {
            lsp_trace("requested extension data = %s", uri);
            if (!::strcmp(uri, LV2_STATE__interface))
            {
                lsp_trace("  state_interface = %p", &state_interface);
                return &state_interface;
            }
            else if (!::strcmp(uri, LV2_WORKER__interface))
            {
                lsp_trace("  worker_interface = %p", &worker_interface);
                return &worker_interface;
            }
            else if (!::strcmp(uri, LV2_INLINEDISPLAY__interface))
            {
                lsp_trace("  inline_display_interface = %p", &inline_display_interface);
                return &inline_display_interface;
            }

            return NULL;
        }

        static ssize_t cmp_descriptors(const LV2_Descriptor *d1, const LV2_Descriptor *d2)
        {
            return strcmp(d1->URI, d2->URI);
        }

        void gen_descriptors()
        {
            // Perform first check that descriptors are initialized
            if (descriptors_initialized)
                return;

            // Lock mutex for lazy initialization
            if (!descriptors_mutex.lock())
                return;
            lsp_finally { descriptors_mutex.unlock(); };

            // Perform test again and leave if all is OK
            if (descriptors_initialized)
                return;
            lsp_finally { descriptors_initialized = true; };

            // Perform size test first
            lsp_trace("generating descriptors...");

            // Generate descriptors
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Skip plugins not compatible with LV2
                    const meta::plugin_t *meta = f->enumerate(i);
                    if ((meta == NULL) || (meta->lv2_uri == NULL))
                        break;

                    // Allocate new descriptor
                    LV2_Descriptor *d = descriptors.add();
                    if (d == NULL)
                    {
                        lsp_warn("Error allocating LV2 descriptor for plugin %s", meta->lv2_uri);
                        continue;
                    }

                    // Initialize descriptor
                    d->URI                  = meta->lv2_uri;
                    d->instantiate          = instantiate;
                    d->connect_port         = connect_port;
                    d->activate             = activate;
                    d->run                  = run;
                    d->deactivate           = deactivate;
                    d->cleanup              = cleanup;
                    d->extension_data       = extension_data;
                }
            }

            // Sort descriptors
            descriptors.qsort(cmp_descriptors);

        #ifdef LSP_TRACE
            lsp_trace("generated %d descriptors:", int(descriptors.size()));
            for (size_t i=0, n=descriptors.size(); i<n; ++i)
            {
                LV2_Descriptor *d = descriptors.uget(i);
                lsp_trace("[%4d] %p: %s", int(i), d, d->URI);
            }
            lsp_trace("generated %d descriptors:", int(descriptors.size()));
        #endif /* LSP_TRACE */
        };

        void drop_descriptors()
        {
            lsp_trace("dropping %d descriptors", int(descriptors.size()));
            descriptors.flush();
        };

        //---------------------------------------------------------------------
        // Static finalizer for the list of descriptors at library finalization
        static StaticFinalizer finalizer(drop_descriptors);

    } /* namespace lv2 */
} /* namespace lsp */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    LV2_SYMBOL_EXPORT
    const LV2_Descriptor *lv2_descriptor(uint32_t index)
    {
    #ifndef LSP_IDE_DEBUG
        IF_DEBUG( lsp::debug::redirect(LV2_LOG_FILE); );
    #endif /* LSP_IDE_DEBUG */
    
        lsp::lv2::gen_descriptors();
        return lsp::lv2::descriptors.get(index);
    }
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
