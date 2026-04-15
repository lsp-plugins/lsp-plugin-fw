/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 9 апр. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_AUDIOBACKEND_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_AUDIOBACKEND_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/audio/iface/factory.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace core
    {
        typedef struct AudioBackendInfo
        {
            LSPString           library;            // Path to the library
            LSPString           uid;                // Unique identifier
            LSPString           display;            // The display name of the backend
            LSPString           lc_key;             // Localization key
            version_t           version;            // Module version
            audio::factory_t   *builtin;            // Pointer to built-in factory
            size_t              factory_id;         // The ID of the factory
            size_t              local_id;           // The ID of the item inside of the factory
            size_t              priority;           // Backend priority
        } AudioBackendInfo;

        /**
         * Scan for all audio backends
         * @param list list to store audio backends
         * @return status of operation
         */
        status_t scan_audio_backends(lltl::parray<AudioBackendInfo> * list);

        /**
         * Cleanup the list of audio backends
         * @param list list of audio backends to clear
         */
        void free_audio_backends(lltl::parray<AudioBackendInfo> * list);

        /**
         * Create audio backend
         * @param backend pointer to store audio backend
         * @param library library object for loading shared object (if needed)
         * @param info backend descriptor
         * @return status of operation
         */
        status_t create_audio_backend(audio::backend_t ** backend, ipc::Library *library, const AudioBackendInfo *info);
    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_AUDIOBACKEND_H_ */
