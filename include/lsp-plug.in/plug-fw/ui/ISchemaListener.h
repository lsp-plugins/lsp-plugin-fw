/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 21 сент. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_ISCHEMALISTENER_H_
#define LSP_PLUG_IN_PLUG_FW_UI_ISCHEMALISTENER_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>

namespace lsp
{
    namespace ui
    {
        class IPort;

        /**
         * Port listener
         */
        class ISchemaListener
        {
            public:
                explicit ISchemaListener();
                virtual ~ISchemaListener();

            public:
                /**
                 * Is called when the schema becomes reloaded
                 * @param port port that caused the change
                 */
                virtual void    reloaded(const tk::StyleSheet *sheet);
        };

    } /* namespace tk */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_UI_ISCHEMALISTENER_H_ */
