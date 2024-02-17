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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_STATIC_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_STATIC_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/types.h>

namespace lsp
{
    namespace lv2
    {
        // Latency ports' metadata for LV2 plugins
        static const meta::port_t latency_port =
        {
            LSP_LV2_LATENCY_PORT, "Latency OUT", meta::U_NONE, meta::R_METER, meta::F_INT | meta::F_LOWER | meta::F_UPPER, 0, MAX_SAMPLE_RATE, 0, 0, NULL
        };

        // Atom ports' metadata for LV2 plugins
        static const meta::port_t atom_ports[] =
        {
            // Input and output ATOM ports
            { LSP_LV2_ATOM_PORT_IN,     "UI Input",     meta::U_NONE,         meta::R_AUDIO_IN,  0, 0, 0, 0, 0, NULL       },
            { LSP_LV2_ATOM_PORT_OUT,    "UI Output",    meta::U_NONE,         meta::R_AUDIO_OUT, 0, 0, 0, 0, 0, NULL       },

            { NULL, NULL }
        };

    } /* namespace lv2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_STATIC_H_ */
