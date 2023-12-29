/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 янв. 2021 г.
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

#include <lsp-plug.in/test-fw/mtest.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/lltl/parray.h>

#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/jack/defs.h>

#include <private/test/repository.h>

MTEST_BEGIN("", standalone)

    typedef struct config_t
    {
        bool add_resource_path;
        const char *plugin_id;
        const char *resource_path;
        lltl::parray<char> args;
    } config_t;

    void parse_cmdline(config_t *cfg, int argc, const char **argv)
    {
        enum params
        {
            STOP_ARGS,
            PLUGIN_ID,
            EXT_RESOURCES,
            RESOURCE_PATH,
        };
        size_t flags = 0;

        #ifdef LSP_NO_BUILTIN_RESOURCES
            cfg->add_resource_path = true;
        #else
            cfg->add_resource_path = false;
        #endif /* LSP_NO_BUILTIN_RESOURCES */
        cfg->plugin_id = NULL;
        cfg->resource_path = NULL;

        cfg->args.add(const_cast<char *>(full_name()));
        for (ssize_t i=0; i < argc; )
        {
            const char *arg = argv[i++];

            if (!(flags & (1 << STOP_ARGS)))
            {
                if (!strcmp(arg, "--external-resources"))
                {
                    if (flags & (1 << EXT_RESOURCES))
                        MTEST_FAIL_MSG("Parameter '%s' already was specified", arg);

                    cfg->add_resource_path = true;
                    flags |= (1 << EXT_RESOURCES);
                }
                else if (!strcmp(arg, "--resource-path"))
                {
                    if (flags & (1 << RESOURCE_PATH))
                        MTEST_FAIL_MSG("Parameter '%s' already was specified", arg);
                    if (i >= argc)
                        MTEST_FAIL_MSG("The path is required after '%s' option", arg);

                    cfg->resource_path = argv[i++];
                    flags |= (1 << RESOURCE_PATH);
                }
                else if (!(flags & (1 << PLUGIN_ID)))
                {
                    cfg->plugin_id          = arg;
                    flags |= (1 << PLUGIN_ID);
                }
                else
                {
                    cfg->args.add(const_cast<char *>(arg));
                    flags |= (1 << STOP_ARGS);
                }
            }
            else
            {
                cfg->args.add(const_cast<char *>(arg));
            }
        }
    }

    void plugin_not_found(const char *id, lltl::parray<meta::plugin_t> *list)
    {
        // Output list
        if (id == NULL)
            fprintf(stderr, "\nPlugin identifier required to be passed as first parameter. Available identifiers:\n");
        else
            fprintf(stderr, "\nInvalid plugin identifier '%s'. Available identifiers:\n", id);
        for (size_t i=0, n=list->size(); i<n; ++i)
        {
            const meta::plugin_t *meta = list->uget(i);
            fprintf(stderr, "  %s\n", meta->uid);
        }

        MTEST_FAIL_SILENT();
    }

    static ssize_t meta_sort_func(const meta::plugin_t *a, const meta::plugin_t *b)
    {
        return strcmp(a->uid, b->uid);
    }

    status_t enumerate_plugins(lltl::parray<meta::plugin_t> *list)
    {
        // Iterate over all available factories
        for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
        {
            for (size_t i=0; ; ++i)
            {
                // Enumerate next element
                const meta::plugin_t *meta = f->enumerate(i);
                if (meta == NULL)
                    break;

                // Add metadata to list
                if (!list->add(const_cast<meta::plugin_t *>(meta)))
                {
                    fprintf(stderr, "Error adding plugin to list: '%s'\n", meta->uid);
                    return STATUS_NO_MEM;
                }
            }
        }

        // Sort the metadata
        list->qsort(meta_sort_func);

        return STATUS_OK;
    }

    MTEST_MAIN
    {
        // Parse config
        config_t cfg;
        parse_cmdline(&cfg, argc, argv);

        // Obtain list of plugins
        lltl::parray<meta::plugin_t> list;
        const meta::plugin_t *plugin = NULL;
        MTEST_ASSERT(enumerate_plugins(&list) == STATUS_OK);
        if (!cfg.plugin_id)
            plugin_not_found(NULL, &list);

        // Check that plugin exists
        for (size_t i=0, n=list.size(); i<n; ++i)
        {
            const meta::plugin_t *meta = list.uget(i);
            if (!strcmp(meta->uid, cfg.plugin_id))
            {
                plugin = meta;
                break;
            }
        }

        if (plugin == NULL)
            plugin_not_found(cfg.plugin_id, &list);

        // Form the list of arguments
        printf("Calling JACK_MAIN_FUNCTION\n");

        // Pass the path to resource directory
        if (cfg.add_resource_path)
        {
            if (!cfg.resource_path)
            {
                io::Path resdir;
                resdir.set(tempdir(), "resources");

            #ifndef LSP_NO_BUILTIN_RESOURCES
                make_repository(&resdir);
            #endif /* LSP_NO_BUILTIN_RESOURCES */

                system::set_env_var(LSP_RESOURCE_PATH_VAR, resdir.as_string());
            }
            else
                system::set_env_var(LSP_RESOURCE_PATH_VAR, cfg.resource_path);
        }

        // Call the main function
        int result = JACK_MAIN_FUNCTION(cfg.plugin_id, cfg.args.size(), const_cast<const char **>(cfg.args.array()));
        MTEST_ASSERT(result == 0);
    }

MTEST_END

#endif /* USE_LIBJACK */
