/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 29 сент. 2024 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/io/Dir.h>

namespace lsp
{
    namespace ctl
    {
        DirController::DirController()
        {
            bValid          = false;
            nFileIndex      = -1;
            nLastRefresh    = 0;
            nRefreshPeriod  = 3 * 1000; // 3 seconds
        }

        DirController::~DirController()
        {
            drop_paths(&vFiles);
        }

        void DirController::drop_paths(lltl::parray<LSPString> *files)
        {
            if (files == NULL)
                return;

            for (lltl::iterator<LSPString> it = files->values(); it; ++it)
            {
                LSPString *s = it.get();
                if (s != NULL)
                    delete s;
            }
            files->flush();
        }

        ssize_t DirController::file_cmp_function(const LSPString *a, const LSPString *b)
        {
        #ifdef PLATFORM_WINDOWS
            return a->compare_to_nocase(b);
        #else
            return a->compare_to(b);
        #endif /* PLATFORM_WINDOWS */
        }

        ssize_t DirController::index_of(lltl::parray<LSPString> *files, const LSPString *name)
        {
            ssize_t first=0, last = files->size() - 1;

            // Binary search in sorted array
            while (first <= last)
            {
                const ssize_t center = (first + last) >> 1;
                const LSPString *value = files->uget(center);
                if (value == NULL)
                    return -1;

                const ssize_t cmp_result = file_cmp_function(name, value);
                if (cmp_result < 0)
                    last    = center - 1;
                else if (cmp_result > 0)
                    first   = center + 1;
                else
                    return center;
            }

            // Record was not found
            return -1;
        }

        bool DirController::valid() const
        {
            return bValid;
        }

        ssize_t DirController::file_index() const
        {
            return (bValid) ? nFileIndex : -1;
        }

        size_t DirController::num_files() const
        {
            return (bValid) ? vFiles.size() : 0;
        }

        const io::Path *DirController::directory() const
        {
            return (bValid) ? &sDirectory : NULL;
        }

        lltl::parray<LSPString> *DirController::files()
        {
            return (bValid) ? &vFiles : NULL;
        }

        void DirController::set(const char *name, const char *value)
        {
            if ((name == NULL) || (value == NULL))
                return;

            if ((!strcmp(name, "period")) || (!strcmp(name, "refresh_period")))
                PARSE_UINT(value, nRefreshPeriod = __);
        }

        bool DirController::sync_file_list(bool force)
        {
            const system::time_millis_t time = system::get_time_millis();
            if (time >= (nLastRefresh + nRefreshPeriod))
                force       = true;

            if (!force)
                return false;
            lsp_finally { nLastRefresh = time; };

            // Prepare list to receive files
            lltl::parray<LSPString> files;
            lsp_finally { drop_paths(&files); };

            // Open directory for reading
            io::Dir dir;
            status_t res = dir.open(&sDirectory);
            if (res != STATUS_OK)
            {
                vFiles.swap(&files);
                return true;
            }
            lsp_finally { dir.close(); };

            // Read directory contents
            LSPString tmp;
            while ((res = dir.read(&tmp)) == STATUS_OK)
            {
                // Check that file extension matches
                if (!tmp.ends_with_nocase(&sFileExt))
                    continue;

                // Add item to list
                LSPString *item = tmp.clone();
                if (item == NULL)
                {
                    res     = STATUS_NO_MEM;
                    break;
                }
                if (!files.add(item))
                {
                    delete item;
                    res     = STATUS_NO_MEM;
                    break;
                }
            }

            // Verify read status
            if (res == STATUS_EOF)
            {
                files.qsort(file_cmp_function);
                vFiles.swap(&files);
            }
            else
                vFiles.clear();

            return true;
        }

        bool DirController::set_current_file(const char *path)
        {
            io::Path full_name;
            if (full_name.set(path) != STATUS_OK)
                return bValid = false;

            return set_current_file(&full_name);
        }

        bool DirController::set_current_file(const LSPString *path)
        {
            io::Path full_name;
            if (full_name.set(path) != STATUS_OK)
                return bValid = false;

            return set_current_file(&full_name);
        }

        bool DirController::set_current_file(const io::Path *path)
        {
            bool valid      = false;
            bool updated    = false;
            lsp_finally {
                if (!valid)
                {
                    sDirectory.clear();
                    sFileExt.clear();
                    nFileIndex      = -1;
                }
                bValid          = valid;
            };

            // Obtain file parameters
            io::Path directory;
            if (path->get_parent(&directory) != STATUS_OK)
                return false;
            LSPString name, ext;
            if (path->get_ext(&ext) != STATUS_OK)
                return false;
            if (path->get_last(&name) != STATUS_OK)
                return false;
            if (!ext.prepend('.'))
                return false;

            bool force_sync = false;
            if (!sFileExt.equals_nocase(&ext))
            {
                sFileExt.swap(&ext);
                force_sync  = true;
            }
            if (!sDirectory.equals(&directory))
            {
                sDirectory.swap(&directory);
                force_sync  = true;
            }

            ssize_t file_index = (!force_sync) ? index_of(&vFiles, &name) : -1;
            if (file_index < 0)
                force_sync  = true;

            // Synchronize file list
            if (sync_file_list(force_sync))
            {
                file_index  = index_of(&vFiles, &name);
                updated     = true;
            }

            // Update selected file index
            nFileIndex      = file_index;
            valid           = true;
            return updated;
        }

    } /* namespace ctl */
} /* namespace lsp */

