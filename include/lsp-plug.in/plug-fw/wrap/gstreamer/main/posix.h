/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_MAIN_POSIX_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_MAIN_POSIX_H_

#ifndef LSP_PLUG_IN_GSTREAMER_MAIN_IMPL
    #error "This header should not be included directly"
#endif /* LSP_PLUG_IN_GSTREAMER_MAIN_IMPL */

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/static.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/factory.h>

#include <dlfcn.h>

namespace lsp::gst
{
    static void *instance               = NULL;
    static Factory *factory             = NULL;

    void free_core_library()
    {
        if (instance != NULL)
        {
            dlclose(instance);
            instance = NULL;
            factory = NULL;
        }
    }

    static StaticFinalizer finalizer(free_core_library);

    static Factory *get_factory()
    {
        // TODO
        return factory;
    }
} // namespace lsp::gst;

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_MAIN_POSIX_H_ */
