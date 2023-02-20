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

#include <stdlib.h>
#include <windows.h>
#include <winreg.h>

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
                WCHAR *ptr  = static_cast<WCHAR *>(realloc(sizeof(WCHAR) * capacity));
                if (ptr == NULL)
                    return false;

                pData       = ptr;
                nCapacity   = capacity;
                return true;
            }

            WCHAR *set(WCHAR *parent, WCHAR *child)
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
        };

        static const reg_key_t registry_keys[] =
        {
            { HKEY_LOCAL_MACHINE, L"SOFTWARE\\VST", "VSTPluginsPath" },
            { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Wow6432Node\\VST", "VSTPluginsPath" },
            { HKEY_LOCAL_MACHINE, L"SOFTWARE\\LSP\\", "VSTInstallPath" },
            { NULL, NULL, NULL }
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
                reinterpret_cast<LPCWSTR>(ptr),
                &hm))
                return STATUS_NOT_FOUND;

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

// TODO
//        static const WCHAR *read_registry(path_string_t *dst, const reg_key_t *key)
//        {
//            PHKEY phKey = NULL;
//            if ((RegOpenKeyW(key->root, key->key, &phKey)) != ERROR_SUCCESS)
//                return NULL;
//            lsp_finally { RegCloseKey(phKey); };
//
//
//        }

        // The factory for creating plugin instances
        static vst2::create_instance_t lookup_factory(HMODULE *hInstance, const WCHAR *path, const version_t *required, bool subdir = true)
        {
            lsp_trace("Searching core library at %s", path);

            // TODO

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
                path_string_t child;
                for (const WCHAR **subdir = home_paths; (factory == NULL) && (subdir != NULL); ++subdir)
                {
                    const WCHAR *vst_path = child.set(homedir, subdir);
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
                if ((factory = lookup_factory(&hLibrary, vst_path, required)) != NULL)
                    return factory;
            }

// TODO
//            // Try to lookup via registry
//            for (const reg_key_t *k=registry_keys; (k->key != NULL) && (factory == NULL); ++k)
//            {
//                const WCHAR *vst_path = read_registry(&dir_str, k);
//                if (vst_path != NULL)
//                {
//                    if ((factory = lookup_factory(&hLibrary, vst_path, required)) != NULL)
//                        return factory;
//                }
//            }

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

#error "Needs to be implemented"

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_MAIN_WINNT_H_ */
