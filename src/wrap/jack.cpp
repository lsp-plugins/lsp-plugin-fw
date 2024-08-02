/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 мая 2016 г.
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

#ifdef USE_LIBJACK

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/ws/ws.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/ipc/Library.h>

#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/ui.h>

#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/wrap/jack/defs.h>
#include <lsp-plug.in/plug-fw/wrap/jack/types.h>
#include <lsp-plug.in/plug-fw/wrap/jack/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/jack/impl/wrapper.h>

#ifdef WITH_UI_FEATURE
    #include <lsp-plug.in/plug-fw/wrap/jack/ui_wrapper.h>
    #include <lsp-plug.in/plug-fw/wrap/jack/impl/ui_wrapper.h>
#endif /* WITH_UI_FEATURE */

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
            resource::ILoader  *pLoader;            // Resource loader
            plug::Module       *pPlugin;            // Plugin
            jack::Wrapper      *pWrapper;           // Plugin wrapper
            ws::timestamp_t     nLastReconnect;     // Last connection time
        #ifdef WITH_UI_FEATURE
            ui::Module         *pUI;                // Plugin UI
            jack::UIWrapper    *pUIWrapper;         // Plugin UI wrapper
            ws::timestamp_t     nLastIconSync;      // Last icon synchronization time
        #endif /* WITH_UI_FEATURE */
            const lltl::darray<connection_t> *pRouting;  // Routing
            bool                bNotify;            // Notify all ports
            volatile bool       bInterrupt;         // Interrupt signal received
        } wrapper_t;

        typedef struct cmdline_t
        {
            const char     *cfg_file;
            const char     *plugin_id;
            void           *parent_id;
            bool            headless;
            bool            list;
            bool            version;
            lltl::darray<connection_t> routing;
        } cmdline_t;

        // JACK wrapper
        static wrapper_t  wrapper;

        static status_t add_connection(cmdline_t *cfg, LSPString *src, LSPString *dst)
        {
            if ((src == NULL) || (src->is_empty()))
            {
                fprintf(stderr, "Not specified source JACK port name in connection string\n");
                return STATUS_INVALID_VALUE;
            }
            if ((dst == NULL) || (dst->is_empty()))
            {
                fprintf(stderr, "Not specified destination JACK port name in connection string\n");
                return STATUS_INVALID_VALUE;
            }

            connection_t *conn = cfg->routing.add();
            if (conn == NULL)
                return STATUS_NO_MEM;
            conn->src   = NULL;
            conn->dst   = NULL;

            conn->src   = src->clone_utf8();
            conn->dst   = dst->clone_utf8();

            if ((conn->src == NULL) || (conn->dst == NULL))
                return STATUS_NO_MEM;

            return STATUS_OK;
        }

        static status_t parse_connection(cmdline_t *cfg, const char *string)
        {
            status_t res;
            LSPString text, src, dst, *str;
            if (!text.set_native(string))
                return STATUS_NO_MEM;

            str = &src;
            lsp_wchar_t curr = 0;
            size_t count = 0;
            for (size_t i=0, n=text.length(); i<n; ++i)
            {
                lsp_wchar_t prev    = curr;
                curr                = text.char_at(i);
                if (prev == '\\')
                {
                    switch (curr)
                    {
                        case '\\':
                        case '/':
                        case ' ':
                        case '=':
                        case ',':
                            break;
                        case 'r': curr = '\r'; break;
                        case 'n': curr = '\n'; break;
                        case 't': curr = '\t'; break;
                        case 'v': curr = '\v'; break;
                        default:
                            if (!str->append(prev))
                                return STATUS_NO_MEM;
                            break;
                    }
                    if (!str->append(curr))
                        return STATUS_NO_MEM;
                    ++count;
                    curr    = 0;
                }
                else
                {
                    switch (curr)
                    {
                        case '\\':
                            break;
                        case '=':
                            ++count;
                            if (str == &dst)
                            {
                                if (!str->append(curr))
                                    return STATUS_NO_MEM;
                            }
                            else // dst == &key
                                str = &dst;
                            break;
                        case ',':
                            // Add and cleanup strings
                            if ((res = add_connection(cfg, &src, &dst)) != STATUS_OK)
                                return res;
                            str     = &src;
                            src.clear();
                            dst.clear();
                            count = 0;
                            break;
                        default:
                            if (!str->append(curr))
                                return STATUS_NO_MEM;
                            ++count;
                            break;
                    }
                }
            }

            // Add last connection if present
            if (count > 0)
            {
                if ((res = add_connection(cfg, &src, &dst)) != STATUS_OK)
                    return res;
            }

            return STATUS_OK;
        }

        status_t parse_cmdline(cmdline_t *cfg, const char *plugin_id, int argc, const char **argv)
        {
            status_t res;

            // Initialize config with default values
            cfg->cfg_file       = NULL;
            cfg->plugin_id      = NULL;
            cfg->parent_id      = NULL;
            cfg->headless       = false;
            cfg->list           = false;
            cfg->version        = false;

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
                    printf("  -c, --config <file>       Load settings file on startup\n");
                    IF_XDND_PROXY_SUPPORT(
                        printf("  --dnd-proxy <id>          Create window as child and DnD proxy of specified window ID\n");
                    )
                    printf("  -h, --help                Output help\n");
                    printf("  -hl, --headless           Launch in console only, without UI\n");
                    if (plugin_id == NULL)
                        printf("  -l, --list                List available plugin identifiers\n");
                    printf("  -v, --version             Output the version of the software\n");
                    printf("  -x, --connect <src>=<dst> Connect input/output JACK port to another\n");
                    printf("                            input/output JACK port when JACK connection\n");
                    printf("                            is estimated. Multiple options are allowed,\n");
                    printf("                            the connection <src>=<dst> pais can be separated\n");
                    printf("                            by comma. Use backslash for escaping characters\n");
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
                else if ((!::strcmp(arg, "--version")) || (!::strcmp(arg, "-v")))
                    cfg->version        = true;
                else if ((plugin_id == NULL) && ((!::strcmp(arg, "--list")) || (!::strcmp(arg, "-l"))))
                    cfg->list           = true;
                else if ((plugin_id == NULL) && (cfg->plugin_id == NULL))
                    cfg->plugin_id      = argv[i++];
                else if ((!::strcmp(arg, "--connect")) || (!::strcmp(arg, "-x")))
                {
                    if (i >= argc)
                    {
                        fprintf(stderr, "Not specified connection string for '%s' parameter\n", arg);
                        return STATUS_BAD_ARGUMENTS;
                    }
                    const char *conn = argv[i++];
                    if ((res = parse_connection(cfg, conn)) != STATUS_OK)
                    {
                        fprintf(stderr, "Error in connection string for '%s' parameter: '%s'\n", arg, conn);
                        return res;
                    }
                }
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

        void destroy_cmdline(cmdline_t *cfg)
        {
            for (size_t i=0, n=cfg->routing.size(); i<n; ++i)
            {
                connection_t *c = cfg->routing.uget(i);
                if (c == NULL)
                    continue;
                if (c->src != NULL)
                    free(const_cast<char *>(c->src));
                if (c->dst != NULL)
                    free(const_cast<char *>(c->dst));
            }
            cfg->routing.flush();
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

        const meta::plugin_t *find_plugin(const char *id)
        {
            if (id == NULL)
                return NULL;

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
                        return meta;
                }
            }

            // No plugin has been found
            return NULL;
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

    #ifdef WITH_UI_FEATURE
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
    #endif /* WITH_UI_FEATURE */

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

        #ifdef WITH_UI_FEATURE
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
        #endif /* WITH_UI_FEATURE */

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

        static status_t output_version(const cmdline_t &cmdline)
        {
            // Create the resource loader
            resource::ILoader *loader = core::create_resource_loader();
            if (loader == NULL)
            {
                lsp_error("No resource loader available");
                return STATUS_NO_DATA;
            }
            lsp_finally { delete loader; };

            // Load package information
            io::IInStream *is = loader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
            if (is == NULL)
            {
                lsp_error("No manifest.json found in resources");
                return STATUS_BAD_STATE;
            }
            lsp_finally {
                is->close();
                delete is;
            };

            // Load manifest
            meta::package_t *manifest = NULL;
            status_t res = meta::load_manifest(&manifest, is);
            if (res != STATUS_OK)
            {
                lsp_error("Error while reading manifest file, error: %d", int(res));
                return res;
            }
            lsp_finally { meta::free_manifest(manifest); };

            // Find related plugin metadata
            const meta::plugin_t *plug = find_plugin(cmdline.plugin_id);

            printf("Package name:      %s\n", manifest->artifact_name);
            printf("Package version:   %d.%d.%d\n",
                manifest->version.major,
                manifest->version.minor,
                manifest->version.micro);
            if (plug != NULL)
            {
                printf("Plugin name:       %s\n", plug->description);
                printf("Plugin version:    %d.%d.%d\n",
                    plug->version.major,
                    plug->version.minor,
                    plug->version.micro);
            }

            // Output the version
            return STATUS_OK;
        }

        static void load_configuration_file(wrapper_t *w, const char *cfg_file)
        {
            status_t res;

        #ifdef WITH_UI_FEATURE
            if (w->pUIWrapper != NULL)
            {
                if ((res = w->pUIWrapper->import_settings(cfg_file, false)) != STATUS_OK)
                    fprintf(stderr, "Error loading configuration file: '%s': %s\n", cfg_file, get_status(res));

                return;
            }
        #endif /* WITH_UI_FEATURE */

            if (w->pWrapper != NULL)
            {
                if ((res = w->pWrapper->import_settings(cfg_file)) != STATUS_OK)
                    fprintf(stderr, "Error loading configuration file: '%s': %s\n", cfg_file, get_status(res));
                return;
            }

            fprintf(stderr, "Error loading configuration file: '%s': no accessible wrapper\n", cfg_file);
        }

        static status_t init_wrapper(wrapper_t *w, const cmdline_t &cmdline)
        {
            status_t res;

            lsp_trace("Initializing wrapper");

            // Create the resource loader
            if ((w->pLoader = core::create_resource_loader()) == NULL)
            {
                lsp_error("No resource loader available");
                return STATUS_NO_DATA;
            }

            // Create plugin module
            if ((res = create_plugin(w, cmdline.plugin_id)) != STATUS_OK)
                return res;

        #ifdef WITH_UI_FEATURE
            // Need to instantiate plugin UI?
            if (!cmdline.headless)
            {
                if ((res = create_ui(w, cmdline.plugin_id)) != STATUS_OK)
                    return res;
            }
        #endif /* WITH_UI_FEATURE */

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
            w->pRouting     = &cmdline.routing;
            w->pWrapper     = new jack::Wrapper(w->pPlugin, w->pLoader);
            if (w->pWrapper == NULL)
                return STATUS_NO_MEM;

            if ((res = w->pWrapper->init()) != STATUS_OK)
                return res;

        #ifdef WITH_UI_FEATURE
            // Initialize UI wrapper
            if (w->pUI != NULL)
            {
                // Create wrapper
                w->pUIWrapper   = new jack::UIWrapper(w->pWrapper, w->pLoader, w->pUI);
                if (w->pUIWrapper == NULL)
                    return STATUS_NO_MEM;

                // Initialize wrapper
                if ((res = w->pUIWrapper->init(NULL)) != STATUS_OK)
                    return res;

                // Show the widget
                tk::Widget * wnd = w->pUI->root();
                wnd->show();
            }
        #endif /* WITH_UI_FEATURE */

            // Load configuration (if specified in parameters)
            if (cmdline.cfg_file != NULL)
                load_configuration_file(w, cmdline.cfg_file);

            return STATUS_OK;
        }

        static status_t sync_state(ws::timestamp_t sched, ws::timestamp_t ctime, void *arg)
        {
            if (arg == NULL)
                return STATUS_BAD_STATE;

            wrapper_t *w            = static_cast<wrapper_t *>(arg);
            jack::Wrapper *jw       = w->pWrapper;
        #ifdef WITH_UI_FEATURE
            jack::UIWrapper *uw     = w->pUIWrapper;
        #endif /* WITH_UI_FEATURE */

            // If connection to JACK was lost - notify
            if (jw->connection_lost())
            {
                // Disconnect wrapper and remember last connection time
                fprintf(stderr, "Connection to JACK has been lost\n");
                jw->disconnect();
            #ifdef WITH_UI_FEATURE
                if (uw != NULL)
                    uw->connection_lost();
            #endif /* WITH_UI_FEATURE */
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
                        if (!w->pRouting->is_empty())
                        {
                            printf("Connecting ports...");
                            jw->set_routing(w->pRouting);
                        }

                        printf("Successfully connected to JACK\n");
                        w->bNotify      = true;
                    }
                    w->nLastReconnect   = ctime;
                }
            }

            // If we are connected - do usual stuff with UI
            if (jw->connected())
            {
            #ifdef WITH_UI_FEATURE
                // Sync state (transfer DSP to UI)
                if (uw != NULL)
                {
                    // Transfer changes from DSP to UI
                    uw->sync(sched);
                    if (w->bNotify)
                    {
                        uw->notify_all();
                        w->bNotify      = false;
                    }

                    // Update icon
                    if ((ctime - w->nLastIconSync) > ICON_SYNC_INTERVAL)
                    {
                        uw->sync_inline_display();
                        w->nLastIconSync = ctime;
                    }
                }
            #endif /* WITH_UI_FEATURE */
            }

            return STATUS_OK;
        }

        static void wait_for_events(wrapper_t *w, wssize_t delay)
        {
            if (delay <= 0)
                return;

        #ifdef WITH_UI_FEATURE
            if (w->pUIWrapper)
            {
                w->pUIWrapper->display()->wait_events(delay);
                return;
            }
        #endif /* WITH_UI_FEATURE */

            system::sleep_msec(delay);
        }

        status_t plugin_main(wrapper_t *w)
        {
            status_t res            = STATUS_OK;
            ws::timestamp_t period  = 40; // 40 ms period (25 frames per second)

            // Initialize wrapper structure and set-up signal handlers
            while (!w->bInterrupt)
            {
                const system::time_millis_t ts1 = system::get_time_millis();

                // Synchronize with JACK
                if ((res = sync_state(ts1, ts1, w)) != STATUS_OK)
                {
                    fprintf(stderr, "Unexpected error, code=%d", res);
                    return res;
                }

            #ifdef WITH_UI_FEATURE
                // Perform main event loop for the UI
                if (w->pUIWrapper != NULL)
                {
                    dsp::context_t ctx;
                    dsp::start(&ctx);

                    w->pUIWrapper->main_iteration();
                    if (!w->bInterrupt)
                        w->bInterrupt   = w->pUIWrapper->main_loop_interrupted();

                    dsp::finish(&ctx);
                }
            #endif /* WITH_UI_FEATURE */

                // Perform a small sleep before new iteration
                const system::time_millis_t ts2 = system::get_time_millis();
                const wssize_t delay    = lsp_max(ts1 + period - ts2, period);
                wait_for_events(w, delay);
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

    LSP_EXPORT_MODIFIER
    int JACK_MAIN_FUNCTION(const char *plugin_id, int argc, const char **argv)
    {
        status_t res            = STATUS_OK;

    #ifndef LSP_IDE_DEBUG
        IF_DEBUG( lsp::debug::redirect("lsp-jack-lib.log"); );
    #endif /* LSP_IDE_DEBUG */
        
        // Parse command-line arguments
        jack::cmdline_t cmdline;
        lsp_finally { destroy_cmdline(&cmdline); };
        if ((res = parse_cmdline(&cmdline, plugin_id, argc, argv)) != STATUS_OK)
            return (res == STATUS_CANCELLED) ? 0 : res;

        // Need just to output version?
        if (cmdline.version)
        {
            if ((res = jack::output_version(cmdline)) != STATUS_OK)
                return -res;
            return 0;
        }

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

        // Output routing if specified
        if (!cmdline.routing.is_empty())
        {
            printf("JACK connection routing:\n");
            for (size_t i=0, n=cmdline.routing.size(); i<n; ++i)
            {
                jack::connection_t *conn = cmdline.routing.uget(i);
                if (conn != NULL)
                    printf("%s -> %s\n", conn->src, conn->dst);
            }
            printf("\n");
        }

        // Initialize DSP
        dsp::init();

        // Initialize wrapper with empty data
        jack::wrapper_t *w      = &jack::wrapper;
        w->pLoader              = NULL;
        w->pPlugin              = NULL;
        w->pWrapper             = NULL;
        w->nLastReconnect       = 0;
    #ifdef WITH_UI_FEATURE
        w->pUI                  = NULL;
        w->pUIWrapper           = NULL;
        w->nLastIconSync        = 0;
    #endif /* WITH_UI_FEATURE */
        w->bNotify              = true;
        w->bInterrupt           = false;

        // Initialize plugin wrapper
        res = jack::init_wrapper(w, cmdline);

        // Destroy JACK wrapper at the end
        lsp_finally { jack::destroy_wrapper(w); };

        // Launch the main event loop
        if (res == STATUS_OK)
            res = jack::plugin_main(w);

        // Return result
        return (res == STATUS_OK) ? 0 : -res;
    }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* USE_LIBJACK */
