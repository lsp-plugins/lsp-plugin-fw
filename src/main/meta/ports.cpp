/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/plug-fw/meta/ports.h>

namespace lsp
{
    namespace meta
    {
        //-------------------------------------------------------------------------
        // Common port name definitions
        const char PORT_NAME_BYPASS[]           = "bypass";

        const char PORT_NAME_INPUT[]            = "in";
        const char PORT_NAME_OUTPUT[]           = "out";
        const char PORT_NAME_MIDI_INPUT[]       = "in_midi";
        const char PORT_NAME_MIDI_OUTPUT[]      = "out_midi";
        const char PORT_NAME_OSC_INPUT[]        = "in_osc";
        const char PORT_NAME_OSC_OUTPUT[]       = "out_osc";
        const char PORT_NAME_SIDECHAIN[]        = "sc";

        const char PORT_NAME_INPUT_L[]          = "in_l";
        const char PORT_NAME_INPUT_R[]          = "in_r";
        const char PORT_NAME_OUTPUT_L[]         = "out_l";
        const char PORT_NAME_OUTPUT_R[]         = "out_r";
        const char PORT_NAME_SIDECHAIN_L[]      = "sc_l";
        const char PORT_NAME_SIDECHAIN_R[]      = "sc_r";

        // Port groups
        const port_group_item_t mono_in_group_ports[] =
        {
            { PORT_NAME_INPUT,          PGR_CENTER      },
            { NULL }
        };

        const port_group_item_t mono_sidechain_group_ports[] =
        {
            { PORT_NAME_SIDECHAIN,      PGR_CENTER      },
            { NULL }
        };

        const port_group_item_t mono_out_group_ports[] =
        {
            { PORT_NAME_OUTPUT,         PGR_CENTER      },
            { NULL }
        };

        const port_group_item_t stereo_in_group_ports[] =
        {
            { PORT_NAME_INPUT_L,        PGR_LEFT        },
            { PORT_NAME_INPUT_R,        PGR_RIGHT       },
            { NULL }
        };

        const port_group_item_t stereo_sidechain_group_portss[] =
        {
            { PORT_NAME_SIDECHAIN_L,    PGR_LEFT        },
            { PORT_NAME_SIDECHAIN_R,    PGR_RIGHT       },
            { NULL }
        };

        const port_group_item_t stereo_out_group_ports[] =
        {
            { PORT_NAME_OUTPUT_L,       PGR_LEFT        },
            { PORT_NAME_OUTPUT_R,       PGR_RIGHT       },
            { NULL }
        };

        const port_group_t mono_plugin_port_groups[] =
        {
            MAIN_MONO_PORT_GROUPS,
            PORT_GROUPS_END
        };

        const port_group_t mono_plugin_sidechain_port_groups[] =
        {
            MAIN_SC_MONO_PORT_GROUPS,
            PORT_GROUPS_END
        };

        const port_group_t mono_to_stereo_plugin_port_groups[] =
        {
            MAIN_MONO2STEREO_PORT_GROUPS,
            PORT_GROUPS_END
        };

        const port_group_t stereo_plugin_port_groups[] =
        {
            MAIN_STEREO_PORT_GROUPS,
            PORT_GROUPS_END
        };

        const port_group_t stereo_plugin_sidechain_port_groups[] =
        {
            MAIN_SC_STEREO_PORT_GROUPS,
            PORT_GROUPS_END
        };

        //-------------------------------------------------------------------------
        // Miscellaneous lists
        const port_item_t file_channels[] =
        {
            { "1", NULL },
            { "2", NULL },
            { "3", NULL },
            { "4", NULL },
            { "5", NULL },
            { "6", NULL },
            { "7", NULL },
            { "8", NULL },
            { NULL, NULL }
        };

        const port_item_t midi_channels[] =
        {
            { "01", NULL },
            { "02", NULL },
            { "03", NULL },
            { "04", NULL },
            { "05", NULL },
            { "06", NULL },
            { "07", NULL },
            { "08", NULL },
            { "09", NULL },
            { "10", NULL },
            { "11", NULL },
            { "12", NULL },
            { "13", NULL },
            { "14", NULL },
            { "15", NULL },
            { "16", NULL },
            { NULL, NULL }
        };

        const port_item_t octaves[] =
        {
            { "-1", NULL },
            { "0", NULL },
            { "1", NULL },
            { "2", NULL },
            { "3", NULL },
            { "4", NULL },
            { "5", NULL },
            { "6", NULL },
            { "7", NULL },
            { "8", NULL },
            { "9", NULL },
            { NULL, NULL }
        };

        const port_item_t notes[] =
        {
            { "C",  NULL },
            { "C#", NULL },
            { "D",  NULL },
            { "D#", NULL },
            { "E",  NULL },
            { "F",  NULL },
            { "F#", NULL },
            { "G",  NULL },
            { "G#", NULL },
            { "A",  NULL },
            { "A#", NULL },
            { "B",  NULL },
            { NULL, NULL }
        };

        const port_item_t fft_windows[] =
        {
            { "Hann",                   "fft.wnd.hann" },
            { "Hamming",                "fft.wnd.hamming" },
            { "Blackman",               "fft.wnd.blackman" },
            { "Lanczos",                "fft.wnd.lanczos" },
            { "Gaussian",               "fft.wnd.gauss" },
            { "Poisson",                "fft.wnd.poisson" },
            { "Parzen",                 "fft.wnd.parzen" },
            { "Tukey",                  "fft.wnd.tukey" },
            { "Welch",                  "fft.wnd.welch" },
            { "Nuttall",                "fft.wnd.nuttall" },
            { "Blackman-Nuttall",       "fft.wnd.blackman_nuttall" },
            { "Blackman-Harris",        "fft.wnd.blackman_harris" },
            { "Hann-Poisson",           "fft.wnd.hann_poisson" },
            { "Bartlett-Hann",          "fft.wnd.bartlett_hann" },
            { "Bartlett-Fejer",         "fft.wnd.bartlett_fejer" },
            { "Triangular",             "fft.wnd.triangular" },
            { "Rectangular",            "fft.wnd.rectangular" },
            { "Flat top",               "fft.wnd.flat_top" },
            { "Cosine",                 "fft.wnd.cosine" },
            { "Squared Cosine",         "fft.wnd.sqr_cosine" },
            { "Cubic",                  "fft.wnd.cubic" },
            { NULL, NULL }
        };

        const port_item_t fft_envelopes[] =
        {
            { "Violet noise",           "fft.env.violet" },
            { "Blue noise",             "fft.env.blue" },
            { "White noise",            "fft.env.white" },
            { "Pink noise",             "fft.env.pink" },
            { "Brown noise",            "fft.env.brown" },
            { "4.5 dB/oct fall-off",    "fft.env.falloff_4_5db" },
            { "4.5 dB/oct raise",       "fft.env.raise_4_5db" },
            { NULL, NULL }
        };
    }
}


