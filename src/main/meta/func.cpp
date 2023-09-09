/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/dsp-units/const.h>
#include <lsp-plug.in/dsp-units/units.h>

#include <locale.h>
#include <errno.h>

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
            { "oct",    "units.oct" },
            { "st",     "units.st" },

            { "bar",    "units.bar" },
            { "beat",   "units.beat" },
            { "min",    "units.min" },
            { "s",      "units.s" },
            { "ms",     "units.ms" },

            { "dB",     "units.db" },
            { "G",      "units.gain" },
            { "G",      "units.gain" },
            { "Np",     "units.neper" },

            { "°",      "units.deg" },
            { "°C",     "units.degc" },
            { "°F",     "units.degf" },
            { "°K",     "units.degk" },
            { "°R",     "units.degr" },

            { "B",      "units.bytes" },
            { "kB",     "units.kbytes" },
            { "MB",     "units.mbytes" },
            { "GB",     "units.gbytes" },
            { "TB",     "units.tbytes" },

            { NULL,     NULL }
        };

        static const port_item_t default_bool[] =
        {
            { "off",    "bool.off" },
            { "on",     "bool.on" },
            { NULL,     NULL }
        };

        static const char *skip_blank(const char *src, size_t required = 0)
        {
            size_t skipped = 0;

            while (true)
            {
                switch (*src)
                {
                    case ' ':
                    case '\n':
                    case '\r':
                    case '\t':
                    case '\v':
                        ++skipped;
                        ++src;
                        break;
                    default:
                        return (skipped >= required) ? src : NULL;
                }
            }
        }

        static inline char tolower(char c)
        {
            return ((c >= 'A') && (c <= 'Z')) ? c + 'a' - 'A' : c;
        }

        static bool check_match(const char *s, const char *pat)
        {
            for (;; ++s, ++pat)
            {
                if (*s == '\0')
                    return *pat == '\0';
                else if (*pat == '\0')
                    return true;
                else if (tolower(*s) != tolower(*pat))
                    return false;
            }
            return true;
        }

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

        bool is_bool_unit(size_t unit)
        {
            switch (unit)
            {
                case U_BOOL:
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

        bool is_enum_unit(size_t unit)
        {
            switch (unit)
            {
                case U_ENUM:
                    return true;
                default:
                    break;
            }
            return false;
        }

        bool is_log_rule(const port_t *port)
        {
            return (port->flags & F_LOG);
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
            if (ptr == NULL)
                return NULL;

            // Copy port metadata
            port_t *meta            = reinterpret_cast<port_t *>(ptr);
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

        bool match_bool(float value)
        {
            return (value == 1.0f) || (value == 0.0f);
        }

        bool match_enum(const port_t *meta, float value)
        {
            float min   = (meta->flags & F_LOWER) ? meta->min: 0.0f;
            float step  = (meta->flags & F_STEP) ? meta->step : 1.0f;

            for (const port_item_t *p = meta->items; (p != NULL) && (p->text != NULL); ++p)
            {
                if (value == min)
                    return true;
                min    += step;
            }

            return false;
        }

        bool match_int(const port_t *meta, float value)
        {
            float start     = (meta->flags & F_LOWER) ? meta->min   : 0;
            float end       = (meta->flags & F_UPPER) ? meta->max   : 0;

            if (start < end)
                return (value >= start) && (value <= end);

            return (value >= end) && (value <= start);
        }

        bool match_float(const port_t *meta, float value)
        {
            float start     = (meta->flags & F_LOWER) ? meta->min   : 0;
            float end       = (meta->flags & F_UPPER) ? meta->max   : 0;

            if (start < end)
                return (value >= start) && (value <= end);

            return (value >= end) && (value <= start);
        }

        bool range_match(const port_t *meta, float value)
        {
            if (meta->unit == U_BOOL)
                return match_bool(value);
            else if (meta->unit == U_ENUM)
                return match_enum(meta, value);
            else if (meta->flags & F_INT)
                return match_int(meta, value);

            return match_float(meta, value);
        }

        void format_float(char *buf, size_t len, const port_t *meta, float value, ssize_t precision, bool units)
        {
            const char *unit    = (units) ? get_unit_name(meta->unit) : NULL;
            float v = (value < 0.0f) ? - value : value;
            size_t tolerance    = 0;

            // Select the tolerance of output value
            if (precision < 0)
            {
                // Determine regular tolerance
                if (v < 0.1f)
                    tolerance   = 4;
                else if (v < 1.0f)
                    tolerance   = 3;
                else if (v < 10.0f)
                    tolerance   = 2;
                else if (v < 100.0f)
                    tolerance   = 1;
                else
                    tolerance   = 0;

                // Now determine normal tolerance
                if (meta->flags & F_STEP)
                {
                    size_t max_tol = 0;
                    float step      = (meta->step < 0.0f) ? - meta->step : meta->step;
                    while ((max_tol < 4) && (truncf(step) <= 0))
                    {
                        step   *= 10;
                        max_tol++;
                    }

                    if (tolerance > max_tol)
                        tolerance = max_tol;
                }
            }
            else
                tolerance   = (precision > 4) ? 4 : precision;

            const char *fmt;
            if (unit != NULL)
            {
                switch (tolerance)
                {
                    case 4:     fmt = "%.4f %s"; break;
                    case 3:     fmt = "%.3f %s"; break;
                    case 2:     fmt = "%.2f %s"; break;
                    case 1:     fmt = "%.1f %s"; break;
                    default:    fmt = "%.0f %s"; break;
                };
                snprintf(buf, len, fmt, value, unit);
            }
            else
            {
                switch (tolerance)
                {
                    case 4:     fmt = "%.4f"; break;
                    case 3:     fmt = "%.3f"; break;
                    case 2:     fmt = "%.2f"; break;
                    case 1:     fmt = "%.1f"; break;
                    default:    fmt = "%.0f"; break;
                };
                snprintf(buf, len, fmt, value);
            }

            if (len > 0)
                buf[len - 1] = '\0';
        }

        void format_int(char *buf, size_t len, const port_t *meta, float value, bool units)
        {
            const char *unit    = (units) ? get_unit_name(meta->unit) : NULL;
            if (unit == NULL)
                snprintf(buf, len, "%ld", long(value));
            else
                snprintf(buf, len, "%ld %s", long(value), unit);
            if (len > 0)
                buf[len - 1] = '\0';
        }

        void format_enum(char *buf, size_t len, const port_t *meta, float value)
        {
            float min   = (meta->flags & F_LOWER) ? meta->min: 0;
            float step  = (meta->flags & F_STEP) ? meta->step : 1.0;

            for (const port_item_t *p = meta->items; (p != NULL) && (p->text != NULL); ++p)
            {
                if (min >= value)
                {
                    ::strncpy(buf, p->text, len);
                    buf[len - 1] = '\0';
                    return;
                }
                min    += step;
            }
            if (len > 0)
                buf[0] = '\0';
        }

        void format_decibels(char *buf, size_t len, const port_t *meta, float value, ssize_t precision, bool units)
        {
            const char *fmt;
            const char *unit    = (units) ? get_unit_name(meta::U_DB) : NULL;
            double mul          = (meta->unit == U_GAIN_AMP) ? 20.0 : 10.0;
            if (value < 0.0f)
                value               = - value;

            value               = mul * log(value) / M_LN10;
            float thresh        = (meta->flags & F_EXT) ? -140.0f : -80.0f;

            if (unit != NULL)
            {
                if (value <= thresh)
                {
                    snprintf(buf, len, "-inf %s", unit);
                    if (len > 0)
                        buf[len - 1] = '\0';
                    return;
                }

                if (precision < 0)
                    fmt = "%.2f %s";
                else if (precision == 1)
                    fmt = "%.1f %s";
                else if (precision == 2)
                    fmt = "%.2f %s";
                else if (precision == 3)
                    fmt = "%.3f %s";
                else
                    fmt = "%.4f %s";

                snprintf(buf, len, fmt, value, unit);
            }
            else
            {
                if (value <= thresh)
                {
                    strcpy(buf, "-inf");
                    return;
                }

                if (precision < 0)
                    fmt = "%.2f";
                else if (precision == 1)
                    fmt = "%.1f";
                else if (precision == 2)
                    fmt = "%.2f";
                else if (precision == 3)
                    fmt = "%.3f";
                else
                    fmt = "%.4f";

                snprintf(buf, len, fmt, value);
            }
            if (len > 0)
                buf[len - 1] = '\0';
        }

        void format_bool(char *buf, size_t len, const port_t *meta, float value)
        {
            const port_item_t *list = (meta->items != NULL) ? meta->items : default_bool;
            if (value >= 0.5f)
                ++list;

            if (list->text != NULL)
            {
                ::strncpy(buf, list->text, len);
                if (len > 0)
                    buf[len - 1] = '\0';
            }
            else if (len > 0)
                buf[0] = '\0';
        }


        void format_value(char *buf, size_t len, const port_t *meta, float value, ssize_t precision, bool units)
        {
            if (meta->unit == U_BOOL)
                format_bool(buf, len, meta, value);
            else if (meta->unit == U_ENUM)
                format_enum(buf, len, meta, value);
            else if ((meta->unit == U_GAIN_AMP) || (meta->unit == U_GAIN_POW))
                format_decibels(buf, len, meta, value, precision, units);
            else if (meta->flags & F_INT)
                format_int(buf, len, meta, value, units);
            else
                format_float(buf, len, meta, value, precision, units);
        }

#define UPDATE_LOCALE(out_var, lc, value) \
        char *out_var = setlocale(lc, NULL); \
        if (out_var != NULL) \
        { \
            size_t ___len = strlen(out_var) + 1; \
            char *___copy = static_cast<char *>(alloca(___len)); \
            memcpy(___copy, out_var, ___len); \
            out_var = ___copy; \
        } \
        setlocale(lc, value);

        status_t parse_bool(float *dst, const char *text, const port_t *meta)
        {
            text    = skip_blank(text);
            float value = 0.0f;

            if (check_match(text, "true"))
            {
                value   = 1.0f;
                text   += 4;
            }
            else if (check_match(text, "on"))
            {
                value   = 1.0f;
                text   += 2;
            }
            else if (check_match(text, "yes"))
            {
                value   = 1.0f;
                text   += 3;
            }
            else if (check_match(text, "t"))
            {
                value   = 1.0f;
                text   += 1;
            }
            else if (check_match(text, "false"))
            {
                value   = 0.0f;
                text   += 5;
            }
            else if (check_match(text, "off"))
            {
                value   = 0.0f;
                text   += 3;
            }
            else if (check_match(text, "no"))
            {
                value   = 0.0f;
                text   += 2;
            }
            else if (check_match(text, "f"))
            {
                value   = 0.0f;
                text   += 1;
            }
            else
            {
                // Update locale
                UPDATE_LOCALE(saved_locale, LC_NUMERIC, "C");
                lsp_finally {
                    if (saved_locale != NULL)
                        ::setlocale(LC_NUMERIC, saved_locale);
                };

                // Parse the floating-point value
                errno       = 0;
                char *end   = NULL;
                value       = ::strtof(text, &end);
                if (errno != 0)
                    return STATUS_INVALID_VALUE;
                text        = end;
                value       = (fabs(value) >= 0.5f) ? 1.0f : 0.0f;
            }

            // Check that there is no data at the end
            text = skip_blank(text);
            if (*text != '\0')
                return STATUS_INVALID_VALUE;

            // Return result
            if (dst != NULL)
                *dst    = value;
            return STATUS_OK;
        }

        status_t parse_enum(float *dst, const char *text, const port_t *meta)
        {
            text        = skip_blank(text);

            // Try to find match to the corresponding enum constant
            float min   = (meta->flags & F_LOWER)   ? meta->min     : 0.0f;
            float step  = (meta->flags & F_STEP)    ? meta->step    : 1.0f;
            float value = min;

            for (const port_item_t *p = meta->items; (p != NULL) && (p->text != NULL); ++p)
            {
                if (check_match(text, p->text))
                {
                    const char *end = skip_blank(text + strlen(p->text));
                    if (*end == '\0')
                    {
                        if (dst != NULL)
                            *dst    = value;
                        return STATUS_OK;
                    }
                }
                value  += step;
            }

            // The value was not found. Try to parse as floating-point
            UPDATE_LOCALE(saved_locale, LC_NUMERIC, "C");
            lsp_finally {
                if (saved_locale != NULL)
                    ::setlocale(LC_NUMERIC, saved_locale);
            };

            // Parse the floating-point value
            errno       = 0;
            char *end   = NULL;
            value       = ::strtof(text, &end);
            if (errno != 0)
                return STATUS_INVALID_VALUE;

            // Validate that there is nothing left at the end
            text        = skip_blank(end);
            if (*text != '\0')
                return STATUS_INVALID_VALUE;

            // Check match
            if (!match_enum(meta, value))
                return STATUS_INVALID_VALUE;

            // Return the result
            if (dst != NULL)
                *dst        = value;

            return STATUS_OK;
        }

        status_t parse_decibels(float *dst, const char *text, const port_t *meta, bool units)
        {
            text = skip_blank(text);

            // Check for -inf
            float value     = 0.0f;
            bool inf        = false;
            if (check_match(text, "-inf"))
            {
                if ((meta->unit == U_GAIN_AMP) || (meta->unit == U_GAIN_POW))
                    value       = 0.0f;
                else
                    value       = -INFINITY;
                text       += 4;
                if (*text != '\0')
                {
                    text        = skip_blank(text, 1);
                    if (text == NULL)
                        return STATUS_INVALID_VALUE;
                }
                inf         = true;
            }
            else if (check_match(text, "+inf"))
            {
                value       = +INFINITY;
                text       += 4;
                if (*text != '\0')
                {
                    text        = skip_blank(text, 1);
                    if (text == NULL)
                        return STATUS_INVALID_VALUE;
                }
                inf         = true;
            }
            else
            {
                UPDATE_LOCALE(saved_locale, LC_NUMERIC, "C");
                lsp_finally {
                    if (saved_locale != NULL)
                        ::setlocale(LC_NUMERIC, saved_locale);
                };

                // Parse the floating-point value
                errno       = 0;
                char *end   = NULL;
                value       = ::strtof(text, &end);
                if ((errno != 0) || (end == text))
                    return STATUS_INVALID_VALUE;
                text        = skip_blank(end);
            }

            // Are the units present?
            if (*text != '\0')
            {
                if (!units)
                    return STATUS_INVALID_VALUE;

                if (check_match(text, "db"))
                {
                    text       += 2;

                    // The input is in decibels, convert to desired metadata type
                    if (!inf)
                    {
                        switch (meta->unit)
                        {
                            case U_DB:
                                break;
                            case U_GAIN_POW:
                                value   = dspu::db_to_power(value);
                                break;
                            case U_NEPER:
                                value   = dspu::db_to_neper(value);
                                break;
                            case U_GAIN_AMP:
                            default:
                                value   = dspu::db_to_gain(value);
                                break;
                        }
                    }
                }
                else if (check_match(text, "np"))
                {
                    text       += 2;

                    // The input is in nepers, convert to desired metadata type
                    if (!inf)
                    {
                        switch (meta->unit)
                        {
                            case U_NEPER:
                                break;
                            case U_GAIN_POW:
                                value   = dspu::neper_to_power(value);
                                break;
                            case U_DB:
                                value   = dspu::neper_to_db(value);
                                break;
                            case U_GAIN_AMP:
                            default:
                                value   = dspu::neper_to_gain(value);
                                break;
                        }
                    }
                }
                else if (check_match(text, "g"))
                {
                    text       += 1;

                    // The input is raw gain, convert to desired metadata type
                    if (!inf)
                    {
                        float thresh    = (meta->flags & F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                        switch (meta->unit)
                        {
                            case U_DB:
                                value   = (value < thresh) ? -INFINITY : dspu::gain_to_db(value);
                                break;
                            case U_NEPER:
                                thresh  = dspu::db_to_neper(thresh);
                                value   = (value < thresh) ? -INFINITY : dspu::gain_to_neper(value);
                                break;
                            case U_GAIN_POW:
                            case U_GAIN_AMP:
                            default:
                                break;
                        }
                    }
                }
                else
                    return STATUS_INVALID_VALUE;

                // Ensure that no more text left
                text = skip_blank(text);
                if (*text != '\0')
                    return STATUS_INVALID_VALUE;
            }
            else if (!inf)
            {
                // Units are not specified
                switch (meta->unit)
                {
                    case U_GAIN_POW:
                        value   = dspu::db_to_power(value);
                        break;
                    case U_GAIN_AMP:
                        value   = dspu::db_to_gain(value);
                        break;
                    default:
                        break;
                }
            }

            // Return the result
            value       = (meta->flags & F_INT) ? truncf(value) : value;
            if (dst != NULL)
                *dst        = value;

            return STATUS_OK;
        }

        status_t parse_note_frequency(float *dst, const char *text, const port_t *meta)
        {
            text = skip_blank(text);

            // Parse the note name
            int note;
            switch (*(text++))
            {
                case 'c': case 'C': note    = 0;  break;
                case 'd': case 'D': note    = 2;  break;
                case 'e': case 'E': note    = 4;  break;
                case 'f': case 'F': note    = 5;  break;
                case 'g': case 'G': note    = 7;  break;
                case 'a': case 'A': note    = 9;  break;
                case 'b': case 'B': note    = 11; break;
                case 'h': case 'H': note    = 11; break;
                default:
                    return STATUS_INVALID_VALUE;
            }

            // Check alterations
            if (*text == '#')
            {
                ++text;
                ++note;

                // Double alteration?
                if (*text == '#')
                {
                    ++text;
                    ++note;
                }
            }
            else if (*text == 'b')
            {
                ++text;
                --note;

                // Double alteration?
                if (*text == 'b')
                {
                    ++text;
                    --note;
                }
            }

            // Parse the octave (if specified)
            text        = skip_blank(text);
            errno       = 0;
            char *end   = NULL;
            long octave = ::strtol(text, &end, 10);
            if ((errno != 0) || (end == text))
            {
                // Unsuccessful parse?
                if (end != text)
                    return STATUS_INVALID_VALUE;
                octave      = 4;
            }

            // Validate the input
            if ((octave < -1) || (octave > 9))
                return STATUS_INVALID_VALUE;
            ssize_t midi_code   = (octave + 1) * 12 + note;
            if ((midi_code < 0) || (midi_code > 127))
                return STATUS_INVALID_VALUE;

            // Check that there are no extra characters at the end
            text        = skip_blank(end);
            if (*text != '\0')
                return STATUS_INVALID_VALUE;

            float value = dspu::midi_note_to_frequency(midi_code);
            if (meta->unit == meta::U_KHZ)
                value  *= 1e-3f;
            else if (meta->unit == meta::U_MHZ)
                value  *= 1e-6f;

            // Store the value and return
            value       = (meta->flags & F_INT) ? truncf(value) : value;
            if (dst != NULL)
                *dst        = value;
            return STATUS_OK;
        }

        status_t parse_frequency(float *dst, const char *text, const port_t *meta, bool units)
        {
            status_t res = parse_note_frequency(dst, text, meta);
            if (res == STATUS_OK)
                return res;

            // Update locale
            UPDATE_LOCALE(saved_locale, LC_NUMERIC, "C");
            lsp_finally {
                if (saved_locale != NULL)
                    ::setlocale(LC_NUMERIC, saved_locale);
            };

            // Parse the floating-point value
            text        = skip_blank(text);
            errno       = 0;
            char *end   = NULL;
            float value = ::strtof(text, &end);
            if ((errno != 0) || (end == text))
                return STATUS_INVALID_VALUE;

            // Ensure that there is no data at the end if units are not allowed
            text        = skip_blank(end);
            if (*text == '\0')
            {
                // No more data?
                if (dst != NULL)
                    *dst        = value;
                return STATUS_OK;
            }
            else if (!units)
                return STATUS_INVALID_VALUE;

            // Parse the unit modifier
            float mod   = 1.0f;
            if (meta->unit == meta::U_KHZ)
            {
                switch (*text)
                {
                    case 'u': mod   = 1e-9f; ++text; break; // micro
                    case 'm': mod   = 1e-6f; ++text; break; // milli
                    case 'k': mod   = 1e+0f; ++text; break; // kilo
                    case 'M': mod   = 1e+3f; ++text; break; // mega
                    case 'G': mod   = 1e+6f; ++text; break; // giga
                    default:  mod   = 1e-3f;  break;
                }
            }
            else if (meta->unit == meta::U_MHZ)
            {
                switch (*text)
                {
                    case 'u': mod   = 1e-12f; ++text; break; // micro
                    case 'm': mod   = 1e-9f; ++text; break; // milli
                    case 'k': mod   = 1e-3f; ++text; break; // kilo
                    case 'M': mod   = 1e+0f; ++text; break; // mega
                    case 'G': mod   = 1e+3f; ++text; break; // giga
                    default:  mod   = 1e-6f;  break;
                }
            }
            else
            {
                switch (*text)
                {
                    case 'u': mod   = 1e-6f; ++text; break; // micro
                    case 'm': mod   = 1e-3f; ++text; break; // milli
                    case 'k': mod   = 1e+3f; ++text; break; // kilo
                    case 'M': mod   = 1e+6f; ++text; break; // mega
                    case 'G': mod   = 1e+9f; ++text; break; // giga
                    default:  mod   = 1.0f;  break;
                }
            }

            // Check the presence of the 'Hz'
            if (check_match(text, "hz"))
                text       += 2;
            text        = skip_blank(text);
            if (*text != '\0')
                return STATUS_INVALID_VALUE;

            // Return the final result
            value       = (meta->flags & F_INT) ? truncf(value * mod) : value * mod;
            if (dst != NULL)
                *dst        = value;
            return STATUS_OK;
        }

        status_t parse_time(float *dst, const char *text, const port_t *meta, bool units)
        {
            // Update locale
            UPDATE_LOCALE(saved_locale, LC_NUMERIC, "C");
            lsp_finally {
                if (saved_locale != NULL)
                    ::setlocale(LC_NUMERIC, saved_locale);
            };

            // Parse the floating-point value
            text        = skip_blank(text);
            errno       = 0;
            char *end   = NULL;
            float value = ::strtof(text, &end);
            if ((errno != 0) || (end == text))
                return STATUS_INVALID_VALUE;
            text        = skip_blank(end);

            // Ensure that there is no data at the end if units are not allowed
            if (*text == '\0')
            {
                // No more data?
                if (dst != NULL)
                    *dst        = value;
                return STATUS_OK;
            }
            else if (!units)
                return STATUS_INVALID_VALUE;

            // Parse the unit modifier
            if (check_match(text, "min"))
            {
                text       += 3;
                switch (meta->unit)
                {
                    case U_SEC:     value *= 60.0f;     break;
                    case U_MSEC:    value *= 60e+3f;    break;
                    case U_MIN:
                    default: break;
                }
            }
            else if (check_match(text, "s"))
            {
                text       += 1;
                switch (meta->unit)
                {
                    case U_MIN:     value /= 60.0f;     break;
                    case U_MSEC:    value *= 1e+3f;     break;
                    case U_SEC:
                    default: break;
                }
            }
            else if (check_match(text, "ms"))
            {
                text       += 2;
                switch (meta->unit)
                {
                    case U_MIN:     value /= 60.0e+3f;  break;
                    case U_SEC:     value *= 1e-3f;     break;
                    case U_MSEC:
                    default: break;
                }
            }
            else if (check_match(text, "us"))
            {
                text       += 2;
                switch (meta->unit)
                {
                    case U_MIN:     value /= 60.0e+6f;  break;
                    case U_SEC:     value *= 1e-6f;     break;
                    case U_MSEC:    value *= 1e-3f;     break;
                    default: break;
                }
            }
            else if (check_match(text, "ns"))
            {
                text       += 2;
                switch (meta->unit)
                {
                    case U_MIN:     value /= 60.0e+9f;  break;
                    case U_SEC:     value *= 1e-9f;     break;
                    case U_MSEC:    value *= 1e-6f;     break;
                    default: break;
                }
            }

            text        = skip_blank(text);
            if (*text != '\0')
                return STATUS_INVALID_VALUE;

            // Return the final result
            value       = (meta->flags & F_INT) ? truncf(value) : value;
            if (dst != NULL)
                *dst        = value;
            return STATUS_OK;
        }

        status_t parse_int(float *dst, const char *text, const port_t *meta, bool units)
        {
            // Update locale
            UPDATE_LOCALE(saved_locale, LC_NUMERIC, "C");
            lsp_finally {
                if (saved_locale != NULL)
                    ::setlocale(LC_NUMERIC, saved_locale);
            };

            // Parse the integer value
            errno       = 0;
            char *end   = NULL;
            long value  = ::strtol(text, &end, 10);
            if ((errno != 0) || (end == text))
                return STATUS_INVALID_VALUE;
            text        = skip_blank(end);

            // Check presence of the unit value
            const char *unit    = (units) ? get_unit_name(meta->unit) : NULL;
            if ((unit != NULL) && (check_match(text, unit)))
                text        = skip_blank(text + strlen(unit));

            // Check that we reached the end of string
            if (*text != '\0')
                return STATUS_INVALID_VALUE;

            // Return the result
            if (dst != NULL)
                *dst    = value;
            return STATUS_OK;
        }

        status_t parse_float(float *dst, const char *text, const port_t *meta, bool units)
        {
            // Update locale
            UPDATE_LOCALE(saved_locale, LC_NUMERIC, "C");
            lsp_finally {
                if (saved_locale != NULL)
                    ::setlocale(LC_NUMERIC, saved_locale);
            };

            // Parse the floating-point value
            errno       = 0;
            char *end   = NULL;
            float value = ::strtof(text, &end);
            if ((errno != 0) || (end == text))
                return STATUS_INVALID_VALUE;
            text        = skip_blank(end);

            // Check presence of the unit value
            const char *unit    = (units) ? get_unit_name(meta->unit) : NULL;
            if ((unit != NULL) && (check_match(text, unit)))
                text        = skip_blank(text + strlen(unit));

            // Check that we reached the end of string
            if (*text != '\0')
                return STATUS_INVALID_VALUE;

            // Return the result
            if (dst != NULL)
                *dst    = value;
            return STATUS_OK;
        }

        status_t parse_value(float *dst, const char *text, const port_t *meta, bool units)
        {
            if ((text == NULL) || (meta == NULL))
                return STATUS_BAD_ARGUMENTS;

            text = skip_blank(text);
            if (*text == '\0')
                return STATUS_BAD_ARGUMENTS;

            if (meta->unit == U_BOOL)
                return parse_bool(dst, text, meta);
            else if (meta->unit == U_ENUM)
                return parse_enum(dst, text, meta);
            else if ((meta->unit == U_GAIN_AMP) || (meta->unit == U_GAIN_POW) || (meta->unit == U_DB) || (meta->unit == U_NEPER))
                return parse_decibels(dst, text, meta, units);
            else if ((meta->unit == U_HZ) || (meta->unit == U_KHZ) || (meta->unit == U_MHZ))
                return parse_frequency(dst, text, meta, units);
            else if ((meta->unit == U_MIN) || (meta->unit == U_SEC) || (meta->unit == U_MSEC))
                return parse_time(dst, text, meta, units);
            else if (meta->flags & F_INT)
                return parse_int(dst, text, meta, units);
            else
                return parse_float(dst, text, meta, units);

            return STATUS_BAD_ARGUMENTS;
        }

        void get_port_parameters(const port_t *p, float *min, float *max, float *step)
        {
            float f_min = 0.0f, f_max = 1.0f, f_step = 0.001f;

            if (p->unit == U_BOOL)
            {
                f_min       = 0.0f;
                f_max       = 1.0f;
                f_step      = 1.0f;
            }
            else if (p->unit == U_ENUM)
            {
                f_min       = (p->flags & F_LOWER) ? p->min : 0.0f;
                f_max       = f_min + list_size(p->items) - 1;
                f_step      = 1.0f;
            }
            else if (p->unit == U_SAMPLES)
            {
                f_min       = p->min;
                f_max       = p->max;
                f_step      = 1.0f;
            }
            else
            {
                f_min       = (p->flags & F_LOWER) ? p->min : 0.0f;
                f_max       = (p->flags & F_UPPER) ? p->max : 1.0f;

                if (p->flags & F_INT)
                    f_step      = (p->flags & F_STEP) ? p->step : 1.0f;
                else
                    f_step      = (p->flags & F_STEP) ? p->step : (f_max - f_min) * 0.001;
            }

            if (min != NULL)
                *min        = f_min;
            if (max != NULL)
                *max        = f_max;
            if (step != NULL)
                *step       = f_step;
        }
    } /* namespace meta */
} /* namespace lsp */


