/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/resource/PrefixLoader.h>
#include <lsp-plug.in/resource/Environment.h>
#include <lsp-plug.in/tk/tk.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/pphash.h>
#include <lsp-plug.in/lltl/ptrset.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/io/IOutSequence.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/expr/Variables.h>

#include <lsp-plug.in/plug-fw/ui/IPort.h>
#include <lsp-plug.in/plug-fw/ui/Module.h>
#include <lsp-plug.in/plug-fw/ui/SwitchedPort.h>
#include <lsp-plug.in/plug-fw/ui/ValuePort.h>
#include <lsp-plug.in/plug-fw/ui/IKVTListener.h>
#include <lsp-plug.in/fmt/config/PullParser.h>
#include <lsp-plug.in/fmt/config/Serializer.h>

namespace lsp
{
    namespace ctl
    {
        class Window;
    }

    namespace ui
    {
        class Module;
        class ISchemaListener;
        class IPlayListener;

        enum import_flags_t
        {
            IMPORT_FLAG_NONE   = 0,
            IMPORT_FLAG_PRESET = 1 << 0,
            IMPORT_FLAG_PATCH  = 1 << 1
        };

        /**
         * UI wrapper
         */
        class IWrapper
        {
            private:
                friend class ControlPort;
                friend class PathPort;
                friend class SwitchedPort;
                friend class UIContext;

            protected:
                enum flags_t
                {
                    F_QUIT          = 1 << 0,       // Quit main loop flag
                    F_CONFIG_DIRTY  = 1 << 1,       // The configuration needs to be saved
                    F_CONFIG_LOCK   = 1 << 2,       // The configuration file is locked for update
                };

            protected:
                tk::Display                    *pDisplay;           // Display object
                tk::Window                     *wWindow;            // The main window
                ctl::Window                    *pWindow;            // The controller for window
                ui::Module                     *pUI;
                resource::ILoader              *pLoader;            // Prefix-based resource loader
                size_t                          nFlags;             // Flags
                wssize_t                        nPlayPosition;      // Playback position of the current file preview
                wssize_t                        nPlayLength;        // Overall playback file length in samples
                expr::Variables                 sGlobalVars;        // Global variables
                plug::position_t                sPosition;          // Melodic position

                lltl::parray<ui::IPort>         vPorts;             // All possible ports
                lltl::parray<ui::IPort>         vSortedPorts;       // Alphabetically-sorted ports
                lltl::parray<ui::SwitchedPort>  vSwitchedPorts;     // Switched ports
                lltl::parray<ui::IPort>         vConfigPorts;       // Configuration ports
                lltl::parray<ui::ValuePort>     vTimePorts;         // Time-related ports
                lltl::parray<ui::IPort>         vCustomPorts;       // Custom-defined ports
                lltl::pphash<LSPString, LSPString> vAliases;        // Port aliases
                lltl::parray<IKVTListener>      vKvtListeners;      // KVT listeners
                lltl::ptrset<ISchemaListener>   vSchemaListeners;   // Schema change listeners
                lltl::parray<IPlayListener>     vPlayListeners;     // List of playback listeners

            protected:
                static ssize_t  compare_ports(const IPort *a, const IPort *b);
                size_t          rebuild_sorted_ports();
                void            global_config_changed(IPort *src);
                status_t        create_alias(const LSPString *id, const LSPString *name);
                status_t        build_ui(const char *path, void *handle = NULL, ssize_t screen = -1);
                void            build_config_header(LSPString *c);
                void            build_global_config_header(LSPString *c);
                status_t        init_visual_schema();
                status_t        load_global_config(config::PullParser *parser);
                status_t        init_global_constants(const tk::StyleSheet *sheet);
                status_t        apply_visual_schema(const tk::StyleSheet *sheet);
                status_t        export_ports(config::Serializer *s, lltl::parray<IPort> *ports, const io::Path *relative);
                status_t        export_kvt(config::Serializer *s, core::KVTStorage *kvt, const io::Path *relative);
                status_t        export_bundle_versions(config::Serializer *s, const lltl::pphash<LSPString, LSPString> *versions);

                status_t        save_global_config(io::IOutSequence *os, const lltl::pphash<LSPString, LSPString> *versions);
                status_t        read_bundle_versions(const io::Path *file, lltl::pphash<LSPString, LSPString> *versions);
                static void     drop_bundle_versions(lltl::pphash<LSPString, LSPString> *versions);
                void            get_bundle_version_key(LSPString *key);

                void            notify_play_position(wssize_t position, wssize_t length);

                IPort          *port_by_id(const char *id);

            protected:
                static bool     set_port_value(ui::IPort *port, const config::param_t *param, size_t flags, const io::Path *base);
                void            position_updated(const plug::position_t *pos);

            public:
                explicit IWrapper(ui::Module *ui, resource::ILoader *loader);
                IWrapper(const IWrapper &) = delete;
                IWrapper(IWrapper &&) = delete;
                virtual ~IWrapper();

                IWrapper & operator = (const IWrapper &) = delete;
                IWrapper & operator = (IWrapper &&) = delete;

                virtual status_t                init(void *root_widget);
                virtual void                    destroy();

            public:
                /**
                 * Get builtin resource loader
                 * @return builtin resource loader
                 */
                inline resource::ILoader       *resources()         { return pLoader;       }

                /**
                 * Get the wrapped UI
                 * @return the pointer to the wrapped UI
                 */
                inline ui::Module              *ui()                { return pUI;           }

                /**
                 * Get window controller
                 * @return window controller
                 */
                inline ctl::Window             *controller()        { return pWindow;       }

                /**
                 * Get the display
                 * @return display
                 */
                inline tk::Display             *display()           { return pDisplay;      }

                /**
                 * Get root window widget
                 * @return root window widget
                 */
                inline tk::Window              *window()            { return wWindow;       }

                /**
                 * Return port by it's identifier
                 *
                 * @param id port identifier
                 * @return pointer to the port instance or NULL
                 */
                IPort                          *port(const char *id);

                /**
                 * Return port by it's identifier
                 *
                 * @param id port identifier
                 * @return pointer to the port instance or NULL
                 */
                IPort                          *port(const LSPString *id);

                /**
                 * Return port by it's index
                 * @param idx port index starting with 0
                 * @return pointer to the port instance or NULL
                 */
                IPort                          *port(size_t idx);

                /**
                 * Bind custom port
                 * @param port custom port to bind
                 * @return status of operation
                 */
                status_t                        bind_custom_port(ui::IPort *port);

                /**
                 * Get overall ports count
                 * @return overall ports count
                 */
                size_t                          ports() const;

                /**
                 * Create alias for the port
                 *
                 * @param alias the port alias name
                 * @param id the original port name to create the alias
                 * @return status of operation
                 */
                status_t                        set_port_alias(const char *alias, const char *id);
                status_t                        set_port_alias(const LSPString *alias, const char *id);
                status_t                        set_port_alias(const char *alias, const LSPString *id);
                status_t                        set_port_alias(const LSPString *alias, const LSPString *id);

                /**
                 * Get UI scaling factor (100.0 means no extra scaling applied)
                 * @param scaling the default value for scaling factor if scaling factor is not supported
                 * @return actual scaling factor (in percent) or default value if scaling factor is not supported
                 */
                virtual float                   ui_scaling_factor(float scaling);

                /**
                 * Notify all ports for estimated connection
                 */
                virtual void                    notify_all();

                inline const plug::position_t *position() const     { return &sPosition;    }

                /**
                 * Lock KVT storage and return pointer to the storage,
                 * this is non-RT-safe operation
                 * @return pointer to KVT storage or NULL if KVT is not supported
                 */
                virtual core::KVTStorage       *kvt_lock();

                /**
                 * Try to lock KVT storage and return pointer to the storage on success
                 * @return pointer to KVT storage or NULL
                 */
                virtual core::KVTStorage       *kvt_trylock();

                /**
                 * Notify the write of the KVT parameter
                 * @param storage KVT storage
                 * @param id kvt parameter identifier
                 * @param value KVT parameter value
                 */
                virtual void                    kvt_notify_write(core::KVTStorage *storage, const char *id, const core::kvt_param_t *value);
                virtual status_t                kvt_subscribe(ui::IKVTListener *listener);
                virtual status_t                kvt_unsubscribe(ui::IKVTListener *listener);

                /**
                 * Release the KVT storage
                 * @return true on success
                 */
                virtual bool                    kvt_release();

                /**
                 * Request plugin for dump of the internal state
                 */
                virtual void                    dump_state_request();

                /**
                 * Perform main iteration. Should be regularly called by the wrapper code
                 */
                virtual void                    main_iteration();

                /**
                 * Signal to quit main loop
                 */
                void                            quit_main_loop();

                /**
                 * check that main loop is still active
                 * @return true if main loop is still active
                 */
                inline bool                     main_loop_interrupted() const       { return nFlags & F_QUIT;       }

                /**
                 * Export settings of the plugin to file/underlying output stream
                 * @param file file name
                 * @param relative use relative paths to the exported file
                 */
                status_t                        export_settings(const char *file, bool relative = false);
                status_t                        export_settings(const io::Path *file, bool relative = false);
                status_t                        export_settings(const LSPString *file, bool relative = false);

                /**
                 * Export settings
                 * @param os output stream to perform export
                 * @param basedir the directory the config file will be written, can be NULL
                 * @return status of operation
                 */
                status_t                        export_settings(io::IOutSequence *os, const char *basedir);
                status_t                        export_settings(io::IOutSequence *os, const LSPString *basedir);
                status_t                        export_settings(io::IOutSequence *os, const io::Path *basedir = NULL);

                /**
                 * Export settings
                 * @param s output configuration serializer
                 * @param basedir the directory the config file will be written, can be NULL
                 * @return status of operation
                 */
                status_t                        export_settings(config::Serializer *s, const char *basedir);
                status_t                        export_settings(config::Serializer *s, const LSPString *basedir);
                virtual status_t                export_settings(config::Serializer *s, const io::Path *basedir = NULL);

                /**
                 * Import settings
                 * @param the source (file name or input sequence)
                 * @param flags different flags (@see import_flags_t)
                 * @return status of operation
                 */
                status_t                        import_settings(const char *file, size_t flags);
                status_t                        import_settings(const io::Path *file, size_t flags);
                status_t                        import_settings(const LSPString *file, size_t flags);
                status_t                        import_settings(io::IInSequence *is, size_t flags, const char *basedir);
                status_t                        import_settings(io::IInSequence *is, size_t flags, const LSPString *basedir);
                status_t                        import_settings(io::IInSequence *is, size_t flags, const io::Path *basedir = NULL);
                status_t                        import_settings(config::PullParser *parser, size_t flags, const char *basedir);
                status_t                        import_settings(config::PullParser *parser, size_t flags, const LSPString *basedir);
                virtual status_t                import_settings(config::PullParser *parser, size_t flags, const io::Path *basedir = NULL);

                /**
                 * Load visual schema for the wrapper
                 * @param the source (file name or input sequence)
                 * @return status of operation
                 */
                virtual status_t                load_visual_schema(const char *file);
                virtual status_t                load_visual_schema(const io::Path *file);
                virtual status_t                load_visual_schema(const LSPString *file);

                virtual status_t                load_stylesheet(tk::StyleSheet *sheet, const char *file);
                virtual status_t                load_stylesheet(tk::StyleSheet *sheet, const io::Path *file);
                virtual status_t                load_stylesheet(tk::StyleSheet *sheet, const LSPString *file);

                /**
                 * Load global configuration file
                 * @param file the path to file to load
                 * @return status of operation
                 */
                virtual status_t                load_global_config(const char *file);
                virtual status_t                load_global_config(const io::Path *file);
                virtual status_t                load_global_config(const LSPString *file);
                virtual status_t                load_global_config(io::IInSequence *is);

                /**
                 * Save global configuration file
                 * @param file the pato to the configuration file
                 * @return status of operation
                 */
                virtual status_t                save_global_config(const char *file);
                virtual status_t                save_global_config(const io::Path *file);
                virtual status_t                save_global_config(const LSPString *file);

                /**
                 * Send request to perform preview playback of the file
                 *
                 * @param file file name or empty/null if need to stop the playback
                 * @param position initial position of playback in samples
                 * @param release indicator for the stop event that informs that the sample won't
                 *        be played anymore and the previously allocated memory should be released
                 * @return status of operation
                 */
                virtual status_t                play_file(const char *file, wsize_t position, bool release);

                /**
                 * Subscribe for the listen update events, listener immediately receives
                 * current playback position on the success subscription
                 * @param listener the listener
                 * @return status of the subscription
                 */
                virtual status_t                play_subscribe(IPlayListener *listener);

                /**
                 * Unsunscribe the listen update events
                 * @param listener
                 * @return status of operation
                 */
                virtual status_t                play_unsubscribe(IPlayListener *listener);

                /**
                 * Add schema listener
                 * @param listener schema listener
                 * @return status of operation
                 */
                virtual status_t                add_schema_listener(ui::ISchemaListener *listener);

                /**
                 * remove schema listener
                 * @param listener schema listener to remove
                 * @return status of operation
                 */
                virtual status_t                remove_schema_listener(ui::ISchemaListener *listener);

                /**
                 * Get package version
                 * @return package version
                 */
                virtual const meta::package_t  *package() const;

                /**
                 * Get plugin metadata
                 * @return plugin metadata
                 */
                const meta::plugin_t           *metadata() const;

                /**
                 * Get global variables
                 * @return global variables
                 */
                virtual expr::Variables        *global_variables();

                /**
                 * Reset the plugin settings
                 * @return status of operation
                 */
                virtual status_t                reset_settings();

                /**
                 * Check that the specified window size is accepted by the host
                 * @param width the requested window width
                 * @param height the requested window height
                 * @return true if the specified window size is accepted by the host
                 */
                virtual bool                    accept_window_size(tk::Window *wnd, size_t width, size_t height);

                /**
                 * Get the plugin format
                 * @return plugin format
                 */
                virtual meta::plugin_format_t   plugin_format() const;
        };

    } /* namespace ui */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_UI_IWRAPPER_H_ */
