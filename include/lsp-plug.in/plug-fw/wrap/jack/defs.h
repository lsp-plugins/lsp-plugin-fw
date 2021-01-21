/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/types.h>

#define JACK_MAIN_FUNCTION          plug_fw_jack_main
#define JACK_MAIN_FUNCTION_NAME     "plug_fw_jack_main"

namespace lsp
{
    typedef int (* jack_main_function_t)(const char *plugin_id, int argc, const char **argv);
}

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    LSP_CSYMBOL_EXPORT
    int JACK_MAIN_FUNCTION(const char *plugin_id, int argc, const char **argv);

#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_DEFS_H_ */
