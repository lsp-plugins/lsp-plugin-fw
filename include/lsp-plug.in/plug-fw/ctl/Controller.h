/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 24 мая 2025 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_CONTROLLER_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_CONTROLLER_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ui
    {
        class UIContext;
    } /* namespace ui */

    namespace ctl
    {
        /**
         * Controller
         */
        class Controller
        {
            protected:
                ui::IWrapper *pWrapper;

            public:
                explicit Controller(ui::IWrapper *wrapper);
                Controller(const Controller &) = delete;
                Controller(Controller &&) = delete;
                virtual ~Controller();

                Controller & operator = (const Controller &) = delete;
                Controller & operator = (Controller &&) = delete;

                /** Initialize widget controller
                 *
                 */
                virtual status_t    init();

                /** Destroy widget controller
                 *
                 */
                virtual void        destroy();

        };

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_CONTROLLER_H_ */
