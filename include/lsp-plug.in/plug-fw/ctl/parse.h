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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_PARSE_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_PARSE_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/stdlib/stdlib.h>

#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

// Data parsing
#define PARSE_INT(var, code) \
    do { \
        long __; \
        if (::lsp::ctl::parse_int(var, &__)) \
            { code; } \
    } while (false)

#define PARSE_UINT(var, code) \
    do { \
        unsigned long __; \
        if (::lsp::ctl::parse_uint(var, &__)) \
            { code; } \
    } while (false)

#define PARSE_LONG(var, code) \
    do { \
        long long __; \
        if (::lsp::ctl::parse_long(var, &__)) \
            { code; } \
    } while (false)

#define PARSE_ULONG(var, code) \
    do { \
        unsigned long long __; \
        if (::lsp::ctl::parse_ulong(var, &__)) \
            { code; } \
    } while (false)

#define PARSE_BOOL(var, code) \
    do { \
        bool __; \
        if (::lsp::ctl::parse_bool(var, &__)) \
            { code; } \
    } while (false)

#define PARSE_FLOAT(var, code) \
    do { \
        float __; \
        if (::lsp::ctl::parse_float(var, &__)) \
            { code; } \
    } while (false)

#define PARSE_DOUBLE(var, code) \
    do { \
        double __; \
        if (::lsp::ctl::parse_double(var, &__)) \
            { code; } \
    } while (false)

#define PARSE_FLAG(var, dst, flag) PARSE_BOOL(var, if (__) dst |= (flag); else dst &= ~(flag))

#define BIND_PORT(ctl, field, id) \
    do { \
        field   = ctl->port(id); \
        if (field != NULL) \
            field->bind(this); \
    } while (false)

#define BIND_EXPR(field, expr) \
    (field).parse(expr);

namespace lsp
{
    namespace ctl
    {
        bool parse_float(const char *arg, float *res);

        bool parse_double(const char *arg, double *res);

        bool parse_bool(const char *arg, bool *res);

        bool parse_int(const char *arg, long *res);

        bool parse_uint(const char *arg, unsigned long *res);

        bool parse_long(const char *arg, long long *res);

        bool parse_ulong(const char *arg, unsigned long long *res);
    }
}



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_PARSE_H_ */
