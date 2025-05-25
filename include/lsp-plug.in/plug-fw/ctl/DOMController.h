/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 мая 2025 г.
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

#ifndef MODULES_LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_CTL_DOMCONTROLLER_H_
#define MODULES_LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_CTL_DOMCONTROLLER_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * DOM Controller
         */
        class DOMController: public Controller
        {
            public:
                static const ctl_class_t metadata;

            public:
                explicit DOMController(ui::IWrapper *wrapper);
                DOMController(const DOMController &) = delete;
                DOMController(DOMController &&) = delete;
                virtual ~DOMController() override;

                DOMController & operator = (const DOMController &) = delete;
                DOMController & operator = (DOMController &&) = delete;

            public:
                /** Set attribute to controller
                 *
                 * @param ctx context
                 * @param name attribute name
                 * @param value attribute value
                 */
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);

                /** Begin configuration of controller
                 *
                 */
                virtual void        begin(ui::UIContext *ctx);

                /** End configuration of controller
                 *
                 */
                virtual void        end(ui::UIContext *ctx);
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* MODULES_LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_CTL_DOMCONTROLLER_H_ */
