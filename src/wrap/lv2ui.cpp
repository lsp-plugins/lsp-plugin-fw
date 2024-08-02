/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/singletone.h>
#include <lsp-plug.in/common/static.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/impl/ui_wrapper.h>

#ifndef LSP_IDE_DEBUG
    #include <lsp-plug.in/plug-fw/wrap/lv2/impl/wrapper.h>
#endif /* LSP_IDE_DEBUG */

#define LV2UI_LOG_FILE      "lsp-lv2ui.log"

namespace lsp
{
    namespace lv2
    {
        //--------------------------------------------------------------------------------------
        // List of LV2 UI descriptors. The list is generated at first demand because
        // of undetermined order of initialization of static objects which may
        // cause undefined behaviour when reading the list of UI factories
        // which can be not fully initialized.
        static lltl::darray<LV2UI_Descriptor> ui_descriptors;
        static lsp::singletone_t ui_library;

        //--------------------------------------------------------------------------------------
        // LV2UI routines
        LV2UI_Handle ui_instantiate(
            const LV2UI_Descriptor*         descriptor,
            const char*                     plugin_uri,
            const char*                     bundle_path,
            LV2UI_Write_Function            write_function,
            LV2UI_Controller                controller,
            LV2UI_Widget*                   widget,
            const LV2_Feature* const*       features)
        {
            // Find plugin metadata
            lsp_trace("descriptor->uri = %s", descriptor->URI);

            // Initialize dsp (if possible)
            dsp::init();

            ui::Module *ui = NULL;

            // Lookup plugin identifier among all registered plugin factories
            for (ui::Factory *f = ui::Factory::root(); (f != NULL) && (ui == NULL); f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *plug_meta = f->enumerate(i);
                    if (plug_meta == NULL)
                        break;
                    if ((plug_meta->uid == NULL) ||
                        (plug_meta->uids.lv2 == NULL) ||
                        (plug_meta->uids.lv2ui == NULL))
                        continue;

                    // Check plugin identifier
                    if (!(::strcmp(plug_meta->uids.lv2, plugin_uri) || ::strcmp(plug_meta->uids.lv2ui, descriptor->URI)))
                    {
                        // Instantiate the plugin UI and return
                        if ((ui = f->create(plug_meta)) != NULL)
                            break;

                        fprintf(stderr, "Plugin UI instantiation error: %s\n", descriptor->URI);
                        return LV2UI_Handle(NULL);
                    }
                }
            }

            // No UI has been found?
            if (ui == NULL)
            {
                fprintf(stderr, "Not found UI for plugin: %s\n", descriptor->URI);
                return LV2UI_Handle(NULL);
            }

            // Create the resource loader
            resource::ILoader *loader = core::create_resource_loader();
            if (loader != NULL)
            {
                // Create LV2 extension handler
                lv2::Extensions *ext = new lv2::Extensions(features,
                        ui->metadata()->uids.lv2, LSP_LV2_TYPES_URI, LSP_LV2_KVT_URI,
                        controller, write_function);
                if (ext != NULL)
                {
                    // Create LV2 plugin wrapper
                    lv2::UIWrapper *wrapper  = new lv2::UIWrapper(ui, loader, ext);
                    if (wrapper != NULL)
                    {
                        // Initialize LV2 plugin wrapper
                        status_t res = wrapper->init(NULL);
                        if (res != STATUS_OK)
                        {
                            lsp_error("Error initializing plugin wrapper, code: %d", int(res));
                            wrapper->destroy(); // The ext, loader and ui will be destroyed here
                            delete wrapper;
                            wrapper = NULL;
                            *widget = NULL;
                        }
                        else
                        {
                            tk::Window *root = wrapper->window();
                            *widget  = reinterpret_cast<LV2UI_Widget>((root != NULL) ? root->handle() : NULL);
                            lsp_trace("returned window handle = %p", *widget);
                        }

                        return reinterpret_cast<LV2UI_Handle>(wrapper);
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
                lsp_error("No resource loader available");

            ui->destroy();
            delete ui;
            return static_cast<LV2UI_Handle>(NULL);
        }

        void ui_cleanup(LV2UI_Handle ui)
        {
            lsp_trace("this = %p", ui);

            UIWrapper *w = reinterpret_cast<UIWrapper *>(ui);
            w->destroy();
            delete w;
        }

        void ui_port_event(
            LV2UI_Handle ui,
            uint32_t     port_index,
            uint32_t     buffer_size,
            uint32_t     format,
            const void*  buffer)
        {
//            lsp_trace("this = %p, idx=%d, size=%d, format=%d, buffer=%p",
//                    ui, int(port_index), int(buffer_size), int(format), buffer);
            if ((buffer_size == 0) || (buffer == NULL))
                return;
            lv2::UIWrapper *w       = reinterpret_cast<lv2::UIWrapper *>(ui);
            w->notify(port_index, buffer_size, format, buffer);
        }

        //--------------------------------------------------------------------------------------
        // LV2UI IdleInterface extension
        int ui_idle(LV2UI_Handle ui)
        {
            lv2::UIWrapper *w       = reinterpret_cast<lv2::UIWrapper *>(ui);
            if (w->ui() == NULL)
                return -1;

            dsp::context_t ctx;
            dsp::start(&ctx);
            w->main_iteration();
            dsp::finish(&ctx);

            return 0;
        }

        static LV2UI_Idle_Interface idle_iface =
        {
            ui_idle
        };

        //--------------------------------------------------------------------------------------
        // LV2UI Resize extension
        int ui_resize(LV2UI_Feature_Handle ui, int width, int height)
        {
            lv2::UIWrapper *w       = reinterpret_cast<lv2::UIWrapper *>(ui);
            return w->resize_ui(width, height);
        }

        static LV2UI_Resize resize_iface =
        {
            NULL,
            ui_resize
        };

        const void* ui_extension_data(const char* uri)
        {
            lsp_trace("requested extension data = %s", uri);
            if (!strcmp(uri, LV2_UI__idleInterface))
            {
                lsp_trace("  idle_interface = %p", &idle_iface);
                return &idle_iface;
            }
            else if (!strcmp(uri, LV2_UI__resize))
            {
                lsp_trace("  resize_interface = %p", &resize_iface);
                return &resize_iface;
            }
            return NULL;
        }

        static ssize_t ui_cmp_descriptors(const LV2UI_Descriptor *d1, const LV2UI_Descriptor *d2)
        {
            return strcmp(d1->URI, d2->URI);
        }

        void ui_gen_descriptors()
        {
            // Perform first check that descriptors are initialized
            if (ui_library.initialized())
                return;

            // Generate descriptors
            lltl::darray<LV2UI_Descriptor> result;
            lsp_finally { result.flush(); };
            lsp_trace("generating UI descriptors...");

            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Skip plugins not compatible with LV2
                    if ((meta->uids.lv2 == NULL) || (meta->uids.lv2ui == NULL))
                        continue;

                    // Allocate new descriptor
                    LV2UI_Descriptor *d     = result.add();
                    if (d == NULL)
                    {
                        lsp_warn("Error allocating LV2 UI descriptor for plugin %s", meta->uids.lv2);
                        continue;
                    }

                    // Initialize descriptor
                    d->URI                  = meta->uids.lv2ui;
                    d->instantiate          = ui_instantiate;
                    d->cleanup              = ui_cleanup;
                    d->port_event           = ui_port_event;
                    d->extension_data       = ui_extension_data;
                }
            }

            // Sort descriptors
            result.qsort(ui_cmp_descriptors);

        #ifdef LSP_TRACE
            lsp_trace("generated %d descriptors:", int(result.size()));
            for (size_t i=0, n=result.size(); i<n; ++i)
            {
                LV2UI_Descriptor *d = result.uget(i);
                lsp_trace("[%4d] %p: %s", int(i), d, d->URI);
            }
        #endif /* LSP_TRACE */

            // Commit the generated list to the global descriptor list
            lsp_singletone_init(ui_library) {
                result.swap(ui_descriptors);
            };
        };

        void ui_drop_descriptors()
        {
            lsp_trace("dropping %d descriptors", int(ui_descriptors.size()));
            ui_descriptors.flush();
        };

        //--------------------------------------------------------------------------------------
        // Static finalizer for the list of UI descriptors at library finalization
        static StaticFinalizer lv2ui_finalizer(ui_drop_descriptors);

    } /* namespace lv2 */
} /* namespace lsp */


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    LV2_SYMBOL_EXPORT
    const LV2UI_Descriptor *lv2ui_descriptor(uint32_t index)
    {
    #ifndef LSP_IDE_DEBUG
        IF_DEBUG( lsp::debug::redirect(LV2UI_LOG_FILE); );
    #endif /* LSP_IDE_DEBUG */
        lsp::lv2::ui_gen_descriptors();
        return lsp::lv2::ui_descriptors.get(index);
    }
#ifdef __cplusplus
}
#endif /* __cplusplus */
