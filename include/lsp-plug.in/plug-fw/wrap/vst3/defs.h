/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 янв. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DEFS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DEFS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>

#if (defined(PLATFORM_WINDOWS)) || (defined(PLATFORM_MACOSX))
    #define IF_VST_RUNLOOP_IFACE(...)
#else
    #define VST_USE_RUNLOOP_IFACE
    #define IF_VST_RUNLOOP_IFACE(...)       __VA_ARGS__
#endif


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DEFS_H_ */
