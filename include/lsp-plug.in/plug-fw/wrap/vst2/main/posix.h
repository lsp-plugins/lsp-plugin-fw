/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/3rdparty/steinberg/vst2.h>

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
            LSP_INSTALL_PREFIX "/lib"
            LSP_INSTALL_PREFIX "/lib32"
            LSP_INSTALL_PREFIX "/bin"
            LSP_INSTALL_PREFIX "/sbin"
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
            LSP_INSTALL_PREFIX "/lib"
            LSP_INSTALL_PREFIX "/lib64"
            LSP_INSTALL_PREFIX "/bin"
            LSP_INSTALL_PREFIX "/sbin"
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

        static void *hInstance = NULL;
        static vst2::create_instance_t factory = NULL;

        // The factory for creating plugin instances
        static vst2::create_instance_t lookup_factory(void **hInstance, const char *path, const version_t *required, bool subdir = true)
        {
            lsp_trace("Searching core library at %s", path);

            // Try to open directory
            DIR *d = opendir(path);
            if (d == NULL)
                return NULL;

            struct dirent *de;
            char *ptr = NULL;

            while ((de = readdir(d)) != NULL)
            {
                // Free previously used string
                if (ptr != NULL)
                    free(ptr);

                // Skip dot and dotdot
                ptr = de->d_name;
                if ((ptr[0] == '.') && ((ptr[1] == '\0') || ((ptr[1] == '.') || (ptr[2] == '\0'))))
                {
                    ptr = NULL;
                    continue;
                }

                // Allocate path string
                ptr = NULL;
                int n = asprintf(&ptr, "%s" FILE_SEPARATOR_S "%s", path, de->d_name);
                if ((n < 0) || (ptr == NULL))
                    continue;

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
                #ifdef EXT_ARTIFACT_GROUP
                    // Skip directory entry if it doesn't contain EXT_ARTIFACT_GROUP (for example, 'lsp-plugins') in name
                    if ((strlen(EXT_ARTIFACT_GROUP) > 0) && (strstr(de->d_name, EXT_ARTIFACT_GROUP) == NULL))
                        continue;
                #endif /* EXT_ARTIFACT_GROUP */
                    if (subdir)
                    {
                        vst2::create_instance_t f = lookup_factory(hInstance, ptr, required, false);
                        if (f != NULL)
                        {
                            free(ptr);
                            closedir(d);
                            return f;
                        }
                    }
                }
                else if (de->d_type == DT_REG)
                {
                #ifdef EXT_ARTIFACT_NAME
                    // Skip directory entry if it doesn't contain EXT_ARTIFACT_NAME (for example, 'lsp-plugins') in name
                    if ((strlen(EXT_ARTIFACT_NAME) > 0) && (strstr(de->d_name, EXT_ARTIFACT_NAME) == NULL))
                        continue;
                #endif /* EXT_ARTIFACT_NAME */


                    // Skip library if it doesn't contain 'lsp-plugins' in name
                    if (strcasestr(de->d_name, ".so") == NULL)
                        continue;

                    lsp_trace("Trying library %s", ptr);

                    // Try to load library
                    void *inst = dlopen (ptr, RTLD_NOW);
                    if (!inst)
                    {
                        lsp_trace("library %s not loaded: %s", ptr, dlerror());
                        continue;
                    }

                    // Fetch version function
                    module_version_t vf = reinterpret_cast<module_version_t>(dlsym(inst, LSP_VERSION_FUNC_NAME));
                    if (!vf)
                    {
                        lsp_trace("version function %s not found: %s", LSP_VERSION_FUNC_NAME, dlerror());
                        // Close library
                        dlclose(inst);
                        continue;
                    }

                    // Check package version
                    const version_t *ret = vf();
                    if ((ret == NULL) || (ret->branch == NULL))
                    {
                        lsp_trace("No version or bad version returned, ignoring binary", ret);
                        // Close library
                        dlclose(inst);
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
                        // Close library
                        dlclose(inst);
                        continue;
                    }

                    // Fetch function
                    vst2::create_instance_t f = reinterpret_cast<vst2::create_instance_t>(dlsym(inst, VST_MAIN_FUNCTION_STR));
                    if (!f)
                    {
                        lsp_trace("function %s not found: %s", VST_MAIN_FUNCTION_STR, dlerror());
                        // Close library
                        dlclose(inst);
                        continue;
                    }

                    *hInstance = inst;
                    free(ptr);
                    closedir(d);
                    return f;
                }
            }

            // Free previously used string, close directory and exit
            if (ptr != NULL)
                free(ptr);
            closedir(d);
            return NULL;
        }

        static vst2::create_instance_t get_main_function(const version_t *required)
        {
            if (factory != NULL)
                return factory;

            lsp_debug("Trying to find CORE library");

            const char *homedir = getenv("HOME");
            char *buf = NULL;

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

            // Initialize factory with NULL
            char path[PATH_MAX];

            // Try to lookup home directory
            if (homedir != NULL)
            {
                lsp_trace("home directory = %s", homedir);
                if (factory == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S ".vst", homedir);
                    factory     = lookup_factory(&hInstance, path, required);
                }
                if (factory == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S ".vst2", homedir);
                    factory     = lookup_factory(&hInstance, path, required);
                }
                if (factory == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S ".lxvst", homedir);
                    factory     = lookup_factory(&hInstance, path, required);
                }
                if (factory == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "vst", homedir);
                    factory     = lookup_factory(&hInstance, path, required);
                }
                if (factory == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "vst2", homedir);
                    factory     = lookup_factory(&hInstance, path, required);
                }
                if (factory == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lxvst", homedir);
                    factory     = lookup_factory(&hInstance, path, required);
                }
            }

            // Try to lookup standard directories
            if (factory == NULL)
            {
                for (const char **p = core_library_paths; (p != NULL) && (*p != NULL); ++p)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "vst", *p);
                    factory     = lookup_factory(&hInstance, path, required);
                    if (factory != NULL)
                        break;
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "vst2", *p);
                    factory     = lookup_factory(&hInstance, path, required);
                    if (factory != NULL)
                        break;
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lxvst", *p);
                    factory     = lookup_factory(&hInstance, path, required);
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
                    factory         = lookup_factory(&hInstance, libpath, required);
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
                        factory     = lookup_factory(&hInstance, *p, required);
                        if (factory != NULL)
                            break;

                        snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "vst", *p);
                        factory     = lookup_factory(&hInstance, path, required);
                        if (factory != NULL)
                            break;

                        snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "vst2", *p);
                        factory     = lookup_factory(&hInstance, path, required);
                        if (factory != NULL)
                            break;

                        snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lxvst", *p);
                        factory     = lookup_factory(&hInstance, path, required);
                        if (factory != NULL)
                            break;
                    }

                    free_library_paths(paths);
                }
            }

            // Delete buffer if allocated
            if (buf != NULL)
                delete [] buf;

            // Return factory instance (if present)
            return factory;
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
    // Get VST Version of the Host
    if (!callback (NULL, audioMasterVersion, 0, 0, NULL, 0.0f))
    {
        lsp_error("audioMastercallback failed request");
        return 0;  // old version
    }

    // Check that we need to instantiate the factory
    lsp_trace("Getting factory");

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
