/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 15 янв. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_STRING_BUF_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_STRING_BUF_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/io/charset.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/string_buf.h>
#include <lsp-plug.in/stdlib/stdlib.h>

namespace lsp
{
    namespace vst3
    {
        string_buf::string_buf()
        {
            u16str      = NULL;
            u8str       = NULL;
            u16cap      = 0;
            u8cap       = 0;
        }

        string_buf::~string_buf()
        {
            if (u16str != NULL)
            {
                free(u16str);
                u16str      = NULL;
            }
            if (u8str != NULL)
            {
                free(u8str);
                u8str       = NULL;
            }
        }

        bool string_buf::u16reserve(size_t cap)
        {
            cap = lsp_min(cap, DEFAULT_CAP);
            if (cap < u16cap)
                return true;

            if (u16str != NULL)
            {
                free(u16str);
                u16cap          = 0;
            }

            u16str  = static_cast<lsp_utf16_t *>(malloc(cap * sizeof(lsp_utf16_t)));
            if (u16str == NULL)
                return false;

            u16cap  = cap;
            return true;
        }

        bool string_buf::u8reserve(size_t cap)
        {
            cap = lsp_min(cap, DEFAULT_CAP);
            if (cap < u8cap)
                return true;

            if (u8str != NULL)
            {
                free(u8str);
                u8cap           = 0;
            }

            u8str   = static_cast<char *>(malloc(cap * sizeof(char)));
            if (u8str == NULL)
                return false;

            u8cap   = cap;
            return true;
        }

        char *string_buf::get_string(Steinberg::Vst::IAttributeList *list, const char *id, int byte_order)
        {
            // Read string from attribute list
            for (size_t cap = lsp_max(u16cap, DEFAULT_CAP); ; cap = cap + (cap >> 1))
            {
                if (!u16reserve(cap))
                    return NULL;

                // Fetch string from attribute list
//                lsp_trace("cap=%d, u16str=%p, list=%p, count=%d",
//                    int(cap), u16str, list, int(cap * sizeof(lsp_utf16_t)));
                Steinberg::tresult res = list->getString(id, to_tchar(u16str), cap * sizeof(lsp_utf16_t));
                if (res != Steinberg::kResultOk)
                {
                    lsp_trace("Failed to fetch string data for parameter id='%s'", id);
                    return NULL;
                }

                // Ensure that the whole string has been parsed (it should be NULL-terminated)
                size_t len = vst3::strnlen_u16(u16str, cap);
//                lsp_trace("len=%d, cap=%d", int(len), int(cap));
                if (len < cap)
                    break;
            }

            // Convert string to UTF-8
            for (size_t cap = lsp_max(u8cap, DEFAULT_CAP); ; cap = cap + (cap >> 1))
            {
                if (!u8reserve(cap))
                    return NULL;

                // Convert string from UTF-16 to UTF-8
                size_t converted = (byte_order == kLittleEndian) ?
                    utf16le_to_utf8(u8str, u16str, cap) :
                    utf16be_to_utf8(u8str, u16str, cap);

                // Ensure that string has been properly converted (number of codepoints should be positive)
                if (converted > 0)
                    break;
            }

//            lsp_trace("data = %s", u8str);

            return u8str;
        }

        bool string_buf::set_string(Steinberg::Vst::IAttributeList *list, const char *id, const char *value)
        {
            // Read string from attribute list
            for (size_t cap = lsp_max(u16cap, DEFAULT_CAP); ; cap = cap + (cap >> 1))
            {
                if (!u16reserve(cap))
                    return NULL;

                // Convert string from UTF-8 to UTF-16
                size_t converted = utf8_to_utf16(u16str, value, u16cap);

                // Ensure that string has been properly converted (number of codepoints should be positive)
                if (converted > 0)
                    break;
            }

            return list->setString(id, to_tchar(u16str)) == Steinberg::kResultOk;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_STRING_BUF_H_ */
