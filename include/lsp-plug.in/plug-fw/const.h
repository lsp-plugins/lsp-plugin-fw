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

#ifndef LSP_PLUG_IN_PLUG_FW_CONST_H_
#define LSP_PLUG_IN_PLUG_FW_CONST_H_

#define BPM_MIN                             1.0f                /* Minimum BPM                                      */
#define BPM_MAX                             1000.0f             /* Maximum BPM                                      */
#define BPM_DEFAULT                         120.0f              /* Default BPM                                      */
#define DEFAULT_TICKS_PER_BEAT              1920.0f             /* Default tick per beat resolution                 */
#define MIDI_EVENTS_MAX                     4096                /* Maximum number of MIDI events per buffer         */

#endif /* LSP_PLUG_IN_PLUG_FW_CONST_H_ */
