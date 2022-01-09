/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 22 янв. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_MAIN_POSIX_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_MAIN_POSIX_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_JACK_MAIN_IMPL
    #error "This header should not be included directly"
#endif /* LSP_PLUG_IN_JACK_MAIN_IMPL */

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/plug-fw/wrap/jack/defs.h>
#include <lsp-plug.in/plug-fw/wrap/common/libpath.h>

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
    namespace jack
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
            LSP_INSTALL_PREFIX "/lib"
            LSP_INSTALL_PREFIX "/lib64"
            LSP_INSTALL_PREFIX "/bin"
            LSP_INSTALL_PREFIX "/sbin"
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

        static jack_main_function_t lookup_jack_main(void **hInstance, const version_t *required, const char *path)
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

                // Skip directory entry if it does not contain FILE_PREFIX (for example, 'lsp-plugins') in name
                if ((strlen(EXT_ARTIFACT_ID)) && (strstr(de->d_name, EXT_ARTIFACT_ID) == NULL))
                    continue;

                // Analyze file
                if (de->d_type == DT_DIR)
                {
                    jack_main_function_t f = lookup_jack_main(hInstance, required, ptr);
                    if (f != NULL)
                    {
                        free(ptr);
                        closedir(d);
                        return f;
                    }
                }
                else if (de->d_type == DT_REG)
                {
                    // Skip library if it doesn't contain 'lsp-plugins' in name
                    if (strstr(de->d_name, ".so") == NULL)
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
                    jack_main_function_t f = reinterpret_cast<jack_main_function_t>(dlsym(inst, JACK_MAIN_FUNCTION_NAME));
                    if (!f)
                    {
                        lsp_trace("function %s not found: %s", JACK_MAIN_FUNCTION_NAME, dlerror());
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

        static jack_main_function_t get_main_function(void **hInstance, const version_t *required, const char *binary_path)
        {
            lsp_debug("Trying to find CORE library");

            char path[PATH_MAX+1];
            jack_main_function_t jack_main  = NULL;

            // Try to find files in current directory
            if (binary_path != NULL)
            {
                strncpy(path, binary_path, PATH_MAX);
                char *rchr  = strrchr(path, FILE_SEPARATOR_C);
                if (rchr != NULL)
                {
                    *rchr       = '\0';
                    jack_main   = lookup_jack_main(hInstance, required, path);
                }
            }

            // Get the home directory
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

            // Scan home directory first
            if ((jack_main == NULL) && (homedir != NULL))
            {
                lsp_trace("home directory = %s", homedir);
                jack_main       = lookup_jack_main(hInstance, required, homedir);

                if (jack_main == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lib", homedir);
                    jack_main       = lookup_jack_main(hInstance, required, path);
                }

                if (jack_main == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lib64", homedir);
                    jack_main       = lookup_jack_main(hInstance, required, path);
                }

                if (jack_main == NULL)
                {
                    snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "bin", homedir);
                    jack_main       = lookup_jack_main(hInstance, required, path);
                }
            }

            // Scan system directories
            if (jack_main == NULL)
            {
                for (const char **p = core_library_paths; (p != NULL) && (*p != NULL); ++p)
                {
                    jack_main       = lookup_jack_main(hInstance, required, *p);
                    if (jack_main != NULL)
                        break;
                }
            }

            // Try to lookup additional directories obtained from file mapping
            if (jack_main == NULL)
            {
                char *libpath = get_library_path();
                if (libpath != NULL)
                {
                    jack_main     = lookup_jack_main(hInstance, required, libpath);
                    ::free(libpath);
                }
            }

            if (jack_main == NULL)
            {
                char **paths = get_library_paths(core_library_paths);
                if (paths != NULL)
                {
                    for (char **p = paths; (p != NULL) && (*p != NULL); ++p)
                    {
                        jack_main     = lookup_jack_main(hInstance, required, *p);
                        if (jack_main != NULL)
                            break;

                        snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lib", *p);
                        jack_main       = lookup_jack_main(hInstance, required, path);
                        if (jack_main != NULL)
                            break;

                        snprintf(path, PATH_MAX, "%s" FILE_SEPARATOR_S "lib64", *p);
                        jack_main       = lookup_jack_main(hInstance, required, path);
                        if (jack_main != NULL)
                            break;
                    }

                    free_library_paths(paths);
                }
            }

            // Delete buffer if allocated
            if (buf != NULL)
                delete [] buf;

            // Return factory instance (if present)
            return jack_main;
        }
    } /* namespace jack */
}; /* namespace lsp */

//------------------------------------------------------------------------
int main(int argc, const char **argv)
{
    void *hInstance;
    static const lsp::version_t version =
    {
        LSP_PLUGIN_PACKAGE_MAJOR,
        LSP_PLUGIN_PACKAGE_MINOR,
        LSP_PLUGIN_PACKAGE_MICRO,
        LSP_PLUGIN_PACKAGE_BRANCH
    };

    lsp::jack_main_function_t jack_main = lsp::jack::get_main_function(&hInstance, &version, argv[0]);
    if (jack_main == NULL)
    {
        lsp_error("Could not find JACK plugin core library");
        return -lsp::STATUS_NOT_FOUND;
    }

    int code = jack_main(JACK_PLUGIN_UID, argc, argv);

    if (hInstance != NULL)
        dlclose(hInstance);

    return code;
}


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_MAIN_POSIX_H_ */
