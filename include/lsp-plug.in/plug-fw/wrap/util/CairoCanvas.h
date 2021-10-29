/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 30 окт. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_UTIL_CAIROCANVAS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_UTIL_CAIROCANVAS_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace wrap
    {
        /**
         * Factory for Cairo canvas
         */
        class CairoCanvasFactory: public plug::ICanvasFactory
        {
            private:
                CairoCanvasFactory(const CairoCanvasFactory &);
                CairoCanvasFactory & operator = (const CairoCanvasFactory &);

            public:
                explicit CairoCanvasFactory();
                virtual ~CairoCanvasFactory();

                /** Create canvas
                 *
                 * @param width initial width of canvas
                 * @param height initial height of canvas
                 * @return pointer to object or NULL if creation of canvas is not possible
                 */
                virtual plug::ICanvas *create_canvas(size_t width, size_t height);
        };

    } /* namespace wrap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_UTIL_CAIROCANVAS_H_ */
