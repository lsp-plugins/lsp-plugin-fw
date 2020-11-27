/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
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

#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/common/alloc.h>

namespace lsp
{
    namespace meta
    {
        typedef struct unit_desc_t
        {
            const char *name;
            const char *lc_key;
        } unit_desc_t;

        const unit_desc_t unit_desc[] =
        {
            { NULL,     NULL },
            { NULL,     NULL },
            { NULL,     NULL },
            { "%",      "units.pc" },

            { "mm",     "units.mm" },
            { "cm",     "units.cm" },
            { "m",      "units.m" },
            { "\"",     "units.inch" },
            { "km",     "units.km" },

            { "m/s",    "units.mps" },
            { "km/h",   "units.kmph" },

            { "samp",   "units.samp" },

            { "Hz",     "units.hz" },
            { "kHz",    "units.khz" },
            { "MHz",    "units.mhz" },
            { "bpm",    "units.bpm" },

            { "cent",   "units.cent" },
            { "oct",    "units.octave" },
            { "st",     "units.st" },

            { "bar",    "units.bar" },
            { "beat",   "units.beat" },
            { "min",    "units.min" },
            { "s",      "units.s" },
            { "ms",     "units.ms" },

            { "dB",     "units.db" },
            { "G",      "units.gain" },
            { "G",      "units.gain" },

            { "°",      "units.deg" },
            { "°C",     "units.degc" },
            { "°F",     "units.degf" },
            { "°K",     "units.degk" },
            { "°R",     "units.degr" },

            NULL
        };

        const char *get_unit_name(size_t unit)
        {
            return ((unit >= 0) && (unit <= U_ENUM)) ?
                    unit_desc[unit].name : NULL;
        }

        const char *get_unit_lc_key(size_t unit)
        {
            return ((unit >= 0) && (unit <= U_ENUM)) ?
                    unit_desc[unit].lc_key : NULL;
        }

        unit_t get_unit(const char *name)
        {
            for (ssize_t i=0; i<= U_ENUM; ++i)
            {
                const char *uname = unit_desc[i].name;
                if ((uname != NULL) && (!strcmp(name, uname)))
                    return unit_t(i);
            }
            return U_NONE;
        }

        bool is_discrete_unit(size_t unit)
        {
            switch (unit)
            {
                case U_BOOL:
                case U_SAMPLES:
                case U_ENUM:
                    return true;
                default:
                    break;
            }
            return false;
        }

        bool is_decibel_unit(size_t unit)
        {
            switch (unit)
            {
                case U_DB:
                case U_GAIN_AMP:
                case U_GAIN_POW:
                    return true;
                default:
                    break;
            }
            return false;
        }

        bool is_gain_unit(size_t unit)
        {
            switch (unit)
            {
                case U_GAIN_AMP:
                case U_GAIN_POW:
                    return true;
                default:
                    break;
            }
            return false;
        }

        bool is_degree_unit(size_t unit)
        {
            switch (unit)
            {
                case U_DEG:
                case U_DEG_CEL:
                case U_DEG_FAR:
                case U_DEG_K:
                case U_DEG_R:
                    return true;
                default:
                    break;
            }
            return false;
        }

        bool is_log_rule(const port_t *port)
        {
            if (port->flags & F_LOG)
                return true;
            return is_decibel_unit(port->unit);
        }

        size_t list_size(const port_item_t *list)
        {
            size_t size = 0;
            for ( ; (list != NULL) && (list->text != NULL); ++list)
                ++size;
            return size;
        }

        float limit_value(const port_t *port, float value)
        {
            if ((port->flags & (F_CYCLIC | F_UPPER | F_LOWER)) == (F_CYCLIC | F_UPPER | F_LOWER))
            {
                if (port->max > port->min)
                {
                    value = port->min + fmodf(value - port->min, port->max - port->min);
                    if (value < port->min)
                        value  += port->max - port->min;
                }
                else if (port->min > port->max)
                {
                    value = port->max + fmodf(value - port->max, port->min - port->max);
                    if (value < port->max)
                        value  += port->min - port->max;
                }
            }

            if (port->flags & F_UPPER)
            {
                if (value > port->max)
                    value = port->max;
            }
            if (port->flags & F_LOWER)
            {
                if (value < port->min)
                    value = port->min;
            }
            return value;
        }

        port_t *clone_port_metadata(const port_t *metadata, const char *postfix)
        {
            if (metadata == NULL)
                return NULL;

            size_t  postfix_len     = (postfix != NULL) ? strlen(postfix) : 0;
            size_t  string_bytes    = 0;
            size_t  elements        = 1; // At least PORTS_END should be present

            for (const port_t *p=metadata; p->id != NULL; ++p)
            {
                elements        ++;
                if (postfix_len > 0)
                    string_bytes    += strlen(p->id) + postfix_len + 1;
            }

            // Calculate the overall allocation size
            size_t to_copy          = sizeof(port_t) * elements;
            string_bytes            = align_size(string_bytes, DEFAULT_ALIGN);
            elements                = align_size(to_copy, DEFAULT_ALIGN);
            size_t allocate         = string_bytes + elements;
            uint8_t *ptr            = static_cast<uint8_t *>(malloc(allocate));
            port_t *meta            = reinterpret_cast<port_t *>(ptr);

            // Copy port metadata
            ::memcpy(meta, metadata, to_copy);

            // Update identifiers if needed
            if (postfix_len > 0)
            {
                port_t *m               = meta;
                char *dst               = reinterpret_cast<char *>(ptr + elements);

                for (const port_t *p=metadata; p->id != NULL; ++p, ++m)
                {
                    m->id                   = dst;
                    size_t slen             = strlen(p->id);
                    memcpy(dst, p->id, slen);
                    dst                    += slen;
                    memcpy(dst, postfix, postfix_len);
                    dst                    += postfix_len;
                    *(dst++)                = '\0';
                }
            }

            return meta;
        }

        void drop_port_metadata(port_t *metadata)
        {
            if (metadata != NULL)
                free(metadata);
        }

        size_t port_list_size(const port_t *metadata)
        {
            size_t count = 0;
            while (metadata->id != NULL)
            {
                count       ++;
                metadata    ++;
            }
            return count;
        }
    }
}


