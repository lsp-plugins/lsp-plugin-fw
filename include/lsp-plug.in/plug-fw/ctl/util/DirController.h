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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_UTIL_DIRCONTROLLER_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_UTIL_DIRCONTROLLER_H_

#include <lsp-plug.in/plug-fw/version.h>

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/runtime/system.h>
#include <lsp-plug.in/io/Path.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * Simple expression-based boolean property
         */
        class DirController
        {
            protected:
                bool                        bValid;         // Valid/invalid state
                ssize_t                     nFileIndex;     // Current file index in the list
                system::time_millis_t       nLastRefresh;   // Last refresh time of file list
                system::time_millis_t       nRefreshPeriod; // Refresh period
                LSPString                   sFileExt;       // Current file extension
                io::Path                    sDirectory;     // Current directory
                lltl::parray<LSPString>     vFiles;         // List of directory files

            protected:
                static void         drop_paths(lltl::parray<LSPString> *files);
                static ssize_t      file_cmp_function(const LSPString *a, const LSPString *b);
                static ssize_t      index_of(lltl::parray<LSPString> *files, const LSPString *name);

            public:
                explicit        DirController();
                DirController(const DirController &) = delete;
                DirController(DirController &&) = delete;
                ~DirController();

                DirController & operator = (const DirController &) = delete;
                DirController & operator = (DirController &&) = delete;

                void                init(ui::IWrapper *wrapper);

            public:
                bool                set_current_file(const char *name);
                bool                set_current_file(const LSPString * name);
                bool                set_current_file(const io::Path * name);

                void                set(const char *name, const char *value);
                bool                sync_file_list(bool force);
                bool                valid() const;
                const io::Path     *directory() const;
                size_t              num_files() const;
                ssize_t             file_index() const;
                lltl::parray<LSPString> * files();
        };
    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_UTIL_DIRCONTROLLER_H_ */
