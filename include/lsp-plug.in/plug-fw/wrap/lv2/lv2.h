/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_LV2_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_LV2_H_

// LV2 includes
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/core/lv2.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/midi/midi.h>
#include <lv2/options/options.h>
#include <lv2/patch/patch.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/port-props/port-props.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/state/state.h>
#include <lv2/time/time.h>
#include <lv2/ui/ui.h>
#include <lv2/units/units.h>
#include <lv2/urid/urid.h>
#include <lv2/worker/worker.h>

// Non-official features
#include <ardour/inline-display.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/ext/osc.h>

// Other header includes
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    inline void lv2_set_string(char *dst, size_t limit, const char *src, size_t len)
    {
        if ((src != NULL) && (len > 0))
        {
            size_t copy     = (len >= limit) ? limit-1 : len;
            ::memcpy(dst, src, len);
            dst[copy]       = '\0';
        }
        else
            dst[0]          = '\0';
    }
} /* namespace lsp */
#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_LV2_H_ */
