/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 2 нояб. 2021 г.
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

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/io/IInStream.h>
#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/common/static.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/stdlib/stdio.h>

#ifdef USE_LADSPA
    #include <ladspa.h>
#else
    #include <lsp-plug.in/3rdparty/ladspa/ladspa.h>
#endif /* USE_LADSPA */

#include <lsp-plug.in/plug-fw/wrap/ladspa/ports.h>
#include <lsp-plug.in/plug-fw/wrap/ladspa/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/ladspa/impl/wrapper.h>

namespace lsp
{
    namespace ladspa
    {
        //---------------------------------------------------------------------
        // List of LADSPA descriptors (generated at startup)
        static lltl::darray<LADSPA_Descriptor> descriptors;
        static ipc::Mutex descriptors_mutex;

        //---------------------------------------------------------------------
        inline bool port_supported(const meta::port_t *port)
        {
            switch (port->role)
            {
                case meta::R_UI_SYNC:
                case meta::R_MESH:
                case meta::R_FBUFFER:
                case meta::R_STREAM:
                    return false;
                default:
                    return true;
            }
            return true;
        }

        LADSPA_Handle instantiate(const LADSPA_Descriptor *descriptor, unsigned long sample_rate)
        {
            // Check sample rate
            if (sample_rate > MAX_SAMPLE_RATE)
            {
                lsp_error("Unsupported sample rate: %ld, maximum supportes sample rate is %ld",
                        long(sample_rate), long(MAX_SAMPLE_RATE));
                return NULL;
            }

            // Initialize DSP
            dsp::init();

            ssize_t index = descriptors.index_of(descriptor);
            if (index < 0)
            {
                lsp_error("Unknown LADSPA descriptor has been passed in the call");
                return NULL;
            }

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

                    // Check plugin identifier
                    if ((meta->ladspa_id == descriptor->UniqueID) &&
                        (!::strcmp(meta->ladspa_lbl, descriptor->Label)))
                    {
                        // Instantiate the plugin and return
                        if ((plugin = f->create(meta)) == NULL)
                        {
                            lsp_error("Plugin instantiation error: %s", meta->ladspa_lbl);
                            return NULL;
                        }
                    }
                }
            }

            // No plugin has been found?
            if (plugin == NULL)
            {
                lsp_error("Unknown plugin identifier: %s", descriptor->Label);
                return NULL;
            }

            // Create resource loader
            resource::ILoader *loader = core::create_resource_loader();
            if (loader != NULL)
            {
                ladspa::Wrapper *wrapper  = new ladspa::Wrapper(plugin, loader);
                if (wrapper != NULL)
                {
                    status_t res = wrapper->init(sample_rate);
                    if (res == STATUS_OK)
                        return reinterpret_cast<LADSPA_Handle>(wrapper);
                    lsp_error("Error initializing plugin wrapper, code: %d", int(res));
                    delete wrapper;
                }
                else
                    lsp_error("Error allocating plugin wrapper");
                delete loader;
            }
            else
                lsp_error("No resource loader available");
            delete plugin;

            // Plugin could not be instantiated
            return NULL;
        }

        void connect_port(LADSPA_Handle Instance, unsigned long Port, LADSPA_Data * DataLocation)
        {
            ladspa::Wrapper *w = reinterpret_cast<ladspa::Wrapper *>(Instance);
            w->connect(Port, DataLocation);
        }

        void activate(LADSPA_Handle Instance)
        {
            ladspa::Wrapper *w = reinterpret_cast<ladspa::Wrapper *>(Instance);
            w->activate();
        }

        void run(LADSPA_Handle Instance, unsigned long SampleCount)
        {
            dsp::context_t ctx;
            ladspa::Wrapper *w = reinterpret_cast<ladspa::Wrapper *>(Instance);

            // Call the plugin for processing
            dsp::start(&ctx);
            w->run(SampleCount);
            dsp::finish(&ctx);
        }

        void deactivate(LADSPA_Handle Instance)
        {
            ladspa::Wrapper *w = reinterpret_cast<ladspa::Wrapper *>(Instance);
            w->deactivate();
        }

        void cleanup(LADSPA_Handle Instance)
        {
            ladspa::Wrapper *w = reinterpret_cast<ladspa::Wrapper *>(Instance);
            resource::ILoader *loader = w->resources();

            // Destroy plugin
            w->destroy();
            delete w;

            // Destroy resource loader
            if (loader != NULL)
                delete loader;
        }

        const char *add_units(const char *s, size_t units)
        {
            const char *unit = meta::get_unit_name(units);
            if (unit == NULL)
                return strdup(s);

            char *ptr = NULL;
            int res = asprintf(&ptr, "%s (%s)", s, unit);

            return ((res < 0) || (ptr == NULL)) ? strdup(s) : ptr;
        }

        char *make_plugin_name(const meta::plugin_t *m)
        {
            if (m->description)
                return strdup(m->description);
            if (m->name)
                return strdup(m->name);
            if (m->uid)
                return strdup(m->uid);
            if (m->ladspa_lbl)
                return strdup(m->ladspa_lbl);
            char *str = NULL;
            if (asprintf(&str, "plugin %u", (unsigned int)m->ladspa_id) >= 0)
                return str;

            return NULL;
        }

        void make_descriptor(LADSPA_Descriptor *d, const meta::package_t *manifest, const meta::plugin_t *m)
        {
            ssize_t n;
            char *str               = NULL;

            d->UniqueID             = m->ladspa_id;
            d->Label                = m->ladspa_lbl;
            d->Properties           = LADSPA_PROPERTY_HARD_RT_CAPABLE;
            d->Name                 = make_plugin_name(m);
            n                       = ((manifest) && (manifest->brand)) ? asprintf(&str, "%s LADSPA", manifest->brand) : -1;
            d->Maker                = (n >= 0) ? str : NULL;
            d->ImplementationData   = const_cast<char *>(m->developer->name);
            d->Copyright            = ((manifest) && (manifest->copyright)) ? strdup(manifest->copyright) : NULL;
            d->PortCount            = 1; // 1 port used for latency output

            // Calculate number of ports
            for (const meta::port_t *port = m->ports; port->id != NULL; ++port)
            {
                if (port_supported(port))
                    d->PortCount ++;
            }

            LADSPA_PortDescriptor *p_descr      = lsp_malloc<LADSPA_PortDescriptor>(d->PortCount);
            const char **p_name                 = lsp_malloc<const char *>(d->PortCount);
            LADSPA_PortRangeHint *p_hint        = lsp_malloc<LADSPA_PortRangeHint>(d->PortCount);

            d->PortDescriptors                  = p_descr;
            d->PortNames                        = p_name;
            d->PortRangeHints                   = p_hint;

            for (const meta::port_t *p = m->ports; p->id != NULL; ++p)
            {
                // Skip ports invisible for LADSPA
                if (!port_supported(p))
                    continue;

                // Generate port descriptor
                switch (p->role)
                {
                    case meta::R_AUDIO:
                        *p_descr = (meta::is_out_port(p)) ? LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO : LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
                        break;
                    case meta::R_CONTROL:
                    case meta::R_METER:
                    default:
                        *p_descr = (meta::is_out_port(p)) ? LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL : LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
                        break;
                }

                *p_name                 = add_units(p->name, p->unit);
                p_hint->HintDescriptor  = 0;

                if (p->unit == meta::U_BOOL)
                {
                    p_hint->HintDescriptor |= LADSPA_HINT_TOGGLED | LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_BELOW;
                    p_hint->HintDescriptor |= (p->start > 0) ? LADSPA_HINT_DEFAULT_1 : LADSPA_HINT_DEFAULT_0;
                    p_hint->LowerBound      = 0.0f;
                    p_hint->UpperBound      = 1.0f;
                }
                else if (p->unit == meta::U_ENUM)
                {
                    p_hint->HintDescriptor  |= LADSPA_HINT_INTEGER | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_BOUNDED_BELOW;
                    p_hint->LowerBound      = (p->flags & meta::F_LOWER) ? p->min : 0;
                    p_hint->UpperBound      = p_hint->LowerBound + list_size(p->items) - 1;

                    if (p->start == p_hint->LowerBound)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_MINIMUM;
                    else if (p->start == p_hint->UpperBound)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_MAXIMUM;
                    else if (p->start == 1.0f)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_1;
                    else if (p->start == 0.0f)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_0;
                }
                else if (p->unit == meta::U_SAMPLES)
                {
//                    p_hint->HintDescriptor  |= LADSPA_HINT_INTEGER;
                    if (p->flags & meta::F_LOWER)
                    {
                        p_hint->HintDescriptor |= LADSPA_HINT_BOUNDED_BELOW;
                        p_hint->LowerBound      = p->min;
                    }
                    if (p->flags & meta::F_UPPER)
                    {
                        p_hint->HintDescriptor |= LADSPA_HINT_BOUNDED_ABOVE;
                        p_hint->UpperBound      = p->max;
                    }
                }
                else
                {
                    if (p->flags & meta::F_LOWER)
                    {
                        p_hint->HintDescriptor |= LADSPA_HINT_BOUNDED_BELOW;
                        p_hint->LowerBound      = p->min;
                    }
                    if (p->flags & meta::F_UPPER)
                    {
                        p_hint->HintDescriptor |= LADSPA_HINT_BOUNDED_ABOVE;
                        p_hint->UpperBound      = p->max;
                    }
                    if (p->flags & meta::F_LOG)
                        p_hint->HintDescriptor |= LADSPA_HINT_LOGARITHMIC;
                }

                // Solve default value
                if ((!meta::is_audio_port(p)) && ((p_hint->HintDescriptor & LADSPA_HINT_DEFAULT_MASK) == LADSPA_HINT_DEFAULT_NONE))
                {
                    if (p->start == 1.0f)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_1;
                    else if (p->start == 0.0f)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_0;
                    else if (p->start == 100.0f)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_100;
                    else if (p->start == 440.0f)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_440;
                    else if ((p->flags & (meta::F_LOWER | meta::F_UPPER))  == (meta::F_LOWER | meta::F_UPPER))
                    {
                        if (p->start <= p->min)
                            p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_MINIMUM;
                        else if (p->start >= p->max)
                            p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_MAXIMUM;
                        else
                        {
                            float factor = (p->flags & meta::F_LOG) ?
                                (logf(p->start) - logf(p->min)) / (logf(p->max) - logf(p->min)) :
                                (p->start - p->min) / (p->max - p->min);

                            if (factor <= 0.33)
                                p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_LOW;
                            else if (factor >= 0.66)
                                p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_HIGH;
                            else
                                p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_MIDDLE;
                        }
                    }
                    else if (p->flags & meta::F_LOWER)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_MINIMUM;
                    else if (p->flags & meta::F_UPPER)
                        p_hint->HintDescriptor |= LADSPA_HINT_DEFAULT_MAXIMUM;
                }

                p_descr++;
                p_name++;
                p_hint++;
            }

            // Describe latency port
            *p_descr                = LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL;
            *p_name                 = strdup("latency");
            p_hint->HintDescriptor  = LADSPA_HINT_INTEGER | LADSPA_HINT_BOUNDED_BELOW;
            p_hint->LowerBound      = 0;
            p_hint->UpperBound      = 0;

            // Complete the LADSPA descriptor
            d->instantiate          = ladspa::instantiate;
            d->connect_port         = ladspa::connect_port;
            d->activate             = ladspa::activate;
            d->run                  = ladspa::run;
            d->run_adding           = NULL;
            d->set_run_adding_gain  = NULL;
            d->deactivate           = ladspa::deactivate;
            d->cleanup              = ladspa::cleanup;
        }

        static ssize_t cmp_descriptors(const LADSPA_Descriptor *d1, const LADSPA_Descriptor *d2)
        {
            return strcmp(d1->Label, d2->Label);
        }

        void gen_descriptors()
        {
            // Perform size test first
            if (descriptors.size() > 0)
                return;

            // Lock mutex and test again
            if (!descriptors_mutex.lock())
                return;
            if (descriptors.size() > 0)
            {
                descriptors_mutex.unlock();
                return;
            }

            // Obtain the manifest
            status_t res;
            meta::package_t *manifest = NULL;
            resource::ILoader *loader = core::create_resource_loader();
            if (loader != NULL)
            {
                io::IInStream *is = loader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
                if (is != NULL)
                {
                    if ((res = meta::load_manifest(&manifest, is)) != STATUS_OK)
                    {
                        lsp_warn("Error loading manifest file, error=%d", int(res));
                        manifest = NULL;
                    }
                    is->close();
                    delete is;
                }
                delete loader;
            }
            if (manifest == NULL)
                lsp_trace("No manifest file found");

            // Generate descriptors
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Skip plugins not compatible with LADSPA
                    if ((meta->ladspa_id == 0) || (meta->ladspa_lbl == NULL))
                        continue;

                    // Allocate new descriptor
                    LADSPA_Descriptor *d = descriptors.add();
                    if (d == NULL)
                    {
                        lsp_warn("Error allocating LADSPA descriptor for plugin %s", meta->ladspa_lbl);
                        continue;
                    }

                    // Initialize descriptor
                    make_descriptor(d, manifest, meta);
                }
            }

            // Sort descriptors
            descriptors.qsort(cmp_descriptors);

            // Free previously loaded manifest data
            if (manifest != NULL)
            {
                meta::free_manifest(manifest);
                manifest = NULL;
            }

            // Unlock descriptor mutex
            descriptors_mutex.unlock();
        };

        void drop_descriptors()
        {
            for (size_t i=0, n=descriptors.size(); i<n; ++i)
            {
                LADSPA_Descriptor *d = descriptors.uget(i);

                if (d->PortNames)
                {
                    for (size_t i=0; i < d->PortCount; ++i)
                    {
                        if (d->PortNames[i])
                            free(const_cast<char *>(d->PortNames[i]));
                    }
                    free(const_cast<char **>(d->PortNames));
                }
                if (d->PortDescriptors)
                    free(const_cast<LADSPA_PortDescriptor *>(d->PortDescriptors));
                if (d->PortRangeHints)
                    free(const_cast<LADSPA_PortRangeHint *>(d->PortRangeHints));

                if (d->Name)
                    free(const_cast<char *>(d->Name));
                if (d->Copyright)
                    free(const_cast<char *>(d->Copyright));
                if (d->Maker)
                    free(const_cast<char *>(d->Maker));
            }

            descriptors.flush();
        };

        //---------------------------------------------------------------------
        static StaticFinalizer finalizer(drop_descriptors);
    }
}

//---------------------------------------------------------------------
LSP_CSYMBOL_EXPORT
const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    IF_DEBUG( lsp::debug::redirect("lsp-ladspa.log"); );

    lsp::ladspa::gen_descriptors();
    return lsp::ladspa::descriptors.get(index);
}
