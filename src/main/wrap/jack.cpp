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
            size_t          nSync;
            JACKWrapper    *pWrapper;
            LSPWindow      *pWindow;
            timespec        nLastReconnect;
        } jack_wrapper_t;

        typedef struct jack_cmdline_t
        {
            const char     *cfg_file;
            const char     *plugin_id;
            void           *parent_id;
            bool            headless;
            bool            list;
        } jack_config_t;

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

        ssize_t jack_metadata_sort_func(const meta::plugin_t *a, const meta::plugin_t *b)
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

        status_t jack_create_plugin(plug::Module **plug, const char *id)
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
                        if ((*plug = f->create(meta)) != NULL)
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

        status_t jack_create_ui(ui::Module **pui, const char *id)
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
                        if ((*pui = f->create(meta)) != NULL)
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

        static status_t jack_ui_sync(ws::timestamp_t time, void *arg)
        {
            if (arg == NULL)
                return STATUS_BAD_STATE;

            jack_wrapper_t *wrapper = static_cast<jack_wrapper_t *>(arg);
            JACKWrapper *jw         = wrapper->pWrapper;

            // If connection to JACK was lost - notify
            if (jw->connection_lost())
            {
                lsp_trace("Connection to JACK was lost");

                // Perform disconnect action
                jw->disconnect();

                // Remember last connection time
                clock_gettime(CLOCK_REALTIME, &wrapper->nLastReconnect);
            }

            // If we are currently in disconnected state - try to perform a connection
            if (jw->disconnected())
            {
                timespec ctime;
                clock_gettime(CLOCK_REALTIME, &ctime);
                wssize_t delta = (ctime.tv_sec - wrapper->nLastReconnect.tv_sec) * 1000 + (ctime.tv_nsec - wrapper->nLastReconnect.tv_nsec) / 1000000;

                // Try each second to make new connection
                if (delta >= 1000)
                {
                    lsp_trace("Trying to connect to JACK");
                    if (jw->connect() == STATUS_OK)
                    {
                        lsp_trace("Successful connected to JACK");
                        wrapper->nSync = 0;
                    }
                    wrapper->nLastReconnect     = ctime;
                }
            }

            // If we are connected - do usual stuff
            if (jw->connected())
            {
                if (!(wrapper->nSync++))
                    wrapper->pWindow->query_resize();
            }

            // Transfer changes from DSP to UI
            wrapper->pWrapper->transfer_dsp_to_ui();

            return STATUS_OK;
        }

        int jack_plugin_main(
            const jack_cmdline_t &cfg,
            plug::Module *plugin, ui::Module *pui
        )
        {
            int status               = STATUS_OK;
            jack_wrapper_t  wrapper;

            // Create wrapper
            lsp_trace("Creating wrapper");
            JACKWrapper w(plugin, pui);

            // Initialize
            lsp_trace("Initializing wrapper");
            status                  = w.init(argc, argv);

            // Load configuration (if specified in parameters)
            if ((status == STATUS_OK) && (cfg.cfg_file != NULL))
            {
                status = pui->import_settings(cfg.cfg_file, false);
                if (status != STATUS_OK)
                    fprintf(stderr, "Error loading configuration file: %s\n", get_status(status));
            }

            // Enter the main lifecycle
            if (status == STATUS_OK)
            {
                dsp::context_t ctx;
                dsp::start(&ctx);

                // Perform initial connection
                lsp_trace("Connecting to JACK server");
                w.connect();
                clock_gettime(CLOCK_REALTIME, &wrapper.nLastReconnect);

                // Create timer for transferring DSP -> UI data
                lsp_trace("Creating timer");
                wrapper.nSync       = 0;
                wrapper.pWrapper    = &w;
                wrapper.pWindow     = pui->root_window();

                LSPTimer tmr;
                tmr.bind(pui->display());
                tmr.set_handler(jack_ui_sync, &wrapper);
                tmr.launch(0, 40); // 25 Hz rate

                // Do UI interaction
                lsp_trace("Calling main function");
                w.show_ui();
                pui->main();
                tmr.cancel();

                dsp::finish(&ctx);
            }
            else
                lsp_error("Error initializing Jack wrapper");

            // Destroy objects
            w.disconnect();
            if (pui != NULL)
            {
                pui->destroy();
                delete pui;
            }
            w.destroy();

            return status;
        }
    }
}

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    using namespace lsp;

    int JACK_MAIN_FUNCTION(const char *plugin_id, int argc, const char **argv)
    {
        status_t res        = STATUS_OK;
        plug::Module *plug  = NULL;
        ui::Module *pui     = NULL;

        #ifdef PLATFORM_POSIX
            signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE signal since JACK can suddenly lose socket connection
        #endif

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

        // Create plugin module
        if ((res = wrap::jack_create_plugin(&plug, plugin_id)) != STATUS_OK)
            return -res;

        // Need to instantiate plugin UI?
        if (!cmdline.headless)
        {
            if ((res = wrap::jack_create_ui(&pui, plugin_id)) != STATUS_OK)
                return -res;
        }

        // Try to launch instantiated objects
        res = jack_plugin_main(cmdline, plug, pui);

        // Destroy objects
        if (pui != NULL)
        {
            pui->destroy();
            delete pui;
        }
        if (plug != NULL)
        {
            plug->destroy();
            delete plug;
        }

        // Output error
        return -res;
    }

//    LSP_LIBRARY_EXPORT
//    const char *JACK_GET_VERSION()
//    {
//        return LSP_MAIN_VERSION;
//    }

#ifdef __cplusplus
}
#endif /* __cplusplus */
