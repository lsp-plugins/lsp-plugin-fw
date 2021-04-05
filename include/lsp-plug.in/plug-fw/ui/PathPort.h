/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_PATHPORT_H_
#define LSP_PLUG_IN_PLUG_FW_UI_PATHPORT_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

namespace lsp
{
    namespace ui
    {
        class PathPort: public IPort
        {
            protected:
                char            sPath[PATH_MAX];
                IWrapper       *pWrapper;

            public:
                explicit PathPort(const meta::port_t *meta, IWrapper *wrapper);
                virtual ~PathPort();

            public:
                virtual void write(const void* buffer, size_t size);

                virtual void *get_buffer();
        };

    } /* namespace ctl */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UI_PATHPORT_H_ */
