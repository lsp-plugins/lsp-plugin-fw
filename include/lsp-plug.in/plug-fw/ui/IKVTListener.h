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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IKVTLISTENER_H_
#define LSP_PLUG_IN_PLUG_FW_UI_IKVTLISTENER_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>

namespace lsp
{
    namespace ui
    {
        /**
         * KVT listener
         */
        class IKVTListener
        {
            public:
                explicit IKVTListener();
                virtual ~IKVTListener();

            public:
                /**
                 * Handle change of the KVT paramter
                 * @param kvt KVT storage
                 * @param id KVT parameter full path identifier
                 * @param value actual KVT parameter value
                 * @return true if listener processed the message, false if ignored
                 */
                virtual bool        changed(core::KVTStorage *kvt, const char *id, const core::kvt_param_t *value);
        };

    } /* namespace tk */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UI_IKVTLISTENER_H_ */
