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

#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/ui.h>

#include <lsp-plug.in/plug-fw/wrap/jack/types.h>
#include <lsp-plug.in/plug-fw/wrap/jack/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/jack/ports.h>

//
//#include <core/lib.h>
//#include <core/debug.h>
//#include <core/status.h>
//#include <core/ipc/NativeExecutor.h>
//
//#include <dsp/dsp.h>
//
//#include <ui/ui_locale.h>
//#include <core/stdlib/string.h>
//
//#include <plugins/plugins.h>
//#include <metadata/plugins.h>
//
//#include <container/const.h>
//#include <container/jack/wrapper.h>
//

#ifdef PLATFORM_POSIX
    #include <signal.h>
#endif /* PLATFORM_POSIX */

#if defined(LSP_TESTING) && (defined(PLATFORM_LINUX) || defined(PLATFORM_BSD))
    #define XDND_PROXY_SUPPORT
    #define IF_XDND_PROXY_SUPPORT(...)  __VA_ARGS__
#else
    #define IF_XDND_PROXY_SUPPORT(...)
#endif

namespace lsp
{
    namespace wrap
    {
        typedef struct jack_wrapper_t
        {
            size_t              nSync;              // Synchronization request
            plug::Module       *pPlugin;            // Plugin
            ui::Module         *pUI;                // Plugin UI
            jack::Wrapper      *pWrapper;           // Plugin wrapper
//            LSPWindow      *pWindow;
            ws::timestamp_t     nLastReconnect;
            volatile bool       bInterrupt;         // Interrupt signal received
        } jack_wrapper_t;

        typedef struct jack_cmdline_t
        {
            const char     *cfg_file;
            const char     *plugin_id;
            void           *parent_id;
            bool            headless;
            bool            list;
        } jack_config_t;

        // JACK wrapper
        static jack_wrapper_t  jack_wrapper;

        status_t jack_parse_cmdline(jack_cmdline_t *cfg, const char *plugin_id, int argc, const char **argv)
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

        static ssize_t jack_metadata_sort_func(const meta::plugin_t *a, const meta::plugin_t *b)
        {
            return strcmp(a->lv2_uid, b->lv2_uid);
        }

        status_t jack_list_plugins()
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
                    maxlen  = lsp_max(maxlen, strlen(meta->lv2_uid));
                }
            }

            // Check that there are plugins in the list
            if (list.is_empty())
            {
                printf("No plugins have been found\n");
                return STATUS_OK;
            }

            // Sort plugin list
            list.qsort(jack_metadata_sort_func);

            // Output sorted plugin list
            char fmt[0x20];
            sprintf(fmt, "  %%%ds  %%s\n", -int(maxlen));

            for (size_t i=0, n=list.size(); i<n; ++i)
            {
                const meta::plugin_t *meta = list.uget(i);
                printf(fmt, meta->lv2_uid, meta->description);
            }

            return STATUS_OK;
        }

        status_t jack_create_plugin(jack_wrapper_t *w, const char *id)
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
                    if (!::strcmp(meta->lv2_uid, id))
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

        status_t jack_create_ui(jack_wrapper_t *w, const char *id)
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
                    if (!::strcmp(meta->lv2_uid, id))
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
            static BOOL WINAPI jack_ctrlc_handler(DWORD dwCtrlType)
            {
                if (dwCtrlType != CTRL_C_EVENT)
                    return FALSE;

                jack_wrapper.bInterrupt     = true;
                return TRUE:
            }

        #elif defined(PLATFORM_POSIX)
            static void jack_sigint_handler(int sig)
            {
                if (sig == SIGINT)
                    jack_wrapper.bInterrupt     = true;
            }
        #endif /* PLATFORM_POSIX */

        static void jack_destroy_wrapper(jack_wrapper_t *w)
        {
            // Disconnect the wrapper
            if (w->pWrapper != NULL)
                w->pWrapper->disconnect();
            // TODO: disconnect UI wrapper

            // Destroy plugin UI
            if (w->pUI != NULL)
            {
                w->pUI->destroy();
                delete w->pUI;
                w->pUI          = NULL;
            }

            // TODO: destroy UI wrapper

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
        }

        static status_t jack_init_wrapper(jack_wrapper_t *w, const jack_cmdline_t &cmdline)
        {
            status_t res;

            lsp_trace("Initializing wrapper");

            // Create plugin module
            if ((res = jack_create_plugin(w, cmdline.plugin_id)) != STATUS_OK)
            {
                jack_destroy_wrapper(w);
                return res;
            }

            // Need to instantiate plugin UI?
            if (!cmdline.headless)
            {
                if ((res = jack_create_ui(w, cmdline.plugin_id)) != STATUS_OK)
                {
                    jack_destroy_wrapper(w);
                    return res;
                }
            }

            #if defined(PLATFORM_WINDOWS)
                // Handle the Ctrl-C event from console
                SetConsoleCtrlHandler(jack_ctrlc_handler, TRUE);
            #elif defined(PLATFORM_POSIX)
                // Ignore SIGPIPE signal since JACK can suddenly lose socket connection
                signal(SIGPIPE, SIG_IGN);
                // Handle the Ctrl-C event from console
                signal(SIGINT, jack_sigint_handler);
            #endif

            // Initialize plugin wrapper
            w->pWrapper     = new jack::Wrapper(w->pPlugin);
            if (w->pWrapper == NULL)
            {
                jack_destroy_wrapper(w);
                return STATUS_NO_MEM;
            }

            if ((res = w->pWrapper->init()) != STATUS_OK)
            {
                jack_destroy_wrapper(w);
                return res;
            }

            // TODO: Initialize UI wrapper
            if (w->pUI!= NULL)
            {
            }

            // TODO: Load configuration (if specified in parameters)
//            if ((status == STATUS_OK) && (cfg.cfg_file != NULL))
//            {
//                status = pui->import_settings(cfg.cfg_file, false);
//                if (status != STATUS_OK)
//                    fprintf(stderr, "Error loading configuration file: %s\n", get_status(status));
//            }

            return STATUS_OK;
        }

        static status_t jack_sync(ws::timestamp_t sched, ws::timestamp_t ctime, void *arg)
        {
            if (arg == NULL)
                return STATUS_BAD_STATE;

            jack_wrapper_t *w       = static_cast<jack_wrapper_t *>(arg);
            jack::Wrapper *jw       = w->pWrapper;

            // If connection to JACK was lost - notify
            if (jw->connection_lost())
            {
                // Disconnect wrapper and remember last connection time
                fprintf(stderr, "Connection to JACK was lost\n");
                jw->disconnect();
                w->nLastReconnect       = ctime;
            }

            // If we are currently in disconnected state - try to perform a connection
            if (jw->disconnected())
            {
                // Try each second to make new connection
                if ((ctime - w->nLastReconnect) >= 1000)
                {
                    printf("Trying to connect to JACK\n");
                    if (jw->connect() == STATUS_OK)
                    {
                        lsp_trace("Successfully connected to JACK");
                        w->nSync        = 0;
                    }
                    w->nLastReconnect   = ctime;
                }
            }

            // If we are connected - do usual stuff
            if (jw->connected())
            {
                // TODO
//                if (!(wrapper->nSync++))
//                    wrapper->pWindow->query_resize();
            // Transfer changes from DSP to UI
//            wrapper->pWrapper->transfer_dsp_to_ui(); // TODO
            }

            return STATUS_OK;
        }

        status_t jack_plugin_main(jack_wrapper_t *w)
        {
            status_t res        = STATUS_OK;
            ssize_t period      = 40; // 40 ms period (25 frames per second)

            if (w->pUI != NULL)
            {
                // TODO
            }
            else
            {
                system::time_t  ctime;
                ws::timestamp_t ts1, ts2;

                // Initialize wrapper structure and set-up signal handlers
                while (!w->bInterrupt)
                {
                    system::get_time(&ctime);
                    ts1     = ctime.seconds * 1000 + ctime.nanos / 1000000;

                    if ((res = jack_sync(ts1, ts1, w)) != STATUS_OK)
                    {
                        fprintf(stderr, "Unexpected error, code=%d", res);
                        return res;
                    }

                    // Perform a small sleep before new iteration
                    system::get_time(&ctime);
                    ts2     = ctime.seconds * 1000 + ctime.nanos / 1000000;

                    wssize_t delay   = ts1 + period - ts2;
                    if (delay > 0)
                        ipc::Thread::sleep(lsp_max(delay, period));
                }
            }

            printf("Plugin execution interrupted\n");

            return res;

            //            // Enter the main lifecycle
            //            if (status == STATUS_OK)
            //            {
            //                dsp::context_t ctx;
            //                dsp::start(&ctx);
            //
            //                // Perform initial connection
            //                lsp_trace("Connecting to JACK server");
            //                w.connect();
            //                clock_gettime(CLOCK_REALTIME, &wrapper.nLastReconnect);
            //
            //                // Create timer for transferring DSP -> UI data
            //                lsp_trace("Creating timer");
            //                wrapper.nSync       = 0;
            //                wrapper.pWrapper    = &w;
            //                wrapper.pWindow     = pui->root_window();
            //
            //                LSPTimer tmr;
            //                tmr.bind(pui->display());
            //                tmr.set_handler(jack_ui_sync, &wrapper);
            //                tmr.launch(0, 40); // 25 Hz rate
            //
            //                // Do UI interaction
            //                lsp_trace("Calling main function");
            //                w.show_ui();
            //                pui->main();
            //                tmr.cancel();
            //
            //                dsp::finish(&ctx);
            //            }
            //            else
            //                lsp_error("Error initializing Jack wrapper");
            //
            //            // Destroy objects
            //            w.disconnect();
            //            if (pui != NULL)
            //            {
            //                pui->destroy();
            //                delete pui;
            //            }
            //            w.destroy();
            //
            //            return status;
        }
    }
}

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
        wrap::jack_wrapper_t *w = &wrap::jack_wrapper;
        w->nSync                = 0;
        w->pPlugin              = NULL;
        w->pUI                  = NULL;
        w->pWrapper             = NULL;
        w->bInterrupt           = false;
        w->nLastReconnect       = 0;

//        lsp_debug_init("jack");
//        init_locale();
        
        // Parse command-line arguments
        wrap::jack_cmdline_t cmdline;
        if ((res = jack_parse_cmdline(&cmdline, plugin_id, argc, argv)) != STATUS_OK)
            return (res == STATUS_CANCELLED) ? 0 : res;

        // Need just to list available plugins?
        if (cmdline.list)
        {
            if ((res = wrap::jack_list_plugins()) != STATUS_OK)
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
        res = wrap::jack_init_wrapper(&wrap::jack_wrapper, cmdline);
        if (res == STATUS_OK) // Try to launch instantiated objects
            res = wrap::jack_plugin_main(&wrap::jack_wrapper);

        // Destroy JACK wrapper
        wrap::jack_destroy_wrapper(&wrap::jack_wrapper);

        // Return result
        return (res == STATUS_OK) ? 0 : -res;
    }

//    LSP_LIBRARY_EXPORT
//    const char *JACK_GET_VERSION()
//    {
//        return LSP_MAIN_VERSION;
//    }

#ifdef __cplusplus
}
#endif /* __cplusplus */
