/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 7 февр. 2022 г.
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

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/util/pluglist_gen/pluglist_gen.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>


namespace lsp
{
    namespace pluglist_gen
    {
        const enumeration_t plugin_groups[] =
        {
            { meta::C_DELAY,        "Delay" },
            { meta::C_REVERB,       "Reverb" },
            { meta::C_DISTORTION,   "Distortion" },
            { meta::C_WAVESHAPER,   "Waveshaper" },
            { meta::C_DYNAMICS,     "Dynamics" },
            { meta::C_AMPLIFIER,    "Amplifier" },
            { meta::C_COMPRESSOR,   "Compressor" },
            { meta::C_ENVELOPE,     "Envelope" },
            { meta::C_EXPANDER,     "Expander" },
            { meta::C_GATE,         "Gate" },
            { meta::C_LIMITER,      "Limiter" },
            { meta::C_FILTER,       "Filter" },
            { meta::C_ALLPASS,      "Allpass" },
            { meta::C_BANDPASS,     "Bandpass" },
            { meta::C_COMB,         "Comb" },
            { meta::C_EQ,           "Equalizer" },
            { meta::C_MULTI_EQ,     "Multiband Equalizer" },
            { meta::C_PARA_EQ,      "Parametric Equalizer" },
            { meta::C_HIGHPASS,     "Highpass" },
            { meta::C_LOWPASS,      "Lowpass" },
            { meta::C_GENERATOR,    "Generator" },
            { meta::C_CONSTANT,     "Constant" },
            { meta::C_INSTRUMENT,   "Instrument" },
            { meta::C_OSCILLATOR,   "Oscillator" },
            { meta::C_MODULATOR,    "Modulator" },
            { meta::C_CHORUS,       "Chorus" },
            { meta::C_FLANGER,      "Flanger" },
            { meta::C_PHASER,       "Phaser" },
            { meta::C_SIMULATOR,    "Simulator" },
            { meta::C_SPATIAL,      "Spatial" },
            { meta::C_SPECTRAL,     "Spectral" },
            { meta::C_PITCH,        "Pitch" },
            { meta::C_UTILITY,      "Utility" },
            { meta::C_ANALYSER,     "Analyser" },
            { meta::C_CONVERTER,    "Converter" },
            { meta::C_FUNCTION,     "Function" },
            { meta::C_MIXER,        "Mixer" },
            { -1, NULL }
        };

        const enumeration_t bundle_groups[] =
        {
            { meta::B_ANALYZERS,        "analyzers" },
            { meta::B_CONVOLUTION,      "convolution" },
            { meta::B_DELAYS,           "delays" },
            { meta::B_DYNAMICS,         "dynamics" },
            { meta::B_EQUALIZERS,       "equalizers" },
            { meta::B_MB_DYNAMICS,      "mb_dynamics" },
            { meta::B_MB_PROCESSING,    "mb_processing" },
            { meta::B_SAMPLERS,         "samplers" },
            { meta::B_UTILITIES,        "utilities" },
            { -1, NULL }
        };

        const char *get_enumeration(int id, const enumeration_t *list)
        {
            for(; list->id >= 0; ++list)
                if (list->id == id)
                    return list->name;
            return NULL;
        }

        status_t parse_cmdline(cmdline_t *cfg, int argc, const char **argv)
        {
            // Initialize config with default values
            cfg->php_out        = NULL;
            cfg->json_out       = NULL;

            // Parse arguments
            int i = 1;

            while (i < argc)
            {
                const char *arg = argv[i++];
                if ((!::strcmp(arg, "--help")) || (!::strcmp(arg, "-h")))
                {
                    printf("Usage: %s [parameters]\n\n", argv[0]);
                    printf("Available parameters:\n");
                    printf("  --json <file>             The name of output JSON descriptor file\n");
                    printf("  --php <file>              The name of output PHP descriptor file\n");
                    printf("\n");

                    return STATUS_CANCELLED;
                }
                else if (!::strcmp(arg, "--php"))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified directory name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->php_out)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->php_out = argv[i++];
                }
                else if (!::strcmp(arg, "--json"))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified file name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->json_out)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->json_out = argv[i++];
                }
                else
                {
                    fprintf(stderr, "Unknown parameter: %s\n", arg);
                    return STATUS_BAD_ARGUMENTS;
                }
            }

            return STATUS_OK;
        }

        static status_t get_manifest(meta::package_t **pkg)
        {
            // Obtain the manifest
            status_t res;

            resource::ILoader *loader = core::create_resource_loader();
            if (loader != NULL)
            {
                io::IInStream *is = loader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
                if (is != NULL)
                {
                    res = meta::load_manifest(pkg, is);
                    res = update_status(res, is->close());
                    delete is;
                }
                else
                    res = loader->last_error();
                delete loader;
            }
            else
                res = STATUS_NOT_FOUND;

            if (res != STATUS_OK)
                fprintf(stderr, "Error loading manifest file, error=%d", int(res));

            return res;
        }

        static ssize_t cmp_plugins(const meta::plugin_t *p1, const meta::plugin_t *p2)
        {
            return strcmp(p1->uid, p2->uid);
        }

        static ssize_t cmp_bundles(const meta::bundle_t *b1, const meta::bundle_t *b2)
        {
            return strcmp(b1->uid, b2->uid);
        }

        status_t scan_plugins(lltl::parray<meta::plugin_t> *plugins)
        {
            // Generate descriptors
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Add the plugin to list
                    if (!plugins->add(const_cast<meta::plugin_t *>(meta)))
                        return STATUS_NO_MEM;
                }
            }

            // Sort descriptors
            plugins->qsort(cmp_plugins);

            return STATUS_OK;
        }

        status_t enum_bundles(
            lltl::parray<meta::bundle_t> *bundles,
            const meta::plugin_t * const *plugins,
            size_t num_plugins)
        {
            lltl::pphash<char, meta::bundle_t> map;

            // Output plugins
            for (size_t i=0; i<num_plugins; ++i)
            {
                const meta::bundle_t *bundle = plugins[i]->bundle;
                if ((bundle == NULL) || (bundle->uid == NULL))
                    continue;

                const meta::bundle_t *xbundle = map.get(bundle->uid);
                if (xbundle == NULL)
                {
                    if (!map.create(bundle->uid, const_cast<meta::bundle_t *>(bundle)))
                        return STATUS_NO_MEM;
                }
                else if (xbundle != bundle)
                {
                    if ((xbundle->group != bundle->group) ||
                        (strcmp(xbundle->name, bundle->name)) ||
                        (strcmp(xbundle->description, bundle->description)) ||
                        (strcmp(xbundle->video_id, bundle->video_id)))
                    {
                        fprintf(stderr, "Conflicting bundles with id=%s detected\n", xbundle->uid);
                        return STATUS_CORRUPTED;
                    }
                }
            }

            // Now we are ready to fetch all the list of bundles
            if (!map.values(bundles))
                return STATUS_NO_MEM;
            bundles->qsort(cmp_bundles);
            map.flush();

            return STATUS_OK;
        }

        status_t generate(const cmdline_t *cmd)
        {
            status_t res = STATUS_OK;
            meta::package_t *pkg = NULL;
            lltl::parray<meta::plugin_t> plugins;
            lltl::parray<meta::bundle_t> bundles;

            const meta::plugin_t *const *pluglist;

            // Read the manifest file and obtain plugin list
            res = get_manifest(&pkg);
            if (res == STATUS_OK)
            {
                res = scan_plugins(&plugins);
                pluglist = plugins.array();
            }

            // Export data
            if ((res == STATUS_OK) && (cmd->php_out))
                res = generate_php(cmd->php_out, pkg, pluglist, plugins.size());
            if ((res == STATUS_OK) && (cmd->json_out))
                res = generate_json(cmd->json_out, pkg, pluglist, plugins.size());

            // Release previously allocated resources
            if (pkg != NULL)
            {
                meta::free_manifest(pkg);
                pkg = NULL;
            }

            return res;
        }

        status_t main(int argc, const char **argv)
        {
            cmdline_t cmd;
            status_t res = parse_cmdline(&cmd, argc, argv);
            return (res == STATUS_OK) ? generate(&cmd) : res;
        }

    } /* namespace pluglist_gen */
} /* namespace lsp */

