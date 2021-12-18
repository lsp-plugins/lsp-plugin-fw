/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_VST2_MAIN_IMPL
    #error "This header should not be included directly"
#endif /* LSP_PLUG_IN_VST2_MAIN_IMPL */

// Do not use tracefile because this file does not use jack-core
#ifdef LSP_TRACEFILE
    #undef LSP_TRACEFILE
#endif /* LSP_TRACEFILE */

#ifndef VST2_PLUGIN_UID
    #error "Plugin metadata identifier not defined"
#endif /* JACK_PLUGIN_UID */

#ifdef LSP_INSTALL_PREFIX
    #define LSP_LIB_PREFIX(x)       LSP_INSTALL_PREFIX x,
#else
    #define LSP_LIB_PREFIX(x)
#endif /* PREFIX */

#if defined(PLATFORM_WINDOWS)
    #include <lsp-plug.in/plug-fw/wrap/vst2/main/winnt.h>
#else
    #include <lsp-plug.in/plug-fw/wrap/vst2/main/posix.h>
#endif /* PLATFORM_WINDOWS */

// This should be included to generate other VST stuff
#include <lsp-plug.in/3rdparty/steinberg/vst2main.h>


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_H_ */
