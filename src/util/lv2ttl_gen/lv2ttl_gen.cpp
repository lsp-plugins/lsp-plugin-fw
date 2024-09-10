/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 дек. 2021 г.
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

#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/util/lv2ttl_gen/lv2ttl_gen.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/static.h>

namespace lsp
{
    namespace lv2ttl_gen
    {
        //-----------------------------------------------------------------------------
        static const plugin_group_t plugin_groups[] =
        {
            { meta::C_DELAY,        "DelayPlugin"       },
            { meta::C_REVERB,       "ReverbPlugin"      },
            { meta::C_DISTORTION,   "DistortionPlugin"  },
            { meta::C_WAVESHAPER,   "WaveshaperPlugin"  },
            { meta::C_DYNAMICS,     "DynamicsPlugin"    },
            { meta::C_AMPLIFIER,    "AmplifierPlugin"   },
            { meta::C_COMPRESSOR,   "CompressorPlugin"  },
            { meta::C_ENVELOPE,     "EnvelopePlugin"    },
            { meta::C_EXPANDER,     "ExpanderPlugin"    },
            { meta::C_GATE,         "GatePlugin"        },
            { meta::C_LIMITER,      "LimiterPlugin"     },
            { meta::C_FILTER,       "FilterPlugin"      },
            { meta::C_ALLPASS,      "AllpassPlugin"     },
            { meta::C_BANDPASS,     "BandpassPlugin"    },
            { meta::C_COMB,         "CombPlugin"        },
            { meta::C_EQ,           "EQPlugin"          },
            { meta::C_MULTI_EQ,     "MultiEQPlugin"     },
            { meta::C_PARA_EQ,      "ParaEQPlugin"      },
            { meta::C_HIGHPASS,     "HighpassPlugin"    },
            { meta::C_LOWPASS,      "LowpassPlugin"     },
            { meta::C_GENERATOR,    "GeneratorPlugin"   },
            { meta::C_CONSTANT,     "ConstantPlugin"    },
            { meta::C_INSTRUMENT,   "InstrumentPlugin"  },
            { meta::C_DRUM,         "InstrumentPlugin"  },
            { meta::C_EXTERNAL,     "InstrumentPlugin"  },
            { meta::C_PIANO,        "InstrumentPlugin"  },
            { meta::C_SAMPLER,      "InstrumentPlugin"  },
            { meta::C_SYNTH,        "InstrumentPlugin"  },
            { meta::C_SYNTH_SAMPLER,"InstrumentPlugin"  },
            { meta::C_OSCILLATOR,   "OscillatorPlugin"  },
            { meta::C_MODULATOR,    "ModulatorPlugin"   },
            { meta::C_CHORUS,       "ChorusPlugin"      },
            { meta::C_FLANGER,      "FlangerPlugin"     },
            { meta::C_PHASER,       "PhaserPlugin"      },
            { meta::C_SIMULATOR,    "SimulatorPlugin"   },
            { meta::C_SPATIAL,      "SpatialPlugin"     },
            { meta::C_SPECTRAL,     "SpectralPlugin"    },
            { meta::C_PITCH,        "PitchPlugin"       },
            { meta::C_UTILITY,      "UtilityPlugin"     },
            { meta::C_ANALYSER,     "AnalyserPlugin"    },
            { meta::C_CONVERTER,    "ConverterPlugin"   },
            { meta::C_FUNCTION,     "FunctionPlugin"    },
            { meta::C_MIXER,        "MixerPlugin"       },
            { -1, NULL }
        };

        static const plugin_unit_t plugin_units[] =
        {
            // Predefined LV2 units
            { meta::U_PERCENT,      "pc",               NULL,                   NULL        },
            { meta::U_MM,           "mm",               NULL,                   NULL        },
            { meta::U_CM,           "cm",               NULL,                   NULL        },
            { meta::U_M,            "m",                NULL,                   NULL        },
            { meta::U_INCH,         "inch",             NULL,                   NULL        },
            { meta::U_KM,           "km",               NULL,                   NULL        },
            { meta::U_HZ,           "hz",               NULL,                   NULL        },
            { meta::U_KHZ,          "khz",              NULL,                   NULL        },
            { meta::U_MHZ,          "mhz",              NULL,                   NULL        },
            { meta::U_BPM,          "bpm",              NULL,                   NULL        },
            { meta::U_CENT,         "cent",             NULL,                   NULL        },
            { meta::U_BAR,          "bar",              NULL,                   NULL        },
            { meta::U_BEAT,         "beat",             NULL,                   NULL        },
            { meta::U_SEC,          "s",                NULL,                   NULL        },
            { meta::U_MSEC,         "ms",               NULL,                   NULL        },
            { meta::U_DB,           "db",               NULL,                   NULL        },
            { meta::U_MIN,          "min",              NULL,                   NULL        },
            { meta::U_DEG,          "degree",           NULL,                   NULL        },
            { meta::U_OCTAVES,      "oct",              NULL,                   NULL        },
            { meta::U_SEMITONES,    "semitone12TET",    NULL,                   NULL        },

            // Custom LV2 units
            { meta::U_SAMPLES,      NULL,               "samples",              "%.0f"      },
            { meta::U_GAIN_AMP,     NULL,               "gain",                 "%.8f"      },
            { meta::U_GAIN_POW,     NULL,               "gain",                 "%.8f"      },

            { meta::U_DEG_CEL,      NULL,               "degrees Celsium",      "%.2f"      },
            { meta::U_DEG_FAR,      NULL,               "degrees Fahrenheit",   "%.2f"      },
            { meta::U_DEG_K,        NULL,               "degrees Kelvin",       "%.2f"      },
            { meta::U_DEG_R,        NULL,               "degrees Rankine",      "%.2f"      },

            { meta::U_NEPER,        NULL,               "Neper",                "%.2f"      },
            { meta::U_LUFS,         NULL,               "LUFS",                 "%.2f"      },

            { meta::U_BYTES,        NULL,               "Bytes",                "%.0f"      },
            { meta::U_KBYTES,       NULL,               "Kilobytes",            "%.2f"      },
            { meta::U_MBYTES,       NULL,               "Megabytes",            "%.2f"      },
            { meta::U_GBYTES,       NULL,               "Gigabytes",            "%.2f"      },
            { meta::U_TBYTES,       NULL,               "Terabytes",            "%.2f"      },

            { -1, NULL, NULL, NULL }
        };

        //-----------------------------------------------------------------------------
        // Port groups and port group elements
        static const int8_t pg_mono_elements[]      = { meta::PGR_CENTER, -1 };
        static const int8_t pg_stereo_elements[]    = { meta::PGR_LEFT, meta::PGR_RIGHT, -1 };
        static const int8_t pg_ms_elements[]        = { meta::PGR_MS_MIDDLE, meta::PGR_MS_SIDE, -1 };
        static const int8_t pg_3_0_elements[]       = { meta::PGR_LEFT, meta::PGR_RIGHT, meta::PGR_REAR_CENTER, -1 };
        static const int8_t pg_4_0_elements[]       = { meta::PGR_LEFT, meta::PGR_CENTER, meta::PGR_RIGHT, meta::PGR_REAR_CENTER, -1 };
        static const int8_t pg_5_0_elements[]       = { meta::PGR_LEFT, meta::PGR_CENTER, meta::PGR_RIGHT, meta::PGR_REAR_LEFT, meta::PGR_REAR_RIGHT, -1 };
        static const int8_t pg_5_1_elements[]       = { meta::PGR_LEFT, meta::PGR_CENTER, meta::PGR_RIGHT, meta::PGR_REAR_LEFT, meta::PGR_REAR_RIGHT, meta::PGR_LO_FREQ, -1 };
        static const int8_t pg_6_1_elements[]       = { meta::PGR_LEFT, meta::PGR_CENTER, meta::PGR_RIGHT, meta::PGR_SIDE_LEFT, meta::PGR_SIDE_RIGHT, meta::PGR_REAR_CENTER, meta::PGR_LO_FREQ, -1 };
        static const int8_t pg_7_1_elements[]       = { meta::PGR_LEFT, meta::PGR_CENTER, meta::PGR_RIGHT, meta::PGR_SIDE_LEFT, meta::PGR_SIDE_RIGHT, meta::PGR_REAR_LEFT, meta::PGR_REAR_RIGHT, meta::PGR_LO_FREQ, -1 };
        static const int8_t pg_7_1w_elements[]      = { meta::PGR_LEFT, meta::PGR_CENTER_LEFT, meta::PGR_CENTER, meta::PGR_CENTER_RIGHT, meta::PGR_RIGHT, meta::PGR_REAR_LEFT, meta::PGR_REAR_RIGHT, meta::PGR_LO_FREQ, -1 };

        static const int8_t * const pg_elements[] =
        {
            pg_mono_elements,       // GRP_MONO
            pg_stereo_elements,     // GRP_STEREO
            pg_ms_elements,         // GRP_MS
            pg_3_0_elements,        // GRP_3_0
            pg_4_0_elements,        // GRP_4_0
            pg_5_0_elements,        // GRP_5_0
            pg_5_1_elements,        // GRP_5_1
            pg_6_1_elements,        // GRP_6_1
            pg_7_1_elements,        // GRP_7_1
            pg_7_1w_elements,       // GRP_7_1W
        };

        //-----------------------------------------------------------------------------
        static const char *get_filename(const char *path)
        {
            if (path == NULL)
                return path;

            // We accept both '/' and '\' file separators
            const char *space = strrchr(path, '/');
            if (space != NULL)
                return &space[1];

            space = strrchr(path, '\\');
            return (space != NULL) ? &space[1] : path;
        }

        status_t parse_cmdline(cmdline_t *cfg, int argc, const char **argv)
        {
            // Initialize config with default values
            cfg->out_dir        = NULL;
            cfg->lv2_binary     = NULL;
            cfg->lv2ui_binary   = NULL;

            // Parse arguments
            int i = 1;

            while (i < argc)
            {
                const char *arg = argv[i++];
                if ((!::strcmp(arg, "--help")) || (!::strcmp(arg, "-h")))
                {
                    printf("Usage: %s [parameters]\n\n", argv[0]);
                    printf("Available parameters:\n");
                    printf("  -i, --input <file>        The name of the LV2 binary\n");
                    printf("  -o, --output <dir>        The name of the output directory for TTL files\n");
                    printf("  -ui, --ui-input <file>    The name of the LV2 UI binary\n");
                    printf("\n");

                    return STATUS_CANCELLED;
                }
                else if ((!::strcmp(arg, "--output")) || (!::strcmp(arg, "-o")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified directory name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->out_dir)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->out_dir = argv[i++];
                }
                else if ((!::strcmp(arg, "--input")) || (!::strcmp(arg, "-i")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified file name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->lv2_binary)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->lv2_binary = argv[i++];
                }
                else if ((!::strcmp(arg, "--ui-input")) || (!::strcmp(arg, "-ui")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified file name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    else if (cfg->lv2ui_binary)
                    {
                        fprintf(stderr, "Duplicate parameter '%s'\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->lv2ui_binary = argv[i++];
                }
                else
                {
                    fprintf(stderr, "Unknown parameter: %s\n", arg);
                    return STATUS_BAD_ARGUMENTS;
                }
            }

            // Validate mandatory arguments
            if (cfg->out_dir == NULL)
            {
                fprintf(stderr, "Output directory name is required\n");
                return STATUS_BAD_ARGUMENTS;
            }
            cfg->lv2_binary = get_filename(cfg->lv2_binary);
            cfg->lv2ui_binary = get_filename(cfg->lv2ui_binary);

            return STATUS_OK;
        }

        static const char *group_designation(size_t role)
        {
            switch (role)
            {
                case meta::PGR_CENTER:          return "center";
                case meta::PGR_CENTER_LEFT:     return "centerLeft";
                case meta::PGR_CENTER_RIGHT:    return "centerRight";
                case meta::PGR_LEFT:            return "left";
                case meta::PGR_LO_FREQ:         return "lowFrequencyEffects";
                case meta::PGR_REAR_CENTER:     return "rearCenter";
                case meta::PGR_REAR_LEFT:       return "rearLeft";
                case meta::PGR_REAR_RIGHT:      return "rearRight";
                case meta::PGR_RIGHT:           return "right";
                case meta::PGR_MS_SIDE:         return "side";
                case meta::PGR_SIDE_LEFT:       return "sideLeft";
                case meta::PGR_SIDE_RIGHT:      return "sideRight";
                case meta::PGR_MS_MIDDLE:       return "center";
                default:
                    break;
            }

            return NULL;
        }

        static inline void emit_header(FILE *out, size_t &count, const char *text)
        {
            if (count == 0)
            {
                fputs(text, out);
                fputc(' ', out);
            }
        }

        static inline void emit_option(FILE *out, size_t &count, bool condition, const char *text)
        {
            if (condition)
            {
                if (count++)
                    fputs(", ", out);
                fputs(text, out);
            }
        }

        static inline void emit_end(FILE *out, size_t &count)
        {
            if (count > 0)
            {
                fprintf(out, " ;\n");
                count = 0;
            }
        }

        static void print_plugin_groups(FILE *out, const meta::plugin_t &m)
        {
            size_t count = 0, emitted = 0;

            lltl::phashset<char> visited;

            emit_header(out, count, "\ta");
            for (const int *c = m.classes; ((c != NULL) && ((*c) >= 0)); ++c)
            {
                // Lookup for predefined plugin group in list
                for (const plugin_group_t *grp = plugin_groups; (grp != NULL) && (grp->id >= 0); ++grp)
                {
                    if (grp->id == *c)
                    {
                        // Prevent from duplicated values
                        if (visited.create(const_cast<char *>(grp->name)))
                        {
                            if (count++)
                                fputs(", ", out);
                            fprintf(out, "lv2:%s", grp->name);
                            ++emitted;
                        }
                        break;
                    }
                }

                // LV2 supports only one plugin class at this moment
                if (emitted > 0)
                    break;
            }

            emit_option(out, count, count <= 0, "lv2:Plugin");
            emit_option(out, count, true, "doap:Project");
            emit_end(out, count);
        }

        static const char *get_module_uid(const char *uri)
        {
            const char *str = strrchr(uri, '/');
            return (str) ? &str[1] : uri;
        }

        static const char *get_module_prefix(char *buf, size_t len, const char *uri)
        {
            const char *str = strrchr(uri, '/');
            str = (str != NULL) ? &str[1] : uri;
            ssize_t src_len = lsp_min(str - uri, ssize_t(len-1));

            memcpy(buf, uri, src_len);
            buf[src_len] = '\0';
            return buf;
        }

        static void print_units(FILE *out, int unit)
        {
            const plugin_unit_t *u  = plugin_units;

            while ((u != NULL) && (u->id >= 0))
            {
                if (u->id == unit)
                {
                    // Check that lv2 contains name
                    if (u->name != NULL)
                        fprintf(out, "\t\tunits:unit units:%s ;\n", u->name);
                    else
                    {
                        const char *symbol = meta::get_unit_name(unit);

                        // Build custom type
                        if (symbol != NULL)
                        {
                            fprintf(out, "\t\tunits:unit [\n");
                            fprintf(out, "\t\t\ta units:Unit ;\n");
                            fprintf(out, "\t\t\trdfs:label \"%s\" ;\n", u->label);
                            fprintf(out, "\t\t\tunits:symbol \"%s\" ;\n", symbol);
                            fprintf(out, "\t\t\tunits:render \"%s %s\" ;\n", u->render, symbol);
                            fprintf(out, "\t\t] ;\n");
                        }
                    }

                    return;
                }
                u++;
            }
        }

        void gen_plugin_ui_ttl(
            FILE *out,
            size_t requirements,
            const meta::plugin_t &m,
            const meta::package_t *manifest,
            const cmdline_t *cmd)
        {
            fprintf(out, "%s:%s\n", LV2TTL_PLUGIN_UI_PREFIX, get_module_uid(m.uids.lv2ui));
            fprintf(out, "\ta ui:" LV2TTL_UI_CLASS " ;\n");
            fprintf(out, "\tlv2:minorVersion %d ;\n", int(LSP_MODULE_VERSION_MINOR(m.version)));
            fprintf(out, "\tlv2:microVersion %d ;\n", int(LSP_MODULE_VERSION_MICRO(m.version)));
            fprintf(out, "\tlv2:requiredFeature urid:map ;\n");
            {
                size_t count = 1;
                fprintf(out, "\tlv2:optionalFeature ui:parent, ui:resize, ui:idleInterface");
                emit_option(out, count, requirements & REQ_INSTANCE, "lv2ext:instance-access");
                fprintf(out, " ;\n");
            }
            fprintf(out, "\tlv2:extensionData ui:idleInterface, ui:resize ;\n");
            fprintf(out, "\tui:binary <%s> ;\n", cmd->lv2ui_binary);
            fprintf(out, "\n");

            size_t ports        = 0;
            size_t port_id      = 0;

            for (const meta::port_t *p = m.ports; (p->id != NULL); ++p)
            {
                // Skip virtual ports
                switch (p->role)
                {
                    case meta::R_MESH:
                    case meta::R_STREAM:
                    case meta::R_FBUFFER:
                    case meta::R_PATH:
                    case meta::R_STRING:
                    case meta::R_SEND_NAME:
                    case meta::R_RETURN_NAME:
                    case meta::R_PORT_SET:
                    case meta::R_MIDI_IN:
                    case meta::R_MIDI_OUT:
                    case meta::R_AUDIO_SEND:
                    case meta::R_AUDIO_RETURN:
                    case meta::R_OSC_IN:
                    case meta::R_OSC_OUT:
                        continue;
                    case meta::R_AUDIO_IN:
                    case meta::R_AUDIO_OUT:
                        port_id++;
                        continue;
                    default:
                        break;
                }

                fprintf(out, "%s [\n", (ports == 0) ? "\tui:portNotification" : " ,");
                fprintf(out, "\t\tui:plugin %s:%s ;\n", LV2TTL_PLUGIN_PREFIX, get_module_uid(m.uids.lv2));
                fprintf(out, "\t\tui:portIndex %d ;\n", int(port_id));

                switch (p->role)
                {
                    case meta::R_METER:
                        if (p->flags & meta::F_PEAK)
                            fprintf(out, "\t\tui:protocol ui:peakProtocol ;\n");
                        else
                            fprintf(out, "\t\tui:protocol ui:floatProtocol ;\n");
                        break;
                    default:
                        fprintf(out, "\t\tui:protocol ui:floatProtocol ;\n");
                        break;
                }

                fprintf(out, "\t]");

                ports++;
                port_id++;
            }

            // Add atom ports for state serialization
            for (size_t i=0; i<2; ++i)
            {
                fprintf(out, "%s [\n", (ports == 0) ? "\tui:portNotification" : " ,");
                fprintf(out, "\t\tui:plugin %s:%s ;\n", LV2TTL_PLUGIN_PREFIX, get_module_uid(m.uids.lv2));
                fprintf(out, "\t\tui:portIndex %d ;\n", int(port_id));
                fprintf(out, "\t\tui:protocol atom:eventTransfer ;\n");
                fprintf(out, "\t\tui:notifyType atom:Sequence ;\n");
                fprintf(out, "\t]");

                ports++;
                port_id++;
            }

            // Add latency report port
            {
                const meta::port_t *p = &lv2::latency_port;
                if ((p->id != NULL) && (p->name != NULL))
                {
                    fprintf(out, "%s [\n", (ports == 0) ? "\tui:portNotification" : " ,");
                    fprintf(out, "\t\tui:plugin %s:%s ;\n", LV2TTL_PLUGIN_PREFIX, get_module_uid(m.uids.lv2));
                    fprintf(out, "\t\tui:portIndex %d ;\n", int(port_id));
                    fprintf(out, "\t\tui:protocol ui:floatProtocol ;\n");
                    fprintf(out, "\t]");

                    ports++;
                    port_id++;
                }
            }

            // Finish port list
            fprintf(out, "\n\t.\n\n");
        }

        static void print_port_groups(FILE *out, const meta::port_t *port, const meta::port_group_t *pg)
        {
            // For each group
            while ((pg != NULL) && (pg->id != NULL))
            {
                // Scan list of port
                for (const meta::port_group_item_t *p = pg->items; p->id != NULL; ++p)
                {
                    if (!strcmp(p->id, port->id))
                    {
                        fprintf(out, "\t\tpg:group %s:%s ;\n", LV2TTL_PORT_GROUP_PREFIX, pg->id);
                        const char *role = group_designation(p->role);
                        if (role != NULL)
                            fprintf(out, "\t\tlv2:designation pg:%s ;\n", role);
                        break;
                    }
                }

                pg++;
            }
        }

        static size_t scan_port_requirements(const meta::port_t *meta)
        {
            size_t result = REQ_TIME;
            for (const meta::port_t *p = meta; p->id != NULL; ++p)
            {
                switch (p->role)
                {
                    case meta::R_PATH:
                        result     |= REQ_PATH_MASK | REQ_INSTANCE;
                        break;
                    case meta::R_STRING:
                    case meta::R_SEND_NAME:
                    case meta::R_RETURN_NAME:
                        result     |= REQ_STRING_MASK | REQ_INSTANCE;
                        break;
                    case meta::R_MESH:
                    case meta::R_STREAM:
                    case meta::R_FBUFFER:
                        result     |= REQ_INSTANCE;
                        break;
                    case meta::R_MIDI_OUT:
                        result     |= REQ_MIDI_OUT;
                        break;
                    case meta::R_MIDI_IN:
                        result     |= REQ_MIDI_IN;
                        break;
                    case meta::R_OSC_OUT:
                        result     |= REQ_OSC_OUT;
                        break;
                    case meta::R_OSC_IN:
                        result     |= REQ_OSC_IN;
                        break;
                    case meta::R_PORT_SET:
                        if ((p->members != NULL) && (p->items != NULL))
                            result         |= scan_port_requirements(p->members);
                        result     |= REQ_INSTANCE;
                        break;
                    default:
                        break;
                }
            }
            return result;
        }

        static size_t scan_requirements(const meta::plugin_t &m)
        {
            size_t result   = 0;

            if (m.uids.lv2ui != NULL)
            {
                if (m.ui_resource != NULL)
                    result |= REQ_LV2UI;
                if (m.extensions & meta::E_INLINE_DISPLAY)
                    result |= REQ_IDISPLAY;
            }

            result |= scan_port_requirements(m.ports);

            if ((m.port_groups != NULL) && (m.port_groups->id != NULL))
                result |= REQ_PORT_GROUPS;

            return result;
        }

        static void emit_prefix(FILE *out, const char *prefix, const char *value)
        {
            ssize_t len = fprintf(out, "@prefix %s: ", prefix);
            while ((len++) < 20)
                fputc(' ', out);
            fprintf(out, "<%s> .\n", value);
        }

        void gen_plugin_ttl(
            FILE *out,
            size_t requirements,
            const meta::plugin_t &m,
            const meta::package_t *manifest,
            const cmdline_t *cmd)
        {
            // Output port groups
            const meta::person_t *dev = m.developer;
            const meta::port_group_t *pg_main_in = NULL, *pg_main_out = NULL;
            lltl::phashset<char> sidechain_ports;

            if (requirements & REQ_PORT_GROUPS)
            {
                for (const meta::port_group_t *pg = m.port_groups; (pg != NULL) && (pg->id != NULL); pg++)
                {
                    const char *grp_type = NULL, *grp_dir = (pg->flags & meta::PGF_OUT) ? "OutputGroup" : "InputGroup";
                    switch (pg->type)
                    {
                        case meta::GRP_1_0:     grp_type = "MonoGroup";                 break;
                        case meta::GRP_2_0:     grp_type = "StereoGroup";               break;
                        case meta::GRP_MS:      grp_type = "MidSideGroup";              break;
                        case meta::GRP_3_0:     grp_type = "ThreePointZeroGroup";       break;
                        case meta::GRP_4_0:     grp_type = "FourPointZeroGroup";        break;
                        case meta::GRP_5_0:     grp_type = "FivePointZeroGroup";        break;
                        case meta::GRP_5_1:     grp_type = "FivePointOneGroup";         break;
                        case meta::GRP_6_1:     grp_type = "SixPointOneGroup";          break;
                        case meta::GRP_7_1:     grp_type = "SevenPointOneGroup";        break;
                        case meta::GRP_7_1W:    grp_type = "SevenPointOneWideGroup";    break;
                        default:
                            break;
                    }

                    fprintf(out, "%s:%s\n", LV2TTL_PORT_GROUP_PREFIX, pg->id);
                    if (grp_type != NULL)
                        fprintf(out, "\ta pg:%s, pg:%s ;\n", grp_type, grp_dir);
                    else
                        fprintf(out, "\ta pg:%s ;\n", grp_dir);

                    if (pg->flags & meta::PGF_SIDECHAIN)
                    {
                        fprintf(out, "\tpg:sideChainOf %s:%s ;\n", LV2TTL_PORT_GROUP_PREFIX, pg->parent_id);

                        for (const meta::port_group_item_t *pi = pg->items; (pi != NULL) && (pi->id != NULL); ++pi)
                            sidechain_ports.create(const_cast<char *>(pi->id));
                    }

                    if (pg->flags & meta::PGF_MAIN)
                    {
                        if (pg->flags & meta::PGF_OUT)
                            pg_main_out     = pg;
                        else
                            pg_main_in      = pg;
                    }

                    fprintf(out, "\tlv2:symbol \"%s\" ;\n", pg->id);
                    fprintf(out, "\trdfs:label \"%s\" ;\n", pg->name);

                    // Output group elements
                    const int8_t * element = pg_elements[pg->type];
                    fprintf(out, "\tpg:element ");
                    for (int i=0; element[i] >= 0; ++i)
                    {
                        if (i > 0)
                            fprintf(out, " , ");
                        fprintf(out, "[\n");
                        fprintf(out, "\t\tlv2:index %d ;\n", i);
                        fprintf(out, "\t\tlv2:designation pg:%s\n", group_designation(element[i]));
                        fprintf(out, "\t]");
                    }
                    fprintf(out, "\n");

                    fprintf(out, "\t.\n\n");
                }
            }

            // Output special parameters
            for (const meta::port_t *p = m.ports; p->id != NULL; ++p)
            {
                switch (p->role)
                {
                    case meta::R_PATH:
                    {
                        if (requirements & REQ_PATCH)
                        {
                            fprintf(out, "%s:%s\n", LV2TTL_PORTS_PREFIX, p->id);
                            fprintf(out, "\ta lv2:Parameter ;\n");
                            fprintf(out, "\trdfs:label \"%s\" ;\n", p->name);
                            fprintf(out, "\trdfs:range atom:Path\n");
                            fprintf(out, "\t.\n\n");
                        }
                        break;
                    }
                    case meta::R_STRING:
                    case meta::R_SEND_NAME:
                    case meta::R_RETURN_NAME:
                    {
                        if (requirements & REQ_PATCH)
                        {
                            fprintf(out, "%s:%s\n", LV2TTL_PORTS_PREFIX, p->id);
                            fprintf(out, "\ta lv2:Parameter ;\n");
                            fprintf(out, "\trdfs:label \"%s\" ;\n", p->name);
                            fprintf(out, "\trdfs:range atom:String\n");
                            fprintf(out, "\t.\n\n");
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            // Output plugin
            fprintf(out, "%s:%s\n", LV2TTL_PLUGIN_PREFIX, get_module_uid(m.uids.lv2));
            print_plugin_groups(out, m);
            fprintf(out, "\tdoap:name \"%s %s\" ;\n", manifest->brand, m.description);
            fprintf(out, "\tlv2:minorVersion %d ;\n", int(LSP_MODULE_VERSION_MINOR(m.version)));
            fprintf(out, "\tlv2:microVersion %d ;\n", int(LSP_MODULE_VERSION_MICRO(m.version)));
            if ((dev != NULL) && (dev->uid != NULL))
                fprintf(out, "\tdoap:developer %s:%s ;\n", LV2TTL_DEVELOPERS_PREFIX, m.developer->uid);
            if (manifest->brand_id != NULL)
                fprintf(out, "\tdoap:maintainer %s:%s ;\n", LV2TTL_DEVELOPERS_PREFIX, manifest->brand_id);
            if (manifest->lv2_license != NULL)
                fprintf(out, "\tdoap:license <%s> ;\n", manifest->lv2_license);
            fprintf(out, "\tlv2:binary <%s> ;\n", cmd->lv2_binary);
            if ((requirements & REQ_LV2UI) && (cmd->lv2ui_binary))
                fprintf(out, "\tui:ui %s:%s ;\n", LV2TTL_PLUGIN_UI_PREFIX, get_module_uid(m.uids.lv2ui));

            // Emit required features
            fprintf(out, "\tlv2:requiredFeature urid:map ;\n");

            // Emit optional features
            fprintf(out, "\tlv2:optionalFeature lv2:hardRTCapable, hcid:queue_draw, work:schedule, opts:options, state:mapPath ;\n");

            // Emit extension data
            fprintf(out, "\tlv2:extensionData state:interface, work:interface, hcid:interface ;\n");

            // Different supported options
            if (requirements & REQ_LV2UI)
                fprintf(out, "\topts:supportedOption ui:updateRate ;\n");

            if (pg_main_in != NULL)
                fprintf(out, "\tpg:mainInput %s:%s ;\n", LV2TTL_PORT_GROUP_PREFIX, pg_main_in->id);
            if (pg_main_out != NULL)
                fprintf(out, "\tpg:mainOutput %s:%s ;\n", LV2TTL_PORT_GROUP_PREFIX, pg_main_out->id);

            // Replacement for LADSPA plugin
            if (m.uids.ladspa_id > 0)
                fprintf(out, "\tdc:replaces <urn:ladspa:%ld> ;\n", long(m.uids.ladspa_id));

            // Separator
            fprintf(out, "\n");

            size_t port_id = 0;

            // Output special parameters
            if (requirements & REQ_PATCH_WR)
            {
                size_t count = 0;
                const meta::port_t *first = NULL;
                for (const meta::port_t *p = m.ports; p->id != NULL; ++p)
                {
                    switch (p->role)
                    {
                        case meta::R_PATH:
                        case meta::R_STRING:
                        case meta::R_SEND_NAME:
                        case meta::R_RETURN_NAME:
                            count++;
                            if (first == NULL)
                                first = p;
                            break;
                        default:
                            break;
                    }
                }

                if (first != NULL)
                {
                    fprintf(out, "\tpatch:writable");
                    if (count > 1)
                    {
                        fprintf(out, "\n");
                        for (const meta::port_t *p = m.ports; p->id != NULL; ++p)
                        {
                            switch (p->role)
                            {
                                case meta::R_PATH:
                                case meta::R_STRING:
                                case meta::R_SEND_NAME:
                                case meta::R_RETURN_NAME:
                                {
                                    fprintf(out, "\t\t%s:%s", LV2TTL_PORTS_PREFIX, p->id);
                                    if (--count)
                                        fprintf(out, " ,\n");
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                        fprintf(out, " ;\n\n");
                    }
                    else
                        fprintf(out, " %s:%s ;\n\n", LV2TTL_PORTS_PREFIX, first->id);
                }
            }

            for (const meta::port_t *p = m.ports; p->id != NULL; ++p)
            {
                // Skip virtual ports
                switch (p->role)
                {
                    case meta::R_MESH:
                    case meta::R_STREAM:
                    case meta::R_FBUFFER:
                    case meta::R_PATH:
                    case meta::R_STRING:
                    case meta::R_SEND_NAME:
                    case meta::R_RETURN_NAME:
                    case meta::R_AUDIO_SEND:
                    case meta::R_AUDIO_RETURN:
                    case meta::R_PORT_SET:
                    case meta::R_MIDI_IN:
                    case meta::R_MIDI_OUT:
                    case meta::R_OSC_IN:
                    case meta::R_OSC_OUT:
                        continue;
                    default:
                        break;
                }

                fprintf(out, "%s [\n", (port_id == 0) ? "\tlv2:port" : " ,");
                fprintf(out, "\t\ta lv2:%s, ", meta::is_out_port(p) ? "OutputPort" : "InputPort");

                switch (p->role)
                {
                    case meta::R_AUDIO_IN:
                    case meta::R_AUDIO_OUT:
                        fprintf(out, "lv2:AudioPort ;\n");
                        break;
                    case meta::R_CONTROL:
                    case meta::R_BYPASS:
                    case meta::R_METER:
                        fprintf(out, "lv2:ControlPort ;\n");
                        break;
                    default:
                        break;
                }

                fprintf(out, "\t\tlv2:index %d ;\n", (int)port_id);
                fprintf(out, "\t\tlv2:symbol \"%s\" ;\n", (p->role == meta::R_BYPASS) ? "enabled" : p->id);
                fprintf(out, "\t\tlv2:name \"%s\" ;\n", (p->role == meta::R_BYPASS) ? "Enabled" : p->name);
                if (p->role == meta::R_BYPASS)
                    fprintf(out, "\t\tlv2:designation lv2:enabled ;\n");

                print_units(out, p->unit);

                size_t p_prop = 0;

                if ((meta::is_audio_in_port(p)) && (sidechain_ports.contains(p->id)))
                {
                    emit_header(out, p_prop, "\t\tlv2:portProperty");
                    emit_option(out, p_prop, true, "lv2:connectionOptional");
                    emit_option(out, p_prop, true, "lv2:isSideChain");
                }

                if (p->flags & meta::F_LOG)
                {
                    emit_header(out, p_prop, "\t\tlv2:portProperty");
                    emit_option(out, p_prop, true, "pp:logarithmic");
                }

                if (p->unit == meta::U_BOOL)
                {
                    emit_header(out, p_prop, "\t\tlv2:portProperty");
                    emit_option(out, p_prop, true, "lv2:toggled");
                    emit_end(out, p_prop);

                    fprintf(out, "\t\tlv2:minimum %d ;\n", 0);
                    fprintf(out, "\t\tlv2:maximum %d ;\n", 1);
                    fprintf(out, "\t\tlv2:default %d ;\n", (p->role == meta::R_BYPASS) ? (1 - int(p->start)) : int(p->start));
                }
                else if (p->unit == meta::U_ENUM)
                {
                    emit_header(out, p_prop, "\t\tlv2:portProperty");
                    emit_option(out, p_prop, true, "lv2:integer");
                    emit_option(out, p_prop, true, "lv2:enumeration");
                    emit_option(out, p_prop, true, "pp:hasStrictBounds");
                    emit_end(out, p_prop);

                    int min  = (p->flags & meta::F_LOWER) ? p->min : 0;
                    int curr = min;
                    size_t count = list_size(p->items);
                    int max  = min + list_size(p->items) - 1;

                    const meta::port_item_t *list = p->items;
                    if (count > 1)
                    {
                        fprintf(out, "\t\tlv2:scalePoint\n");
                        for ( ; list->text != NULL; ++list)
                        {
                            fprintf(out, "\t\t\t[ rdfs:label \"%s\"; rdf:value %d ]", list->text, curr);
                            if (--count)
                                fprintf(out, " ,\n");
                            else
                                fprintf(out, " ;\n");
                            curr ++;
                        }
                    } else if (count > 0)
                        fprintf(out, "\t\tlv2:scalePoint [ rdfs:label \"%s\"; rdf:value %d ] ;\n", list->text, curr);

                    fprintf(out, "\t\tlv2:minimum %d ;\n", min);
                    fprintf(out, "\t\tlv2:maximum %d ;\n", max);
                    fprintf(out, "\t\tlv2:default %d ;\n", int(p->start));
                }
                else if (p->unit == meta::U_SAMPLES)
                {
                    emit_header(out, p_prop, "\t\tlv2:portProperty");
                    emit_option(out, p_prop, true, "lv2:integer");
                    if ((p->flags & (meta::F_LOWER | meta::F_UPPER)) == (meta::F_LOWER | meta::F_UPPER))
                        emit_option(out, p_prop, true, "pp:hasStrictBounds");
                    emit_end(out, p_prop);

                    if (p->flags & meta::F_LOWER)
                        fprintf(out, "\t\tlv2:minimum %d ;\n", int(p->min));
                    if (p->flags & meta::F_UPPER)
                        fprintf(out, "\t\tlv2:maximum %d ;\n", int(p->max));
                    fprintf(out, "\t\tlv2:default %d ;\n", int(p->start));
                }
                else if (p->flags & meta::F_INT)
                {
                    emit_header(out, p_prop, "\t\tlv2:portProperty");
                    emit_option(out, p_prop, true, "lv2:integer");
                    if ((p->flags & (meta::F_LOWER | meta::F_UPPER)) == (meta::F_LOWER | meta::F_UPPER))
                        emit_option(out, p_prop, true, "pp:hasStrictBounds");
                    emit_end(out, p_prop);

                    if (p->flags & meta::F_LOWER)
                        fprintf(out, "\t\tlv2:minimum %d ;\n", int(p->min));
                    if (p->flags & meta::F_UPPER)
                        fprintf(out, "\t\tlv2:maximum %d ;\n", int(p->max));
                    if ((p->role == meta::R_CONTROL) || (p->role == meta::R_METER))
                        fprintf(out, "\t\tlv2:default %d ;\n", int(p->start));
                }
                else
                {
                    if ((p->flags & (meta::F_LOWER | meta::F_UPPER)) == (meta::F_LOWER | meta::F_UPPER))
                    {
                        emit_header(out, p_prop, "\t\tlv2:portProperty");
                        emit_option(out, p_prop, true, "pp:hasStrictBounds");
                    }
                    emit_end(out, p_prop);

                    if (p->flags & meta::F_LOWER)
                        fprintf(out, "\t\tlv2:minimum %.6f ;\n", p->min);
                    if (p->flags & meta::F_UPPER)
                        fprintf(out, "\t\tlv2:maximum %.6f ;\n", p->max);
                    if ((p->role == meta::R_CONTROL) || (p->role == meta::R_METER))
                        fprintf(out, "\t\tlv2:default %.6f ;\n", p->start);
                }

                emit_end(out, p_prop);

                // Output all port groups of the port
                if (requirements & REQ_PORT_GROUPS)
                    print_port_groups(out, p, m.port_groups);

                fprintf(out, "\t]");
                port_id++;
            }

            // Add atom ports for state serialization
            for (size_t i=0; i<2; ++i)
            {
                const meta::port_t *p = &lv2::atom_ports[i];

                fprintf(out, "%s [\n", (port_id == 0) ? "\tlv2:port" : " ,");
                fprintf(out, "\t\ta lv2:%s, atom:AtomPort ;\n", (i > 0) ? "OutputPort" : "InputPort");
                fprintf(out, "\t\tatom:bufferType atom:Sequence ;\n");

                fprintf(out, "\t\tatom:supports atom:Sequence");
                if (requirements & REQ_PATCH)
                    fprintf(out, ", patch:Message");
                if (requirements & REQ_TIME)
                    fprintf(out, ", time:Position");
                if ((i == 0) && (requirements & REQ_MIDI_IN))
                    fprintf(out, ", midi:MidiEvent");
                else if ((i == 1) && (requirements & REQ_MIDI_OUT))
                    fprintf(out, ", midi:MidiEvent");
                fprintf(out, " ;\n");

                const char *p_id    = p->id;
                const char *p_name  = p->name;
                const char *comm    = NULL;
                if (meta::is_in_port(p))
                {
                    comm            = "UI -> DSP communication";
                    if (requirements & REQ_MIDI_IN)
                    {
                        p_id            = LSP_LV2_MIDI_PORT_IN;
                        p_name          = "MIDI Input, UI Input";
                        comm            = "MIDI IN, UI -> DSP communication";
                    }
                }
                else
                {
                    comm            = "DSP -> UI communication";
                    if (requirements & REQ_MIDI_IN)
                    {
                        p_id            = LSP_LV2_MIDI_PORT_OUT;
                        p_name          = "MIDI Output, UI Output";
                        comm            = "MIDI OUT, DSP -> UI communication";
                    }
                }

                long bufsize    = lv2::lv2_all_port_sizes(m.ports, meta::is_in_port(p), meta::is_out_port(p));
                if (m.extensions & meta::E_KVT_SYNC)
                    bufsize        += OSC_BUFFER_MAX;
                if (m.extensions & meta::E_FILE_PREVIEW)
                    bufsize        += size_t(PATH_MAX + 0x100);

                if (!(requirements & REQ_MIDI))
                    fprintf(out, "\t\tlv2:portProperty lv2:connectionOptional ;\n");
                fprintf(out, "\t\tlv2:designation lv2:control ;\n");
                fprintf(out, "\t\tlv2:index %d ;\n", int(port_id));
                fprintf(out, "\t\tlv2:symbol \"%s\" ;\n", p_id);
                fprintf(out, "\t\tlv2:name \"%s\" ;\n", p_name);
                fprintf(out, "\t\trdfs:comment \"%s\" ;\n", comm);
                fprintf(out, "\t\trsz:minimumSize %ld ;\n", bufsize * 2);
                fprintf(out, "\t]");

                port_id++;
            }

            // Add latency reporting port
            {
                const meta::port_t *p = &lv2::latency_port;
                if ((p->id != NULL) && (p->name != NULL))
                {
                    fprintf(out, "%s [\n", (port_id == 0) ? "\tlv2:port" : " ,");
                    fprintf(out, "\t\ta lv2:%s, lv2:ControlPort ;\n", (meta::is_out_port(p)) ? "OutputPort" : "InputPort");
                    fprintf(out, "\t\tlv2:index %d ;\n", int(port_id));
                    fprintf(out, "\t\tlv2:symbol \"%s\" ;\n", p->id);
                    fprintf(out, "\t\tlv2:name \"%s\" ;\n", p->name);
                    fprintf(out, "\t\tlv2:designation lv2:latency ;\n");
                    fprintf(out, "\t\trdfs:comment \"DSP -> Host latency report\" ;\n");

                    size_t p_prop = 0;
                    emit_header(out, p_prop, "\t\tlv2:portProperty");
                    emit_option(out, p_prop, (p->flags & (meta::F_LOWER | meta::F_UPPER)) == (meta::F_LOWER | meta::F_UPPER), "pp:hasStrictBounds");
                    emit_option(out, p_prop, p->flags & meta::F_INT, "lv2:integer");
                    emit_option(out, p_prop, true, "lv2:reportsLatency");
                    emit_end(out, p_prop);

                    if (p->flags & meta::F_LOWER)
                        fprintf(out, "\t\tlv2:minimum %d ;\n", int(p->min));
                    if (p->flags & meta::F_UPPER)
                        fprintf(out, "\t\tlv2:maximum %d ;\n", int(p->max));
                    fprintf(out, "\t\tlv2:default %d ;\n", int(p->start));
                    fprintf(out, "\t]");

                    port_id++;
                }
            }

            // Finish port list
            fprintf(out, "\n\t.\n");
        }

        void gen_plugin_ttl_file(
            const meta::plugin_t &m,
            const meta::package_t *manifest,
            const cmdline_t *cmd)
        {
            char fname[PATH_MAX];
            FILE *out = NULL;
            snprintf(fname, sizeof(fname)-1, "%s/%s.ttl", cmd->out_dir, get_module_uid(m.uids.lv2));
            size_t requirements     = scan_requirements(m);

            // Generate manifest.ttl
            if (!(out = fopen(fname, "w+")))
                return;
            printf("Writing file %s\n", fname);

            // Output header
            emit_prefix(out, "doap", "http://usefulinc.com/ns/doap#");
            emit_prefix(out, "dc", "http://purl.org/dc/terms/");
            emit_prefix(out, "foaf", "http://xmlns.com/foaf/0.1/");
            emit_prefix(out, "rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
            emit_prefix(out, "rdfs", "http://www.w3.org/2000/01/rdf-schema#");
            emit_prefix(out, "lv2", LV2_CORE_PREFIX);
            if (requirements & REQ_INSTANCE)
                emit_prefix(out, "lv2ext", "http://lv2plug.in/ns/ext/");
            emit_prefix(out, "pp", LV2_PORT_PROPS_PREFIX);
            if (requirements & REQ_PORT_GROUPS)
                emit_prefix(out, "pg", LV2_PORT_GROUPS_PREFIX);
            if (requirements & REQ_LV2UI)
                emit_prefix(out, "ui", LV2_UI_PREFIX);
            emit_prefix(out, "units", LV2_UNITS_PREFIX);
            emit_prefix(out, "atom", LV2_ATOM_PREFIX);
            emit_prefix(out, "urid", LV2_URID_PREFIX);
            emit_prefix(out, "opts", LV2_OPTIONS_PREFIX);
            emit_prefix(out, "work", LV2_WORKER_PREFIX);
            emit_prefix(out, "rsz", LV2_RESIZE_PORT_PREFIX);
            if (requirements & REQ_PATCH)
                emit_prefix(out, "patch", LV2_PATCH_PREFIX);
            emit_prefix(out, "state", LV2_STATE_PREFIX);
            if (requirements & REQ_MIDI)
                emit_prefix(out, "midi", LV2_MIDI_PREFIX);
            if (requirements & REQ_TIME)
                emit_prefix(out, "time", LV2_TIME_URI "#");
            emit_prefix(out, "hcid", LV2_INLINEDISPLAY_PREFIX);
            emit_prefix(out, LV2TTL_PLUGIN_PREFIX, get_module_prefix(fname, sizeof(fname), m.uids.lv2));
            if (requirements & REQ_PORT_GROUPS)
            {
                snprintf(fname, sizeof(fname), "%s/port_groups#", m.uids.lv2);
                emit_prefix(out, LV2TTL_PORT_GROUP_PREFIX, fname);
            }
            if (requirements & REQ_LV2UI)
                emit_prefix(out, LV2TTL_PLUGIN_UI_PREFIX, get_module_prefix(fname, sizeof(fname), m.uids.lv2ui));
            emit_prefix(out, LV2TTL_DEVELOPERS_PREFIX, LSP_LV2_BASE_URI "developers/");
            if (requirements & REQ_PATCH)
                emit_prefix(out, LV2TTL_PORTS_PREFIX, LSP_LV2_BASE_URI "/ports#");
            fprintf(out, "\n\n");

            fprintf(out, "hcid:queue_draw\n\ta lv2:Feature\n\t.\n");
            fprintf(out, "hcid:interface\n\ta lv2:ExtensionData\n\t.\n\n");

            // Output developer and maintainer objects
            const meta::person_t *dev = m.developer;
            if ((dev != NULL) && (dev->uid != NULL))
            {
                fprintf(out, "%s:%s\n", LV2TTL_DEVELOPERS_PREFIX, dev->uid);
                fprintf(out, "\ta foaf:Person");
                if (dev->name != NULL)
                    fprintf(out, " ;\n\tfoaf:name \"%s\"", dev->name);
                if (dev->nick != NULL)
                    fprintf(out, " ;\n\tfoaf:nick \"%s\"", dev->nick);
                if (dev->mailbox != NULL)
                    fprintf(out, " ;\n\tfoaf:mbox <mailto:%s>", dev->mailbox);
                if (dev->homepage != NULL)
                    fprintf(out, " ;\n\tfoaf:homepage <%s#%s>", dev->homepage, dev->uid);
                fprintf(out, "\n\t.\n\n");
            }

            fprintf(out, "%s:%s\n", LV2TTL_DEVELOPERS_PREFIX, manifest->brand_id);
            fprintf(out, "\ta foaf:Person");
            fprintf(out, " ;\n\tfoaf:name \"%s LV2\"", manifest->brand);
            fprintf(out, " ;\n\tfoaf:mbox <mailto:%s>", manifest->email);
            fprintf(out, " ;\n\tfoaf:homepage <%s>", manifest->site);
            fprintf(out, "\n\t.\n");

            // Output plugin metadata
            if (cmd->lv2_binary != NULL)
            {
                fprintf(out, "\n\n");
                gen_plugin_ttl(out, requirements, m, manifest, cmd);
            }

            // Output plugin UIs
            if ((requirements & REQ_LV2UI) && (cmd->lv2ui_binary != NULL))
            {
                fprintf(out, "\n\n");
                gen_plugin_ui_ttl(out, requirements, m, manifest, cmd);
            }

            fclose(out);
        }

        void gen_manifest(
            const meta::package_t *manifest,
            const lltl::parray<meta::plugin_t> &plugins,
            const cmdline_t *cmd)
        {
            char fname[PATH_MAX];
            char prefix[PATH_MAX], ui_prefix[PATH_MAX];

            snprintf(fname, sizeof(fname)-1, "%s/manifest.ttl", cmd->out_dir);
            FILE *out = NULL;

            // Generate manifest.ttl
            if (!(out = fopen(fname, "w+")))
                return;
            printf("Writing file %s\n", fname);

            emit_prefix(out, "lv2", LV2_CORE_PREFIX);
            emit_prefix(out, "rdfs", "http://www.w3.org/2000/01/rdf-schema#");
            emit_prefix(out, "ui", LV2_UI_PREFIX);

            // Generate and emit prefixes for plugin and plugin UI
            prefix[0] = '\0';
            ui_prefix[0] = '\0';

            // Find first prefix for UI and DSP module and emit it as short symbolic name
            if (cmd->lv2_binary != NULL)
            {
                for (size_t i=0, n=plugins.size(); i<n; ++i)
                {
                    const meta::plugin_t *m = plugins.uget(i);
                    if (m->uids.lv2)
                    {
                        get_module_prefix(prefix, sizeof(prefix), m->uids.lv2);
                        emit_prefix(out, LV2TTL_PLUGIN_PREFIX, prefix);
                        break;
                    }
                }
            }
            if (cmd->lv2ui_binary != NULL)
            {
                for (size_t i=0, n=plugins.size(); i<n; ++i)
                {
                    const meta::plugin_t *m = plugins.uget(i);
                    if (m->uids.lv2ui)
                    {
                        get_module_prefix(ui_prefix, sizeof(ui_prefix), m->uids.lv2ui);
                        emit_prefix(out, LV2TTL_PLUGIN_UI_PREFIX, ui_prefix);
                        break;
                    }
                }
            }

            // Emit information about plugins
            if (cmd->lv2_binary != NULL)
            {
                size_t prefix_len = strlen(prefix);
                fprintf(out, "\n\n");
                for (size_t i=0, n=plugins.size(); i<n; ++i)
                {
                    const meta::plugin_t *m = plugins.uget(i);
                    if (m->uids.lv2 != NULL)
                    {
                        if (strncmp(prefix, m->uids.lv2, prefix_len) == 0)
                            fprintf(out, "%s:%s\n", LV2TTL_PLUGIN_PREFIX, &m->uids.lv2[prefix_len]);
                        else
                            fprintf(out, "<%s>\n", m->uids.lv2);
                        fprintf(out, "\ta lv2:Plugin ;\n");
                        fprintf(out, "\tlv2:binary <%s> ;\n", cmd->lv2_binary);
                        fprintf(out, "\trdfs:seeAlso <%s.ttl> .\n\n", get_module_uid(m->uids.lv2));
                    }
                }
            }

            // Emit information about plugin UIs
            if (cmd->lv2ui_binary != NULL)
            {
                size_t ui_prefix_len = strlen(ui_prefix);
                fprintf(out, "\n\n");
                for (size_t i=0, n=plugins.size(); i<n; ++i)
                {
                    const meta::plugin_t *m = plugins.uget(i);
                    if ((m->uids.lv2 != NULL) && (m->uids.lv2ui != NULL))
                    {
                        if (strncmp(ui_prefix, m->uids.lv2ui, ui_prefix_len) == 0)
                            fprintf(out, "%s:%s\n", LV2TTL_PLUGIN_UI_PREFIX, &m->uids.lv2ui[ui_prefix_len]);
                        else
                            fprintf(out, "<%s>\n", m->uids.lv2ui);
                        fprintf(out, "\ta ui:" LV2TTL_UI_CLASS " ;\n");
                        fprintf(out, "\tlv2:binary <%s> ;\n", cmd->lv2ui_binary);
                        fprintf(out, "\trdfs:seeAlso <%s.ttl> .\n\n", get_module_uid(m->uids.lv2));
                    }
                }
            }

            fclose(out);
        }

        static ssize_t cmp_by_lv2_uri(const meta::plugin_t *a, const meta::plugin_t *b)
        {
            return strcmp(a->uids.lv2, b->uids.lv2);
        }

        status_t main(int argc, const char **argv)
        {
            cmdline_t cmd;
            meta::package_t *manifest = NULL;
            lltl::parray<meta::plugin_t> plugins;

            // Parse command-line arguments
            status_t res = parse_cmdline(&cmd, argc, argv);
            if (res == STATUS_CANCELLED)
                return STATUS_OK;
            else if (res != STATUS_OK)
                return res;

            // Enumerate list of all plugins
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if ((meta == NULL) || (meta->uids.lv2 == NULL))
                        break;

                    // Add metadata to list
                    if (!plugins.add(const_cast<meta::plugin_t *>(meta)))
                    {
                        fprintf(stderr, "Error obtaining plugin list\n");
                        return STATUS_NO_MEM;
                    }
                }
            }
            plugins.qsort(cmp_by_lv2_uri);

            // Obtain the manifest
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
            {
                lsp_error("No manifest file found");
                return STATUS_NO_DATA;
            }

            // Generate manifest file
            gen_manifest(manifest, plugins, &cmd);

            // Generate TTL files for each plugin
            for (size_t i=0, n=plugins.size(); i<n; ++i)
            {
                const meta::plugin_t *m = plugins.uget(i);
                if (m != NULL)
                    gen_plugin_ttl_file(*m, manifest, &cmd);
            }

            // Release the manifest
            meta::free_manifest(manifest);
            manifest = NULL;

            return STATUS_OK;
        }
    } /* namespace lv2ttl_gen */
} /* namespace lsp */
