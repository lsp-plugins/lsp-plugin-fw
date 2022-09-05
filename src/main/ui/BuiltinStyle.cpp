/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 2 окт. 2021 г.
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

#include <private/ui/BuiltinStyle.h>

namespace lsp
{
    namespace ui
    {
        BuiltinStyle *BuiltinStyle::pRoot    = NULL;

        BuiltinStyle::BuiltinStyle(tk::IStyleFactory *init)
        {
            pInit   = init;
            pNext   = pRoot;
            pRoot   = this;
        }

        status_t BuiltinStyle::init_schema(tk::Schema *schema)
        {
            // Form the list of registered styles
            lltl::parray<tk::IStyleFactory> list;
            for (BuiltinStyle *curr = root(); curr != NULL; curr = curr->next())
            {
                if (!list.add(curr->init()))
                    return STATUS_NO_MEM;
            }

            // Pass the list to the schema
            return schema->add(&list);
        }


    } //namespace ui
} // namespace lsp

