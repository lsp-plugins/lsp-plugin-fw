/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/util/validator/validator.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/static.h>
#include <lsp-plug.in/stdlib/stdio.h>

#ifdef WITH_UI_FEATURE
    #include <lsp-plug.in/plug-fw/ui.h>
#endif /* WITH_UI_FEATURE */

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
            meta::E_FILE_PREVIEW |
            meta::E_SHM_TRACKING;

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
                case meta::C_DRUM: return "DRUM"; break;
                case meta::C_EXTERNAL: return "EXTERNAL"; break;
                case meta::C_PIANO: return "PIANO"; break;
                case meta::C_SAMPLER: return "SAMPLER"; break;
                case meta::C_SYNTH: return "SYNTH"; break;
                case meta::C_SYNTH_SAMPLER: return "SYNTH_SAMPLER"; break;
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

        static bool validate_identifier(const char *id)
        {
            size_t count = 0;
            for (; *id != '\0'; ++id)
            {
                const char ch = *id;

                if ((ch >= 'a') && (ch <= 'z'))
                    ++count;
                else if ((ch >= 'A') && (ch <= 'Z'))
                    ++count;
                else if ((ch >= '0') && (ch <= '9'))
                {
                    if ((count++) <= 0)
                        return false;
                }
                else if (ch != '_')
                    return false;
            }

            return count > 0;
        }

        static bool is_float_value_port(const meta::port_t *port)
        {
            switch (port->role)
            {
                case meta::R_CONTROL:
                case meta::R_METER:
                case meta::R_BYPASS:
                case meta::R_PORT_SET:
                    return true;
                default:
                    break;
            }
            return false;
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
            else if (!validate_identifier(port->id)) // Check that port identifier is valid
                validation_error(ctx, "Invalid port identifier '%s' for plugin uid='%s', allowed characters are: a-z, A-Z, _, 0-9",
                    port->id, meta->uid);

            // Ensure that port identifier doesn't clash any other port identifier
            const meta::port_t *clash = ctx->port_ids.get(port->id);
            if (clash != NULL)
                validation_error(ctx, "Port id='%s' clashes another port id='%s' for plugin uid='%s'",
                    clash->id, port->id, meta->uid);
            else if (!(ctx->port_ids.create(port->id, const_cast<meta::port_t *>(port))))
                allocation_error(ctx);

            // Ensure that port short name doesn't clash with any othe port name
            if (port->short_name != NULL)
            {
                clash       = ctx->port_short_names.get(port->short_name);
                if (clash != NULL)
                    validation_error(ctx, "Port id='%s' short name '%s' clashes short name of port id='%s' for plugin uid='%s'",
                        clash->id, port->short_name, port->id, meta->uid);
                else if (!(ctx->port_short_names.create(port->short_name, const_cast<meta::port_t *>(port))))
                    allocation_error(ctx);
            }

            // Additional checks for specific port types
            switch (port->role)
            {
                case meta::R_AUDIO_IN:
                case meta::R_AUDIO_OUT:
                case meta::R_AUDIO_SEND:
                case meta::R_AUDIO_RETURN:
                case meta::R_CONTROL:
                case meta::R_METER:
                case meta::R_MESH:
                case meta::R_FBUFFER:
                case meta::R_PATH:
                    break;
                case meta::R_STRING:
                    if (port->value == NULL)
                    {
                        validation_error(ctx, "The default string value should be specified for plugin uid='%s', STRING port='%s'",
                            meta->uid, port->id);
                    }
                    break;
                case meta::R_SEND_NAME:
                    if (port->value == NULL)
                    {
                        validation_error(ctx, "The default string value should be specified for plugin uid='%s', SEND_NAME port='%s'",
                            meta->uid, port->id);
                    }
                    break;
                case meta::R_RETURN_NAME:
                    if (port->value == NULL)
                    {
                        validation_error(ctx, "The default string value should be specified for plugin uid='%s', RETURN_NAME port='%s'",
                            meta->uid, port->id);
                    }
                    break;
                case meta::R_MIDI_IN:
                {
                    if ((++ctx->midi_in) == 2)
                    {
                        validation_error(ctx, "More than one MIDI input port specified for plugin uid='%s', port='%s'",
                            meta->uid, port->id);
                    }
                    break;
                }
                case meta::R_MIDI_OUT:
                {
                    if ((++ctx->midi_out) == 2)
                    {
                        validation_error(ctx, "More than one MIDI output port specified for plugin uid='%s', port='%s",
                            meta->uid, port->id);
                    }
                    break;
                }
                case meta::R_PORT_SET:
                {
                    // Ensure that PORT_SET contains items
                    if (port->items == NULL)
                    {
                        validation_error(ctx, "List items not specified for the PORT SET port id='s' uid='%s'",
                            meta->uid);
                        return;
                    }
                    return;
                }
                case meta::R_OSC_IN:
                {
                    if ((++ctx->osc_in) == 2)
                    {
                        validation_error(ctx, "More than one OSC input port specified for plugin uid='%s'",
                            meta->uid);
                    }
                    break;
                }

                case meta::R_OSC_OUT:
                {
                    if ((++ctx->osc_out) == 2)
                    {
                        validation_error(ctx, "More than one OSC output port specified for plugin uid='%s'",
                            meta->uid);
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


            // Check that port range is valid
            if (is_float_value_port(port))
            {
                if (((port->flags & (meta::F_LOWER | meta::F_UPPER))  == (meta::F_LOWER | meta::F_UPPER)) &&
                    (port->min == port->max))
                {
                    validation_error(ctx, "Plugin uid='%s': port='%s' type='%s' defined value range is zero: "
                        "min=%f, max=%f",
                        meta->uid, port->id,
                        meta::port_role_name(port->role),
                        port->min, port->max);
                }
            }
        }

        void validate_ports(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port, const char *postfix)
        {
            // Validate port set port
            validate_port(ctx, meta, port);
            ladspa::validate_port(ctx, meta, port);
            lv2::validate_port(ctx, meta, port);
            vst2::validate_port(ctx, meta, port);
            vst3::validate_port(ctx, meta, port);
            jack::validate_port(ctx, meta, port);
            clap::validate_port(ctx, meta, port);
            gst::validate_port(ctx, meta, port);

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
                    if (ctx->gen_ports.add(cm))
                    {
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
                    }
                    else
                    {
                        meta::drop_port_metadata(cm);
                        allocation_error(ctx);
                        return;
                    }
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

        void validate_send_return(context_t *ctx, const meta::plugin_t *meta)
        {
            typedef struct group_t
            {
                const meta::port_t *desc;
                lltl::parray<meta::port_t> items;
            } group_t;

            lltl::pphash<char, group_t> groups;
            lsp_finally {
                for (lltl::iterator<group_t> it = groups.values(); it; ++it)
                    delete it.get();
            };

            // Pass 1: fill all information about send/return groups
            for (const meta::port_t *p = meta->ports; (p != NULL) && (p->id != NULL); ++p)
            {
                if ((meta::is_send_name(p)) || (meta::is_return_name(p)))
                {
                    group_t **res = groups.create(p->id);
                    if (res == NULL)
                    {
                        validation_error(ctx, "Failed to create send/return id='%s' for plugin uid=%s", p->id, meta->uid);
                        continue;
                    }

                    group_t *group = new group_t;
                    if (group == NULL)
                    {
                        validation_error(ctx, "Failed to allocate memory for send/return id='%s' for plugin uid=%s", p->id, meta->uid);
                        continue;
                    }

                    group->desc = p;
                    *res        = group;
                }
            }

            // Pass 2: fill all information about sends and returns
            for (const meta::port_t *p = meta->ports; (p != NULL) && (p->id != NULL); ++p)
            {
                if ((meta::is_audio_send_port(p)) || (meta::is_audio_return_port(p)))
                {
                    const char *type = (meta::is_audio_send_port(p)) ? "Send" : "Return";

                    // Ensure that mapping to the group is specified
                    if ((p->value == NULL) || (strlen(p->value) <= 0))
                    {
                        validation_error(ctx, "%s id='%s' has no group specified for plugin uid=%s", type, p->id, meta->uid);
                        continue;
                    }

                    // Ensture that group exists
                    group_t *group = groups.get(p->value);
                    if (group == NULL)
                    {
                        validation_error(ctx, "%s id='%s' is set to belong to send/return group '%s' but group '%s' does not exist for plugin uid=%s",
                            type, p->id, p->value, p->value, meta->uid);
                        continue;
                    }

                    // Check that group type matches the member type
                    if ((meta::is_audio_send_port(p)) && (!meta::is_send_name(group->desc)))
                    {
                        validation_error(ctx, "%s id='%s' is set to belong to return group '%s' but group '%s' accepts only ports of type 'AUDIO_RETURN' for plugin uid=%s",
                            type, p->id, p->value, p->value, meta->uid);
                    }
                    else if ((meta::is_audio_return_port(p)) && (!meta::is_return_name(group->desc)))
                    {
                        validation_error(ctx, "%s id='%s' is set to belong to send group '%s' but group '%s' accepts only ports of type 'AUDIO_SEND' for plugin uid=%s",
                            type, p->id, p->value, p->value, meta->uid);
                    }

                    // Check that proper index has been specified
                    ssize_t index = p->start;
                    if ((index < 0) || (index > 0x100))
                    {
                        validation_error(ctx, "%s id='%s' has invalid group index %d for plugin uid=%s",
                            type, p->id, int(index), meta->uid);
                        continue;
                    }

                    while (group->items.size() <= size_t(index))
                        group->items.append(static_cast<meta::port_t *>(NULL));

                    const meta::port_t *xport = group->items.uget(index);
                    if (xport != NULL)
                    {
                        validation_error(ctx, "%s id='%s' conflicts with port id='%s': has duplicate group index %d for group id='%s', plugin uid=%s",
                            type, p->id, xport->id, int(index), group->desc->id, meta->uid);
                        continue;
                    }
                    group->items.set(index, const_cast<meta::port_t *>(p));
                }
            }

            // Pass 3: validate groups for empty elements
            for (lltl::iterator<group_t> it = groups.values(); it; ++it)
            {
                group_t *group = it.get();
                if (group == NULL)
                    continue;

                const char *type = (meta::is_send_name(group->desc)) ? "Send group" : "Return group";

                if (group->items.size() <= 0)
                {
                    validation_error(ctx, "%s id='%s' has no nested audio send/return items for plugin uid=%s",
                        type, group->desc->id, meta->uid);
                    continue;
                }

                for (size_t i=0, n=group->items.size(); i<n; ++i)
                {
                    const meta::port_t *p = group->items.uget(i);
                    if (p == NULL)
                    {
                        validation_error(ctx, "%s id='%s' has missing send/return item with index %d for plugin uid=%s",
                            type, group->desc->id, int(i), meta->uid);
                    }
                }
            }
        }

        const meta::port_group_t *find_port_group(const meta::plugin_t *meta, const char *uid)
        {
            for (const meta::port_group_t *pg = meta->port_groups; pg->id != NULL; ++pg)
                if (!strcmp(pg->id, uid))
                    return pg;
            return NULL;
        }

        const meta::port_t *find_port(const meta::plugin_t *meta, const char *uid)
        {
            for (const meta::port_t *p = meta->ports; p->id != NULL; ++p)
                if (!strcmp(p->id, uid))
                    return p;
            return NULL;
        }

        void validate_port_groups(context_t *ctx, const meta::plugin_t *meta)
        {
            // Validate only if there are some port groups
            if (meta->port_groups == NULL)
                return;

            // List of visited ports and port groups
            lltl::phashset<char> visited_groups;
            lltl::pphash<char, char> visited_ports;
            lsp_finally {
                visited_ports.flush();
                visited_groups.flush();
            };

            const meta::port_group_t *main_in = NULL;
            const meta::port_group_t *main_out = NULL;

            // Process all port groups
            for (const meta::port_group_t *pg = meta->port_groups; pg->id != NULL; ++pg)
            {
                // Validate port group identifier
                if (!validate_identifier(pg->id))
                    validation_error(ctx, "Plugin uid='%s' has invalid port group identifier id='%s', allowed characters are: a-z, A-Z, _, 0-9",
                        meta->uid, pg->id);

                // Check that main input and output groups are unique
                if (meta::is_main_group(pg))
                {
                    if (meta::is_in_group(pg))
                    {
                        if (main_in == NULL)
                            main_in = pg;
                        else
                            validation_error(ctx, "Plugin uid='%s' main input port group id='%s' clashes with main input port group id='%s': only one main input group is allowed",
                                meta->uid, pg->id, main_in->id);
                    }
                    else
                    {
                        if (main_out == NULL)
                            main_out = pg;
                        else
                            validation_error(ctx, "Plugin uid='%s' main output port group id='%s' clashes with main output port group id='%s': only one main output group is allowed",
                                meta->uid, pg->id, main_in->id);
                    }

                    // Check that flags are valid
                    if (meta::is_sidechain_group(pg))
                        validation_error(ctx, "Plugin uid='%s' main %s port group id='%s' is also marked as sidechain group, sidechain groups can not be main groups",
                            meta->uid, (meta::is_in_group(pg)) ? "input" : "output", pg->id);
                }

                // Check that we did not process group with such name
                if (!visited_groups.create(const_cast<char *>(pg->id)))
                {
                    if (visited_groups.contains(pg->id))
                        validation_error(ctx, "Plugin uid='%s' has duplicate port group id='%s'",
                            meta->uid, pg->id);
                    else
                        allocation_error(ctx);
                    continue;
                }

                // Check that provided parent group identifier is valid
                if (pg->parent_id != NULL)
                {
                    const meta::port_group_t *parent = find_port_group(meta, pg->parent_id);
                    if (parent == NULL)
                        validation_error(ctx, "Could not find parent group id='%s' for group id='%s' of plugin uid='%s'",
                            pg->id, pg->parent_id, meta->uid);
                }

                // Validate items
                uint32_t role_mask = 0;

                for (const meta::port_group_item_t *item = pg->items; item->id != NULL; ++item)
                {
                    const meta::port_t *p = find_port(meta, item->id);
                    if (p == NULL)
                    {
                        validation_error(ctx, "Port '%s' is listed in group '%s' but does not exist for plugin uid='%s'",
                            item->id, pg->id, meta->uid);
                        continue;
                    }

                    // Check that port type and direction matches the port group
                    if (meta::is_out_group(pg))
                    {
                        if (!meta::is_audio_out_port(p))
                        {
                            validation_error(ctx, "The port '%s' of output group '%s' should also be an R_AUDIO_OUT port for plugin uid='%s'",
                                item->id, pg->id, meta->uid);
                        }
                    }
                    else
                    {
                        if (!meta::is_audio_in_port(p))
                            validation_error(ctx, "The port '%s' of input group '%s' should also be an R_AUDIO_IN port for plugin uid='%s'",
                                item->id, pg->id, meta->uid);
                    }

                    // Check for ambiguity of port roles within the group
                    if (role_mask & (1 << item->role))
                    {
                        validation_error(ctx, "The port '%s' of group '%s' has ambiguous role. That means, that there is already port with such role present in the group for plugin uid='%s'",
                            item->id, pg->id, meta->uid);
                    }
                    else
                        role_mask  |= (1 << item->role);

                    // Ensure that each port has unique group assignment
                    char *port_group = visited_ports.get(item->id);
                    if (port_group != NULL)
                    {
                        validation_error(
                            ctx,
                            "The port '%s' is defined as a part of group '%s' but also belongs to group '%s' for plugin uid='%s'. Ports should have unique group assignments.",
                            item->id, pg->id, port_group, meta->uid);
                    }
                    else if (!visited_ports.create(item->id, const_cast<char *>(pg->id)))
                        validation_error(
                            ctx,
                            "Could not remember group '%s' for port '%s' of plugin uid='%s'.",
                            pg->id, item->id, meta->uid);
                }
            }
        }

    #ifdef WITH_UI_FEATURE
        const meta::plugin_t *find_ui(const meta::plugin_t *meta)
        {
            for (ui::Factory *f = ui::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    const meta::plugin_t *ui = f->enumerate(i);
                    if (ui == NULL)
                        break;

                    if (strcmp(meta->uid, ui->uid) == 0)
                        return ui;
                }
            }

            return NULL;
        }

        void validate_ui(context_t *ctx, const meta::plugin_t *meta)
        {
            const meta::plugin_t *ui = find_ui(meta);
            if (ui == NULL)
            {
                if (meta->ui_resource != NULL)
                    validation_error(ctx, "Plugin uid='%s' has no UI but provides ui_resource='%s'",
                        meta->uid, meta->ui_resource);
                if (meta->ui_resource != NULL)
                    validation_error(ctx, "Plugin uid='%s' has no UI but provides ui_presets='%s'",
                        meta->uid, meta->ui_presets);
            }
            else
            {
                if (meta->ui_resource == NULL)
                    validation_error(ctx, "Plugin uid='%s' has UI but does not provide ui_resource", meta->uid);
            }
        }
    #else
        void validate_ui(context_t *ctx, const meta::plugin_t *meta)
        {
        }
    #endif /* WITH_UI_FEATURE */

        void validate_package(context_t *ctx, const meta::package_t *pkg)
        {
            // Validate parameters
            if (pkg->brand == NULL)
                validation_error(ctx, "Manifest does not provide brand name");

            // Call the nested validators for each plugin format
            ladspa::validate_package(ctx, pkg);
            lv2::validate_package(ctx, pkg);
            vst2::validate_package(ctx, pkg);
            vst3::validate_package(ctx, pkg);
            jack::validate_package(ctx, pkg);
            clap::validate_package(ctx, pkg);
            gst::validate_package(ctx, pkg);
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
            ctx->clap_port_ids.flush();
            ctx->vst3_port_ids.flush();
            ctx->port_short_names.flush();

            // Validate name
            if (meta->name == NULL)
                validation_error(ctx, "Not specified plugin name for plugin uid='%s'", meta->uid);

            // Validate description
            if (meta->description == NULL)
                validation_error(ctx, "Not specified plugin description for plugin uid='%s'", meta->uid);

            // Validate acronym
            if (meta->acronym != NULL)
            {
                if ((clash = ctx->lsp_acronyms.get(meta->acronym)) != NULL)
                    validation_error(ctx, "Model acronym '%s' for plugin uid='%s' clashes the acronym for plugin uid='%s'",
                        meta->acronym, meta->uid, clash->uid);
                else if (!ctx->lsp_acronyms.create(meta->acronym, const_cast<meta::plugin_t *>(meta)))
                    allocation_error(ctx);
            }
            else
                validation_error(ctx, "Not specified plugin acronym for plugin uid='%s'", meta->uid);

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

            // Validate port groups
            validate_port_groups(ctx, meta);

            // Validate ports
            validate_ports(ctx, meta);
            validate_send_return(ctx, meta);

            // Validate the UI
            validate_ui(ctx, meta);

            // Call the nested validators for each plugin format
            ladspa::validate_plugin(ctx, meta);
            lv2::validate_plugin(ctx, meta);
            vst2::validate_plugin(ctx, meta);
            vst3::validate_plugin(ctx, meta);
            jack::validate_plugin(ctx, meta);
            clap::validate_plugin(ctx, meta);
            gst::validate_plugin(ctx, meta);
        }

    } /* namespace validator */
} /* namespace lsp */



