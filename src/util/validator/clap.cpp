/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 янв. 2023 г.
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

#include <clap/clap.h>
#include <lsp-plug.in/plug-fw/util/validator/validator.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/wrap/clap/helpers.h>

namespace lsp
{
    namespace validator
    {
        namespace clap
        {
            static const char *decode_clap_feature(int type)
            {
                switch (type)
                {
                    case meta::CF_INSTRUMENT:       return CLAP_PLUGIN_FEATURE_INSTRUMENT; break;
                    case meta::CF_AUDIO_EFFECT:     return CLAP_PLUGIN_FEATURE_AUDIO_EFFECT; break;
                    case meta::CF_NOTE_EFFECT:      return CLAP_PLUGIN_FEATURE_NOTE_EFFECT; break;
                    case meta::CF_ANALYZER:         return CLAP_PLUGIN_FEATURE_ANALYZER; break;
                    case meta::CF_SYNTHESIZER:      return CLAP_PLUGIN_FEATURE_SYNTHESIZER; break;
                    case meta::CF_SAMPLER:          return CLAP_PLUGIN_FEATURE_SAMPLER; break;
                    case meta::CF_DRUM:             return CLAP_PLUGIN_FEATURE_DRUM; break;
                    case meta::CF_DRUM_MACHINE:     return CLAP_PLUGIN_FEATURE_DRUM_MACHINE; break;
                    case meta::CF_FILTER:           return CLAP_PLUGIN_FEATURE_FILTER; break;
                    case meta::CF_PHASER:           return CLAP_PLUGIN_FEATURE_PHASER; break;
                    case meta::CF_EQUALIZER:        return CLAP_PLUGIN_FEATURE_EQUALIZER; break;
                    case meta::CF_DEESSER:          return CLAP_PLUGIN_FEATURE_DEESSER; break;
                    case meta::CF_PHASE_VOCODER:    return CLAP_PLUGIN_FEATURE_PHASE_VOCODER; break;
                    case meta::CF_GRANULAR:         return CLAP_PLUGIN_FEATURE_GRANULAR; break;
                    case meta::CF_FREQUENCY_SHIFTER:return CLAP_PLUGIN_FEATURE_FREQUENCY_SHIFTER; break;
                    case meta::CF_PITCH_SHIFTER:    return CLAP_PLUGIN_FEATURE_PITCH_SHIFTER; break;
                    case meta::CF_DISTORTION:       return CLAP_PLUGIN_FEATURE_DISTORTION; break;
                    case meta::CF_TRANSIENT_SHAPER: return CLAP_PLUGIN_FEATURE_TRANSIENT_SHAPER; break;
                    case meta::CF_COMPRESSOR:       return CLAP_PLUGIN_FEATURE_COMPRESSOR; break;
                    case meta::CF_EXPANDER:         return CLAP_PLUGIN_FEATURE_EXPANDER; break;
                    case meta::CF_GATE:             return CLAP_PLUGIN_FEATURE_GATE; break;
                    case meta::CF_LIMITER:          return CLAP_PLUGIN_FEATURE_LIMITER; break;
                    case meta::CF_FLANGER:          return CLAP_PLUGIN_FEATURE_FLANGER; break;
                    case meta::CF_CHORUS:           return CLAP_PLUGIN_FEATURE_CHORUS; break;
                    case meta::CF_DELAY:            return CLAP_PLUGIN_FEATURE_DELAY; break;
                    case meta::CF_REVERB:           return CLAP_PLUGIN_FEATURE_REVERB; break;
                    case meta::CF_TREMOLO:          return CLAP_PLUGIN_FEATURE_TREMOLO; break;
                    case meta::CF_GLITCH:           return CLAP_PLUGIN_FEATURE_GLITCH; break;
                    case meta::CF_UTILITY:          return CLAP_PLUGIN_FEATURE_UTILITY; break;
                    case meta::CF_PITCH_CORRECTION: return CLAP_PLUGIN_FEATURE_PITCH_CORRECTION; break;
                    case meta::CF_RESTORATION:      return CLAP_PLUGIN_FEATURE_RESTORATION; break;
                    case meta::CF_MULTI_EFFECTS:    return CLAP_PLUGIN_FEATURE_MULTI_EFFECTS; break;
                    case meta::CF_MIXING:           return CLAP_PLUGIN_FEATURE_MIXING; break;
                    case meta::CF_MASTERING:        return CLAP_PLUGIN_FEATURE_MASTERING; break;
                    case meta::CF_MONO:             return CLAP_PLUGIN_FEATURE_MONO; break;
                    case meta::CF_STEREO:           return CLAP_PLUGIN_FEATURE_STEREO; break;
                    case meta::CF_SURROUND:         return CLAP_PLUGIN_FEATURE_SURROUND; break;
                    case meta::CF_AMBISONIC:        return CLAP_PLUGIN_FEATURE_AMBISONIC; break;
                    default: break;
                };
                return "<unknown>";
            }

            static void validate_features(context_t *ctx, const meta::plugin_t *meta)
            {
                // Ensure that classes are defined
                if (meta->clap_features == NULL)
                {
                    validation_error(ctx, "Not specified CLAP features for plugin uid='%s'", meta->uid);
                    return;
                }

                size_t num_features = 0;
                size_t num_required = 0;
                for (const int *p = meta->clap_features; *p >= 0; ++p)
                {
                    // Increment number of classes
                    ++num_features;

                    // Check that feature is valid
                    if (*p >= meta::CF_TOTAL)
                        validation_error(ctx, "Unknown plugin CLAP feature with id=%d for plugin uid='%s'", *p, meta->uid);

                    // Check for required features
                    switch (*p)
                    {
                        case meta::CF_INSTRUMENT:
                        case meta::CF_AUDIO_EFFECT:
                        case meta::CF_NOTE_EFFECT:
                        case meta::CF_ANALYZER:
                            ++ num_required;
                            break;
                        default:
                            break;
                    }

                    // Check for duplicates
                    for (const int *t = p+1; *t >= 0; ++t)
                        if (*t == *p)
                        {
                            validation_error(ctx, "Duplicate plugin CLAP feature with id=%s (%d) for plugin uid='%s'",
                                decode_clap_feature(*p), *p, meta->uid);
                        }
                }

                // Check that classes match
                if (num_features <= 0)
                    validation_error(ctx, "Unspecified plugin classes for plugin uid='%s'", meta->uid);

                // Check list of required features
                if (num_required <= 0)
                    validation_error(ctx, "Unspecified plugin CLAP mandatory features for plugin uid='%s', should be one of '%s', '%s', '%s', '%s'",
                        meta->uid, CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
                        CLAP_PLUGIN_FEATURE_NOTE_EFFECT, CLAP_PLUGIN_FEATURE_ANALYZER);
                else if (num_required > 1)
                    validation_error(ctx, "Too many plugin CLAP mandatory features for plugin uid='%s', should be one of '%s', '%s', '%s', '%s'",
                        meta->uid, CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
                        CLAP_PLUGIN_FEATURE_NOTE_EFFECT, CLAP_PLUGIN_FEATURE_ANALYZER);
            }

            size_t count_ports(const meta::port_group_t *pg)
            {
                size_t count = 0;
                if (pg == NULL)
                    return count;
                for (const meta::port_group_item_t *it = pg->items; (it != NULL) && (it->id != NULL); ++it)
                {
                    ++count;
                }
                return count;
            }

            void validate_groups(context_t *ctx, const meta::plugin_t *meta)
            {
                const meta::port_group_t *main_in = NULL;
                const meta::port_group_t *main_out = NULL;

                // Check port groups
                if (meta->port_groups == NULL)
                    validation_error(ctx, "Plugin uid='%s' has no defined port groups", meta->uid);

                // Iterate over main groups and check the designation to main
                for (const meta::port_group_t *pg = meta->port_groups; (pg != NULL) && (pg->id != NULL); ++pg)
                {
                    if (!(pg->flags & meta::PGF_MAIN))
                        continue;

                    if (pg->flags & meta::PGF_OUT)
                    {
                        if (main_out != NULL)
                            validation_error(ctx, "Plugin uid='%s' has duplicate output main group id='%s', previous is: id='%s'", meta->uid, pg->id, main_out->id);
                        if (count_ports(pg) <= 0)
                            validation_error(ctx, "Plugin uid='%s' output port group id='%s' has no members", meta->uid, pg->id);
                        main_out    = pg;
                    }
                    else
                    {
                        if (main_in!= NULL)
                            validation_error(ctx, "Plugin uid='%s' has duplicate input main group id='%s', previous is: id='%s'", meta->uid, pg->id, main_in->id);
                        if (count_ports(pg) <= 0)
                            validation_error(ctx, "Plugin uid='%s' input port group id='%s', has no members", meta->uid, pg->id);
                        main_in     = pg;
                    }
                }
            }

            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {
                // Validate CLAP identifier
                if (meta->clap_uid == NULL)
                {
                    if (meta->clap_features != NULL)
                        validation_error(ctx, "Plugin uid='%s' has no CLAP identifier but provides CLAP features", meta->uid);
                    return;
                }

                // Check conflicts
                const meta::plugin_t *clash = ctx->clap_ids.get(meta->clap_uid);
                if (clash != NULL)
                    validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate CLAP identifier '%s'",
                        meta->uid, clash->uid, meta->clap_uid);
                else if (!ctx->clap_ids.create(meta->clap_uid, const_cast<meta::plugin_t *>(meta)))
                    allocation_error(ctx);

                // Validate features
                validate_features(ctx, meta);

                // Validate port groups
                validate_groups(ctx, meta);
            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
                // Check that control port does not clash with another control port
                if (meta::is_control_port(port) && meta::is_in_port(port))
                {
                    clap_id uid = lsp::clap::clap_hash_string(port->id);
                    const meta::port_t *clash = ctx->clap_port_ids.get(&uid);
                    if (clash != NULL)
                        validation_error(ctx, "Port id='%s' clashes port id='%s' for plugin uid='%s'",
                            port->id, clash->id, meta->uid);
                    else if (!ctx->clap_port_ids.create(&uid, const_cast<meta::port_t *>(port)))
                        allocation_error(ctx);
                }
            }
        } /* namespace clap */
    } /* namespace validator */
} /* namespace lsp */




