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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IWRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_UI_IWRAPPER_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/resource/ILoader.h>
#include <lsp-plug.in/resource/Environment.h>
#include <lsp-plug.in/tk/tk.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>

#include <lsp-plug.in/plug-fw/ui/IPort.h>
#include <lsp-plug.in/plug-fw/ui/Module.h>
#include <lsp-plug.in/plug-fw/ui/SwitchedPort.h>

namespace lsp
{
    namespace ctl
    {
        class Widget;
    }

    namespace ui
    {
        class Module;

        /**
         * UI wrapper
         */
        class IWrapper
        {
            private:
                IWrapper & operator = (const IWrapper &);

                friend class ControlPort;
                friend class PathPort;
                friend class SwitchedPort;
                friend class UIContext;

            protected:
                ui::Module                 *pUI;
                resource::ILoader          *pLoader;

                lltl::parray<IPort>         vPorts;             // All possible ports
                lltl::parray<IPort>         vSortedPorts;       // Alphabetically-sorted ports
                lltl::parray<SwitchedPort>  vSwitchedPorts;     // Switched ports
                lltl::parray<IPort>         vConfigPorts;       // Configuration ports
                lltl::parray<IPort>         vTimePorts;         // Time-related ports
                lltl::parray<IPort>         vCustomPorts;       // Custom-defined ports
                lltl::pphash<LSPString, LSPString> vAliases;    // Port aliases
                lltl::parray<ctl::Widget>   vControllers;       // Controllers

                bool                        bSaveConfig;        // Save configuration flag

            protected:
                static ssize_t  compare_ports(const IPort *a, const IPort *b);
                size_t          rebuild_sorted_ports();
                void            global_config_changed(IPort *src);
                status_t        create_alias(const LSPString *id, const LSPString *name);
                status_t        build_ui(const char *path);

            public:
                explicit IWrapper(ui::Module *ui, resource::ILoader *loader);
                virtual ~IWrapper();

                virtual status_t    init();
                virtual void        destroy();

            public:
                /**
                 * Get builtin resource loader
                 * @return builtin resource loader
                 */
                inline resource::ILoader   *resources()         { return pLoader;       }

                /**
                 * Get the wrapped UI
                 * @return the pointer to the wrapped UI
                 */
                inline ui::Module          *ui()                { return pUI;           }

                /**
                 * Return port by it's identifier
                 *
                 * @param id port identifier
                 * @return pointer to the port instance or NULL
                 */
                IPort                      *port(const char *id);

                /**
                 * Return port by it's identifier
                 *
                 * @param id port identifier
                 * @return pointer to the port instance or NULL
                 */
                IPort                      *port(const LSPString *id);

                /**
                 * Return port by it's index
                 * @param idx port index starting with 0
                 * @return pointer to the port instance or NULL
                 */
                IPort                      *port(size_t idx);

                /**
                 * Get overall ports count
                 * @return overall ports count
                 */
                size_t                      ports() const;

                /**
                 * Create alias for the port
                 *
                 * @param alias the port alias name
                 * @param id the original port name to create the alias
                 * @return status of operation
                 */
                status_t                    set_port_alias(const char *alias, const char *id);
                status_t                    set_port_alias(const LSPString *alias, const char *id);
                status_t                    set_port_alias(const char *alias, const LSPString *id);
                status_t                    set_port_alias(const LSPString *alias, const LSPString *id);

                /** Callback method, executes when the UI has been shown
                 *
                 */
                virtual void                ui_activated();

                /** Callback method, executes when the UI has been hidden
                 *
                 */
                virtual void                ui_deactivated();

                /**
                 * Lock KVT storage and return pointer to the storage,
                 * this is non-RT-safe operation
                 * @return pointer to KVT storage or NULL if KVT is not supported
                 */
                virtual core::KVTStorage   *kvt_lock();

                /**
                 * Try to lock KVT storage and return pointer to the storage on success
                 * @return pointer to KVT storage or NULL
                 */
                virtual core::KVTStorage   *kvt_trylock();

                /**
                 * Release the KVT storage
                 * @return true on success
                 */
                virtual bool                kvt_release();

                /**
                 * Request plugin for dump of the internal state
                 */
                virtual void                dump_state_request();
        };
    }

} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UI_IWRAPPER_H_ */
