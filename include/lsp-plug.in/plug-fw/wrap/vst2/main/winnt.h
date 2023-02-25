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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_WINNT_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_WINNT_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_VST2_MAIN_IMPL
    #error "This header should not be included directly"
#endif /* LSP_PLUG_IN_VST2_MAIN_IMPL */

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/finally.h>
#include <lsp-plug.in/common/static.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/defs.h>

#include <ctype.h>
#include <stdlib.h>
#include <windows.h>
#include <winreg.h>
#include <wctype.h>

namespace lsp
{
    namespace vst2
    {
        //-------------------------------------------------------------------------
        static HMODULE                  hLibrary;
        static vst2::create_instance_t  factory = NULL;

        //-------------------------------------------------------------------------
        constexpr size_t PATH_LENGTH_DFL     = PATH_MAX * 2;

        typedef struct path_string_t
        {
            WCHAR      *pData;
            size_t      nCapacity;

            path_string_t()
            {
                pData       = static_cast<WCHAR *>(malloc(sizeof(WCHAR) * PATH_LENGTH_DFL));
                nCapacity   = PATH_LENGTH_DFL;
            }

            ~path_string_t()
            {
                if (pData != NULL)
                {
                    free(pData);
                    pData       = NULL;
                }
                nCapacity   = 0;
            }

            bool grow(size_t capacity)
            {
                if (capacity < nCapacity)
                    return true;

                capacity = ((capacity + PATH_LENGTH_DFL - 1) / PATH_LENGTH_DFL) * PATH_LENGTH_DFL;
                WCHAR *ptr  = static_cast<WCHAR *>(realloc(pData, sizeof(WCHAR) * capacity));
                if (ptr == NULL)
                    return false;

                pData       = ptr;
                nCapacity   = capacity;
                return true;
            }

            WCHAR *set(const WCHAR *value)
            {
                size_t cap = wcslen(value) + 1;
                if (!grow(cap))
                    return NULL;

                memcpy(&pData[0], value, cap);
                return pData;
            }

            WCHAR *set(const WCHAR *parent, const WCHAR *child)
            {
                size_t len1 = wcslen(parent);
                size_t len2 = wcslen(child);
                size_t cap = len1 + len2 + 2;
                if (!grow(cap))
                    return NULL;

                memcpy(&pData[0], parent, len1);
                pData[len1] = '\\';
                memcpy(&pData[len1 + 1], child, len2);
                pData[len1 + len2 + 1] = '0';

                return pData;
            }
        } path_string_t;

        //-------------------------------------------------------------------------
        typedef struct reg_key_t
        {
            HKEY root;
            const WCHAR *key;
            const WCHAR *value;
            IF_TRACE( const char *log; )
        } reg_key_t;

        static const reg_key_t registry_keys[] =
        {
            {
                HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\LSP\\",
                L"VSTInstallPath",
                IF_TRACE( "HKEY_LOCAL_MACHINE\\SOFTWARE\\LSP\\VSTInstallPath", )
            },
            {
                HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\VST",
                L"VSTPluginsPath",
                IF_TRACE( "HKEY_LOCAL_MACHINE\\SOFTWARE\\VST\\VSTPluginsPath", )
            },
            {
                HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Wow6432Node\\VST",
                L"VSTPluginsPath",
                IF_TRACE( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\VST\\VSTPluginsPath", )
            },
            {
                HKEY_LOCAL_MACHINE,
                NULL,
                NULL,
                IF_TRACE( NULL, )
            }
        };

        static const WCHAR *home_paths[] =
        {
            L".vst",
            L".vst2",
            L"vst",
            L"vst2",
            NULL
        };

        //-------------------------------------------------------------------------
        static const WCHAR *get_env_var(path_string_t *dst, const WCHAR *name)
        {
            DWORD bufsize = ::GetEnvironmentVariableW(name, NULL, 0);
            if (bufsize == 0)
                return NULL;

            if (!dst->grow(bufsize))
                return NULL;

            bufsize = ::GetEnvironmentVariableW(name, dst->pData, bufsize);
            return (bufsize != 0) ? dst->pData : NULL;
        }

        static const WCHAR *get_home_dir(path_string_t *dst)
        {
            DWORD bufsize1 = ::GetEnvironmentVariableW(L"HOMEDRIVE", NULL, 0);
            if (bufsize1 == 0)
                return NULL;
            DWORD bufsize2 = ::GetEnvironmentVariableW(L"HOMEPATH", NULL, 0);
            if (bufsize2 == 0)
                return NULL;

            if (!dst->grow(bufsize1 + bufsize2 + 1))
                return NULL;

            bufsize1 = ::GetEnvironmentVariableW(L"HOMEDRIVE", dst->pData, bufsize1);
            if (bufsize1 == 0)
                return NULL;
            bufsize2 = ::GetEnvironmentVariableW(L"HOMEPATH", &dst->pData[bufsize1], bufsize2);
            if (bufsize2 == 0)
                return NULL;

            return dst->pData;
        }

        static const WCHAR *get_library_path(path_string_t *dst)
        {
            // Get module handle
            HMODULE hm = NULL;

            if (!::GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&hLibrary),
                &hm))
                return NULL;

            // Get the path to the module
            DWORD capacity = PATH_MAX + 1, length = 0;

            while (true)
            {
                // Allocate the buffer
                if (!dst->grow(capacity))
                    return NULL;

                // Try to obtain the file name
                length = ::GetModuleFileNameW(hm, dst->pData, capacity);
                if (length == 0)
                    return NULL;

                // Analyze result
                DWORD error = GetLastError();
                if (error == ERROR_SUCCESS)
                    break;
                else if (error != ERROR_INSUFFICIENT_BUFFER)
                    return NULL;

                // Increase capacity by 1.5
                capacity += (capacity >> 1);
            }

            // Remove the last path name
            WCHAR *split = wcsrchr(dst->pData, '\\');
            if (split != NULL)
                *split = '\0';

            return dst->pData;
        }

        static const WCHAR *read_registry(path_string_t *dst, const reg_key_t *key, size_t index)
        {
            DWORD dwType = 0;
            DWORD cbData = 0;

            // Obtain the size of value associated with the key
            LSTATUS res = RegGetValueW(
                key->root, // hkey
                key->key, // lpSubKey
                key->value, // lpValue
                RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_MULTI_SZ | RRF_RT_REG_SZ, // dwFlags
                &dwType, // pdwType
                NULL, // pvData
                &cbData  // pcbData
            );
            if (res != ERROR_SUCCESS)
                return NULL;

            // Allocate the buffer
            path_string_t tmp;
            if (!tmp.grow(cbData + 1))
                return NULL;

            // Now obtain the value
            res = RegGetValueW(
                key->root, // hkey
                key->key, // lpSubKey
                key->value, // lpValue
                RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_MULTI_SZ | RRF_RT_REG_SZ, // dwFlags
                &dwType, // pdwType
                tmp.pData, // pvData
                &cbData  // pcbData
            );
            if (res != ERROR_SUCCESS)
                return NULL;

            // Simple string?
            if (dwType == REG_SZ)
                return dst->set(tmp.pData);

            // String with expanded environment variables?
            if (dwType == REG_EXPAND_SZ)
            {
                // Get the path to the module
                DWORD capacity = PATH_MAX + 1;

                // Allocate the buffer
                if (!dst->grow(capacity))
                    return NULL;

                // Try to obtain the file name
                size_t length = ::ExpandEnvironmentStringsW(tmp.pData, dst->pData, capacity);
                if (length < capacity)
                    return dst->pData;

                // Allocate the buffer again
                if (!dst->grow(length + 1))
                    return NULL;

                length = ::ExpandEnvironmentStringsW(tmp.pData, dst->pData, capacity);
                return (length > 0) ? tmp.pData : NULL;
            }

            // Multiple strings?
            WCHAR *ptr = tmp.pData;
            for (size_t i=0; ; ++i)
            {
                if (*ptr == 0)
                    return NULL;
                if (i == index)
                    return tmp.set(ptr);
                ptr    += wcslen(ptr) + 1;
            }

            return NULL;
        }

        static bool is_dots(const WCHAR *name)
        {
            return ((name[0] == '.') && ((name[1] == '\0') || ((name[1] == '.') && (name[2] == '\0'))));
        }

    #ifdef EXT_ARTIFACT_NAME
        static bool contains_artifact_name(const WCHAR *file, const char *name)
        {
            if (strlen(EXT_ARTIFACT_NAME) <= 0)
                return false;
            for (; *file != '\0'; ++file)
            {
                for (size_t i=0; ; ++i)
                {
                    if (name[i] == '\0')
                        return true;
                    if (towupper(file[i]) != toupper(name[i]))
                        break;
                }
            }

            return false;
        }
    #endif /* EXT_ARTIFACT_NAME */

        static bool file_name_is_dll(const WCHAR *name)
        {
            size_t len = wcslen(name);
            if (len < 4)
                return false;
            name += len - 4;
            return (name[0] == '.') &&
                (towupper(name[1]) == 'D') &&
                (towupper(name[2]) == 'L') &&
                (towupper(name[3]) == 'L');
        }

        // The factory for creating plugin instances
        static vst2::create_instance_t lookup_factory(HMODULE *hInstance, const WCHAR *path, const version_t *required, bool subdir = true)
        {
            IF_TRACE( debug::log_string log_path = path; );
            lsp_trace("Searching core library at %s", log_path.c_str());

            path_string_t pattern;
            WIN32_FIND_DATAW dirent;
            if (!pattern.set(path, L"*"))
                return NULL;

            // Lookup files in folders
            HANDLE hFD = FindFirstFileW(pattern.pData, &dirent);
            if (hFD == INVALID_HANDLE_VALUE)
                return NULL;
            lsp_finally { FindClose(hFD); };

            do
            {
                // Skip dot and dotdot
                if (is_dots(dirent.cFileName))
                    continue;

                if (dirent.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (subdir)
                    {
                        path_string_t subdir_name;
                        if (!subdir_name.set(path, dirent.cFileName))
                            continue;

                        vst2::create_instance_t f = lookup_factory(hInstance, subdir_name.pData, required, false);
                        if (f != NULL)
                            return f;
                    }
                }
                else if (!(dirent.dwFileAttributes & FILE_ATTRIBUTE_DEVICE))
                {
                #ifdef EXT_ARTIFACT_NAME
                    // Skip directory entry if it doesn't contain EXT_ARTIFACT_NAME (for example, 'lsp-plugins') in name
                    if (!(contains_artifact_name(dirent.cFileName, EXT_ARTIFACT_NAME)))
                        continue;
                #endif /* EXT_ARTIFACT_NAME */

                    // Check that file is a shared library
                    if (!file_name_is_dll(dirent.cFileName))
                        continue;

                    // Form the full path to the file
                    path_string_t library_file_name;
                    if (!library_file_name.set(path, dirent.cFileName))
                        continue;

                    IF_TRACE( debug::log_string trace_library_file_name = library_file_name; );
                    lsp_trace("Trying library %s", trace_library_file_name.c_str());

                    // Try to load library
                    HMODULE hLib = LoadLibraryW(library_file_name.pData);
                    if (hLib == NULL)
                        continue;
                    lsp_finally {
                        if (hLib != NULL)
                            FreeLibrary(hLib);
                    };

                    // Fetch version function
                    module_version_t vf = reinterpret_cast<module_version_t>(GetProcAddress(hLib, LSP_VERSION_FUNC_NAME));
                    if (!vf)
                    {
                        lsp_trace("version function %s not found: code=%d", LSP_VERSION_FUNC_NAME, int(GetLastError()));
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
                             (strcmp(ret->branch, required->branch) != 0))
                    {
                        lsp_trace("wrong version %d.%d.%d '%s' returned, expected %d.%d.%d '%s', ignoring binary",
                            ret->major, ret->minor, ret->micro, ret->branch,
                            required->major, required->minor, required->micro, required->branch);
                        continue;
                    }

                    // Fetch function
                    vst2::create_instance_t f = reinterpret_cast<vst2::create_instance_t>(GetProcAddress(hLib, VST_MAIN_FUNCTION_STR));
                    if (!f)
                    {
                        lsp_trace("function %s not found: code=%d", VST_MAIN_FUNCTION_STR, int(GetLastError()));
                        continue;
                    }

                    lsp_trace("  obtained the library instance: %p, factory function: %p", hLib, f);

                    *hInstance = hLib;
                    hLib = NULL;
                    return f;
                }
            } while (FindNextFileW(hFD, &dirent));

            return NULL;
        }

        static vst2::create_instance_t get_main_function(const version_t *required)
        {
            if (factory != NULL)
                return factory;

            lsp_debug("Trying to find CORE library");
            path_string_t dir_str;

            // Try to lookup the library path
            const WCHAR *bundle = get_library_path(&dir_str);
            if (bundle != NULL)
            {
                if ((factory = lookup_factory(&hLibrary, bundle, required)) != NULL)
                    return factory;
            }

            // Try to lookup binaries inside of home directory
            const WCHAR *homedir = get_home_dir(&dir_str);
            if (homedir != NULL)
            {
                IF_TRACE( debug::log_string log_homedir = homedir; );
                lsp_trace("User home directory: %s", log_homedir.c_str());

                path_string_t child;
                for (const WCHAR **subdir = home_paths; *subdir != NULL; ++subdir)
                {
                    const WCHAR *vst_path = child.set(homedir, *subdir);
                    if (vst_path != NULL)
                    {
                        if ((factory = lookup_factory(&hLibrary, vst_path, required)) != NULL)
                            return factory;
                    }
                }
            }

            // Try to lookup via env var
            const WCHAR *vst_path = get_env_var(&dir_str, L"VST_PATH");
            if (vst_path != NULL)
            {
                IF_TRACE( debug::log_string log_vst_path = vst_path; );
                lsp_trace("Environment VST_PATH directory: %s", log_vst_path.c_str());

                if ((factory = lookup_factory(&hLibrary, vst_path, required)) != NULL)
                    return factory;
            }

            // Try to lookup via registry
            for (const reg_key_t *k=registry_keys; k->key != NULL; ++k)
            {
                for (size_t index=0; ; ++index)
                {
                    const WCHAR *reg_vst_path = read_registry(&dir_str, k, index);
                    if (reg_vst_path == NULL)
                        break;

                    IF_TRACE(debug::log_string log_reg_vst_path = reg_vst_path;);
                    lsp_trace("Registry %s [%d] directory: %s", k->log, int(index), log_reg_vst_path.c_str());

                    if ((factory = lookup_factory(&hLibrary, reg_vst_path, required)) != NULL)
                        return factory;
                }
            }

            // Nothing found
            return NULL;
        }

        //-------------------------------------------------------------------------
        void free_core_library()
        {
            if (hLibrary != NULL)
            {
                ::FreeLibrary(hLibrary);
                hLibrary = NULL;
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

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_WINNT_H_ */
