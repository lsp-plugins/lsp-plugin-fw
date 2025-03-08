/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/common/singletone.h>
#include <lsp-plug.in/common/static.h>
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
#include <lsp-plug.in/plug-fw/wrap/jack/factory.h>
#include <lsp-plug.in/plug-fw/wrap/jack/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/jack/impl/factory.h>
#include <lsp-plug.in/plug-fw/wrap/jack/impl/wrapper.h>

#ifdef WITH_UI_FEATURE
    #include <lsp-plug.in/plug-fw/wrap/jack/ui_wrapper.h>
    #include <lsp-plug.in/plug-fw/wrap/jack/impl/ui_wrapper.h>
#endif /* WITH_UI_FEATURE */

#if defined(LSP_TESTING) && (defined(PLATFORM_LINUX) || defined(PLATFORM_BSD))
    #define XDND_PROXY_SUPPORT
    #define IF_XDND_PROXY_SUPPORT(...)  __VA_ARGS__
#else
    #define IF_XDND_PROXY_SUPPORT(...)
#endif

#define RECONNECT_INTERVAL          1000u   /* 1 second     */
#define ICON_SYNC_INTERVAL          200u    /* 5 FPS        */
#define JACK_LOG_FILE               "lsp-jack-lib.log"

namespace lsp
{
    namespace jack
    {
        //---------------------------------------------------------------------
        static lsp::singletone_t library;
        static Factory *plugin_factory = NULL;

        static Factory *get_plugin_factory()
        {
            if (!library.initialized())
            {
                // Initialize DSP
                dsp::init();

                // Create new factory and set trigger for disposal
                Factory *factory      = new Factory();
                if (factory == NULL)
                    return NULL;
                lsp_finally {
                    if (factory != NULL)
                        factory->release();
                };
                lsp_trace("created factory %p", factory);

                // Commit the factory
                lsp_singletone_init(library) {
                    lsp::swap(plugin_factory, factory);
                };
            }

            // Return the obtained factory
            Factory *result = plugin_factory;
            result->acquire();
            lsp_trace("returning factory %p", result);

            return result;
        }

        void drop_factory()
        {
            if (plugin_factory != NULL)
            {
                lsp_trace("releasing plugin factory %p", plugin_factory);
                if (plugin_factory->release() == 0)
                    plugin_factory = NULL;
            }
        };

        //---------------------------------------------------------------------
        // Static finalizer for the list of descriptors at library finalization
        static StaticFinalizer finalizer(drop_factory);

        //---------------------------------------------------------------------
        // JACK plugin loop definitions
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

        struct LSP_HIDDEN_MODIFIER PluginLoop: public IPluginLoop
        {
            public:
                cmdline_t           sCmdLine;           // Command-line arguments
                resource::ILoader  *pLoader;            // Resource loader
                jack::Factory      *pFactory;           // Plugin factory
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

            private:
                void                wait_for_events(wssize_t delay);
                status_t            init_modules();
                status_t            sync_state(ws::timestamp_t sched, ws::timestamp_t ctime);
                void                load_configuration_file(const char *cfg_file);

            public:
                PluginLoop();
                PluginLoop(const PluginLoop &) = delete;
                PluginLoop(PluginLoop &&) = delete;
                PluginLoop & operator = (const PluginLoop &) = delete;
                PluginLoop & operator = (PluginLoop &&) = delete;
                virtual ~PluginLoop() override;

            public: // IPluginLoop
                virtual int         run() override;
                virtual void        cancel() override;

            public:
                status_t init(const char *plugin_id, int argc, const char **argv);

        };

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
            snprintf(fmt, sizeof(fmt), "  %%%ds  %%s\n", -int(maxlen));

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

        PluginLoop::PluginLoop()
        {
            sCmdLine.cfg_file       = NULL;
            sCmdLine.plugin_id      = NULL;
            sCmdLine.parent_id      = NULL;
            sCmdLine.headless       = false;
            sCmdLine.list           = false;
            sCmdLine.version        = false;

            pFactory                = NULL;
            pLoader                 = NULL;
            pPlugin                 = NULL;
            pWrapper                = NULL;
            nLastReconnect          = 0;
        #ifdef WITH_UI_FEATURE
            pUI                     = NULL;
            pUIWrapper              = NULL;
            nLastIconSync           = 0;
        #endif /* WITH_UI_FEATURE */
            pRouting                = NULL;
            bNotify                 = true;
            bInterrupt              = false;
        }

        PluginLoop::~PluginLoop()
        {
            // Disconnect the wrapper
            if (pWrapper != NULL)
                pWrapper->disconnect();

        #ifdef WITH_UI_FEATURE
            // Destroy plugin UI
            if (pUI != NULL)
            {
                pUI->pre_destroy();
                pUI->destroy();
                delete pUI;
                pUI             = NULL;
            }

            // Destroy UI wrapper
            if (pUIWrapper != NULL)
            {
                pUIWrapper->destroy();
                delete pUIWrapper;
                pUIWrapper      = NULL;
            }
        #endif /* WITH_UI_FEATURE */

            // Destroy plugin
            if (pPlugin != NULL)
            {
                pPlugin->destroy();
                delete pPlugin;
                pPlugin         = NULL;
            }

            // Destroy wrapper
            if (pWrapper != NULL)
            {
                pWrapper->destroy();
                delete pWrapper;
                pWrapper        = NULL;
            }

            // Destroy resource loader
            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader         = NULL;
            }

            // Release factory
            if (pFactory != NULL)
            {
                pFactory->release();
                pFactory        = NULL;
            }

            // Destroy command line
            destroy_cmdline(&sCmdLine);
        }

        status_t PluginLoop::init(const char *plugin_id, int argc, const char **argv)
        {
            lsp_trace("Initializing plugin loop");

            status_t res            = STATUS_OK;

            // Parse command-line arguments
            if ((res = parse_cmdline(&sCmdLine, plugin_id, argc, argv)) != STATUS_OK)
                return (res == STATUS_CANCELLED) ? 0 : res;

            // Need just to output version?
            if (sCmdLine.version)
            {
                if ((res = jack::output_version(sCmdLine)) != STATUS_OK)
                    return -res;
                return 0;
            }

            // Need just to list available plugins?
            if (sCmdLine.list)
            {
                if ((res = jack::list_plugins()) != STATUS_OK)
                    return -res;
                return 0;
            }

            // Plugin identifier has been specified?
            if (sCmdLine.plugin_id == NULL)
            {
                fprintf(stderr, "Not specified plugin identifier, exiting\n");
                return -STATUS_NOT_FOUND;
            }

            // Output routing if specified
            if (!sCmdLine.routing.is_empty())
            {
                printf("JACK connection routing:\n");
                for (size_t i=0, n=sCmdLine.routing.size(); i<n; ++i)
                {
                    jack::connection_t *conn = sCmdLine.routing.uget(i);
                    if (conn != NULL)
                        printf("%s -> %s\n", conn->src, conn->dst);
                }
                printf("\n");
            }

            // Get the factory
            if ((pFactory = get_plugin_factory()) == NULL)
            {
                lsp_error("Could not obtain plugin factory");
                return STATUS_NO_DATA;
            }

            // Create the resource loader
            if ((pLoader = core::create_resource_loader()) == NULL)
            {
                lsp_error("No resource loader available");
                return STATUS_NO_DATA;
            }

            // Create plugin module
            if ((res = pFactory->create_plugin(&pPlugin, sCmdLine.plugin_id)) != STATUS_OK)
            {
                if (res == STATUS_NOT_FOUND)
                    fprintf(stderr, "Unknown plugin identifier: %s\n", sCmdLine.plugin_id);
                else
                    fprintf(stderr, "Error instantiating plugin '%s': code=%d\n", sCmdLine.plugin_id, int(res));

                return res;
            }

        #ifdef WITH_UI_FEATURE
            // Need to instantiate plugin UI?
            if (!sCmdLine.headless)
            {
                if ((res = pFactory->create_ui(&pUI, sCmdLine.plugin_id)) != STATUS_OK)
                {
                    if (res != STATUS_NOT_FOUND)
                    {
                        fprintf(stderr, "Error instantiating UI for plugin '%s': code=%d\n", sCmdLine.plugin_id, int(res));
                        return res;
                    }

                    fprintf(stderr, "Not found UI for plugin: %s, will continue in headless mode\n", sCmdLine.plugin_id);
                }
            }
        #endif /* WITH_UI_FEATURE */

            // Initialize plugin wrapper
            pRouting        = &sCmdLine.routing;
            pWrapper        = new jack::Wrapper(pFactory, pPlugin, pLoader);
            if (pWrapper == NULL)
                return STATUS_NO_MEM;

            if ((res = pWrapper->init()) != STATUS_OK)
                return res;

        #ifdef WITH_UI_FEATURE
            // Initialize UI wrapper
            if (pUI != NULL)
            {
                // Create UI wrapper
                pUIWrapper      = new jack::UIWrapper(pWrapper, pLoader, pUI);
                if (pUIWrapper == NULL)
                    return STATUS_NO_MEM;

                // Initialize wrapper
                if ((res = pUIWrapper->init(NULL)) != STATUS_OK)
                    return res;
            }
        #endif /* WITH_UI_FEATURE */

            // Load configuration (if specified in parameters)
            if (sCmdLine.cfg_file != NULL)
                load_configuration_file(sCmdLine.cfg_file);

            return STATUS_OK;
        }

        status_t PluginLoop::run()
        {
            status_t res            = STATUS_OK;
            ws::timestamp_t period  = 40; // 40 ms period (25 frames per second)

            // Cleanup interrupt flag
            bInterrupt              = false;

        #ifdef WITH_UI_FEATURE
            // Show the widget
            if (pUI != NULL)
            {
                tk::Widget * wnd        = pUI->root();
                if (wnd != NULL)
                    wnd->show();
            }
        #endif /* WITH_UI_FEATURE */

            // Initialize wrapper structure and set-up signal handlers
            while (!bInterrupt)
            {
                const system::time_millis_t ts1 = system::get_time_millis();

                // Synchronize with JACK
                if ((res = sync_state(ts1, ts1)) != STATUS_OK)
                {
                    fprintf(stderr, "Unexpected error, code=%d", res);
                    return res;
                }

            #ifdef WITH_UI_FEATURE
                // Perform main event loop for the UI
                if (pUIWrapper != NULL)
                {
                    dsp::context_t ctx;
                    dsp::start(&ctx);

                    pUIWrapper->main_iteration();
                    if (!bInterrupt)
                        bInterrupt      = pUIWrapper->main_loop_interrupted();

                    dsp::finish(&ctx);
                }
            #endif /* WITH_UI_FEATURE */

                // Perform a small sleep before new iteration
                const system::time_millis_t ts2 = system::get_time_millis();
                const wssize_t delay    = lsp_max(ts1 + period - ts2, period);
                wait_for_events(delay);
            }

            fprintf(stderr, "\nPlugin execution interrupted\n");

            return res;
        }

        void PluginLoop::load_configuration_file(const char *cfg_file)
        {
            status_t res;

        #ifdef WITH_UI_FEATURE
            if (pUIWrapper != NULL)
            {
                if ((res = pUIWrapper->import_settings(cfg_file, false)) != STATUS_OK)
                    fprintf(stderr, "Error loading configuration file: '%s': %s\n", cfg_file, get_status(res));

                return;
            }
        #endif /* WITH_UI_FEATURE */

            if (pWrapper != NULL)
            {
                if ((res = pWrapper->import_settings(cfg_file)) != STATUS_OK)
                    fprintf(stderr, "Error loading configuration file: '%s': %s\n", cfg_file, get_status(res));
                return;
            }

            fprintf(stderr, "Error loading configuration file: '%s': no accessible wrapper\n", cfg_file);
        }

        status_t PluginLoop::sync_state(ws::timestamp_t sched, ws::timestamp_t ctime)
        {
            jack::Wrapper *jw       = pWrapper;
        #ifdef WITH_UI_FEATURE
            jack::UIWrapper *uw     = pUIWrapper;
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
                nLastReconnect      = ctime;
            }

            // If we are currently in disconnected state - try to perform a connection
            if (jw->disconnected())
            {
                // Try each second to make new connection
                if ((ctime - nLastReconnect) >= RECONNECT_INTERVAL)
                {
                    printf("Trying to connect to JACK\n");
                    if (jw->connect() == STATUS_OK)
                    {
                        if (!pRouting->is_empty())
                        {
                            printf("Connecting ports...");
                            jw->set_routing(pRouting);
                        }

                        printf("Successfully connected to JACK\n");
                        bNotify             = true;
                    }
                    nLastReconnect      = ctime;
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
                    if (bNotify)
                    {
                        uw->notify_all();
                        bNotify         = false;
                    }

                    // Update icon
                    if ((ctime - nLastIconSync) > ICON_SYNC_INTERVAL)
                    {
                        uw->sync_inline_display();
                        nLastIconSync   = ctime;
                    }
                }
            #endif /* WITH_UI_FEATURE */
            }

            return STATUS_OK;
        }

        void PluginLoop::wait_for_events(wssize_t delay)
        {
            if (delay <= 0)
                return;

        #ifdef WITH_UI_FEATURE
            if (pUIWrapper)
            {
                pUIWrapper->display()->wait_events(delay);
                return;
            }
        #endif /* WITH_UI_FEATURE */

            system::sleep_msec(delay);
        }

        void PluginLoop::cancel()
        {
            bInterrupt      = true;
        }
    } /* namespace jack */
} /* namespace lsp */

namespace lsp
{
    IPluginLoop::~IPluginLoop()
    {
    }
} /* namespaec lsp */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    using namespace lsp;

    LSP_EXPORT_MODIFIER
    status_t JACK_CREATE_PLUGIN_LOOP(lsp::IPluginLoop **loop, const char *plugin_id, int argc, const char **argv)
    {
    #ifndef LSP_IDE_DEBUG
        IF_DEBUG( lsp::debug::redirect(JACK_LOG_FILE); );
    #endif /* LSP_IDE_DEBUG */

        // Initialize DSP
        dsp::init();

        // Create loop
        jack::PluginLoop *w         = new jack::PluginLoop();
        if (w == NULL)
            return STATUS_NO_MEM;
        lsp_finally {
            if (w != NULL)
                delete w;
        };

        // Initialize loop
        status_t res = w->init(plugin_id, argc, argv);
        if (res != STATUS_OK)
            return res;

        // Return loop
        *loop       = release_ptr(w);
        return STATUS_OK;
    }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* USE_LIBJACK */
