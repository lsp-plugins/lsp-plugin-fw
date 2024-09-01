/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-send
 * Created on: 1 сент. 2024 г.
 *
 * lsp-plugins-send is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-send is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-send. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_FACTORY_H_

#include <lsp-plug.in/plug-fw/wrap/clap/factory.h>
#include <lsp-plug.in/plug-fw/wrap/clap/wrapper.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace clap
    {
        Factory::Factory()
        {
            nReferences = 1;
            pLoader     = NULL;
            pManifest   = NULL;

            generate_descriptors();
        }

        Factory::~Factory()
        {
            drop_descriptors();

            if (pManifest != NULL)
            {
                meta::free_manifest(pManifest);
                pManifest   = NULL;
            }

            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader     = NULL;
            }
        }

        size_t Factory::acquire()
        {
            return atomic_add(&nReferences, 1) + 1;
        }

        size_t Factory::release()
        {
            atomic_t ref_count = atomic_add(&nReferences, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        ssize_t Factory::cmp_descriptors(const clap_plugin_descriptor_t *d1, const clap_plugin_descriptor_t *d2)
        {
            return strcmp(d1->id, d2->id);
        }

        meta::package_t *Factory::load_manifest(resource::ILoader *loader)
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

        const char * const *Factory::make_feature_list(const meta::plugin_t *meta)
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

        void Factory::generate_descriptors()
        {
            // Obtain the resource loader
            lsp_trace("Obtaining resource loader...");
            pLoader = core::create_resource_loader();
            if (pLoader == NULL)
                return;

            // Obtain the manifest
            lsp_trace("Obtaining manifest...");
            meta::package_t *manifest = load_manifest(pLoader);
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
            vDescriptors.swap(result);
            pManifest = release_ptr(manifest);
        }

        void Factory::drop_descriptor(clap_plugin_descriptor_t *d)
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

        void Factory::drop_descriptors()
        {
            lsp_trace("dropping %d descriptors", int(vDescriptors.size()));
            for (size_t i=0, n=vDescriptors.size(); i<n; ++i)
                drop_descriptor(vDescriptors.uget(i));
            vDescriptors.flush();
        }

        const meta::package_t *Factory::manifest() const
        {
            return pManifest;
        }

        resource::ILoader *Factory::resources()
        {
            return pLoader;
        }

        size_t Factory::descriptors_count() const
        {
            return vDescriptors.size();
        }

        const clap_plugin_descriptor_t *Factory::descriptor(size_t index) const
        {
            return vDescriptors.get(index);
        }

        Wrapper *Factory::instantiate(const clap_host_t *host, const clap_plugin_descriptor_t *descriptor) const
        {
            // Find the plugin by it's identifier in the list of plugin factories
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Find the corresponding CLAP plugin
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    if (!strcmp(meta->uids.clap, descriptor->id))
                    {
                        // Create module
                        plug::Module *plugin = f->create(meta);
                        if (plugin == NULL)
                        {
                            lsp_warn("Failed instantiation of CLAP plugin %s", descriptor->id);
                            return NULL;
                        }
                        lsp_finally {
                            if (plugin != NULL)
                                delete plugin;
                        };

                        // Create the wrapper
                        clap::Wrapper *wrapper    = new clap::Wrapper(plugin, const_cast<Factory *>(this), pManifest, pLoader, host);
                        if (wrapper == NULL)
                        {
                            lsp_warn("Error creating wrapper");
                            return NULL;
                        }

                        plugin              = NULL;

                        return wrapper;
                    }
                }
            }

            lsp_warn("Invalid CLAP plugin identifier: %s", descriptor->id);
            return NULL;
        }

    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_IMPL_FACTORY_H_ */
