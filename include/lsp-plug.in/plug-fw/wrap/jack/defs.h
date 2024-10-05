/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 22 янв. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_DEFS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_DEFS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>

#define JACK_CREATE_PLUGIN_LOOP         jack_create_plugin_loop
#define JACK_CREATE_PLUGIN_LOOP_NAME    LSP_STRINGIFY(JACK_CREATE_PLUGIN_LOOP)

namespace lsp
{
    /**
     * Plugin loop interface
     */
    struct IPluginLoop
    {
        /**
         * Destroy plugin loop
         */
        virtual ~IPluginLoop();

        /**
         * Enter main plugin loop
         */
        virtual status_t run() = 0;

        /**
         * Cancel main plugin loop
         */
        virtual void cancel() = 0;
    };

    /**
     * Create run loop
     * @param loop pointer to store loop handle
     * @param plugin_id plugin identifier string
     * @param argc number of additional arguments
     * @param argv list of additional arguments
     * @return status of operation
     */
    typedef status_t (* jack_create_plugin_loop_t)(IPluginLoop **loop, const char *plugin_id, int argc, const char **argv);
}

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    LSP_EXPORT_MODIFIER
    int JACK_CREATE_PLUGIN_LOOP(lsp::IPluginLoop **loop, const char *plugin_id, int argc, const char **argv);


#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_DEFS_H_ */
