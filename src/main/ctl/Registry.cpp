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

#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        Registry::Registry()
        {
        }

        Registry::~Registry()
        {
            do_destroy();
        }

        void Registry::destroy()
        {
            do_destroy();
        }

        void Registry::do_destroy()
        {
            // Destroy all widgets in reverse order
            for (size_t i=vControllers.size(); (i--) > 0;)
            {
                ctl::Widget *w = vControllers.uget(i);
//                lsp_trace("c = %p", w);

                if (w != NULL)
                {
                    w->destroy();
                    delete w;
                }
            }
            vControllers.flush();
        }

        status_t Registry::add(ctl::Widget *w)
        {
//            lsp_trace("c = %p", w);

            if (w == NULL)
                return STATUS_BAD_ARGUMENTS;

            if (vControllers.contains(w))
                return STATUS_ALREADY_EXISTS;

            return (vControllers.add(w)) ? STATUS_OK : STATUS_NO_MEM;
        }
    }
}


