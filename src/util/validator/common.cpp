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

#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/util/validator/validator.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/static.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace validator
    {
        constexpr size_t E_ALL_EXTENSIONS =
            meta::E_INLINE_DISPLAY |
            meta::E_3D_BACKEND |
            meta::E_OSC |
            meta::E_KVT_SYNC |
            meta::E_DUMP_STATE |
            meta::E_FILE_PREVIEW;

        static const char *decode_plugin_class(int type)
        {
            switch (type)
            {
                case meta::C_DELAY: return "DELAY"; break;
                case meta::C_REVERB: return "REVERB"; break;
                case meta::C_DISTORTION: return "DISTORTION"; break;
                case meta::C_WAVESHAPER: return "WAVESHAPER"; break;
                case meta::C_DYNAMICS: return "DYNAMICS"; break;
                case meta::C_AMPLIFIER: return "AMPLIFIER"; break;
                case meta::C_COMPRESSOR: return "COMPRESSOR"; break;
                case meta::C_ENVELOPE: return "ENVELOPE"; break;
                case meta::C_EXPANDER: return "EXPANDER"; break;
                case meta::C_GATE: return "GATE"; break;
                case meta::C_LIMITER: return "LIMITER"; break;
                case meta::C_FILTER: return "FILTER"; break;
                case meta::C_ALLPASS: return "ALLPASS"; break;
                case meta::C_BANDPASS: return "BANDPASS"; break;
                case meta::C_COMB: return "COMB"; break;
                case meta::C_EQ: return "EQ"; break;
                case meta::C_MULTI_EQ: return "MULTI_EQ"; break;
                case meta::C_PARA_EQ: return "PARA_EQ"; break;
                case meta::C_HIGHPASS: return "HIGHPASS"; break;
                case meta::C_LOWPASS: return "LOWPASS"; break;
                case meta::C_GENERATOR: return "GENERATOR"; break;
                case meta::C_CONSTANT: return "CONSTANT"; break;
                case meta::C_INSTRUMENT: return "INSTRUMENT"; break;
                case meta::C_OSCILLATOR: return "OSCILLATOR"; break;
                case meta::C_MODULATOR: return "MODULATOR"; break;
                case meta::C_CHORUS: return "CHORUS"; break;
                case meta::C_FLANGER: return "FLANGER"; break;
                case meta::C_PHASER: return "PHASER"; break;
                case meta::C_SIMULATOR: return "SIMULATOR"; break;
                case meta::C_SPATIAL: return "SPATIAL"; break;
                case meta::C_SPECTRAL: return "SPECTRAL"; break;
                case meta::C_PITCH: return "PITCH"; break;
                case meta::C_UTILITY: return "UTILITY"; break;
                case meta::C_ANALYSER: return "ANALYZER"; break;
                case meta::C_CONVERTER: return "CONVERTER"; break;
                case meta::C_FUNCTION: return "FUNCTION"; break;
                case meta::C_MIXER: return "MIXER"; break;
                default: break;
            };
            return "<unknown>";
        }

        static void validate_classes(context_t *ctx, const meta::plugin_t *meta)
        {
            // Ensure that classes are defined
            if (meta->classes == NULL)
            {
                validation_error(ctx, "Not specified classes for plugin uid='%s'", meta->uid);
                return;
            }

            size_t num_classes = 0;
            for (const int *p = meta->classes; *p >= 0; ++p)
            {
                // Increment number of classes
                ++num_classes;

                // check that class is valid
                if (*p >= meta::C_TOTAL)
                    validation_error(ctx, "Unknown plugin class with id=%d for plugin uid='%s'", *p, meta->uid);

                // Check for duplicates
                for (const int *t = p+1; *t >= 0; ++t)
                    if (*t == *p)
                    {
                        validation_error(ctx, "Duplicate plugin class with id=%s (%d) for plugin uid='%s'",
                            decode_plugin_class(*p), *p, meta->uid);
                    }
            }

            // Check that classes match
            if (num_classes <= 0)
                validation_error(ctx, "Unspecified plugin classes for plugin uid='%s'", meta->uid);
        }

        void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
        {
            // Check that port identifier is present
            if (port->id == NULL)
                validation_error(ctx, "Not specified port identifier for plugin uid='%s'", meta->uid);

            // Ensure that port identifier doesn't clash any other port identifier
            const meta::port_t *clash = ctx->port_ids.get(port->id);
            if (clash != NULL)
                validation_error(ctx, "Port id='%s' clashes another port id='%s' for plugin uid='%s'",
                    clash->id, port->id, meta->uid);
            else if (!(ctx->port_ids.create(port->id, const_cast<meta::port_t *>(port))))
                allocation_error(ctx);

            // Additional checks for specific port types
            switch (port->role)
            {
                case meta::R_UI_SYNC:
                    validation_error(ctx, "Unallowed meta::R_UI_SYNC port type for port='%s', plugin uid='%s'",
                        port->id, meta->uid);
                    return;
                case meta::R_AUDIO:
                case meta::R_CONTROL:
                case meta::R_METER:
                case meta::R_MESH:
                case meta::R_FBUFFER:
                case meta::R_PATH:
                    break;
                case meta::R_MIDI:
                {
                    if (meta::is_in_port(port))
                    {
                        if ((++ctx->midi_in) == 2)
                        {
                            validation_error(ctx, "More than one MIDI input port specified for plugin uid='%s', port='%s'",
                                meta->uid, port->id);
                        }
                    }
                    else
                    {
                        if ((++ctx->midi_out) == 2)
                        {
                            validation_error(ctx, "More than one MIDI output port specified for plugin uid='%s', port='%s",
                                meta->uid, port->id);
                        }
                    }
                    break;
                }
                case meta::R_PORT_SET:
                {
                    // Ensure that PORT_SET contains items
                    if (port->items == NULL)
                    {
                        validation_error(ctx, "List items not specified for the port set port id='s' uid='%s'",
                            meta->uid);
                        return;
                    }
                    return;
                }
                case meta::R_OSC:
                {
                    if (meta::is_in_port(port))
                    {
                        if ((++ctx->osc_in) == 2)
                        {
                            validation_error(ctx, "More than one OSC input port specified for plugin uid='%s'",
                                meta->uid);
                        }
                    }
                    else
                    {
                        if ((++ctx->osc_out) == 2)
                        {
                            validation_error(ctx, "More than one OSC output port specified for plugin uid='%s'",
                                meta->uid);
                        }
                    }
                    break;
                }
                case meta::R_BYPASS:
                {
                    if (!meta::is_in_port(port))
                    {
                        validation_error(ctx, "Bypass port is specified as output port for plugin uid='%s'",
                            meta->uid);
                        break;
                    }
                    if ((++ctx->bypass) == 2)
                    {
                        validation_error(ctx, "More than one bypass output port specified for plugin uid='%s'",
                            meta->uid);
                    }

                    break;
                }
                case meta::R_STREAM:
                    break;
                default:
                    validation_error(ctx, "Unknown port type %d for port='%s', plugin uid='%s'",
                        int(port->role), port->id, meta->uid);
                    return;
            }
        }

        void validate_ports(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port, const char *postfix)
        {
            // Validate port set port
            validate_port(ctx, meta, port);
            ladspa::validate_port(ctx, meta, port);
            lv2::validate_port(ctx, meta, port);
            vst2::validate_port(ctx, meta, port);
            jack::validate_port(ctx, meta, port);
            clap::validate_port(ctx, meta, port);

            if ((port->role == meta::R_PORT_SET) && (port->items != NULL))
            {
                // Process each row
                char postfix_buf[MAX_PARAM_ID_BYTES];
                size_t num_rows = meta::list_size(port->items);

                // Generate nested ports
                for (size_t row=0; row<num_rows; ++row)
                {
                    // Generate postfix
                    snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                    // Clone port metadata
                    meta::port_t *cm        = meta::clone_port_metadata(port->members, postfix_buf);
                    if (cm == NULL)
                    {
                        allocation_error(ctx);
                        return;
                    }
                    lsp_finally {
                        if (cm != NULL)
                            meta::drop_port_metadata(cm);
                    };
                    if (ctx->gen_ports.add(cm))
                        cm      = NULL;
                    else
                        allocation_error(ctx);

                    // Perform checks of ports in the group
                    size_t col          = 0;
                    for (; cm->id != NULL; ++cm, ++col)
                    {
                        if (meta::is_growing_port(cm))
                            cm->start    = cm->min + ((cm->max - cm->min) * row) / float(num_rows);
                        else if (meta::is_lowering_port(cm))
                            cm->start    = cm->max - ((cm->max - cm->min) * row) / float(num_rows);

                        // Recursively call for port validation
                        validate_ports(ctx, meta, cm, postfix_buf);
                    } // for cm
                } // for row
            } // meta::R_PORT_SET
        }

        void validate_ports(context_t *ctx, const meta::plugin_t *meta)
        {
            if (meta->ports == NULL)
            {
                validation_error(ctx, "Not specified plugin name for plugin uid='%s'", meta->uid);
                return;
            }

            // Drop all generated ports
            lsp_finally {
                for (size_t i=0, n=ctx->gen_ports.size(); i<n; ++i)
                    meta::drop_port_metadata(ctx->gen_ports.uget(i));
                ctx->gen_ports.flush();
            };

            // Create ports
            if (!ctx->port_ids.create(lsp::lv2::latency_port.id, const_cast<meta::port_t *>(&lsp::lv2::latency_port)))
                allocation_error(ctx);
            for (const meta::port_t *p = lsp::lv2::atom_ports; p->id != NULL; ++p)
            {
                if (!ctx->port_ids.create(p->id, const_cast<meta::port_t *>(p)))
                    allocation_error(ctx);
            }

            // Validate all metadata ports
            for (const meta::port_t *port = meta->ports; port->id != NULL; ++port)
                validate_ports(ctx, meta, port, NULL);
        }

        void validate_port_groups(context_t *ctx, const meta::plugin_t *meta)
        {
            if (meta->port_groups == NULL)
                return;
        }

        void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
        {
            const meta::plugin_t *clash = NULL;

            ctx->midi_in    = 0;
            ctx->midi_out   = 0;
            ctx->osc_in     = 0;
            ctx->osc_out    = 0;
            ctx->bypass     = 0;
            ctx->port_ids.flush();
            ctx->clap_ids.flush();

            // Validate name
            if (meta->name == NULL)
                validation_error(ctx, "Not specified plugin name for plugin uid='%s'", meta->uid);

            // Validate description
            if (meta->description == NULL)
                validation_error(ctx, "Not specified plugin description for plugin uid='%s'", meta->uid);

            // Validate acronym
            if (meta->acronym == NULL)
                validation_error(ctx, "Not specified plugin acronym for plugin uid='%s'", meta->uid);
            if ((clash = ctx->lsp_acronyms.get(meta->acronym)) != NULL)
                validation_error(ctx, "Model acronym '%s' for plugin uid='%s' clashes the acronym for plugin uid='%s'",
                    meta->acronym, meta->uid, clash->uid);
            if (!ctx->lsp_acronyms.create(meta->acronym, const_cast<meta::plugin_t *>(meta)))
                allocation_error(ctx);

            // Validate presence of developer and add to list of developers
            if (meta->developer != NULL)
            {
                if (!ctx->developers.contains(meta->developer))
                {
                    if (!ctx->developers.add(const_cast<meta::person_t *>(meta->developer)))
                        allocation_error(ctx);
                }
            }
            else
                validation_error(ctx, "Plugin plugin uid='%s' has no developer specified", meta->uid);

            // Validate that plugin has specified unique identifier
            if (meta->uid == NULL)
                validation_error(ctx, "Not specified plugin unique identifier for plugin name='%s', description='%s', acronym='%s'",
                    meta->name, meta->description, meta->acronym);

            // Validate classes
            validate_classes(ctx, meta);

            // Validate extensions
            if ((meta->extensions & (~E_ALL_EXTENSIONS)) != meta::E_NONE)
                validation_error(ctx, "Plugin has invalid set of extensions=0x%x uid='%s", int(meta->extensions), meta->uid);

            // Validate presence of the bundle and add to list of bundles
            if (meta->bundle != NULL)
            {
                if (!ctx->bundles.contains(meta->bundle))
                {
                    if (!ctx->bundles.add(const_cast<meta::bundle_t *>(meta->bundle)))
                        allocation_error(ctx);
                }
            }
            else
                validation_error(ctx, "Not specified bundle for plugin uid='%s'", meta->uid);

            // Validate version

            // Validate port groups
            validate_port_groups(ctx, meta);

            // Validate ports
            validate_ports(ctx, meta);

            // Call the nested validators for each plugin format
            ladspa::validate_plugin(ctx, meta);
            lv2::validate_plugin(ctx, meta);
            vst2::validate_plugin(ctx, meta);
            jack::validate_plugin(ctx, meta);
            clap::validate_plugin(ctx, meta);

//            const char             *lv2_uri;        // LV2 URI
//            const char             *lv2ui_uri;      // LV2 UI URI
//            const char             *vst2_uid;       // Steinberg VST 2.x ID of the plugin
//            const uint32_t          ladspa_id;      // LADSPA ID of the plugin
//            const char             *ladspa_lbl;     // LADSPA unique label of the plugin
//            const char             *clap_uid;       // Unique identifier for CLAP format
// TODO          const version_t         version;        // Version of the plugin
//            const int              *clap_features;  // List of CLAP plugin features
//            const char             *ui_resource;    // Location of the UI file resource
//            const char             *ui_presets;     // Prefix of the preset location
        }

    } /* namespace validator */
} /* namespace lsp */



