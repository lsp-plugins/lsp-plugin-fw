/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 22 апр. 2021 г.
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

#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/core/config.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/fmt/config/types.h>
#include <lsp-plug.in/dsp-units/units.h>

namespace lsp
{
    namespace core
    {
        bool format_relative_path(LSPString *value, const char *path, const io::Path *base)
        {
            if (base == NULL)
                return false;

            io::Path xpath;
            if (xpath.set(path) != STATUS_OK)
                return false;
            if (xpath.as_relative(base) != STATUS_OK)
                return false;

            return value->append(xpath.as_string());
        }

        bool parse_relative_path(io::Path *path, const io::Path *base, const char *value, size_t len)
        {
            if ((base == NULL) || (len <= 0))
                return false;

            LSPString svalue;
            if (!svalue.set_utf8(value, len))
                return false;

            // Do nothing with built-in path
            if (svalue.starts_with_ascii(LSP_BUILTIN_PREFIX))
                return path->set(&svalue) == STATUS_OK;

            // This method won't accept absolute path stored in svalue
            if (path->set(base, &svalue) != STATUS_OK)
                return false;

            return (path->canonicalize() == STATUS_OK);
        }

        status_t serialize_port_value(config::Serializer *s,
            const meta::port_t *meta, const void *data, const io::Path *base, size_t flags
        )
        {
            LSPString comment, value;

            switch (meta->role)
            {
                case meta::R_PORT_SET:
                case meta::R_CONTROL:
                case meta::R_BYPASS:
                {
                    // Serialize meta information
                    const char *unit = meta::get_unit_name(meta->unit);
                    if (unit != NULL)
                        LSP_BOOL_ASSERT(comment.fmt_append_utf8("%s [%s]", meta->name, unit), STATUS_NO_MEM);
                    else if (meta->unit == meta::U_BOOL)
                        LSP_BOOL_ASSERT(comment.fmt_append_utf8("%s [boolean]", meta->name), STATUS_NO_MEM);
                    else
                        LSP_BOOL_ASSERT(comment.append_utf8(meta->name), STATUS_NO_MEM);

                    if ((meta->flags & (meta::F_LOWER | meta::F_UPPER)) ||
                        (meta->unit == meta::U_ENUM) ||
                        (meta->unit == meta::U_BOOL))
                    {
                        if (meta::is_discrete_unit(meta->unit) || (meta->flags & meta::F_INT))
                        {
                            if (meta->unit != meta::U_BOOL)
                            {
                                if (meta->unit == meta::U_ENUM)
                                {
                                    int value       = meta->min + list_size(meta->items) - 1;
                                    LSP_BOOL_ASSERT(comment.fmt_append_utf8(": %d..%d", int(meta->min), int(value)), STATUS_NO_MEM);
                                }
                                else
                                    LSP_BOOL_ASSERT(comment.fmt_append_utf8(": %d..%d", int(meta->min), int(meta->max)), STATUS_NO_MEM);
                            }
                            else
                                LSP_BOOL_ASSERT(comment.append_utf8(": true/false"), STATUS_NO_MEM);
                        }
                        else if (!(meta->flags & meta::F_EXT))
                        {
                            LSP_BOOL_ASSERT(comment.fmt_append_utf8(": %.8f..%.8f", meta->min, meta->max), STATUS_NO_MEM);
                        }
                        else
                        {
                            LSP_BOOL_ASSERT(comment.fmt_append_utf8(": %.12f..%.12f", meta->min, meta->max), STATUS_NO_MEM);
                        }
                    }

                    // Enumerate all possible values for enum
                    if ((meta->unit == meta::U_ENUM) && (meta->items != NULL))
                    {
                        int value   = meta->min;
                        for (const meta::port_item_t *item = meta->items; item->text != NULL; ++item)
                            LSP_BOOL_ASSERT(comment.fmt_append_utf8("\n  %d: %s", value++, item->text), STATUS_NO_MEM);
                    }

                    if (comment.length() > 0)
                    {
                        LSP_STATUS_ASSERT(s->write_comment(&comment));
                    }

                    // Serialize value
                    float v = *(static_cast<const float *>(data));
                    if (is_discrete_unit(meta->unit) || (meta->flags & meta::F_INT))
                    {
                        if (is_bool_unit(meta->unit))
                        {
                            LSP_STATUS_ASSERT(s->write_bool(meta->id, (v >= 0.5f), flags));
                        }
                        else
                        {
                            LSP_STATUS_ASSERT(s->write_i32(meta->id, int(v), flags));
                        }
                    }
                    else
                    {
                        if (meta->flags & meta::F_EXT)
                            flags  |= config::SF_PREC_LONG;
                        if (meta::is_decibel_unit(meta->unit))
                        {
                            flags  |= config::SF_DECIBELS;

                            if (meta->unit == meta::U_DB)
                            {
                                if (v < -250.0f)
                                    v   = -INFINITY;
                                else if (v > 250.0f)
                                    v   = +INFINITY;
                            }
                            else
                            {
                                if (fabsf(v) > 1e+40)
                                    v   = +INFINITY;
                                else if (fabsf(v) < 1e-40)
                                    v   = -INFINITY;
                                else
                                    v   = (meta->unit == meta::U_GAIN_AMP) ? dspu::gain_to_db(v) : dspu::power_to_db(v);
                            }
                        }
                        LSP_STATUS_ASSERT(s->write_f32(meta->id, v, flags));
                    }

                    // No flags
                    break;
                }
                case meta::R_PATH:
                {
                    // Write comment
                    LSP_BOOL_ASSERT(comment.fmt_append_utf8("%s [pathname]", meta->name), STATUS_NO_MEM);
                    if (comment.length() > 0)
                    {
                        LSP_STATUS_ASSERT(s->write_comment(&comment));
                    }

                    // Write path
                    flags |= config::SF_QUOTED;
                    const char *path    = static_cast<const char *>(data);
                    if ((path != NULL) && (strlen(path) > 0))
                    {
                        if (format_relative_path(&value, path, base))
                            path    = value.get_utf8();
                    }
                    LSP_STATUS_ASSERT(s->write_string(meta->id, path, flags));

                    break;
                }
                default:
                    return STATUS_BAD_TYPE;
            }
            return STATUS_OK;
        }

    } /* namespace core */
} /* namespace lsp */


