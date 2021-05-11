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
#include <lsp-plug.in/io/IOutSequence.h>
#include <lsp-plug.in/io/Path.h>

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
                enum flags_t
                {
                    F_QUIT          = 1 << 0,       // Quit main loop flag
                    F_SAVE_CONFIG   = 1 << 1        // Save configuration flag
                };

            protected:
                ui::Module                 *pUI;
                resource::ILoader          *pLoader;
                size_t                      nFlags;             // Flags

                lltl::parray<IPort>         vPorts;             // All possible ports
                lltl::parray<IPort>         vSortedPorts;       // Alphabetically-sorted ports
                lltl::parray<SwitchedPort>  vSwitchedPorts;     // Switched ports
                lltl::parray<IPort>         vConfigPorts;       // Configuration ports
                lltl::parray<IPort>         vTimePorts;         // Time-related ports
                lltl::parray<IPort>         vCustomPorts;       // Custom-defined ports
                lltl::pphash<LSPString, LSPString> vAliases;    // Port aliases
                lltl::parray<ctl::Widget>   vControllers;       // Controllers

            protected:
                static ssize_t  compare_ports(const IPort *a, const IPort *b);
                size_t          rebuild_sorted_ports();
                void            global_config_changed(IPort *src);
                status_t        create_alias(const LSPString *id, const LSPString *name);
                status_t        build_ui(const char *path);
                void            build_config_header(LSPString *c);

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

                /**
                 * Signal to quit main loop
                 */
                void                        quit_main_loop();

                /**
                 * Add controller
                 * @param widget
                 * @return
                 */
                bool                        add_controller(ctl::Widget *widget);

                /**
                 * check that main loop is still active
                 * @return true if main loop is still active
                 */
                inline bool                 main_loop_interrupted() const       { return nFlags & F_QUIT;       }

                /**
                 * Export settings of the plugin to file/underlying output stream
                 * @param file file name
                 * @param relative use relative paths to the exported file
                 */
                virtual status_t            export_settings(const char *file, bool relative = false);
                virtual status_t            export_settings(const io::Path *file, bool relative = false);
                virtual status_t            export_settings(const LSPString *file, bool relative = false);

                /**
                 * Export settings
                 * @param os output stream to perform export
                 * @param relative the paths will be written relative to the passed path name, can be NULL
                 * @return status of operation
                 */
                virtual status_t            export_settings(io::IOutSequence *os, const char *relative);
                virtual status_t            export_settings(io::IOutSequence *os, const LSPString *relative);
                virtual status_t            export_settings(io::IOutSequence *os, const io::Path *relative = NULL);

                /**
                 * Import settings
                 * @param the source (file name or input sequence)
                 * @return status of operation
                 */
                virtual status_t            import_settings(const char *file);
                virtual status_t            import_settings(const io::Path *file);
                virtual status_t            import_settings(const LSPString *file);
                virtual status_t            import_settings(io::IInSequence *is);

                /**
                 * Get package version
                 * @return package version
                 */
                virtual const meta::package_t  *package() const;
        };
    }

} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UI_IWRAPPER_H_ */
