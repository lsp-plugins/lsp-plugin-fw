/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 июн. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IREGISTRY_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_IREGISTRY_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/parray.h>

namespace lsp
{
    namespace ctl
    {
        class Widget;

        /**
         * Registry for controllers
         */
        class Registry
        {
            private:
                Registry & operator = (const Registry &);
                Registry(const Registry &);

            protected:
                lltl::parray<Widget>    vControllers; // List of registered controllers

            protected:
                void                    do_destroy();

            public:
                explicit Registry();
                virtual ~Registry();

                virtual void            destroy();

            public:
                virtual status_t        add(ctl::Widget *w);

            public:
                inline size_t           size() const                { return vControllers.size();       }
                inline Widget          *get(size_t index)           { return vControllers.get(index);   }
        };
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IREGISTRY_H_ */
