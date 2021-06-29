/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 21 апр. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_CONFIG_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_CONFIG_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/fmt/config/Serializer.h>

namespace lsp
{
    namespace core
    {
        /**
         * Serialize port value
         *
         * @param s configuration serializer
         * @param meta port metadata
         * @param data port data
         * @param base base path (can be NULL)
         * @param flags serialization flags
         * @return status of operation or STATUS_BAD_TYPE if port can not be serialized
         */
        status_t serialize_port_value(config::Serializer *s,
                const meta::port_t *meta, const void *data, const io::Path *base, size_t flags
        );

        /**
         * Format relative path to the output string
         * @param value destination string to store value
         * @param path path to format
         * @param base base path to use
         * @return true if value has been formatted
         */
        bool format_relative_path(LSPString *value, const char *path, const io::Path *base);

        /**
         * Extract relative path to the output path
         * @param path destination path to store relative value
         * @param base base path to use
         * @param value the original value to parse
         * @param len the length in octetx of the original value
         * @return true if the relative path has been extracted
         */
        bool parse_relative_path(io::Path *path, const io::Path *base, const char *value, size_t len);
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_CONFIG_H_ */
