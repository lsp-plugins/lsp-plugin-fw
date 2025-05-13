/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_META_PORTS_H_
#define LSP_PLUG_IN_PLUG_FW_META_PORTS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

// Port definitions for metadata
#define AUDIO_INPUT(id, label) \
    { id, label, NULL, U_NONE, R_AUDIO_IN, 0, 0, 0, 0, 0, NULL, NULL, NULL    }
#define AUDIO_OUTPUT(id, label) \
    { id, label, NULL, U_NONE, R_AUDIO_OUT, 0, 0, 0, 0, 0, NULL, NULL, NULL    }
#define MIDI_IN_CHANNEL(id, label) \
    { id, label, NULL, U_NONE, R_MIDI_IN, 0, 0, 0, 0, 0, NULL, NULL, NULL    }
#define MIDI_OUT_CHANNEL(id, label) \
    { id, label, NULL, U_NONE, R_MIDI_OUT, 0, 0, 0, 0, 0, NULL, NULL, NULL    }
#define OSC_IN_CHANNEL(id, label) \
    { id, label, NULL, U_NONE, R_OSC_IN, 0, 0, 0, 0, 0, NULL, NULL, NULL    }
#define OSC_OUT_CHANNEL(id, label) \
    { id, label, NULL, U_NONE, R_OSC_OUT, 0, 0, 0, 0, 0, NULL, NULL, NULL    }
#define FILE_CHANNEL(id, label) \
    { id, label, NULL, U_ENUM, R_CONTROL, F_INT, 0, 0, 0, 0, file_channels, NULL, NULL }

#define STATUS(id, label) \
    { id, label, NULL, U_NONE, R_METER, F_INT | F_UPPER | F_LOWER, 0, STATUS_MAX, STATUS_UNSPECIFIED, 0, NULL, NULL, NULL }
#define MESH(id, label, dim, points) \
    { id, label, NULL, U_NONE, R_MESH, 0, 0.0, 0.0, points, dim, NULL, NULL, NULL }
#define STREAM(id, label, dim, frames, capacity) \
    { id, label, NULL, U_NONE, R_STREAM, 0, dim, frames, capacity, 0.0f, NULL, NULL, NULL }
#define FBUFFER(id, label, rows, cols) \
    { id, label, NULL, U_NONE, R_FBUFFER, 0, 0.0, 0.0, rows, cols, NULL, NULL, NULL }
#define PATH(id, label) \
    { id, label, NULL, U_STRING, R_PATH, 0, 0, 0, 0, 0, NULL, NULL, NULL }
#define TRIGGER(id, label, alias)  \
    { id, label, alias, U_BOOL, R_CONTROL, F_TRG, 0, 0, 0.0f, 0, NULL, NULL, NULL }
#define SWITCH(id, label, alias, dfl)  \
    { id, label, alias, U_BOOL, R_CONTROL, 0, 0, 0, dfl, 0, NULL, NULL, NULL }
#define COMBO(id, label, alias, dfl, list) \
    { id, label, alias, U_ENUM, R_CONTROL, 0, 0, 0, dfl, 0, list, NULL, NULL }
#define COMBO_START(id, label, dfl, list, min) \
    { id, label, NULL, U_ENUM, R_CONTROL, F_MIN, min, 0, dfl, 0, list, NULL, NULL }
#define BLINK(id, label) \
    { id, label, NULL, U_BOOL, R_METER, 0, 0, 0, 0, 0, NULL, NULL, NULL }

#define CONTROL_ALL(id, label, alias, units, min, max, dfl, step) \
    { id, label, alias, units, R_CONTROL, F_LOWER | F_UPPER | F_STEP, min, max, dfl, step, NULL, NULL, NULL }
#define CONTROL(id, label, alias, units, limits) \
    CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, limits ## _DFL, limits ## _STEP)
#define CONTROL_DFL(id, label, alias, units, limits, dfl) \
    CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, dfl, limits ## _STEP)

#define LOW_CONTROL_ALL(id, label, alias, units, min, max, dfl, step) \
    { id, label, NULL, units, R_CONTROL, F_LOWER | F_UPPER | F_STEP | F_LOWERING, min, max, dfl, step, NULL, NULL, NULL }
#define LOW_CONTROL(id, label, alias, units, limits) \
    LOW_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, limits ## _DFL, limits ## _STEP)
#define LOW_CONTROL_DFL(id, label, alias, units, limits, dfl) \
    LOW_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, dfl, limits ## _STEP)

#define INT_CONTROL_ALL(id, label, alias, units, min, max, dfl, step) \
    { id, label, NULL, units, R_CONTROL, F_LOWER | F_UPPER | F_STEP | F_INT, min, max, dfl, step, NULL, NULL, NULL }
#define INT_CONTROL(id, label, alias, units, limits) \
    INT_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, limits ## _DFL, limits ## _STEP)
#define INT_CONTROL_DFL(id, label, alias, units, limits, dfl) \
    INT_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, dfl, limits ## _STEP)

#define LOG_CONTROL_ALL(id, label, alias, units, min, max, dfl, step) \
    { id, label, alias, units, R_CONTROL, F_LOWER | F_UPPER | F_STEP | F_LOG, min, max, dfl, step, NULL, NULL, NULL }
#define LOG_CONTROL(id, label, alias, units, limits) \
    LOG_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, limits ## _DFL, limits ## _STEP)
#define LOG_CONTROL_DFL(id, label, alias, units, limits, dfl) \
    LOG_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, dfl, limits ## _STEP)

#define EXT_LOG_CONTROL_ALL(id, label, alias, units, min, max, dfl, step) \
    { id, label, alias, units, R_CONTROL, F_LOWER | F_UPPER | F_STEP | F_LOG | F_EXT, min, max, dfl, step, NULL, NULL, NULL }
#define EXT_LOG_CONTROL(id, label, alias, units, limits) \
    EXT_LOG_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, limits ## _DFL, limits ## _STEP)
#define EXT_LOG_CONTROL_DFL(id, label, alias, units, limits, dfl) \
    EXT_LOG_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, dfl, limits ## _STEP)

#define CYC_CONTROL_ALL(id, label, alias, units, min, max, dfl, step) \
    { id, label, alias, units, R_CONTROL, F_LOWER | F_UPPER | F_STEP | F_CYCLIC, min, max, dfl, step, NULL, NULL, NULL }
#define CYC_CONTROL(id, label, alias, units, limits) \
    CYC_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, limits ## _DFL, limits ## _STEP)
#define CYC_CONTROL_DFL(id, label, alias, units, limits, dfl) \
    CYC_CONTROL_ALL(id, label, alias, units, limits ## _MIN, limits ## _MAX, dfl, limits ## _STEP)

#define AMP_GAIN(id, label, alias, dfl, max) \
    LOG_CONTROL_ALL(id, label, alias, U_GAIN_AMP, 0.0f, max, dfl, GAIN_AMP_S_0_5_DB)
#define AMP_GAIN_RANGE(id, label, alias, dfl, min, max) \
    LOG_CONTROL_ALL(id, label, alias, U_GAIN_AMP, min, max, dfl, GAIN_AMP_S_0_5_DB)
#define AMP_GAIN1(id, label, alias, dfl)        AMP_GAIN(id, label, alias, dfl, 1.0f)
#define AMP_GAIN10(id, label, alias, dfl)       AMP_GAIN(id, label, alias, dfl, 10.0f)
#define AMP_GAIN100(id, label, alias, dfl)      AMP_GAIN(id, label, alias, dfl, 100.0f)
#define AMP_GAIN1000(id, label, alias, dfl)     AMP_GAIN(id, label, alias, dfl, 1000.0f)
#define AMP_GAIN10000(id, label, alias, dfl)    AMP_GAIN(id, label, alias, dfl, 10000.0f)

#define PAN_CTL(id, label, alias, dfl) \
    CONTROL_ALL(id, label, alias, U_PERCENT, -100.0f, 100.0f, dfl, 0.1f)
#define PERCENTS(id, label, alias, dfl, step) \
    CONTROL_ALL(id, label, alias, U_PERCENT, 0.0f, 100.0f, dfl, step)

#define HUE_CTL(id, label, dfl) \
    { id, label, NULL, U_NONE, R_CONTROL, F_UPPER | F_LOWER | F_STEP | F_CYCLIC, 0.0f, 1.0f, (dfl), 0.25f/360.0f, NULL, NULL     }

#define UNLIMITED_METER(id, label, units, dfl) \
    { id, label, NULL, units, R_METER, 0, 0.0f, 0.0f, dfl, 0.0f, NULL, NULL, NULL }
#define METER(id, label, units, limits) \
    { id, label, NULL, units, R_METER, F_LOWER | F_UPPER | F_STEP, limits ## _MIN, limits ## _MAX, limits ## _DFL, limits ## _STEP, NULL, NULL, NULL }
#define METERZ(id, label, units, limits) \
    { id, label, NULL, units, R_METER, F_LOWER | F_UPPER | F_STEP, limits ## _MIN, limits ## _MAX, 0.0f, 0.0f, NULL, NULL, NULL }
#define METER_MINMAX(id, label, units, min, max) \
    { id, label, NULL, units, R_METER, F_LOWER | F_UPPER | F_STEP, min, max, min, 0.0f, NULL, NULL, NULL }

#define INT_METER_UNLIMITED(id, label, units, dfl) \
    { id, label, NULL, units, R_METER, F_INT, 0.0f, 0.0f, dfl, 0.0f, NULL, NULL, NULL }
#define INT_METER_ALL(id, label, units, min, max, dfl, step) \
    { id, label, NULL, units, R_METER, F_LOWER | F_UPPER | F_STEP | F_INT, min, max, dfl, step, NULL, NULL, NULL }
#define INT_METER(id, label, units, limits) \
    INT_METER_ALL(id, label, units, limits ## _MIN, limits ## _MAX, limits ## _DFL, limits ## _STEP)

#define ENUM_METER(id, label, dfl, list) \
    { id, label, NULL, U_ENUM, R_METER, F_INT, 0.0f, 0.0f, dfl, 0.0f, list, NULL, NULL }

#define PORT_SET(id, label, keys, ports)  \
    { id, label, NULL, U_ENUM, R_PORT_SET, 0, 0, 0, 0, 0, keys, ports, NULL }
#define OUT_PERCENTS(id, label) \
    { id, label, NULL, U_PERCENT, R_METER, F_LOWER | F_UPPER, 0, 100, 0, 0, NULL, NULL }
#define METER_GAIN(id, label, max) \
    { id, label, NULL, U_GAIN_AMP, R_METER, F_LOG | F_UPPER | F_LOWER | F_PEAK, 0, max, 0.0f, 0, NULL, NULL, NULL }
#define METER_GAIN_DFL(id, label, max, dfl) \
    { id, label, NULL, U_GAIN_AMP, R_METER, F_LOG | F_UPPER | F_LOWER | F_PEAK, 0, max, dfl, 0, NULL, NULL, NULL }
#define METER_OUT_GAIN(id, label, max) \
    { id, label, NULL, U_GAIN_AMP, R_METER, F_LOG | F_UPPER | F_LOWER, 0, max, 0.0f, 0, NULL, NULL, NULL }
#define LUFS_METER(id, label, max) \
    { id, label, NULL, U_LUFS, R_METER, F_UPPER | F_LOWER, -72.0f, max, -72.0f, 0, NULL, NULL, NULL }
#define METER_GAIN10(id, label)                 METER_GAIN(id, label, 10.0f)
#define METER_GAIN20(id, label)                 METER_GAIN(id, label, 20.0f)
#define METER_PERCENT(id, label)                { id, label, NULL, U_PERCENT, R_METER, F_UPPER | F_LOWER, 0.0f, 100.0f, 0.0f, 0.1f, NULL, NULL, NULL }

#define STRING(id, label, length)               { id, label, NULL, U_NONE, R_STRING, F_LOWER | F_UPPER, 0, length, 0, 0, NULL, NULL, "" }
#define STRING_DFL(id, label, length, text)     { id, label, NULL, U_NONE, R_STRING, F_LOWER | F_UPPER, 0, length, 0, 0, NULL, NULL, text }
#define OPT_STRING(id, label, length)           { id, label, NULL, U_NONE, R_STRING, F_LOWER | F_UPPER | F_OPTIONAL, 0, length, 0, 0, NULL, NULL, "" }
#define OPT_STRING_DFL(id, label, length, text) { id, label, NULL, U_NONE, R_STRING, F_LOWER | F_UPPER | F_OPTIONAL, 0, length, 0, 0, NULL, NULL, text }

#define SEND_NAME(id, label)                    { id, label, NULL, U_NONE, R_SEND_NAME, F_LOWER | F_UPPER, 0, MAX_SHM_SEGMENT_NAME_BYTES, 0, 0, NULL, NULL, "" }
#define OPT_SEND_NAME(id, label)                { id, label, NULL, U_NONE, R_SEND_NAME, F_LOWER | F_UPPER | F_OPTIONAL, 0, MAX_SHM_SEGMENT_NAME_BYTES, 0, 0, NULL, NULL, "" }
#define RETURN_NAME(id, label)                  { id, label, NULL, U_NONE, R_RETURN_NAME, F_LOWER | F_UPPER, 0, MAX_SHM_SEGMENT_NAME_BYTES, 0, 0, NULL, NULL, "" }
#define OPT_RETURN_NAME(id, label)              { id, label, NULL, U_NONE, R_RETURN_NAME, F_LOWER | F_UPPER | F_OPTIONAL, 0, MAX_SHM_SEGMENT_NAME_BYTES, 0, 0, NULL, NULL, "" }

#define AUDIO_SEND(id, label, index, group) \
    { id, label, NULL, U_NONE, R_AUDIO_SEND, 0, 0, 0, index, 0, NULL, NULL, group             }
#define OPT_AUDIO_SEND(id, label, index, group) \
    { id, label, NULL, U_NONE, R_AUDIO_SEND, F_OPTIONAL, 0, 0, index, 0, NULL, NULL, group    }
#define AUDIO_RETURN(id, label, index, group) \
    { id, label, NULL, U_NONE, R_AUDIO_RETURN, 0, 0, 0, index, 0, NULL, NULL, group           }
#define OPT_AUDIO_RETURN(id, label, index, group) \
    { id, label, NULL, U_NONE, R_AUDIO_RETURN, F_OPTIONAL, 0, 0, index, 0, NULL, NULL, group  }

#define SEND_MONO(id, channel, comment) \
    SEND_NAME(id, comment), \
    AUDIO_SEND(channel, comment " input", 0, id)
#define RETURN_MONO(id, channel, comment) \
    RETURN_NAME(id, comment), \
    AUDIO_RETURN(channel, comment " output", 0, id)

#define SEND_STEREO(id, channel, comment) \
    SEND_NAME(id, comment), \
    AUDIO_SEND(channel "l", comment " output Left", 0, id), \
    AUDIO_SEND(channel "r", comment " output Right", 1, id)
#define RETURN_STEREO(id, channel, comment) \
    RETURN_NAME(id, comment), \
    AUDIO_RETURN(channel "l", comment " input Left", 0, id), \
    AUDIO_RETURN(channel "r", comment " input Right", 1, id)

#define OPT_SEND_MONO(id, channel, comment) \
    OPT_SEND_NAME(id, comment), \
    OPT_AUDIO_SEND(channel, comment " input", 0, id)
#define OPT_RETURN_MONO(id, channel, comment) \
    OPT_RETURN_NAME(id, comment), \
    OPT_AUDIO_RETURN(channel, comment " input", 0, id)

#define OPT_SEND_STEREO(id, channel, comment) \
    OPT_SEND_NAME(id, comment), \
    OPT_AUDIO_SEND(channel "l", comment " input Left", 0, id), \
    OPT_AUDIO_SEND(channel "r", comment " input Right", 1, id)
#define OPT_RETURN_STEREO(id, channel, comment) \
    OPT_RETURN_NAME(id, comment), \
    OPT_AUDIO_RETURN(channel "l", comment " input Left", 0, id), \
    OPT_AUDIO_RETURN(channel "r", comment " input Right", 1, id)

#define PORTS_END   \
    { NULL, NULL, NULL }

// Reduced ports
#define AUDIO_INPUT_MONO    AUDIO_INPUT(PORT_NAME_INPUT, "Input")
#define AUDIO_INPUT_LEFT    AUDIO_INPUT(PORT_NAME_INPUT_L, "Input L")
#define AUDIO_INPUT_RIGHT   AUDIO_INPUT(PORT_NAME_INPUT_R, "Input R")
#define AUDIO_INPUT_A       AUDIO_INPUT("in_a", "Input A")
#define AUDIO_INPUT_B       AUDIO_INPUT("in_b", "Input B")
#define AUDIO_INPUT_N(n)    AUDIO_INPUT("in" #n, "Input " #n)

#define AUDIO_OUTPUT_MONO   AUDIO_OUTPUT(PORT_NAME_OUTPUT, "Output")
#define AUDIO_OUTPUT_LEFT   AUDIO_OUTPUT(PORT_NAME_OUTPUT_L, "Output L")
#define AUDIO_OUTPUT_RIGHT  AUDIO_OUTPUT(PORT_NAME_OUTPUT_R, "Output R")
#define AUDIO_OUTPUT_A      AUDIO_OUTPUT("out_a", "Output A")
#define AUDIO_OUTPUT_B      AUDIO_OUTPUT("out_b", "Output B")
#define AUDIO_OUTPUT_N(n)   AUDIO_OUTPUT("out" #n, "Output " #n)

#define AUDIO_INPUT_STEREO \
    AUDIO_INPUT_LEFT, \
    AUDIO_INPUT_RIGHT

#define AUDIO_OUTPUT_STEREO \
    AUDIO_OUTPUT_LEFT, \
    AUDIO_OUTPUT_RIGHT

#define MIDI_INPUT          MIDI_IN_CHANNEL(PORT_NAME_MIDI_INPUT, "Midi input")
#define MIDI_OUTPUT         MIDI_OUT_CHANNEL(PORT_NAME_MIDI_OUTPUT, "Midi output")

#define OSC_INPUT           OSC_IN_CHANNEL(PORT_NAME_OSC_INPUT, "OSC input")
#define OSC_OUTPUT          OSC_OUT_CHANNEL(PORT_NAME_OSC_OUTPUT, "OSC output")

#define IN_GAIN             AMP_GAIN("g_in", "Input gain", "Input gain", GAIN_AMP_0_DB, GAIN_AMP_P_60_DB)
#define OUT_GAIN            AMP_GAIN("g_out", "Output gain", "Output gain", GAIN_AMP_0_DB, GAIN_AMP_P_60_DB)

#define DRY_GAIN(g)         AMP_GAIN10("dry", "Dry amount", "Dry amount", g)
#define DRY_GAIN_L(g)       AMP_GAIN10("dry_l", "Dry amount Left", "Dry amount L", g)
#define DRY_GAIN_R(g)       AMP_GAIN10("dry_r", "Dry amount Right", "Dry amount R", g)

#define WET_GAIN(g)         AMP_GAIN10("wet", "Wet amount", "Wet amount", g)
#define WET_GAIN_L(g)       AMP_GAIN10("wet_l", "Wet amount Left", "Wet amount L", g)
#define WET_GAIN_R(g)       AMP_GAIN10("wet_r", "Wet amount Right", "Wet amount R", g)

#define DRYWET(perc)        PERCENTS("drywet", "Dry/Wet balance", "Dry/Wet", perc, 0.1f)
#define DRYWET_L(perc)      PERCENTS("dwmix_l", "Dry/Wet balance Left", "Dry/Wet L", perc, 0.1f)
#define DRYWET_R(perc)      PERCENTS("dwmix_r", "Dry/Wet balance Right", "Dry/Wet R", perc, 0.1f)

#define BYPASS              { PORT_NAME_BYPASS, "Bypass", NULL, U_BOOL, R_BYPASS, F_UPPER | F_LOWER, 0, 1, 0, 0, NULL, NULL, NULL }


// Port configurations
#define PORTS_MONO_PLUGIN   \
    AUDIO_INPUT_MONO,       \
    AUDIO_OUTPUT_MONO       \

#define PORTS_MONO_SIDECHAIN        AUDIO_INPUT(PORT_NAME_SIDECHAIN, "Sidechain input")
#define PORTS_STEREO_SIDECHAIN      AUDIO_INPUT(PORT_NAME_SIDECHAIN_L, "Sidechain input Left"), AUDIO_INPUT(PORT_NAME_SIDECHAIN_R, "Sidechain input Right")

#define PORTS_STEREO_PLUGIN \
    AUDIO_INPUT_STEREO, \
    AUDIO_OUTPUT_STEREO

#define PORTS_MIDI_CHANNEL  \
    MIDI_INPUT,             \
    MIDI_OUTPUT

#define PORTS_OSC_CHANNEL   \
    OSC_INPUT,              \
    OSC_OUTPUT

// Port groups
#define MAIN_MONO_IN_PORT_GROUP \
    { "mono_in",        "Mono Input",       GRP_MONO,       PGF_IN | PGF_MAIN,          mono_in_group_ports         }

#define MAIN_MONO_OUT_PORT_GROUP \
    { "mono_out",       "Mono Output",      GRP_MONO,       PGF_OUT | PGF_MAIN,         mono_out_group_ports        }

#define MAIN_MONO_SC_IN_PORT_GROUP \
    { "sidechain_in",   "Sidechain Input",  GRP_MONO,       PGF_IN | PGF_SIDECHAIN,     mono_sidechain_group_ports, "mono_in"  }

#define MAIN_STEREO_IN_PORT_GROUP \
    { "stereo_in",      "Stereo Input",     GRP_STEREO,     PGF_IN | PGF_MAIN,          stereo_in_group_ports       }

#define MAIN_STEREO_OUT_PORT_GROUP \
    { "stereo_out",     "Stereo Output",    GRP_STEREO,     PGF_OUT | PGF_MAIN,         stereo_out_group_ports      }

#define MAIN_STEREO_SC_IN_PORT_GROUP \
    { "sidechain_in",   "Sidechain Input",  GRP_STEREO,     PGF_IN | PGF_SIDECHAIN,     stereo_sidechain_group_portss, "stereo_in" }

#define MAIN_MONO_PORT_GROUPS \
    MAIN_MONO_IN_PORT_GROUP, \
    MAIN_MONO_OUT_PORT_GROUP

#define MAIN_SC_MONO_PORT_GROUPS \
    MAIN_MONO_IN_PORT_GROUP, \
    MAIN_MONO_OUT_PORT_GROUP, \
    MAIN_MONO_SC_IN_PORT_GROUP

#define MAIN_MONO2STEREO_PORT_GROUPS \
    MAIN_MONO_IN_PORT_GROUP, \
    MAIN_STEREO_OUT_PORT_GROUP

#define MAIN_STEREO_PORT_GROUPS \
    MAIN_STEREO_IN_PORT_GROUP, \
    MAIN_STEREO_OUT_PORT_GROUP

#define MAIN_SC_STEREO_PORT_GROUPS \
    MAIN_STEREO_IN_PORT_GROUP, \
    MAIN_STEREO_OUT_PORT_GROUP, \
    MAIN_STEREO_SC_IN_PORT_GROUP

#define MONO_PORT_GROUP_PORT(id, a) \
    static const port_group_item_t id ## _ports[] = \
    { \
        { a, PGR_CENTER     }, \
        { NULL              } \
    }

#define STEREO_PORT_GROUP_PORTS(id, a, b) \
    static const port_group_item_t id ## _ports[] = \
    { \
        { a, PGR_LEFT       }, \
        { b, PGR_RIGHT      }, \
        { NULL } \
    }

#define MS_PORT_GROUP_PORTS(id, a, b) \
    static const port_group_item_t id ## _ports[] = \
    { \
        { a, PGR_MS_MIDDLE  }, \
        { b, PGR_MS_SIDE    }, \
        { NULL } \
    }

#define PORT_GROUPS_END     { NULL, NULL }

namespace lsp
{
    namespace meta
    {
        // Common stereo port names
        extern const char PORT_NAME_BYPASS[];
        extern const char PORT_NAME_INPUT[];
        extern const char PORT_NAME_OUTPUT[];
        extern const char PORT_NAME_MIDI_INPUT[];
        extern const char PORT_NAME_MIDI_OUTPUT[];
        extern const char PORT_NAME_OSC_INPUT[];
        extern const char PORT_NAME_OSC_OUTPUT[];
        extern const char PORT_NAME_SIDECHAIN[];
        extern const char PORT_NAME_INPUT_L[];
        extern const char PORT_NAME_INPUT_R[];
        extern const char PORT_NAME_OUTPUT_L[];
        extern const char PORT_NAME_OUTPUT_R[];
        extern const char PORT_NAME_SIDECHAIN_L[];
        extern const char PORT_NAME_SIDECHAIN_R[];

        // Port groups
        extern const port_group_item_t mono_in_group_ports[];
        extern const port_group_item_t mono_sidechain_group_ports[];
        extern const port_group_item_t mono_out_group_ports[];

        extern const port_group_item_t stereo_in_group_ports[];
        extern const port_group_item_t stereo_sidechain_group_ports[];
        extern const port_group_item_t stereo_out_group_ports[];

        extern const port_group_t mono_plugin_port_groups[];
        extern const port_group_t mono_plugin_sidechain_port_groups[];

        extern const port_group_t mono_to_stereo_plugin_port_groups[];
        extern const port_group_t stereo_plugin_port_groups[];
        extern const port_group_t stereo_plugin_sidechain_port_groups[];

        // Miscellaneous lists
        extern const port_item_t file_channels[];
        extern const port_item_t midi_channels[];
        extern const port_item_t octaves[];
        extern const port_item_t notes[];
        extern const port_item_t fft_windows[];
        extern const port_item_t fft_envelopes[];

    } /* namespace meta */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_META_PORTS_H_ */
