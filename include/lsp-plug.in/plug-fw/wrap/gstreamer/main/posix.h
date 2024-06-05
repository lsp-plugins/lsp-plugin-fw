/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_MAIN_POSIX_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_MAIN_POSIX_H_

#ifndef LSP_PLUG_IN_GSTREAMER_MAIN_IMPL
    #error "This header should not be included directly"
#endif /* LSP_PLUG_IN_GSTREAMER_MAIN_IMPL */

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/singletone.h>
#include <lsp-plug.in/common/static.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/plug-fw/wrap/common/libpath.h>
#include <lsp-plug.in/plug-fw/wrap/gstreamer/defs.h>

// System libraries
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

namespace lsp
{
    namespace gst
    {
        static void *instance               = NULL;
        static gst::IFactory *factory       = NULL;
        static lsp::singletone_t library;

    #ifdef ARCH_32BIT
        static const char *core_library_paths[] =
        {
        #ifdef LSP_INSTALL_PREFIX
            LSP_INSTALL_PREFIX "/lib",
            LSP_INSTALL_PREFIX "/lib32",
            LSP_INSTALL_PREFIX "/bin",
            LSP_INSTALL_PREFIX "/sbin",
        #endif /* LSP_INSTALL_PREFIX */

            "/usr/local/lib32",
            "/usr/lib32",
            "/lib32",
            "/usr/local/lib",
            "/usr/lib",
            "/lib",
            "/usr/local/bin",
            "/usr/bin",
            "/bin",
            "/usr/local/sbin",
            "/usr/sbin",
            "/sbin",
            NULL
        };
    #endif /* ARCH_32BIT */

    #ifdef ARCH_64BIT
        static const char *core_library_paths[] =
        {
        #ifdef LSP_INSTALL_PREFIX
            LSP_INSTALL_PREFIX "/lib",
            LSP_INSTALL_PREFIX "/lib64",
            LSP_INSTALL_PREFIX "/bin",
            LSP_INSTALL_PREFIX "/sbin",
        #endif /* LSP_INSTALL_PREFIX */

            "/usr/local/lib64",
            "/usr/lib64",
            "/lib64",
            "/usr/local/lib",
            "/usr/lib",
            "/lib",
            "/usr/local/bin",
            "/usr/bin",
            "/bin",
            "/usr/local/sbin",
            "/usr/sbin",
            "/sbin",
            NULL
        };
    #endif /* ARCH_32_BIT */

        static bool is_shared_object(const char *str)
        {
            size_t len = strlen(str);
            if (len < 3)
                return false;
            str += len - 3;
            return
                (str[0] == '.') &&
                (str[1] == 's') &&
                (str[2] == 'o');
        }

        static gst::IFactory *lookup_factory(void **hInstance, const version_t *required, const char *path)
        {
            lsp_trace("Searching core library at %s", path);

            // Try to open directory
            DIR *d = opendir(path);
            if (d == NULL)
                return NULL;
            lsp_finally {
                closedir(d);
            };

            struct dirent *de;
            char *ptr = NULL;

            while ((de = readdir(d)) != NULL)
            {
                // Free previously used string
                if (ptr != NULL)
                    free(ptr);

                // Skip dot and dotdot
                ptr = de->d_name;
                if ((ptr[0] == '.') && ((ptr[1] == '\0') || ((ptr[1] == '.') && (ptr[2] == '\0'))))
                {
                    ptr = NULL;
                    continue;
                }

                // Allocate path string
                ptr = NULL;
                int n = asprintf(&ptr, "%s" FILE_SEPARATOR_S "%s", path, de->d_name);
                if ((n < 0) || (ptr == NULL))
                    continue;
                lsp_finally {
                    if (ptr != NULL)
                        free(ptr);
                };

                // Need to clarify file type?
                if ((de->d_type == DT_UNKNOWN) || (de->d_type == DT_LNK))
                {
                    struct stat st;
                    if (stat(ptr, &st) < 0)
                        continue;

                    // Patch the d_type value
                    if (S_ISDIR(st.st_mode))
                        de->d_type  = DT_DIR;
                    else if (S_ISREG(st.st_mode))
                        de->d_type  = DT_REG;
                }

                // Scan symbolic link if present
                if (de->d_type == DT_LNK)
                {
                    struct stat st;
                    if (stat(ptr, &st) != 0)
                        continue;

                    if (S_ISDIR(st.st_mode))
                        de->d_type = DT_DIR;
                    else if (S_ISREG(st.st_mode))
                        de->d_type = DT_REG;
                }

                // Analyze file
                if (de->d_type == DT_DIR)
                {
                #ifdef LSP_PLUGIN_ARTIFACT_GROUP
                    // Skip directory entry if it does not contain LSP_PLUGIN_ARTIFACT_GROUP (for example, 'lsp-plugins') in name
                    if ((strlen(LSP_PLUGIN_ARTIFACT_GROUP)) && (strstr(de->d_name, LSP_PLUGIN_ARTIFACT_GROUP) == NULL))
                        continue;
                #endif /* LSP_PLUGIN_ARTIFACT_GROUP */

                    gst::IFactory *f = lookup_factory(hInstance, required, ptr);
                    if (f != NULL)
                        return f;
                }
                else if (de->d_type == DT_REG)
                {
                #ifdef LSP_PLUGIN_LIBRARY_NAME
                    if (strcmp(de->d_name, LSP_PLUGIN_LIBRARY_NAME) != 0)
                        continue;
                #endif /* LSP_PLUGIN_LIBRARY_NAME */

                #ifdef LSP_PLUGIN_ARTIFACT_NAME
                    // Skip directory entry if it does not contain LSP_PLUGIN_ARTIFACT_NAME (for example, 'lsp-plugins') in name
                    if ((strlen(LSP_PLUGIN_ARTIFACT_NAME)) && (strstr(de->d_name, LSP_PLUGIN_ARTIFACT_NAME) == NULL))
                        continue;
                #endif /* LSP_PLUGIN_ARTIFACT_NAME */

                    // Skip library if it doesn't contain gst format specifier
                    if (strstr(de->d_name, "-gstreamer-") == NULL)
                        continue;

                    // Skip library if it doesn't contain 'lsp-plugins' in name
                    if (!is_shared_object(de->d_name))
                        continue;

                    lsp_trace("Trying library %s", ptr);

                    // Try to load library
                    void *inst = dlopen(ptr, RTLD_NOW);
                    if (!inst)
                    {
                        lsp_trace("library %s not loaded: %s", ptr, dlerror());
                        continue;
                    }
                    lsp_finally {
                        if (inst)
                            dlclose(inst);
                    };

                    // Fetch version function
                    module_version_t vf = reinterpret_cast<module_version_t>(dlsym(inst, LSP_VERSION_FUNC_NAME));
                    if (!vf)
                    {
                        lsp_trace("version function %s not found: %s", LSP_VERSION_FUNC_NAME, dlerror());
                        continue;
                    }

                    // Check package version
                    const version_t *ret = vf();
                    if ((ret == NULL) || (ret->branch == NULL))
                    {
                        lsp_trace("No version or bad version returned, ignoring binary", ret);
                        continue;
                    }
                    else if ((ret->major != required->major) ||
                             (ret->minor != required->minor) ||
                             (ret->micro != required->micro) ||
                             (strcmp(ret->branch, required->branch) != 0)
                        )
                    {
                        lsp_trace("wrong version %d.%d.%d '%s' returned, expected %d.%d.%d '%s', ignoring binary",
                                ret->major, ret->minor, ret->micro, ret->branch,
                                required->major, required->minor, required->micro, required->branch
                            );
                        continue;
                    }

                    // Fetch function
                    gst::factory_function_t f = reinterpret_cast<gst::factory_function_t>(dlsym(inst, GSTREAMER_FACTORY_FUNCTION_NAME));
                    if (!f)
                    {
                        lsp_trace("function %s not found: %s", GSTREAMER_FACTORY_FUNCTION_NAME, dlerror());
                        continue;
                    }

                    // Obtain the factory
                    gst::IFactory *factory = f();
                    if (!factory)
                    {
                        lsp_trace("could not obtain factory: %s", GSTREAMER_FACTORY_FUNCTION_NAME, dlerror());
                        continue;
                    }

                    *hInstance = release_ptr(inst);
                    return factory;
                }
            }

            return NULL;
        }

        static gst::IFactory *get_gstreamer_factory(void **hInstance, const version_t *required)
        {
            lsp_debug("Trying to find GStreamer CORE library");

            char path[PATH_MAX+1];
            gst::IFactory *factory = NULL;

            // Try to lookup the same directory the shared object is located
            char *libpath = get_library_path();
            if (libpath != NULL)
            {
                lsp_finally { free(libpath); };
                if ((factory = lookup_factory(hInstance, required, libpath)) != NULL)
                    return factory;
            }

            // Get the home directory
            const char *homedir = getenv("HOME");
            char *buf = NULL;
            lsp_finally {
                if (buf != NULL)
                    delete [] buf;
            };

            if (homedir == NULL)
            {
                struct passwd pwd, *result;
                size_t bufsize;

                bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
                if (bufsize <= 0)           // Value was indeterminate
                    bufsize = 0x10000;          // Should be more than enough

                // Create buffer and fetch home directory
                buf = new char[bufsize];
                if (buf != NULL)
                {
                    if (getpwuid_r(getuid(), &pwd, buf, bufsize, &result) == 0)
                        homedir = result->pw_dir;
                }
            }

            // Scan home directory first
            if ((factory == NULL) && (homedir != NULL))
            {
                lsp_trace("home directory = %s", homedir);
                factory     = lookup_factory(hInstance, required, homedir);

                if (factory == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lib", homedir);
                    factory         = lookup_factory(hInstance, required, path);
                }

                if (factory == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lib64", homedir);
                    factory         = lookup_factory(hInstance, required, path);
                }

                if (factory == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "bin", homedir);
                    factory         = lookup_factory(hInstance, required, path);
                }
            }

            // Scan system directories
            if (factory == NULL)
            {
                for (const char **p = core_library_paths; (p != NULL) && (*p != NULL); ++p)
                {
                    factory         = lookup_factory(hInstance, required, *p);
                    if (factory != NULL)
                        break;
                }
            }

            // Try to lookup additional directories obtained from file mapping
            if (factory == NULL)
            {
                char *libpath = get_library_path();
                if (libpath != NULL)
                {
                    factory         = lookup_factory(hInstance, required, libpath);
                    ::free(libpath);
                }
            }

            if (factory == NULL)
            {
                char **paths = get_library_paths(core_library_paths);
                if (paths != NULL)
                {
                    for (char **p = paths; (p != NULL) && (*p != NULL); ++p)
                    {
                        factory         = lookup_factory(hInstance, required, *p);
                        if (factory != NULL)
                            break;

                        snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lib", *p);
                        factory         = lookup_factory(hInstance, required, path);
                        if (factory != NULL)
                            break;

                        snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lib64", *p);
                        factory         = lookup_factory(hInstance, required, path);
                        if (factory != NULL)
                            break;
                    }

                    free_library_paths(paths);
                }
            }

            // Return factory instance (if present)
            return factory;
        }

        void free_core_library()
        {
            if (instance != NULL)
            {
                dlclose(instance);
                instance = NULL;
                factory = NULL;
            }
        }

        static StaticFinalizer finalizer(free_core_library);

        static gst::IFactory *lookup_factory()
        {
            // Check that data already has been initialized
            if (library.initialized())
                return factory;

            static const lsp::version_t version =
            {
                LSP_PLUGIN_PACKAGE_MAJOR,
                LSP_PLUGIN_PACKAGE_MINOR,
                LSP_PLUGIN_PACKAGE_MICRO,
                LSP_PLUGIN_PACKAGE_BRANCH
            };

            void *dl_instance = NULL;
            gst::IFactory *dl_factory = get_gstreamer_factory(&dl_instance, &version);

            // Commit the loaded factory
            lsp_singletone_init(library) {
                instance    = release_ptr(dl_instance);
                factory     = release_ptr(dl_factory);
            };

            // Release instance if we failed to commit it and return the actual factory
            if (dl_instance != NULL)
                dlclose(dl_instance);

            return factory;
        }

    } // namespace gst
} // namespace lsp

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_MAIN_POSIX_H_ */
