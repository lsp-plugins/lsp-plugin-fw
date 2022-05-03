/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 апр. 2021 г.
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

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/stdlib/stdlib.h>

#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

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

namespace lsp
{
    namespace ctl
    {
        static const file_format_t file_formats[] =
        {
            { "wav",        "*.wav",        "files.audio.wave",         ".wav",     io::PathPattern::NONE },
            { "lspc",       "*.lspc",       "files.config.lspc",        ".lspc",    io::PathPattern::NONE },
            { "cfg",        "*.cfg",        "files.config.lsp",         ".cfg",     io::PathPattern::NONE },
            { "audio",      "*.wav",        "files.audio.supported",    ".wav",     io::PathPattern::NONE },
            { "audio_lspc", "*.wav|*.lspc", "files.audio.audio_lspc",   ".wav",     io::PathPattern::NONE },
            { "obj3d",      "*.obj",        "files.3d.wavefront",       ".obj",     io::PathPattern::NONE },
            { "all",        "*",            "files.all",                "",         io::PathPattern::NONE },
            { NULL, NULL, NULL, 0 }
        };

        inline bool is_blank(char c)
        {
            switch (c)
            {
                case ' ':
                case '\n':
                case '\r':
                case '\f':
                case '\t':
                    return true;
                default:
                    break;
            }
            return false;
        }

        char *skip_whitespace(const char *v)
        {
            if (v == NULL)
                return NULL;
            while (is_blank(*v))
                ++v;
            return const_cast<char *>(v);
        }

        bool parse_float(const char *arg, float *res)
        {
            UPDATE_LOCALE(saved_locale, LC_NUMERIC, "C");
            errno = 0;
            char *end   = NULL;

            arg         = skip_whitespace(arg);
            float value = strtof(arg, &end);

            bool success = (errno == 0);
            if ((end != NULL) && (success))
            {
                // Skip spaces
                end = skip_whitespace(end);

                if (((end[0] == 'd') || (end[0] == 'D')) &&
                    ((end[1] == 'b') || (end[1] == 'B')))
                {
                    value   = expf(value * M_LN10 * 0.05);
                    end    += 2;
                }

                end     = skip_whitespace(end);
                success = (*end == '\0');
            }

            if (saved_locale != NULL)
                setlocale(LC_NUMERIC, saved_locale);

            if ((res != NULL) && (success))
                *res        = value;
            return success;
        }

        bool parse_double(const char *arg, double *res)
        {
            UPDATE_LOCALE(saved_locale, LC_NUMERIC, "C");
            errno = 0;
            char *end       = NULL;
            arg             = skip_whitespace(arg);
            double value    = strtod(arg, &end);

            bool success    = (errno == 0);
            if ((end != NULL) && (success))
            {
                // Skip spaces
                end = skip_whitespace(end);

                if (((end[0] == 'd') || (end[0] == 'D')) &&
                    ((end[1] == 'b') || (end[1] == 'B')))
                {
                    value   = expf(value * M_LN10 * 0.05);
                    end    += 2;
                }

                end     = skip_whitespace(end);
                success = (*end == '\0');
            }

            if (saved_locale != NULL)
                setlocale(LC_NUMERIC, saved_locale);

            if ((res != NULL) && (success))
                *res        = value;
            return success;
        }

        bool parse_bool(const char *arg, bool *res)
        {
            arg             = skip_whitespace(arg);

            bool b = !::strcasecmp(arg, "true");
            if (! b ) \
                b = !::strcasecmp(arg, "1");

            if (res != NULL)
                *res    = b;

            return true;
        }

        bool parse_int(const char *arg, long *res)
        {
            errno = 0;
            char *end = NULL;
            long v = ::strtol(arg, &end, 10);
            if (errno != 0)
                return false;
            end     = skip_whitespace(end);
            if (*end != '\0')
                return false;

            *res    = v;
            return true;
        }

        bool parse_uint(const char *arg, unsigned long *res)
        {
            errno = 0;
            char *end = NULL;
            unsigned long v = ::strtoul(arg, &end, 10);
            if (errno != 0)
                return false;
            end     = skip_whitespace(end);
            if (*end != '\0')
                return false;

            *res    = v;
            return true;
        }

        bool parse_long(const char *arg, long long *res)
        {
            errno = 0;
            char *end = NULL;
            #ifdef PLATFORM_BSD
                long long v = ::strtol(arg, &end, 10);
            #else
                long long v = ::strtoll(arg, &end, 10);
            #endif

            if (errno != 0)
                return false;
            end     = skip_whitespace(end);
            if (*end != '\0')
                return false;

            *res    = v;
            return true;
        }

        bool parse_ulong(const char *arg, unsigned long long *res)
        {
            errno = 0;
            char *end = NULL;
            #ifdef PLATFORM_BSD
                unsigned long long v = ::strtoul(arg, &end, 10);
            #else
                unsigned long long v = ::strtoull(arg, &end, 10);
            #endif

            if (errno != 0)
                return false;
            end     = skip_whitespace(end);
            if (*end != '\0')
                return false;

            *res    = v;
            return true;
        }

        status_t parse_file_formats(lltl::parray<file_format_t> *fmt, const char *variable)
        {
            lltl::parray<file_format_t> tmp;

            while (true)
            {
                // Seek for first non-blank character
                while (is_blank(*variable))
                    ++variable;
                if (*variable == '\0')
                    break;

                // Search for ',' separator
                const char *s = strchr(variable, ',');
                const char *end = (s == NULL) ? strchr(variable, '\0') : s;

                // Trim blank characters at the end
                while ((end > variable) && (is_blank(end[-1])))
                    --end;

                // Add new item if substring is not empty
                size_t n = end - variable;
                if (n > 0)
                {
                    // Lookup for matching format enumeration constant and add to list
                    for (const file_format_t *f = file_formats; f->id != NULL; ++f)
                    {
                        if (!strncasecmp(f->id, variable, n))
                        {
                            if (!tmp.add(const_cast<file_format_t *>(f)))
                                return STATUS_NO_MEM;

                            break;
                        }
                    }
                }

                if (s == NULL)
                    break;
                variable = s + 1;
            }

            // Apply new data
            fmt->swap(&tmp);

            return STATUS_OK;
        }
    }
}


