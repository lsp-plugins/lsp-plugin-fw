/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 15 янв. 2024 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_STRING_BUF_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_STRING_BUF_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace vst3
    {
        /**
         * String buffer, easy helper for converting data
         */
        class string_buf
        {
            public:
                static constexpr size_t DEFAULT_CAP        = PATH_MAX / 2;

            private:
                lsp_utf16_t    *u16str;
                char           *u8str;
                size_t          u16cap;
                size_t          u8cap;

            public:
                string_buf();
                string_buf(const string_buf &) = delete;
                string_buf(string_buf &&) = delete;
                ~string_buf();

                string_buf & operator = (const string_buf &) = delete;
                string_buf & operator = (string_buf &&) = delete;

            public:
                /**
                 * Reserve some capacity for the UTF-16 string buffer
                 * @param cap capacity to reserve
                 * @return true on success
                 */
                bool u16reserve(size_t cap);

                /**
                 * Reserve some capacity for the UTF-8 string buffer
                 * @param cap capacity to reserve
                 * @return true on success
                 */
                bool u8reserve(size_t cap);

            public:
                /**
                 * Get UTF-16 string from attribute list
                 *
                 * @param list list to read the string
                 * @param id identifier of parameter
                 * @param byte_order byte order of the original string
                 * @return pointer to decoded string in UTF-8 representation or NULL on error
                 */
                char *get_string(Steinberg::Vst::IAttributeList *list, const char *id, int byte_order);

                /**
                 * Set UTF-16 string to the attribute list
                 *
                 * @param list destination attribute list
                 * @param id parameter identifier
                 * @param value UTF-8 string to set
                 * @return true on success
                 */
                bool set_string(Steinberg::Vst::IAttributeList *list, const char *id, const char *value);
        };

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_STRING_BUF_H_ */
