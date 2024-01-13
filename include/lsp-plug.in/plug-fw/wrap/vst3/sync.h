/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 13 янв. 2024 г.
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

#ifndef _LSP_PLUG_IN_PLUG_FW_WRAP_VST3_SYNC_H_
#define _LSP_PLUG_IN_PLUG_FW_WRAP_VST3_SYNC_H_

#include <lsp-plug.in/plug-fw/version.h>

namespace lsp
{
    namespace vst3
    {
        /**
         * Data synchronization callback interface
         */
        class IDataSync
        {
            public:
                virtual ~IDataSync() = default;

            public:
                /**
                 * Callback for synchronizing data
                 */
                virtual void sync_data() = 0;
        };
    } /* namespace vst3 */
} /* namespace lsp */


#endif /* _LSP_PLUG_IN_PLUG_FW_WRAP_VST3_SYNC_H_ */
