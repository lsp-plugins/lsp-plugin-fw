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

#ifndef PRIVATE_UI_BUILTINSTYLE_H_
#define PRIVATE_UI_BUILTINSTYLE_H_

#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ui
    {
        class LSP_HIDDEN_MODIFIER BuiltinStyle
        {
            private:
                static BuiltinStyle        *pRoot;
                BuiltinStyle               *pNext;
                tk::IStyleFactory          *pInit;

            public:
                explicit BuiltinStyle(tk::IStyleFactory *init);

            public:
                static inline BuiltinStyle *root() { return pRoot; }
                inline BuiltinStyle        *next() { return pNext; }
                inline tk::IStyleFactory   *init() { return pInit; }

            public:
                static status_t     init_schema(tk::Schema *schema);
        };

        #define LSP_UI_BUILTIN_STYLE_VAR(Name) Style ## Builtin

        #define LSP_UI_BUILTIN_STYLE(Style, Name, Parents) \
            static ::lsp::tk::StyleFactory<Style> Style ## Factory(Name, Parents); \
            \
            static ::lsp::ui::BuiltinStyle Style ## Builtin(& Style ## Factory);


    } // namespace ui
} //namespace lsp

#endif /* PRIVATE_UI_BUILTINSTYLE_H_ */
