/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 10 мая 2021 г.
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
#include <lsp-plug.in/stdlib/stdio.h>

#define DIGITS_DFL              5

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(Indicator)
            status_t res;
            if (!name->equals_ascii("indicator"))
                return STATUS_NOT_FOUND;

            tk::Indicator *w = new tk::Indicator(context->display());
            if (w == NULL)
                return STATUS_NO_MEM;
            if ((res = context->widgets()->add(w)) != STATUS_OK)
            {
                delete w;
                return res;
            }

            if ((res = w->init()) != STATUS_OK)
                return res;

            ctl::Indicator *wc  = new ctl::Indicator(context->wrapper(), w);
            if (ctl == NULL)
                return STATUS_NO_MEM;

            *ctl = wc;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(Indicator)


        void Indicator::PropListener::notify(tk::atom_t property)
        {
            if (pIndicator == NULL)
                return;
            tk::Widget *w = pIndicator->wWidget;
            if (w == NULL)
                return;

            tk::atom_t atom = w->display()->atom_id("modern");
            if (property == atom)
            {
                pIndicator->parse_format();
                if (pIndicator->pPort != NULL)
                    pIndicator->notify(pIndicator->pPort, ui::PORT_NONE);
            }
        }

        //-----------------------------------------------------------------
        const ctl_class_t Indicator::metadata   = { "Indicator", &Widget::metadata };

        Indicator::Indicator(ui::IWrapper *wrapper, tk::Indicator *widget):
            Widget(wrapper, widget),
            sListener(this)
        {
            pClass          = &metadata;

            enFormat        = FT_UNKNOWN;
            nDigits         = 0;
            nFlags          = 0;

            pPort           = NULL;
        }

        Indicator::~Indicator()
        {
        }

        status_t Indicator::init()
        {
            LSP_STATUS_ASSERT(Widget::init());

            sFormat.set_ascii("f5.1!");

            tk::Indicator *ind = tk::widget_cast<tk::Indicator>(wWidget);
            if (ind != NULL)
            {
                sColor.init(pWrapper, ind->color());
                sTextColor.init(pWrapper, ind->text_color());
                sIPadding.init(pWrapper, ind->ipadding());

                parse_format();

                ind->style()->bind_bool("modern", &sListener);
            }

            return STATUS_OK;
        }

        void Indicator::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            tk::Indicator *ind = tk::widget_cast<tk::Indicator>(wWidget);
            if (ind != NULL)
            {
                bind_port(&pPort, "id", name, value);

                sColor.set("color", name, value);
                sTextColor.set("text.color", name, value);
                sTextColor.set("tcolor", name, value);
                sIPadding.set("ipadding", name, value);
                sIPadding.set("ipad", name, value);

                if (set_value(&sFormat, "format", name, value))
                    parse_format();
                if (set_param(ind->modern(), "modern", name, value))
                    parse_format();

                set_param(ind->spacing(), "spacing", name, value);
                set_param(ind->dark_text(), "text.dark", name, value);
                set_param(ind->dark_text(), "tdark", name, value);
                set_font(ind->font(), "font", name, value);
            }

            return Widget::set(ctx, name, value);
        }

        void Indicator::commit_value(float value)
        {
            tk::Indicator *ind = tk::widget_cast<tk::Indicator>(wWidget);
            if (ind == NULL)
                return;

            if (pPort != NULL)
            {
                const meta::port_t *meta = pPort->metadata();
                if (meta != NULL)
                {
                    if (meta->unit == meta::U_GAIN_AMP)
                        value = 20.0 * logf(value) / M_LN10;
                    else if (meta->unit == meta::U_GAIN_POW)
                        value = 10.0 * logf(value) / M_LN10;
                }
            }

            // Set the text
            LSPString text;
            if (ind->rows()->get() != 1)
                ind->rows()->set(1);
            if (ind->columns()->get() != ssize_t(nDigits))
                ind->columns()->set(nDigits);
            if (format(&text, value))
                ind->text()->set_raw(&text);
        }

        void Indicator::notify(ui::IPort *port, size_t flags)
        {
            Widget::notify(port, flags);
            if ((port == pPort) && (pPort != NULL))
                commit_value(pPort->value());
        }

        void Indicator::end(ui::UIContext *ctx)
        {
            Widget::end(ctx);
            if (pPort != NULL)
                commit_value(pPort->value());
        }

        bool Indicator::parse_long(char *p, char **ret, long *value)
        {
            *ret        = p;
            if (!isdigit(*p))
                return false;

            errno       = 0;
            long digits = strtol(p, ret, 10);
            if (errno != 0)
                return false;

            *value      = digits;
            return true;
        }

        bool Indicator::parse_format()
        {
            enFormat        = FT_UNKNOWN;
            nDigits         = 0;
            nFlags          = 0;
            vFormat.clear();

            const char *format = sFormat.get_ascii();

            tk::Indicator *ind = tk::widget_cast<tk::Indicator>(wWidget);
            bool modern     = (ind != NULL) ? ind->modern()->get() : false;

            // Get predicates
            char *p         = const_cast<char *>(format);
            while (true)
            {
                if (*p == '+')
                    nFlags      |= IF_PLUS;
                else if (*p == '-')
                {
                    nFlags      |= IF_SIGN;
                    nDigits     ++;
                }
                else if (*p == '0')
                    nFlags      |= IF_PAD_ZERO;
                else
                    break;
                p++;
            }

            char c      = *(p++);
            if (c == '\0')
                return false;

            // Single value ?
            if ((c == 'f') || (c == 'i'))
            {
                if ((*p) == 'x')
                {
                    nFlags     |= IF_NO_ZERO;
                    p++;
                }

                long digits = DIGITS_DFL;
                parse_long(p, &p, &digits);

                // Add item
                fmt_t *item     = vFormat.append();
                if (item == NULL)
                    return false;
                item->type      = c;
                item->digits    = digits;
                item->precision = 0;

                nDigits        += digits;
                enFormat        = (c == 'i') ? FT_INT : FT_FLOAT;

                if (*p == '.')
                {
                    nFlags     |= IF_DOT;
                    if (modern)
                        nDigits     ++;
                }
                else if (*p != ',')
                    return (*p == '\0');

                ++p;
                if (enFormat == FT_INT)
                    return (*p == '\0');

                if (parse_long(p, &p, &digits))
                    item->precision = (digits < 0) ? 0 : digits;

                if (*p == '!')
                {
                    nFlags     |= IF_FIXED_PREC;
                    ++p;
                }
                else if (*p == '+')
                {
                    nFlags     |= IF_TOLERANCE;
                    ++p;
                }

                return (*p == '\0');
            }

            // Time
            enFormat        = FT_TIME;
            if (nFlags & IF_PLUS)
                nDigits         ++;
            while (true)
            {
                switch (c)
                {
                    case ':': // Separators, they do not take digits
                    case '.':
                    {
                        fmt_t *item     = vFormat.append();
                        if (item == NULL)
                            return false;
                        item->type      = c;
                        item->digits    = 0;
                        item->precision = 0;
                        if (modern)
                            nDigits        += 1;
                        break;
                    }
                    case 'u': // Microseconds 000000-999999
                    {
                        long digits     = 6;
                        parse_long(p, &p, &digits);

                        fmt_t *item     = vFormat.append();
                        if (item == NULL)
                            return false;
                        item->type      = c;
                        item->digits    = digits;
                        item->precision = 0;
                        nDigits        += digits;
                        break;
                    }
                    case 'S': // Second 00-59
                    case 'M': // Minute 00-59
                    case 'H': // Hour 00-23
                    case 'h': // Hour 01-12
                    {
                        fmt_t *item     = vFormat.append();
                        if (item == NULL)
                            return false;
                        item->type      = c;
                        item->digits    = 2;
                        item->precision = 0;
                        nDigits        += 2;
                        break;
                    }
                    case 'D': // Day
                    {
                        long digits         = 1;
                        parse_long(p, &p, &digits);
                        if (digits <= 0)
                            digits      = 1;

                        fmt_t *item     = vFormat.append();
                        if (item == NULL)
                            return false;
                        item->type      = c;
                        item->digits    = digits;
                        item->precision = 0;
                        nDigits        += digits;
                        break;
                    }
                    default:
                        return false;
                }

                // Next character
                c   = *(p++);
                if (c == '\0')
                    return true;
            }

            return false;
        }

        bool Indicator::fmt_time(LSPString *buf, double value)
        {
            char tmp[64];

            // Do not format NAN
            if (value == NAN)
                return false;

            char c_sign         = (value < 0.0) ? '-' : (value > 0.0) ? '+' : ' ';
            char c_pad          = (nFlags & IF_PAD_ZERO) ? '0' : ' ';
            if (value < 0.0)
                value       = -value;

            bool overflow = false;

            if (nFlags & (IF_SIGN | IF_PLUS))
            {
                if ((!(nFlags & IF_PLUS)) && (c_sign == '+'))
                    c_sign = ' ';
                if (!buf->append(c_sign))
                    return false;
            }
            else if (c_sign == '-')
                overflow = true;

            // Find day marker and validate overflow
            size_t n_items = vFormat.size();
            for (size_t i=0; i < n_items; ++i)
            {
                fmt_t *item     = vFormat.uget(i);
                if (item->type != 'D')
                    continue;

                size_t field = size_t(value) / (60 * 60 * 24);
                size_t digits = 1;
                while (field >= 10)
                {
                    field /= 10;
                    digits++;
                }
                if (digits > item->digits)
                {
                    overflow = true;
                    break;
                }
            }

            for (size_t i=0; i < n_items; ++i)
            {
                fmt_t *item     = vFormat.uget(i);
                size_t field    = 0;
                size_t digits   = item->digits;

                if (!overflow)
                {
                    switch (item->type)
                    {
                        case 'u': // Microseconds 000000-999999
                        {
                            double intf;
                            double fract = modf(value, &intf);
                            for (size_t j=0; j<digits; ++j)
                                fract       *= 10.0;
                            field           = fract;
                            break;
                        }
                        case 'S': // Second 00-59
                            field           = size_t(value) % 60;
                            break;
                        case 'M': // Minute 00-59
                            field           = (size_t(value) / 60) % 60;
                            break;
                        case 'H': // Hour 00-23
                            field           = (size_t(value) / (60 * 60)) % 24;
                            break;
                        case 'h': // Hour 01-12
                            field           = (size_t(value) / (60 * 60)) % 12;
                            if (!field)
                                field           = 12;
                            break;
                        case 'D': // Day
                            field           = size_t(value) / (60 * 60 * 24);
                            break;
                        default:
                            if (!buf->append(item->type))
                                return false;
                            continue;
                    }

                    digits = snprintf(tmp, sizeof(tmp), "%ld", (long)(field));
                    ssize_t pad = item->digits - digits;
                    while ((pad--) > 0)
                    {
                        if (!buf->append(c_pad))
                            return false;
                    }
                    for (size_t i=0; i<digits; ++i)
                    {
                        if (!buf->append(tmp[i]))
                            return false;
                    }
                }
                else
                {
                    switch (item->type)
                    {
                        case 'u': case 'S': case 'M': case 'H':
                        case 'h': case 'D':
                            break;
                        default:
                            if (!buf->append(item->type))
                                return false;
                            continue;
                    }

                    for (size_t i=0; i<digits; ++i)
                    {
                        if (!buf->append(c_sign))
                            return false;
                    }
                }
            }

            return true;
        }

        bool Indicator::fmt_float(LSPString *buf, double value)
        {
            char tmp[64];
            fmt_t *descr    = vFormat.uget(0);

            // Do not format NAN
            if (isnan(value))
                return false;
            if (isinf(value))
            {
                char sign = (signbit(value)) ? '-' : '+';
                for (size_t i=0; i<nDigits; ++i)
                {
                    if (!buf->append(sign))
                        return false;
                }
                return true;
            }

            tk::Indicator *ind  = tk::widget_cast<tk::Indicator>(wWidget);
            bool modern         = (ind != NULL) ? ind->modern()->get() : false;
            ssize_t digits      = nDigits;
            if ((nFlags & IF_DOT) && (modern))
                --digits;

            // FLOAT FORMAT: {s1}{pad}{s2}{zero}{int_p}{dot}{frac_p}
            ssize_t s1 = 0, pad = 0, s2 = 0, z_p = 0, int_p = 0, frac_p = 0;

            // Remember sign and make value positive
            char c_sign = (value < 0.0) ? '-' : (value > 0.0) ? '+' : ' ';
            char c_pad  = (nFlags & IF_PAD_ZERO) ? '0' : ' ';
            if (value < 0.0)
                value   = -value;

            // Calculate s1 and s2
            if (nFlags & IF_SIGN)
                s1  ++;
            else if (c_sign == '-')
            {
                if (nFlags & IF_PAD_ZERO)
                    s1  ++;
                else
                    s2  ++;
            }
            else if (c_sign == '+')
            {
                if (nFlags & IF_PLUS)
                    s2  ++;
            }

            // Now calculate maximum available value
            if (s1 || s2)
                digits  --;

            // Calculate integer part
            double tmp_v         = value;
            while (truncf(tmp_v) > 0.0)
            {
                tmp_v       *= 0.1f;
                int_p       ++;
            }

            // Calculate zero digit
            if ((int_p <= 0) && (!(nFlags & IF_NO_ZERO)))
                z_p++;

            double max_value     =  1.0;
            for (ssize_t i=z_p; i<digits; ++i)
                max_value  *= 10.0;

            // Calculate fraction part
            if (!(nFlags & IF_FIXED_PREC))
            {
                // Maximize fraction part
                if (value == 0.0)
                {
                    int_p       = 0; // No integer part for 0
                    frac_p      = (nFlags & IF_TOLERANCE) ? (digits - z_p - int_p) : descr->precision;
                }
                else
                {
                    tmp_v       = value;

                    while (true)
                    {
                        tmp_v       *= 10.0;
                        if (truncf(tmp_v) >= max_value)
                            break;
                        if ((frac_p >= descr->precision) && (!(nFlags & IF_TOLERANCE)))
                            break;
                        frac_p++;
                    }
                }
            }
            else
                frac_p  = descr->precision;

            // Calculate padding
            pad = digits - z_p - int_p - frac_p;
            if (pad >= 0) // Okay
            {
                // Normalize and format
                for (ssize_t i=0; i < frac_p; ++i)
                    value  *= 10.0;
                ssize_t frac_len = snprintf(tmp, sizeof(tmp), "%ld", (unsigned long)(value));
                char *p = tmp;

                // Output value
                // FLOAT FORMAT: {s1}{pad}{s2}{int_p}{dot}{frac_p}

                // Extra sign
                if (s1 > 0)
                {
                    if (!buf->append(c_sign))
                        return false;
                }

                // Padding
                while ((pad--) > 0)
                {
                    if (!buf->append(c_pad))
                        return false;
                }

                // Sign
                if (s2 > 0)
                {
                    if (!buf->append(c_sign))
                        return false;
                }

                // Integer part (zero)
                if (z_p > 0)
                {
                    if (!buf->append('0'))
                        return false;
                }

                // Integer part (non-zero)
                while ((int_p--) > 0)
                {
                    char c  = (*p == '\0') ? '0' : *(p++);
                    if (!buf->append(c))
                        return false;
                }

                // Fraction part: check if need to place dot
                if ((frac_p > 0) || (nFlags & IF_DOT))
                {
                    if (!buf->append('.'))
                        return false;
                }

                while (frac_p > 0)
                {
                    char c  = (frac_p > frac_len) ? '0' :
                            (*p == '\0') ? '0' : *(p++);
                    if (!buf->append(c))
                        return false;
                    frac_p--;
                }
            }
            else // Overflow
            {
                // Extra sign
                if (s1 || s2)
                {
                    if (!buf->append(c_sign))
                        return false;
                }

                // Re-calculate attributes
                if (c_sign == ' ')
                    c_sign = '*';

                frac_p  = (descr->precision >= digits) ? digits - 1 : descr->precision;
                int_p   = digits - frac_p;

                // Integer part
                while ((int_p--) > 0)
                {
                    if (!buf->append(c_sign))
                        return false;
                }

                // Fraction part: check if need to place dot
                if ((frac_p > 0) || (nFlags & IF_DOT))
                {
                    if (!buf->append('.'))
                        return false;
                }

                while ((frac_p--) > 0)
                {
                    if (!buf->append(c_sign))
                        return false;
                }
            }

            return true;
        }

        bool Indicator::fmt_int(LSPString *buf, ssize_t value)
        {
            char tmp[64];

            size_t digits   = nDigits;

            if (value < 0)
            {
                value = -value;

                if (nFlags & (IF_SIGN | IF_PAD_ZERO))
                {
                    if (!buf->append('-'))
                        return false;
                    if ((--digits) <= 0)
                        return true;
                }

                // Calculate maximum value
                ssize_t max_value = 1;
                for (size_t i=0; i<digits; ++i)
                    max_value  *= 10;

                // Fill with error if overflow
                if (value >= max_value)
                {
                    for (size_t i=0; i<digits; ++i)
                    {
                        if (!buf->append('-'))
                            return false;
                    }
                    return true;
                }

                char pad = (nFlags & IF_PAD_ZERO) ? '0' : ' ';
                const char *format = "-%ld";
                if (nFlags & (IF_SIGN | IF_PAD_ZERO))
                    format          = "%ld";

                // Pad value
                int count       = snprintf(tmp, sizeof(tmp), format, long(value));
                int padding     = digits - count;
                while ((padding--) > 0)
                {
                    if (!buf->append(pad))
                        return false;
                }

                if (!buf->append_ascii(tmp, count))
                    return false;
            }
            else if (value > 0)
            {
                // Calculate maximum value
                ssize_t max_value = (nFlags & (IF_SIGN | IF_PLUS)) ? 1 : 10;
                for (size_t i=1; i<digits; ++i)
                    max_value  *= 10;

                // Fill with error if overflow
                if (value >= max_value)
                {
                    for (size_t i=0; i<digits; ++i)
                    {
                        if (!buf->append('+'))
                            return false;
                    }
                    return true;
                }

                char pad = (nFlags & IF_PAD_ZERO) ? '0' : ' ';
                const char *format = "%ld";

                // Sign has to be at the left
                if (nFlags & IF_SIGN)
                {
                    if (!buf->append((nFlags & IF_PLUS) ? '+' : pad))
                        return false;
                    if ((--digits) <= 0)
                        return true;
                }
                else if (nFlags & IF_PLUS)
                    format = "+%ld";

                // Pad value
                int count = snprintf(tmp, sizeof(tmp), format, long(value));
                int padding     = digits - count;
                while ((padding--) > 0)
                {
                    if (!buf->append(pad))
                        return false;
                }

                if (!buf->append_ascii(tmp, count))
                    return false;
            }
            else
            {
                if ((digits > 1) && (nFlags & IF_SIGN))
                {
                    if (!buf->append(' '))
                        return false;
                    digits      --;
                }
                while (digits > 1)
                {
                    if (!buf->append((nFlags & IF_PAD_ZERO) ? '0' : ' '))
                        return false;
                    digits--;
                }

                if (!buf->append('0'))
                    return false;
            }

            return true;
        }

        bool Indicator::format(LSPString *buf, double value)
        {
            bool result = false;

            if (!vFormat.is_empty())
            {
                switch (enFormat)
                {
                    case FT_INT:
                        result = fmt_int(buf, ssize_t(value));
                        break;
                    case FT_TIME:
                        result = fmt_time(buf, value);
                        break;
                    case FT_FLOAT:
                        result = fmt_float(buf, value);
                        break;
                    default:
                        break;
                }
            }

            if (!result)
            {
                buf->clear();
                for (size_t i=0; i<nDigits; ++i)
                {
                    if (!buf->append('*'))
                        return false;
                }
            }

            return true;
        }
    } /* namespace ctl */
} /* namespace lsp */


