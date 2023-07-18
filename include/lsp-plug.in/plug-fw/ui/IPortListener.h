/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IPORTLISTENER_H_
#define LSP_PLUG_IN_PLUG_FW_UI_IPORTLISTENER_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>

namespace lsp
{
    namespace ui
    {
        class IPort;

        enum notify_flags_t
        {
            PORT_NONE       = 0,
            PORT_USER_EDIT  = 1 << 0
        };

        /**
         * Port listener
         */
        class IPortListener
        {
            public:
                explicit IPortListener();
                virtual ~IPortListener();

            public:
                /**
                 * Is called when the port value has been changed
                 * @param port port that caused the change
                 * @param edit indicates that the parameter is changed within the user's edit action, @see notify_flags_t
                 */
                virtual void notify(IPort *port, size_t flags);

                /**
                 * Is called when the metadata of port has been changed
                 * @param port port that caused the change
                 */
                virtual void sync_metadata(IPort *port);
        };

    } /* namespace ui */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UI_IPORTLISTENER_H_ */
