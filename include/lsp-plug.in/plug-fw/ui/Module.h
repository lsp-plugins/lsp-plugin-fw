/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_MODULE_H_
#define LSP_PLUG_IN_PLUG_FW_UI_MODULE_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/tk/tk.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>

namespace lsp
{
    namespace ui
    {
        class IWrapper;

        /**
         * UI Module
         */
        class Module
        {
            private:
                Module &operator = (const Module &);

            protected:
                const meta::plugin_t       *pMetadata;
                IWrapper                   *pWrapper;

                lltl::parray<IPort>         vPorts;
                lltl::parray<IPort>         vCustomPorts;
                lltl::parray<IPort>         vSortedPorts;
                lltl::parray<IPort>         vConfigPorts;
                lltl::parray<tk::Widget>    vTkWidgets;

            public:
                explicit Module(const meta::plugin_t *meta);
                virtual ~Module();

                /** Initialize UI
                 *
                 * @param wrapper plugin wrapper
                 * @return status of operation
                 */
                virtual status_t            init(IWrapper *wrapper);

                /**
                 * Destroy the UI
                 */
                virtual void                destroy();

            public:
                inline const meta::plugin_t    *metadata() const        { return pMetadata;         };
                inline IWrapper                *wrapper()               { return pWrapper;          };

            public:
                /** Method executed when the time position of plugin was updated
                 *
                 */
                void                        position_updated(const plug::position_t *pos);

                /** Add plugin port to UI
                 *
                 * @param port UI port to communicate with plugin
                 * @return status of operation
                 */
                status_t                    add_port(IPort *port);

                /** Add custom port to UI
                 *
                 * @param port custom UI port
                 * @return status of operation
                 */
                status_t                    add_custom_port(IPort *port);

                /** Get INTERNAL port by name
                 *
                 * @param name port name
                 * @return internal port
                 */
                IPort                      *port(const char *name);

                /** Get port count
                 *
                 * @return number of ports
                 */
                inline size_t               ports_count() const     { return vPorts.size(); }

                /**
                 * Synchronize state of meta ports
                 */
                void                        sync_meta_ports();

                /**
                 * Notify the write of the KVT parameter
                 * @param storage KVT storage
                 * @param id kvt parameter identifier
                 * @param value KVT parameter value
                 */
                virtual void                kvt_write(core::KVTStorage *storage, const char *id, const core::kvt_param_t *value);

        };
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_UI_MODULE_H_ */
