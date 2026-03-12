/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 февр. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_DOCUMENTATION_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_DOCUMENTATION_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ctl
    {
        class Documentation
        {
            private:
                ui::IWrapper       *pWrapper;

            private:
                void                read_path_param(LSPString *value, const char *port_id);
                bool                open_manual_file(const char *fmt...);

            public:
                explicit Documentation(ui::IWrapper *wrapper);
                Documentation(const Documentation &) = delete;
                Documentation(Documentation &&) = delete;
                ~Documentation();

                Controller & operator = (const Controller &) = delete;
                Controller & operator = (Controller &&) = delete;

                /**
                 * Initialize controller
                 */
                status_t    init();
                void        destroy();

            public:
                status_t    show_plugin_manual(const meta::plugin_t *plugin = NULL);
                status_t    show_ui_manual();
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_DOCUMENTATION_H_ */
