/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>

#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>

#include <lsp-plug.in/plug-fw/wrap/lv2/lv2.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/static.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>


#define LV2TTL_PLUGIN_PREFIX                "plug"
#define LV2TTL_PLUGIN_UI_PREFIX             "plug_ui"
#define LV2TTL_PORT_GROUP_PREFIX            "plug_pg"
#define LV2TTL_PORTS_PREFIX                 "plug_p"
#define LV2TTL_DEVELOPERS_PREFIX            "plug_dev"

// Different LV2 UI classes for different platforms
#if defined(PLATFORM_LINUX) || defined(PLATFORM_BSD)
    #define LV2TTL_UI_CLASS                             "X11UI"
#elif defined(PLATFORM_WINDOWS)
    #define LV2TTL_UI_CLASS                             "WindowsUI"
#elif defined(PLATFORM_MACOSX)
    #define LV2TTL_UI_CLASS                             "CocoaUI"
#elif defined(PLATFORM_UNIX_COMPATIBLE)
    #define LV2TTL_UI_CLASS                             "X11UI"
#else
    #error "Could not determine LV2 UI class for target platform"
#endif

namespace lsp
{
    namespace lv2
    {
        enum requirements_t
        {
            REQ_PATCH       = 1 << 0,
            REQ_STATE       = 1 << 1,
            REQ_LV2UI       = 1 << 2,
            REQ_PORT_GROUPS = 1 << 3,
            REQ_WORKER      = 1 << 4,
            REQ_MIDI_IN     = 1 << 5,
            REQ_MIDI_OUT    = 1 << 6,
            REQ_PATCH_WR    = 1 << 7,
            REQ_INSTANCE    = 1 << 8,
            REQ_TIME        = 1 << 9,
            REQ_IDISPLAY    = 1 << 10,
            REQ_OSC_IN      = 1 << 11,
            REQ_OSC_OUT     = 1 << 12,
            REQ_MAP_PATH    = 1 << 13,

            REQ_PATH_MASK   = REQ_PATCH | REQ_STATE | REQ_MAP_PATH | REQ_WORKER | REQ_PATCH_WR,
            REQ_MIDI        = REQ_MIDI_IN | REQ_MIDI_OUT
        };

        typedef struct plugin_group_t
        {
            int            id;
            const char    *name;
        } plugin_group_t;

        const plugin_group_t plugin_groups[] =
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

        typedef struct plugin_unit_t
        {
            int             id;         // Internal ID from metadata
            const char     *name;       // LV2 name of the unit
            const char     *label;      // Custom label of the unit (if name is not present)
            const char     *render;     // Formatting of the unit (if name is not present)
        } plugin_unit_t;

        const plugin_unit_t plugin_units[] =
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

            { meta::U_BYTES,        NULL,               "Bytes",                "%.0f"      },
            { meta::U_KBYTES,       NULL,               "Kilobytes",            "%.2f"      },
            { meta::U_MBYTES,       NULL,               "Megabytes",            "%.2f"      },
            { meta::U_GBYTES,       NULL,               "Gigabytes",            "%.2f"      },
            { meta::U_TBYTES,       NULL,               "Terabytes",            "%.2f"      },

            { -1, NULL, NULL, NULL }
        };

        inline void emit_header(FILE *out, size_t &count, const char *text)
        {
            if (count == 0)
            {
                fputs(text, out);
                fputc(' ', out);
            }
        }

        inline void emit_option(FILE *out, size_t &count, bool condition, const char *text)
        {
            if (condition)
            {
                if (count++)
                    fputs(", ", out);
                fputs(text, out);
            }
        }

        inline void emit_end(FILE *out, size_t &count)
        {
            if (count > 0)
            {
                fprintf(out, " ;\n");
                count = 0;
            }
        }

        static void print_plugin_groups(FILE *out, const meta::plugin_t &m)
        {
            size_t count = 0;

            emit_header(out, count, "\ta");
            for (const int *c = m.classes; ((c != NULL) && ((*c) >= 0)); ++c)
            {
                // Lookup for predefined plugin group in list
                for (const plugin_group_t *grp = plugin_groups; (grp != NULL) && (grp->id >= 0); ++grp)
                {
                    if (grp->id == *c)
                    {
                        if (count++)
                            fputs(", ", out);
                        fprintf(out, "lv2:%s", grp->name);
                        break;
                    }
                }
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
            if (str == NULL)
                str = uri;

            ssize_t src_len = lsp_min(str - uri, ssize_t(len));
            memcpy(buf, uri, src_len);
            buf[len - 1] = '\0';
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

        void gen_plugin_ui_ttl(FILE *out, size_t requirements, const meta::plugin_t &m, const char *lv2ui_binary)
        {
            fprintf(out, "%s:%s\n", LV2TTL_PLUGIN_UI_PREFIX, get_module_uid(m.lv2ui_uri));
            fprintf(out, "\ta ui:" LV2TTL_UI_CLASS " ;\n");
            fprintf(out, "\tlv2:minorVersion %d ;\n", int(LSP_MODULE_VERSION_MINOR(m.version)));
            fprintf(out, "\tlv2:microVersion %d ;\n", int(LSP_MODULE_VERSION_MICRO(m.version)));
            fprintf(out, "\tlv2:requiredFeature urid:map, ui:idleInterface ;\n");
            {
                size_t count = 1;
                fprintf(out, "\tlv2:optionalFeature ui:parent, ui:resize");
                emit_option(out, count, requirements & REQ_INSTANCE, "lv2ext:instance-access");
                fprintf(out, " ;\n");
            }
            fprintf(out, "\tlv2:extensionData ui:idleInterface, ui:resize ;\n");
            fprintf(out, "\tui:binary <%s> ;\n", lv2ui_binary);
            fprintf(out, "\n");

            size_t ports        = 0;
            size_t port_id      = 0;

            for (const meta::port_t *p = m.ports; (p->id != NULL); ++p)
            {
                // Skip virtual ports
                switch (p->role)
                {
                    case meta::R_UI_SYNC:
                    case meta::R_MESH:
                    case meta::R_STREAM:
                    case meta::R_FBUFFER:
                    case meta::R_PATH:
                    case meta::R_PORT_SET:
                    case meta::R_MIDI:
                    case meta::R_OSC:
                        continue;
                    case meta::R_AUDIO:
                        port_id++;
                        continue;
                    default:
                        break;
                }

                fprintf(out, "%s [\n", (ports == 0) ? "\tui:portNotification" : ",");
                fprintf(out, "\t\tui:plugin %s:%s ;\n", LV2TTL_PLUGIN_PREFIX, get_module_uid(m.lv2_uri));
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

                fprintf(out, "\t] ");

                ports++;
                port_id++;
            }

            // Add atom ports for state serialization
            for (size_t i=0; i<2; ++i)
            {
                fprintf(out, "%s [\n", (ports == 0) ? "\tui:portNotification" : " ,");
                fprintf(out, "\t\tui:plugin %s:%s ;\n", LV2TTL_PLUGIN_PREFIX, get_module_uid(m.lv2_uri));
                fprintf(out, "\t\tui:portIndex %d ;\n", int(port_id));
                fprintf(out, "\t\tui:protocol atom:eventTransfer ;\n");
                fprintf(out, "\t\tui:notifyType atom:Sequence ;\n");
                fprintf(out, "\t]");

                ports++;
                port_id++;
            }

            // Add latency report port
            {
                const meta::port_t *p = &latency_port;
                if ((p->id != NULL) && (p->name != NULL))
                {
                    fprintf(out, "%s [\n", (ports == 0) ? "\tui:portNotification" : " ,");
                    fprintf(out, "\t\tui:plugin %s:%s ;\n", LV2TTL_PLUGIN_PREFIX, get_module_uid(m.lv2_uri));
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
                        const char *role = NULL;
                        switch (p->role)
                        {
                            case meta::PGR_CENTER:          role = "center";                break;
                            case meta::PGR_CENTER_LEFT:     role = "centerLeft";            break;
                            case meta::PGR_CENTER_RIGHT:    role = "centerRight";           break;
                            case meta::PGR_LEFT:            role = "left";                  break;
                            case meta::PGR_LO_FREQ:         role = "lowFrequencyEffects";   break;
                            case meta::PGR_REAR_CENTER:     role = "rearCenter";            break;
                            case meta::PGR_REAR_LEFT:       role = "rearLeft";              break;
                            case meta::PGR_REAR_RIGHT:      role = "rearRight";             break;
                            case meta::PGR_RIGHT:           role = "right";                 break;
                            case meta::PGR_MS_SIDE:         role = "side";                  break;
                            case meta::PGR_SIDE_LEFT:       role = "sideLeft";              break;
                            case meta::PGR_SIDE_RIGHT:      role = "sideRight";             break;
                            case meta::PGR_MS_MIDDLE:       role = "center";                break;
                            default:
                                break;
                        }
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
                    case meta::R_MESH:
                    case meta::R_STREAM:
                    case meta::R_FBUFFER:
                        result     |= REQ_INSTANCE;
                        break;
                    case meta::R_MIDI:
                        if (meta::is_out_port(p))
                            result     |= REQ_MIDI_OUT;
                        else
                            result     |= REQ_MIDI_IN;
                        break;
                    case meta::R_OSC:
                        if (meta::is_out_port(p))
                            result     |= REQ_OSC_OUT;
                        else
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

            if (m.lv2ui_uri != NULL)
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
            while (len < 20)
                fputc(' ', out);
            fprintf(out, "<%s> .\n", value);
        }

        void gen_plugin_ttl(
            const char *path,
            const meta::plugin_t &m,
            const meta::package_t *manifest,
            const char *lv2_binary,
            const char *lv2ui_binary)
        {
            char fname[PATH_MAX];
            FILE *out = NULL;
            snprintf(fname, sizeof(fname)-1, "%s/%s.ttl", path, get_module_uid(m.lv2_uri));
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
                emit_prefix(out, "pg", "LV2_PORT_GROUPS_PREFIX");
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
            emit_prefix(out, LV2TTL_PLUGIN_PREFIX, get_module_prefix(fname, sizeof(fname), m.lv2_uri));
            if (requirements & REQ_PORT_GROUPS)
            {
                snprintf(fname, sizeof(fname), "%s/port_groups#", m.lv2_uri);
                emit_prefix(out, LV2TTL_PORT_GROUP_PREFIX, fname);
            }
            if (requirements & REQ_LV2UI)
                emit_prefix(out, LV2TTL_PLUGIN_UI_PREFIX, get_module_prefix(fname, sizeof(fname), m.lv2ui_uri));
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
            fprintf(out, "\n\t.\n\n");

            // Output port groups
            const meta::port_group_t *pg_main_in = NULL, *pg_main_out = NULL;

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
                        fprintf(out, "\tpg:sideChainOf %s:%s ;\n", LV2TTL_PORT_GROUP_PREFIX, pg->parent_id);
                    if (pg->flags & meta::PGF_MAIN)
                    {
                        if (pg->flags & meta::PGF_OUT)
                            pg_main_out     = pg;
                        else
                            pg_main_in      = pg;
                    }

                    fprintf(out, "\tlv2:symbol \"%s\" ;\n", pg->id);
                    fprintf(out, "\trdfs:label \"%s\"\n", pg->name);
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
                    default:
                        break;
                }
            }

            // Output plugin
            fprintf(out, "%s:%s\n", LV2TTL_PLUGIN_PREFIX, get_module_uid(m.lv2_uri));
            print_plugin_groups(out, m);
            fprintf(out, "\tdoap:name \"%s %s\" ;\n", manifest->brand, m.description);
            fprintf(out, "\tlv2:minorVersion %d ;\n", int(LSP_MODULE_VERSION_MINOR(m.version)));
            fprintf(out, "\tlv2:microVersion %d ;\n", int(LSP_MODULE_VERSION_MICRO(m.version)));
            if ((dev != NULL) && (dev->uid != NULL))
                fprintf(out, "\tdoap:developer %s:%s ;\n", LV2TTL_DEVELOPERS_PREFIX, m.developer->uid);
            fprintf(out, "\tdoap:maintainer %s:%s ;\n", LV2TTL_DEVELOPERS_PREFIX, manifest->brand_id);
            fprintf(out, "\tdoap:license <%s> ;\n", manifest->lv2_license);
            fprintf(out, "\tlv2:binary <%s> ;\n", lv2_binary);
            if (requirements & REQ_LV2UI)
                fprintf(out, "\tui:ui %s:%s ;\n", LV2TTL_PLUGIN_UI_PREFIX, get_module_uid(m.lv2ui_uri));

            fprintf(out, "\n");

            // Emit required features
            fprintf(out, "\tlv2:requiredFeature urid:map ;\n");

            // Emit optional features
            fprintf(out, "\tlv2:optionalFeature lv2:hardRTCapable, hcid:queue_draw, work:schedule, opts:options, state:mapPath ;\n");

            // Emit extension data
            fprintf(out, "\tlv2:extensionData state:interface, work:interface, hcid:interface ;\n");

            // Different supported options
            if (requirements & REQ_LV2UI)
                fprintf(out, "\topts:supportedOption ui:updateRate ;\n\n");

            if (pg_main_in != NULL)
                fprintf(out, "\tpg:mainInput %s:%s ;\n", LV2TTL_PORT_GROUP_PREFIX, pg_main_in->id);
            if (pg_main_out != NULL)
                fprintf(out, "\tpg:mainOutput %s:%s ;\n", LV2TTL_PORT_GROUP_PREFIX, pg_main_out->id);

            // Replacement for LADSPA plugin
            if (m.ladspa_id > 0)
                fprintf(out, "\n\tdc:replaces <urn:ladspa:%ld> ;\n", long(m.ladspa_id));

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
                    case meta::R_UI_SYNC:
                    case meta::R_MESH:
                    case meta::R_STREAM:
                    case meta::R_FBUFFER:
                    case meta::R_PATH:
                    case meta::R_PORT_SET:
                    case meta::R_MIDI:
                    case meta::R_OSC:
                        continue;
                    default:
                        break;
                }

                fprintf(out, "%s [\n", (port_id == 0) ? "\tlv2:port" : " ,");
                fprintf(out, "\t\ta lv2:%s, ", meta::is_out_port(p) ? "OutputPort" : "InputPort");

                switch (p->role)
                {
                    case meta::R_AUDIO:
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
                        emit_end(out, p_prop);
                    }

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
                const meta::port_t *p = &atom_ports[i];

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

                long bufsize    = lv2_all_port_sizes(m.ports, meta::is_in_port(p), meta::is_out_port(p));
                if (m.extensions & meta::E_KVT_SYNC)
                    bufsize        += OSC_BUFFER_MAX;

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
                const meta::port_t *p = &latency_port;
                if ((p->id != NULL) && (p->name != NULL))
                {
                    fprintf(out, "%s [\n", (port_id == 0) ? "\tlv2:port" : " ,");
                    fprintf(out, "\t\ta lv2:%s, lv2:ControlPort ;\n", (p->flags & meta::F_OUT) ? "OutputPort" : "InputPort");
                    fprintf(out, "\t\tlv2:index %d ;\n", int(port_id));
                    fprintf(out, "\t\tlv2:symbol \"%s\" ;\n", p->id);
                    fprintf(out, "\t\tlv2:name \"%s\" ;\n", p->name);
                    fprintf(out, "\t\trdfs:comment \"DSP -> Host latency report\" ;\n");

                    if ((p->flags & (meta::F_LOWER | meta::F_UPPER)) == (meta::F_LOWER | meta::F_UPPER))
                        fprintf(out, "\t\tlv2:portProperty pp:hasStrictBounds ;\n");
                    if (p->flags & meta::F_INT)
                        fprintf(out, "\t\tlv2:portProperty lv2:integer ;\n");
                    fprintf(out, "\t\tlv2:portProperty lv2:reportsLatency ;\n");

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
            fprintf(out, "\n\t.\n\n");

            // Output plugin UIs
            if ((requirements & REQ_LV2UI) && (lv2ui_binary != NULL))
                gen_plugin_ui_ttl(out, requirements, m, lv2ui_binary);

            fclose(out);
        }

        void gen_manifest(
            const char *path,
            const meta::package_t *manifest,
            const lltl::parray<meta::plugin_t> &plugins,
            const char *lv2_binary,
            const char *lv2ui_binary)
        {
            char fname[PATH_MAX];
            char prefix[PATH_MAX], ui_prefix[PATH_MAX];

            snprintf(fname, sizeof(fname)-1, "%s/manifest.ttl", path);
            FILE *out = NULL;

            // Generate manifest.ttl
            if (!(out = fopen(fname, "w+")))
                return;
            printf("Writing file %s\n", fname);

            emit_prefix(out, "lv2", LV2_CORE_PREFIX);
            emit_prefix(out, "rdfs", "http://www.w3.org/2000/01/rdf-schema#");

            // Generate and emit prefixes for plugin and plugin UI
            prefix[0] = '\0';
            ui_prefix[0] = '\0';

            for (size_t i=0, n=plugins.size(); i<n; ++i)
            {
                const meta::plugin_t *m = plugins.uget(i);
                if (m->lv2_uri)
                {
                    get_module_prefix(prefix, sizeof(prefix), m->lv2_uri);
                    emit_prefix(out, LV2TTL_PLUGIN_PREFIX, prefix);
                }
            }
            for (size_t i=0, n=plugins.size(); i<n; ++i)
            {
                const meta::plugin_t *m = plugins.uget(i);
                if (m->lv2ui_uri)
                {
                    get_module_prefix(ui_prefix, sizeof(ui_prefix), m->lv2ui_uri);
                    emit_prefix(out, LV2TTL_PLUGIN_UI_PREFIX, prefix);
                }
            }

            // Emit information about plugins
            size_t prefix_len = strlen(prefix);
            fprintf(out, "\n\n");
            for (size_t i=0, n=plugins.size(); i<n; ++i)
            {
                const meta::plugin_t *m = plugins.uget(i);
                if (m->lv2_uri != NULL)
                {
                    if (strncmp(prefix, m->lv2_uri, prefix_len) == 0)
                        fprintf(out, "%s:%s\n", LV2TTL_PLUGIN_PREFIX, &m->lv2_uri[prefix_len]);
                    else
                        fprintf(out, "<%s>\n", m->lv2_uri);
                    fprintf(out, "\ta lv2:Plugin ;\n");
                    fprintf(out, "\tlv2:binary <%s> ;\n", lv2_binary);
                    fprintf(out, "\trdfs:seeAlso <%s.ttl> .\n\n", get_module_uid(m->lv2_uri));
                }
            }

            // Emit information about plugin UIs
            size_t ui_prefix_len = strlen(ui_prefix);
            fprintf(out, "\n\n");
            for (size_t i=0, n=plugins.size(); i<n; ++i)
            {
                const meta::plugin_t *m = plugins.uget(i);
                if ((m->lv2_uri != NULL) && (m->lv2ui_uri != NULL))
                {
                    if (strncmp(prefix, m->lv2ui_uri, ui_prefix_len) == 0)
                        fprintf(out, "%s:%s\n", LV2TTL_PLUGIN_UI_PREFIX, &m->lv2ui_uri[ui_prefix_len]);
                    else
                        fprintf(out, "<%s>\n", m->lv2ui_uri);
                    fprintf(out, "\ta ui:" LV2TTL_UI_CLASS " ;\n");
                    fprintf(out, "\tlv2:binary <%s> ;\n", lv2ui_binary);
                    fprintf(out, "\trdfs:seeAlso <%s.ttl> .\n\n", get_module_uid(m->lv2_uri));
                }
            }

            fclose(out);
        }

        static ssize_t cmp_by_lv2_uri(const meta::plugin_t *a, const meta::plugin_t *b)
        {
            return strcmp(a->lv2_uri, b->lv2_uri);
        }

        status_t gen_ttl(const char *path, const char *lv2_binary, const char *lv2ui_binary)
        {
            meta::package_t *manifest = NULL;
            lltl::parray<meta::plugin_t> plugins;

            // Enumerate list of all plugins
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
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
            status_t res;
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
            gen_manifest(path, manifest, plugins, lv2_binary, lv2ui_binary);

            // Generate TTL files for each plugin
            for (size_t i=0, n=plugins.size(); i<n; ++i)
            {
                const meta::plugin_t *m = plugins.uget(i);
                if (m != NULL)
                    gen_plugin_ttl(path, *m, manifest, lv2_binary, lv2ui_binary);
            }

            // Release the manifest
            meta::free_manifest(manifest);
            manifest = NULL;

            return STATUS_OK;
        }
    } /* namespace lv2 */
} /* namespace lsp */

#ifndef LSP_IDE_DEBUG
int main(int argc, const char **argv)
{
    if (argc <= 0)
        fprintf(stderr, "required destination path");
    lsp::lv2::gen_ttl(argv[1]);

    return 0;
}
#endif /* LSP_IDE_DEBUG */



