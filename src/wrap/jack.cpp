/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 11 мая 2016 г.
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

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/ws/ws.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/ipc/Library.h>
#include <lsp-plug.in/resource/DirLoader.h>

#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/ui.h>

#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/wrap/jack/defs.h>
#include <lsp-plug.in/plug-fw/wrap/jack/types.h>
#include <lsp-plug.in/plug-fw/wrap/jack/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/jack/ui_wrapper.h>

#ifdef PLATFORM_POSIX
    #include <signal.h>
#endif /* PLATFORM_POSIX */

#if defined(LSP_TESTING) && (defined(PLATFORM_LINUX) || defined(PLATFORM_BSD))
    #define XDND_PROXY_SUPPORT
    #define IF_XDND_PROXY_SUPPORT(...)  __VA_ARGS__
#else
    #define IF_XDND_PROXY_SUPPORT(...)
#endif

#define RECONNECT_INTERVAL          1000u   /* 1 second     */
#define ICON_SYNC_INTERVAL          200u    /* 5 FPS        */

namespace lsp
{
    namespace jack
    {
        typedef struct wrapper_t
        {
            size_t              nSync;              // Synchronization request
            bool                bNotify;            // Notify all ports
            resource::ILoader  *pLoader;            // Resource loader
            plug::Module       *pPlugin;            // Plugin
            ui::Module         *pUI;                // Plugin UI
            jack::Wrapper      *pWrapper;           // Plugin wrapper
            jack::UIWrapper    *pUIWrapper;         // Plugin UI wrapper
            ws::timestamp_t     nLastReconnect;     // Last connection time
            ws::timestamp_t     nLastIconSync;      // Last icon synchronization time
            volatile bool       bInterrupt;         // Interrupt signal received
        } wrapper_t;

        typedef struct cmdline_t
        {
            const char     *cfg_file;
            const char     *plugin_id;
            void           *parent_id;
            bool            headless;
            bool            list;
        } cmdline_t;

        // JACK wrapper
        static wrapper_t  wrapper;

        status_t parse_cmdline(cmdline_t *cfg, const char *plugin_id, int argc, const char **argv)
        {
            // Initialize config with default values
            cfg->cfg_file       = NULL;
            cfg->plugin_id      = NULL;
            cfg->parent_id      = NULL;
            cfg->headless       = false;
            cfg->list           = false;

            // Parse arguments
            int i = 1;

            while (i < argc)
            {
                const char *arg = argv[i++];
                if ((!::strcmp(arg, "--help")) || (!::strcmp(arg, "-h")))
                {
                    printf("Usage: %s [parameters]%s\n\n",
                            argv[0],
                            (plugin_id != NULL) ? "" : " plugin-id"
                    );
                    printf("Available parameters:\n");
                    printf("  -c, --config <file>   Load settings file on startup\n");
                    IF_XDND_PROXY_SUPPORT(
                        printf("  --dnd-proxy <id>      Create window as child and DnD proxy of specified window ID\n");
                    )
                    printf("  -h, --help            Output help\n");
                    printf("  -hl, --headless       Launch in console only, without UI\n");
                    printf("  -l, --list            List available plugin identifiers\n");
                    printf("\n");

                    return STATUS_CANCELLED;
                }
                else if ((!::strcmp(arg, "--config")) || (!::strcmp(arg, "-c")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified file name for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    cfg->cfg_file = argv[i++];
                }
                else if ((!::strcmp(arg, "--headless")) || (!::strcmp(arg, "-hl")))
                    cfg->headless       = true;
                else if ((plugin_id == NULL) && ((!::strcmp(arg, "--list")) || (!::strcmp(arg, "-l"))))
                    cfg->list           = true;
                else if ((plugin_id == NULL) && (cfg->plugin_id == NULL))
                    cfg->plugin_id      = argv[i++];
            #ifdef XDND_PROXY_SUPPORT
                else if (!::strcmp(arg, "--dnd-proxy"))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified window hex identifier for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }

                    union {
                        void *ptr;
                        long data;
                    } parent;
                    parent.data = long(::strtol(argv[i++], NULL, 16));
                    cfg->parent_id  = parent.ptr;
                }
            #endif
                else
                {
                    fprintf(stderr, "Unknown parameter: %s\n", arg);
                    return STATUS_BAD_ARGUMENTS;
                }
            }

            // Override plugin identifier if not specified
            if (cfg->plugin_id == NULL)
                cfg->plugin_id      = plugin_id;

            return STATUS_OK;
        }

        static ssize_t metadata_sort_func(const meta::plugin_t *a, const meta::plugin_t *b)
        {
            return strcmp(a->uid, b->uid);
        }

        status_t list_plugins()
        {
            lltl::parray<meta::plugin_t> list;
            size_t maxlen = 0;

            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Add metadata to list
                    if (!list.add(const_cast<meta::plugin_t *>(meta)))
                    {
                        fprintf(stderr, "Error obtaining plugin list\n");
                        return STATUS_NO_MEM;
                    }

                    // Estimate maximum length of plugin
                    maxlen  = lsp_max(maxlen, strlen(meta->uid));
                }
            }

            // Check that there are plugins in the list
            if (list.is_empty())
            {
                printf("No plugins have been found\n");
                return STATUS_OK;
            }

            // Sort plugin list
            list.qsort(metadata_sort_func);

            // Output sorted plugin list
            char fmt[0x20];
            sprintf(fmt, "  %%%ds  %%s\n", -int(maxlen));

            for (size_t i=0, n=list.size(); i<n; ++i)
            {
                const meta::plugin_t *meta = list.uget(i);
                printf(fmt, meta->uid, meta->description);
            }

            return STATUS_OK;
        }

        status_t create_plugin(wrapper_t *w, const char *id)
        {
            // Lookup plugin identifier among all registered plugin factories
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Check plugin identifier
                    if (!::strcmp(meta->uid, id))
                    {
                        // Instantiate the plugin and return
                        if ((w->pPlugin = f->create(meta)) != NULL)
                            return STATUS_OK;

                        fprintf(stderr, "Plugin instantiation error: %s\n", id);
                        return STATUS_NO_MEM;
                    }
                }
            }

            // No plugin has been found
            fprintf(stderr, "Unknown plugin identifier: %s\n", id);
            return STATUS_BAD_ARGUMENTS;
        }

        status_t create_ui(wrapper_t *w, const char *id)
        {
            // Lookup plugin identifier among all registered plugin factories
            for (ui::Factory *f = ui::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    // Check plugin identifier
                    if (!::strcmp(meta->uid, id))
                    {
                        // Instantiate the plugin UI and return
                        if ((w->pUI = f->create(meta)) != NULL)
                            return STATUS_OK;

                        fprintf(stderr, "Plugin UI instantiation error: %s\n", id);
                        return STATUS_NO_MEM;
                    }
                }
            }

            // No plugin has been found
            fprintf(stderr, "Not found UI for plugin: %s, will continue in headless mode\n", id);
            return STATUS_OK;
        }

        #if defined(PLATFORM_WINDOWS)
            static BOOL WINAPI ctrlc_handler(DWORD dwCtrlType)
            {
                if (dwCtrlType != CTRL_C_EVENT)
                    return FALSE;

                wrapper.bInterrupt     = true;
                return TRUE:
            }

        #elif defined(PLATFORM_POSIX)
            static void sigint_handler(int sig)
            {
                if (sig == SIGINT)
                    wrapper.bInterrupt     = true;
            }
        #endif /* PLATFORM_POSIX */

        static void destroy_wrapper(wrapper_t *w)
        {
            // Disconnect the wrapper
            if (w->pWrapper != NULL)
                w->pWrapper->disconnect();

            // Destroy plugin UI
            if (w->pUI != NULL)
            {
                w->pUI->pre_destroy();
                w->pUI->destroy();
                delete w->pUI;
                w->pUI          = NULL;
            }

            // Destroy UI wrapper
            if (w->pUIWrapper != NULL)
            {
                w->pUIWrapper->destroy();
                delete w->pUIWrapper;
                w->pUIWrapper   = NULL;
            }

            // Destroy plugin
            if (w->pPlugin != NULL)
            {
                w->pPlugin->destroy();
                delete w->pPlugin;
                w->pPlugin      = NULL;
            }

            // Destroy wrapper
            if (w->pWrapper != NULL)
            {
                w->pWrapper->destroy();
                delete w->pWrapper;
                w->pWrapper     = NULL;
            }

            // Destroy resource loader
            if (w->pLoader != NULL)
            {
                delete w->pLoader;
                w->pLoader      = NULL;
            }
        }

        static status_t init_wrapper(wrapper_t *w, const cmdline_t &cmdline)
        {
            status_t res;

            lsp_trace("Initializing wrapper");

            // Create the resource loader
            if ((w->pLoader = core::create_resource_loader()) == NULL)
            {
                lsp_error("No resource loader available");
                destroy_wrapper(w);
                return STATUS_NO_DATA;
            }

            // Create plugin module
            if ((res = create_plugin(w, cmdline.plugin_id)) != STATUS_OK)
            {
                destroy_wrapper(w);
                return res;
            }

            // Need to instantiate plugin UI?
            if (!cmdline.headless)
            {
                if ((res = create_ui(w, cmdline.plugin_id)) != STATUS_OK)
                {
                    destroy_wrapper(w);
                    return res;
                }
            }

            #if defined(PLATFORM_WINDOWS)
                // Handle the Ctrl-C event from console
                SetConsoleCtrlHandler(ctrlc_handler, TRUE);
            #elif defined(PLATFORM_POSIX)
                // Ignore SIGPIPE signal since JACK can suddenly lose socket connection
                signal(SIGPIPE, SIG_IGN);
                // Handle the Ctrl-C event from console
                signal(SIGINT, sigint_handler);
            #endif

            // Initialize plugin wrapper
            w->pWrapper     = new jack::Wrapper(w->pPlugin, w->pLoader);
            if (w->pWrapper == NULL)
            {
                destroy_wrapper(w);
                return STATUS_NO_MEM;
            }

            if ((res = w->pWrapper->init()) != STATUS_OK)
            {
                destroy_wrapper(w);
                return res;
            }

            // Initialize UI wrapper
            if (w->pUI != NULL)
            {
                // Create wrapper
                w->pUIWrapper   = new jack::UIWrapper(w->pWrapper, w->pLoader, w->pUI);
                if (w->pUIWrapper == NULL)
                {
                    destroy_wrapper(w);
                    return STATUS_NO_MEM;
                }

                // Initialize wrapper
                if ((res = w->pUIWrapper->init()) != STATUS_OK)
                {
                    destroy_wrapper(w);
                    return res;
                }

                // Show the widget
                tk::Widget * wnd = w->pUI->root();
                wnd->show();
            }

            // Load configuration (if specified in parameters)
            if (cmdline.cfg_file != NULL)
            {
                if (w->pUIWrapper != NULL)
                {
                    if ((res = w->pUIWrapper->import_settings(cmdline.cfg_file, false)) != STATUS_OK)
                        fprintf(stderr, "Error loading configuration file: '%s': %s\n", cmdline.cfg_file, get_status(res));
                }
                else if (w->pWrapper != NULL)
                {
                    if ((res = w->pWrapper->import_settings(cmdline.cfg_file)) != STATUS_OK)
                        fprintf(stderr, "Error loading configuration file: '%s': %s\n", cmdline.cfg_file, get_status(res));
                }
                else
                    fprintf(stderr, "Error loading configuration file: '%s': no accessible wrapper\n", cmdline.cfg_file);
            }

            return STATUS_OK;
        }

        static status_t sync_state(ws::timestamp_t sched, ws::timestamp_t ctime, void *arg)
        {
            if (arg == NULL)
                return STATUS_BAD_STATE;

            wrapper_t *w       = static_cast<wrapper_t *>(arg);
            jack::Wrapper *jw       = w->pWrapper;

            // If connection to JACK was lost - notify
            if (jw->connection_lost())
            {
                // Disconnect wrapper and remember last connection time
                fprintf(stderr, "Connection to JACK has been lost\n");
                jw->disconnect();
                w->nLastReconnect       = ctime;
            }

            // If we are currently in disconnected state - try to perform a connection
            if (jw->disconnected())
            {
                // Try each second to make new connection
                if ((ctime - w->nLastReconnect) >= RECONNECT_INTERVAL)
                {
                    printf("Trying to connect to JACK\n");
                    if (jw->connect() == STATUS_OK)
                    {
                        printf("Successfully connected to JACK\n");
                        w->nSync        = 0;
                        w->bNotify      = true;
                    }
                    w->nLastReconnect   = ctime;
                }
            }

            // If we are connected - do usual stuff
            if (jw->connected())
            {
                // Sync state (transfer DSP to UI)
                if (w->pUIWrapper != NULL)
                {
                    // Transfer changes from DSP to UI
                    w->pUIWrapper->sync(sched);
                    if (w->bNotify)
                    {
                        w->pUIWrapper->notify_all();
                        w->bNotify      = false;
                    }

                    // Update icon
                    if ((ctime - w->nLastIconSync) > ICON_SYNC_INTERVAL)
                    {
                        w->pUIWrapper->sync_inline_display();
                        w->nLastIconSync = ctime;
                    }
                }
            }

            return STATUS_OK;
        }

        status_t plugin_main(wrapper_t *w)
        {
            status_t res        = STATUS_OK;
            ssize_t period      = 40; // 40 ms period (25 frames per second)

            system::time_t  ctime;
            ws::timestamp_t ts1, ts2;

            // Initialize wrapper structure and set-up signal handlers
            while (!w->bInterrupt)
            {
                system::get_time(&ctime);
                ts1     = ws::timestamp_t(ctime.seconds) * 1000 + ctime.nanos / 1000000;

                // Synchronize with JACK
                if ((res = sync_state(ts1, ts1, w)) != STATUS_OK)
                {
                    fprintf(stderr, "Unexpected error, code=%d", res);
                    return res;
                }

                // Perform main event loop for the UI
                if (w->pUIWrapper != NULL)
                {
                    w->pUIWrapper->main_iteration();
                    if (!w->bInterrupt)
                        w->bInterrupt   = w->pUIWrapper->main_loop_interrupted();
                }

                // Perform a small sleep before new iteration
                system::get_time(&ctime);
                ts2     = ws::timestamp_t(ctime.seconds) * 1000 + ctime.nanos / 1000000;

                wssize_t delay   = ts1 + period - ts2;
                if (delay > 0)
                    w->pUIWrapper->display()->wait_events(lsp_max(delay, period));
            }

            fprintf(stderr, "\nPlugin execution interrupted\n");

            return res;
        }
    } /* namespace jack */
} /* namespace wrap */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    using namespace lsp;

    LSP_CSYMBOL_EXPORT
    int JACK_MAIN_FUNCTION(const char *plugin_id, int argc, const char **argv)
    {
        status_t res            = STATUS_OK;

        // Initialize wrapper with empty data
        jack::wrapper_t *w      = &jack::wrapper;
        w->nSync                = 0;
        w->bNotify              = true;
        w->pLoader              = NULL;
        w->pPlugin              = NULL;
        w->pUI                  = NULL;
        w->pWrapper             = NULL;
        w->pUIWrapper           = NULL;
        w->bInterrupt           = false;
        w->nLastReconnect       = 0;

//        lsp_debug_init("jack");
//        init_locale();
        
        // Parse command-line arguments
        jack::cmdline_t cmdline;
        if ((res = parse_cmdline(&cmdline, plugin_id, argc, argv)) != STATUS_OK)
            return (res == STATUS_CANCELLED) ? 0 : res;

        // Need just to list available plugins?
        if (cmdline.list)
        {
            if ((res = jack::list_plugins()) != STATUS_OK)
                return -res;
            return 0;
        }

        // Plugin identifier has been specified?
        if (cmdline.plugin_id == NULL)
        {
            fprintf(stderr, "Not specified plugin identifier, exiting\n");
            return -STATUS_NOT_FOUND;
        }

        // Initialize DSP
        dsp::init();

        // Initialize plugin wrapper
        res = jack::init_wrapper(w, cmdline);
        if (res == STATUS_OK) // Try to launch instantiated objects
            res = jack::plugin_main(w);

        // Destroy JACK wrapper
        jack::destroy_wrapper(w);

        // Return result
        return (res == STATUS_OK) ? 0 : -res;
    }

#ifndef LSP_IDE_DEBUG
    LSP_CSYMBOL_EXPORT
    LSP_DEF_VERSION_FUNC_HEADER
    {
        static const ::lsp::version_t v =
        {
            PLUGIN_PACKAGE_MAJOR,
            PLUGIN_PACKAGE_MINOR,
            PLUGIN_PACKAGE_MICRO,
            PLUGIN_PACKAGE_BRANCH
        };

        return &v;
    }
#endif /* LSP_IDE_DEBUG */

#ifdef __cplusplus
}
#endif /* __cplusplus */
