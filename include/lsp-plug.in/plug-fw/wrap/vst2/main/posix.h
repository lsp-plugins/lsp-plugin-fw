/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 18 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_POSIX_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_POSIX_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_VST2_MAIN_IMPL
    #error "This header should not be included directly"
#endif /* LSP_PLUG_IN_VST2_MAIN_IMPL */

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/static.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/defs.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/main.h>
#include <lsp-plug.in/plug-fw/wrap/common/libpath.h>
#include <steinberg/vst2.h>

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
    namespace vst2
    {
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
            "/usr/lib" ,
            "/lib",
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
            "/usr/lib" ,
            "/lib",
            NULL
        };
    #endif /* ARCH_64BIT */

        static const char *home_locations[] =
        {
            ".vst",
            ".vst2",
            ".lxvst",
            "vst",
            "vst2",
            "lxvst",
            NULL
        };

        static const char *std_locations[] =
        {
            "vst",
            "vst2",
            "lxvst",
            NULL
        };

        static void *hInstance = NULL;
        static vst2::create_instance_t factory = NULL;

        static bool is_dots(const char *name)
        {
            return ((name[0] == '.') && ((name[1] == '\0') || ((name[1] == '.') && (name[2] == '\0'))));
        }

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

        // The factory for creating plugin instances
        static vst2::create_instance_t lookup_factory(void **hInstance, const char *path, const version_t *required, bool subdir = true)
        {
            lsp_trace("Searching core library at %s", path);

            // Try to open directory
            DIR *d = opendir(path);
            if (d == NULL)
                return NULL;
            lsp_finally { closedir(d); };

            struct dirent *de;
            char *ptr = NULL;

            while ((de = readdir(d)) != NULL)
            {
                // Skip dot and dotdot
                if (is_dots(de->d_name))
                    continue;

                // Allocate path string
                int n = asprintf(&ptr, "%s" FILE_SEPARATOR_S "%s", path, de->d_name);
                if ((n < 0) || (ptr == NULL))
                    continue;
                lsp_finally {
                    if (ptr != NULL)
                    {
                        free(ptr);
                        ptr = NULL;
                    }
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

                // Analyze file
                if (de->d_type == DT_DIR)
                {
                #ifdef LSP_PLUGIN_ARTIFACT_GROUP
                    // Skip directory entry if it does not contain LSP_PLUGIN_ARTIFACT_GROUP (for example, 'lsp-plugins') in name
                    if ((strlen(LSP_PLUGIN_ARTIFACT_GROUP)) && (strstr(de->d_name, LSP_PLUGIN_ARTIFACT_GROUP) == NULL))
                        continue;
                #endif /* LSP_PLUGIN_ARTIFACT_GROUP */
                    if (subdir)
                    {
                        vst2::create_instance_t f = lookup_factory(hInstance, ptr, required, false);
                        if (f != NULL)
                            return f;
                    }
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

                    // Skip library if it doesn't contain vst2 format specifier
                    if (strstr(de->d_name, "-vst2") == NULL)
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
                        if (inst != NULL)
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
                    vst2::create_instance_t f = reinterpret_cast<vst2::create_instance_t>(dlsym(inst, VST_MAIN_FUNCTION_STR));
                    if (!f)
                    {
                        lsp_trace("function %s not found: %s", VST_MAIN_FUNCTION_STR, dlerror());
                        continue;
                    }

                    lsp_trace("  obtained the library instance: %p, factory function: %p", inst, f);

                    *hInstance = inst;
                    inst = NULL;
                    return f;
                }
            }

            return NULL;
        }

        static vst2::create_instance_t get_main_function(const version_t *required)
        {
            if (factory != NULL)
                return factory;

            lsp_debug("Trying to find CORE library");

            // Try to lookup the same directory the shared object is located
            char *libpath = get_library_path();
            if (libpath != NULL)
            {
                lsp_finally { free(libpath); };
                if ((factory = lookup_factory(&hInstance, libpath, required)) != NULL)
                    return factory;
            }

            // Allocate temporary path string
            char *path = static_cast<char *>(malloc(PATH_MAX * sizeof(char)));
            if (path == NULL)
                return NULL;
            lsp_finally { free(path); };

            // Try to lookup inside of the home directory
            {
                // Obtain the home directory
                const char *homedir = getenv("HOME");
                char *buf = NULL;
                lsp_finally {
                    if (buf != NULL)
                        free(buf);
                };
                if (homedir == NULL)
                {
                    struct passwd pwd, *result;
                    size_t bufsize;

                    bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
                    if (bufsize <= 0)           // Value was indeterminate
                        bufsize = 0x10000;      // Should be more than enough

                    // Create buffer and fetch home directory
                    buf = static_cast<char *>(malloc(bufsize * sizeof(char)));
                    if (buf != NULL)
                    {
                        if (getpwuid_r(getuid(), &pwd, buf, bufsize, &result) == 0)
                            homedir = result->pw_dir;
                    }
                }

                // Try to lookup specific subdirectories inside of the home directory
                if (homedir != NULL)
                {
                    lsp_trace("home directory = %s", homedir);
                    for (const char **subdir = home_locations; (subdir != NULL) && (*subdir != NULL); ++subdir)
                    {
                        snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "%s", homedir, *subdir);
                        if ((factory = lookup_factory(&hInstance, path, required)) != NULL)
                            return factory;
                    }
                }
            }

            // Try to lookup standard directories
            for (const char **p = core_library_paths; (p != NULL) && (*p != NULL); ++p)
            {
                for (const char **subdir = home_locations; (subdir != NULL) && (*subdir != NULL); ++subdir)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "%s", *p, *subdir);
                    if ((factory = lookup_factory(&hInstance, path, required)) != NULL)
                        return factory;
                }
            }

            // Try to lookup extended library paths
            char **paths = get_library_paths(core_library_paths);
            lsp_finally { free_library_paths(paths); };

            for (char **p = paths; (p != NULL) && (*p != NULL); ++p)
            {
                if ((factory = lookup_factory(&hInstance, *p, required)) != NULL)
                    return factory;

                for (const char **subdir = std_locations; (subdir != NULL) && (*subdir != NULL); ++subdir)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "%s", *p, *subdir);
                    if ((factory = lookup_factory(&hInstance, path, required)) != NULL)
                        return factory;
                }
            }

            return NULL;
        }

        void free_core_library()
        {
            if (hInstance != NULL)
            {
                dlclose(hInstance);
                hInstance = NULL;
                factory = NULL;
            }
        }

        static StaticFinalizer finalizer(free_core_library);
    } /* namespace vst2 */
} /* namespace lsp */


// The main function
VST_MAIN(callback)
{
    IF_DEBUG( lsp::debug::redirect("lsp-vst2-loader.log"); );

    // Get VST Version of the Host
    if (!callback (NULL, audioMasterVersion, 0, 0, NULL, 0.0f))
    {
        lsp_error("audioMastercallback failed request");
        return 0;  // old version
    }

    // Check that we need to instantiate the factory
    lsp_trace("Getting factory for plugin %s", VST2_PLUGIN_UID);

    static const lsp::version_t version =
    {
        LSP_PLUGIN_PACKAGE_MAJOR,
        LSP_PLUGIN_PACKAGE_MINOR,
        LSP_PLUGIN_PACKAGE_MICRO,
        LSP_PLUGIN_PACKAGE_BRANCH
    };

    lsp::vst2::create_instance_t f = lsp::vst2::get_main_function(&version);

    // Create effect
    AEffect *effect     = NULL;

    if (f != NULL)
        effect = f(VST2_PLUGIN_UID, callback);
    else
        lsp_error("Could not find VST core library");

    // Return VST AEffect structure
    return effect;
}


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_POSIX_H_ */
