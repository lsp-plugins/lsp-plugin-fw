/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 февр. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UTIL_VST3_MODINFO_VST3_MODINFO_H_
#define LSP_PLUG_IN_PLUG_FW_UTIL_VST3_MODINFO_VST3_MODINFO_H_

#include <lsp-plug.in/plug-fw/version.h>

namespace lsp
{
    namespace vst3_modinfo
    {

        /**
         * Execute main function of the utility
         * @param argc number of arguments
         * @param argv list of arguments
         * @return status of operation
         */
        int main(int argc, const char **argv);

    } /* namespace vst3_modinfo */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_UTIL_VST3_MODINFO_VST3_MODINFO_H_ */
