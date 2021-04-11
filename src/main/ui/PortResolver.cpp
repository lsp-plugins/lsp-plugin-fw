/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 апр. 2021 г.
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

#include <lsp-plug.in/plug-fw/ui/PortResolver.h>

namespace lsp
{
    namespace ui
    {
        PortResolver::PortResolver()
        {
            pWrapper    = NULL;
        }

        PortResolver::PortResolver(ui::IWrapper *wrapper)
        {
            pWrapper    = wrapper;
        }

        PortResolver::~PortResolver()
        {
            pWrapper    = NULL;
        }

        void PortResolver::init(ui::IWrapper *wrapper)
        {
            pWrapper    = wrapper;
        }

        status_t PortResolver::resolve(expr::value_t *value, const char *name, size_t num_indexes, const ssize_t *indexes)
        {
            LSPString path;
            if (!path.set_utf8(name))
                return STATUS_NO_MEM;
            for (size_t i=0; i<num_indexes; ++i)
                if (!path.fmt_append_utf8("_%d", int(indexes[i])))
                    return STATUS_NO_MEM;

            ui::IPort *p    = (pWrapper != NULL) ? pWrapper->port(path.get_utf8()) : NULL;
            if (p == NULL)
                return STATUS_NOT_FOUND;

            value->type     = expr::VT_FLOAT;
            value->v_float  = p->value();

            return on_resolved(&path, p);
        }

        status_t PortResolver::resolve(expr::value_t *value, const LSPString *name, size_t num_indexes, const ssize_t *indexes)
        {
            LSPString path;
            if (num_indexes > 0)
            {
                if (!path.set(name))
                    return STATUS_NO_MEM;
                for (size_t i=0; i<num_indexes; ++i)
                    if (!path.fmt_append_utf8("_%d", int(indexes[i])))
                        return STATUS_NO_MEM;
                name = &path;
            }

            ui::IPort *p    = (pWrapper != NULL) ? pWrapper->port(name->get_utf8()) : NULL;
            if (p == NULL)
                return STATUS_NOT_FOUND;

            value->type     = expr::VT_FLOAT;
            value->v_float  = p->value();

            return on_resolved(name, p);
        }

        status_t PortResolver::on_resolved(const LSPString *name, ui::IPort *p)
        {
            return on_resolved(name->get_utf8(), p);
        }

        status_t PortResolver::on_resolved(const char *name, ui::IPort *p)
        {
            return STATUS_OK;
        }

    } /* namespace java */
} /* namespace lsp */


