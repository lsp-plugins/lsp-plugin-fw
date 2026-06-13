/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 апр. 2026 г.
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

#include <lsp-plug.in/audio/iface/builtin.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/ipc/Library.h>
#include <lsp-plug.in/io/Dir.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/plug-fw/core/AudioBackend.h>

namespace lsp
{
    namespace core
    {
        static constexpr const char *AUDIO_LIBRARY_FILE_PART    = "lsp-audio";
        static const ::lsp::version_t plugin_fw_version         = LSP_DEFINE_VERSION(LSP_PLUGIN_FW);
        static const ::lsp::version_t audio_iface_version       = LSP_DEFINE_VERSION(LSP_AUDIO_IFACE);

    #ifdef PLATFORM_POSIX
        #ifdef ARCH_32BIT
            static const char *library_paths[] =
            {
            #ifdef LSP_INSTALL_PREFIX
                LSP_INSTALL_PREFIX "/lib",
                LSP_INSTALL_PREFIX "/lib64",
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
        #endif

        #ifdef ARCH_64BIT
            static const char *library_paths[] =
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
        #endif /* ARCH_64_BIT */
    #endif /* PLATFORM_POSIX */

        void free_audio_backends(AudioBackendInfoList * list)
        {
            if (list == NULL)
                return;

            for (lltl::iterator<AudioBackendInfo> it=list->values(); it; ++it)
            {
                AudioBackendInfo * const info = it.get();
                if (info != NULL)
                {
                    version_destroy(&info->version);
                    delete info;
                }
            }
            list->flush();
        }

        static bool check_duplicate(AudioBackendInfoList *list, const AudioBackendInfo *info)
        {
            for (lltl::iterator<AudioBackendInfo> it = list->values(); it; ++it)
            {
                const AudioBackendInfo *src = it.get();

                if ((src->uid.equals(&info->uid))
                    && (src->display.equals(&info->display))
                    && (src->lc_key.equals(&info->lc_key))
                    && (version_cmp(src->version, info->version) == 0))
                    return true;
            }

            return false;
        }

        static status_t commit_audio_factory(
            AudioBackendInfoList *list,
            const LSPString *path,
            size_t factory_id,
            audio::factory_t *factory,
            const version_t *mversion)
        {
            for (size_t local_id=0; ; ++local_id)
            {
                // Get metadata record
                const audio::backend_metadata_t *meta = factory->metadata(factory, local_id);
                if (meta == NULL)
                    break;
                else if (meta->id == NULL)
                    continue;

                // Create library descriptor
                AudioBackendInfo *info = new AudioBackendInfo();
                if (info == NULL)
                    return STATUS_NO_MEM;
                lsp_finally {
                    if (info != NULL)
                        delete info;
                };

                info->builtin           = (path != NULL) ? NULL : factory;
                info->factory_id        = factory_id;
                info->local_id          = local_id;
                info->priority          = meta->priority;
                version_copy(&info->version, mversion);
                if (path != NULL)
                {
                    if (!info->library.set(path))
                        return STATUS_NO_MEM;
                }

                if ((!info->uid.set_utf8(meta->id)) ||
                    (!info->display.set_utf8((meta->display != NULL) ? meta->display : meta->id)) ||
                    (!info->lc_key.set_utf8((meta->lc_key != NULL) ? meta->lc_key : meta->id)))
                    return STATUS_NO_MEM;

                // Check for duplicates
                if (check_duplicate(list, info))
                {
                    lsp_trace("    library %s provides duplicated backend %s (%s)",
                        info->library.get_native(),
                        info->uid.get_native(),
                        info->display.get_native());
                    return STATUS_DUPLICATED;
                }

                // Add backend descriptor to the list
                if (!list->add(info))
                    return STATUS_NO_MEM;
                info = NULL;
            }

            return STATUS_OK;
        }

        static status_t register_audio_backend(AudioBackendInfoList *list, const LSPString *path)
        {
            ipc::Library lib;

            lsp_trace("  probing library %s", path->get_native());

            // Open library
            status_t res = lib.open(path);
            if (res != STATUS_OK)
                return res;
            lsp_finally { lib.close(); };

            // Perform audio interface version control
            module_version_t vfunc = reinterpret_cast<module_version_t>(lib.import(LSP_AUDIO_IFACE_VERSION_FUNC_NAME));
            const version_t *mversion = (vfunc != NULL) ? vfunc() : NULL; // Obtain interface version
            if (mversion == NULL)
            {
                lsp_trace("    not provided audio backend interface version");
                return STATUS_INCOMPATIBLE;
            }
            else if (version_cmp(&audio_iface_version, mversion) != 0)
            {
                lsp_trace("    mismatched audio backend interface version: %d.%d.%d-%s vs %d.%d.%d-%s",
                    audio_iface_version.major, audio_iface_version.minor, audio_iface_version.micro, audio_iface_version.branch,
                    mversion->major, mversion->minor, mversion->micro, mversion->branch);
                return STATUS_INCOMPATIBLE;
            }

            // Get the module version
            vfunc = reinterpret_cast<module_version_t>(lib.import(LSP_VERSION_FUNC_NAME));
            mversion = (vfunc != NULL) ? vfunc() : NULL; // Obtain interface version
            if (mversion == NULL)
            {
                lsp_trace("    missing module version function");
                return STATUS_INCOMPATIBLE;
            }

            // Lookup function
            audio::factory_function_t func = reinterpret_cast<audio::factory_function_t>(lib.import(LSP_AUDIO_FACTORY_FUNCTION_NAME));
            if (func == NULL)
            {
                lsp_trace("    missing factory function %s", LSP_AUDIO_FACTORY_FUNCTION_NAME);
                return STATUS_NOT_FOUND;
            }

            // Try to instantiate factory
            size_t found = 0;
            for (int factory_id=0; ; ++factory_id)
            {
                audio::factory_t * const factory  = func(factory_id);
                if (factory == NULL)
                    break;

                // Fetch metadata and store
                res = commit_audio_factory(list, path, factory_id, factory, mversion);
                ++found;
            }

            // Close the library
            return (found > 0) ? res : STATUS_NOT_FOUND;
        }

        static status_t register_audio_backend(AudioBackendInfoList *list, const io::Path *path)
        {
            if (path == NULL)
                return STATUS_BAD_ARGUMENTS;
            return register_audio_backend(list, path->as_string());
        }

//        static status_t register_audio_backend(AudioBackendInfoList *list, const char *path)
//        {
//            LSPString tmp;
//            if (path == NULL)
//                return STATUS_BAD_ARGUMENTS;
//            if (!tmp.set_utf8(path))
//                return STATUS_NO_MEM;
//            return register_audio_backend(list, &tmp);
//        }

        static void lookup_audio_backends(AudioBackendInfoList *list, const io::Path *path, const char *part)
        {
            io::Dir dir;

            lsp_trace("Lookup audio backend in directory:%s", path->as_native());

            status_t res = dir.open(path);
            if (res != STATUS_OK)
                return;

            io::Path child;
            LSPString item, substring;
            if (!substring.set_utf8(part))
                return;

            io::fattr_t fattr;
            while ((res = dir.read(&item, false)) == STATUS_OK)
            {
                if (item.index_of(&substring) < 0)
                    continue;
                if (!ipc::Library::valid_library_name(&item))
                    continue;

                if ((res = child.set(path, &item)) != STATUS_OK)
                    continue;
                if ((res = child.stat(&fattr)) != STATUS_OK)
                    continue;

                switch (fattr.type)
                {
                    case io::fattr_t::FT_DIRECTORY:
                    case io::fattr_t::FT_BLOCK:
                    case io::fattr_t::FT_CHARACTER:
                        continue;
                    default:
                        register_audio_backend(list, &child);
                        break;
                }
            }
        }

        void lookup_audio_backends(AudioBackendInfoList * list, const char *path, const char *part)
        {
            io::Path tmp;
            if (tmp.set(path) != STATUS_OK)
                return;
            lookup_audio_backends(list, &tmp, part);
        }

//        static void lookup_audio_backends(AudioBackendInfoList *list, const LSPString *path, const char *part)
//        {
//            io::Path tmp;
//            if (tmp.set(path) != STATUS_OK)
//                return;
//            lookup_audio_backends(list, &tmp, part);
//        }

        static ssize_t compare_backends(const AudioBackendInfo *a, const AudioBackendInfo *b)
        {
            return a->uid.compare_to(&b->uid);
        }

        status_t scan_audio_backends(AudioBackendInfoList *list)
        {
            if (list == NULL)
                return STATUS_BAD_ARGUMENTS;

            status_t res;
            AudioBackendInfoList tmp;
            lsp_finally { free_audio_backends(&tmp); };

            // Scan for built-in libraries
            for (int factory_id=0; ; ++factory_id)
            {
                audio::factory_t * const factory = audio::Factory::enumerate(factory_id);
                if (factory == NULL)
                    break;

                // Commit factory to the list of backends
                if ((res = commit_audio_factory(&tmp, NULL, factory_id, factory, &plugin_fw_version)) != STATUS_OK)
                    return res;
            }

            // Scan for another locations
            io::Path path;
            res = ipc::Library::get_self_file(&path);
            if (res == STATUS_OK)
                res     = path.parent();
            if (res == STATUS_OK)
                lookup_audio_backends(&tmp, &path, AUDIO_LIBRARY_FILE_PART);

            // Scan for standard paths
        #ifdef PLATFORM_POSIX
            for (const char **paths = library_paths; *paths != NULL; ++paths)
                lookup_audio_backends(&tmp, *paths, AUDIO_LIBRARY_FILE_PART);
        #endif /* PLATFORM_POSIX */

            // Sort backends by identifier
            tmp.qsort(compare_backends);

            tmp.swap(list);

            return STATUS_OK;
        }

        status_t copy_backends(AudioBackendInfoList * dst, const AudioBackendInfoList * src)
        {
            // Create temporary list
            AudioBackendInfoList tmp;
            if (!tmp.reserve(src->size()))
                return STATUS_NO_MEM;
            lsp_finally { free_audio_backends(&tmp); };

            // Fill temporary list with data
            for (lltl::iterator<const AudioBackendInfo> it=src->values(); it; ++it)
            {
                const AudioBackendInfo *s = it.get();
                if (s == NULL)
                    continue;

                // Create backend info
                AudioBackendInfo *d = new AudioBackendInfo;
                if (d == NULL)
                    return STATUS_NO_MEM;
                if (!tmp.add(d))
                {
                    delete d;
                    return STATUS_NO_MEM;
                }

                // Copy backend info
                if (!d->library.set(&s->library))
                    return STATUS_NO_MEM;
                if (!d->uid.set(&s->uid))
                    return STATUS_NO_MEM;
                if (!d->display.set(&s->display))
                    return STATUS_NO_MEM;
                if (!d->lc_key.set(&s->lc_key))
                    return STATUS_NO_MEM;

                version_copy(&d->version, &s->version);

                d->builtin      = s->builtin;
                d->factory_id   = s->factory_id;
                d->local_id     = s->local_id;
                d->priority     = s->priority;
            }

            // Commit result
            tmp.swap(dst);
            return STATUS_OK;
        }

        status_t add_backends(AudioBackendInfoList * dst, const AudioBackendInfoList * src)
        {
            // Create temporary list
            AudioBackendInfoList tmp;
            lsp_finally { free_audio_backends(&tmp); };

            // Copy backends to temporary list
            status_t res = copy_backends(dst, src);
            if (res != STATUS_OK)
                return res;

            // Append temporary list to the destination list
            if (!dst->add(tmp))
                return STATUS_NO_MEM;

            // Free temporary list
            tmp.flush();
            return STATUS_OK;
        }

        status_t create_audio_backend(audio::backend_t ** backend, ipc::Library *library, const AudioBackendInfo *info)
        {
            status_t res;
            audio::backend_t * result;

            if (info->builtin != NULL)
            {
                if ((result = info->builtin->create(info->builtin, info->local_id)) == NULL)
                    return STATUS_NO_MEM;
                *backend = result;
                return STATUS_OK;
            }

            // Open library
            if ((res = library->open(&info->library)) != STATUS_OK)
            {
                lsp_error("Failed to load library %s: code=%d", info->library.get_native(), int(res));
                return res;
            }
            bool close_library = true;
            lsp_finally {
                if (close_library)
                    library->close();
            };

            // Perform audio interface version control
            module_version_t vfunc = reinterpret_cast<module_version_t>(library->import(LSP_AUDIO_IFACE_VERSION_FUNC_NAME));
            const version_t *mversion = (vfunc != NULL) ? vfunc() : NULL; // Obtain interface version
            if (mversion == NULL)
            {
                lsp_error("Failed to load library %s: not provided audio backend interface version", info->library.get_native());
                return STATUS_INCOMPATIBLE;
            }
            else if (version_cmp(&audio_iface_version, mversion) != 0)
            {
                lsp_error(
                    "Failed to load library %s: mismatched audio backend interface version: %d.%d.%d-%s vs %d.%d.%d-%s",
                    info->library.get_native(),
                    audio_iface_version.major, audio_iface_version.minor, audio_iface_version.micro, audio_iface_version.branch,
                    mversion->major, mversion->minor, mversion->micro, mversion->branch);
                return STATUS_INCOMPATIBLE;
            }

            // Get the module version
            vfunc = reinterpret_cast<module_version_t>(library->import(LSP_VERSION_FUNC_NAME));
            mversion = (vfunc != NULL) ? vfunc() : NULL; // Obtain interface version
            if (mversion == NULL)
            {
                lsp_error("Failed to load library %s: missing module version function", info->library.get_native());
                return STATUS_INCOMPATIBLE;
            }

            // Lookup factory function
            audio::factory_function_t factory_func = reinterpret_cast<audio::factory_function_t>(library->import(LSP_AUDIO_FACTORY_FUNCTION_NAME));
            if (factory_func == NULL)
            {
                lsp_error("Failed to load library %s: missing factory function %s", info->library.get_native(), LSP_AUDIO_FACTORY_FUNCTION_NAME);
                return STATUS_NOT_FOUND;
            }

            // Get audio factory
            audio::factory_t * const factory = factory_func(int(info->factory_id));
            if (factory == NULL)
            {
                lsp_error("Failed to load library %s: unable to obtain factory with index %d", info->library.get_native(), int(info->factory_id));
                return STATUS_NOT_FOUND;
            }

            // Now intantiate client
            if ((result = factory->create(factory, info->local_id)) == NULL)
                return STATUS_NO_MEM;

            // Return successful result
            *backend = result;
            close_library = false;

            return STATUS_OK;
        }

    } /* namespace core */
} /* namespace lsp */


