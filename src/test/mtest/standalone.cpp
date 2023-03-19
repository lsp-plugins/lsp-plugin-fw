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

#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/jack/defs.h>

MTEST_BEGIN("", standalone)

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
        // Obtain list of plugins
        lltl::parray<meta::plugin_t> list;
        const meta::plugin_t *plugin = NULL;
        MTEST_ASSERT(enumerate_plugins(&list) == STATUS_OK);
        if (argc <= 0)
            plugin_not_found(NULL, &list);

        // Check that plugin exists
        for (size_t i=0, n=list.size(); i<n; ++i)
        {
            const meta::plugin_t *meta = list.uget(i);
            if (!strcmp(meta->uid, argv[0]))
            {
                plugin = meta;
                break;
            }
        }

        if (plugin == NULL)
            plugin_not_found(argv[0], &list);

        // Form the list of arguments
        printf("Preparing to call JACK_MAIN_FUNCION\n");
        const char ** args = reinterpret_cast<const char **>(alloca(argc * sizeof(const char *)));
        MTEST_ASSERT(args != NULL);

        args[0] = full_name();
        for (ssize_t i=1; i < argc; ++i)
            args[i] = argv[i];

        printf("Calling JACK_MAIN_FUNCTION\n");

        // Pass the path to resource directory
    #ifdef LSP_IDE_DEBUG
        io::Path resdir;
        resdir.set(tempdir(), "resources");
        system::set_env_var("LSP_RESOURCE_PATH", resdir.as_string());
    #endif /* LSP_IDE_DEBUG */

        // Call the main function
        int result = JACK_MAIN_FUNCTION(argv[0], argc, args);
        MTEST_ASSERT(result == 0);
    }

MTEST_END

#endif /* USE_LIBJACK */
