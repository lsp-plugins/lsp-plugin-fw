/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 окт. 2026 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_QUIRKS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_QUIRKS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/runtime/LSPString.h>

#include <steinberg/vst3.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>

namespace lsp
{
    namespace vst3
    {
        typedef uint32_t host_quirks_t;

        enum host_quirk_t
        {
            /**
             * Detected host that have potential memory leaks on sending VST3 messages.
             */
            HQUIRK_CUSTOM_VST3_MESSAGE      = 1 << 0,

            /**
             * Detected host that does not accept strict bus arrangements.
             */
            HQUIRK_FLEX_BUS_ARRANGEMENTS    = 1 << 1,
        };

        static inline bool check_host_string(const LSPString & host_name, LSPString & temp, const char *text)
        {
            if (!temp.set_ascii(text))
                return false;

            return host_name.index_of(&temp) >= 0;
        }

        inline host_quirks_t get_host_quirks(Steinberg::Vst::IHostApplication *host)
        {
            Steinberg::Vst::String128 host_name;
            if (host->getName(host_name) != Steinberg::kResultOk)
                return false;

            LSPString name, check;
            if (!name.set_utf16(to_utf16(host_name)))
                return false;

            lsp_trace("Obtained VST3 Host Name: %s", name.get_native());
            name.tolower();

            // Bitwig Studio
            if (check_host_string(name, check, "bitwig studio"))
                return HQUIRK_CUSTOM_VST3_MESSAGE;

            // Audirvana
            if (check_host_string(name, check, "audirvana"))
                return HQUIRK_FLEX_BUS_ARRANGEMENTS;

            // Some crap hosts that can not even identify themselves
            if (check_host_string(name, check, "my vst3 hostapplication"))
                return HQUIRK_FLEX_BUS_ARRANGEMENTS;

            return 0;
        }

        inline void log_host_quirks(host_quirks_t quirks)
        {
            if (quirks & HQUIRK_CUSTOM_VST3_MESSAGE)
                lsp_trace("Using Steinberg::Vst::IMessage workaround");
            if (quirks & HQUIRK_FLEX_BUS_ARRANGEMENTS)
                lsp_trace("Using Steinberg::vst::IAudioProcessor::setBusArrangements workaround");
        }

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_QUIRKS_H_ */
