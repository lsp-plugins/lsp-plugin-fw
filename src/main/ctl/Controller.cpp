/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 мая 2025 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/ctl/parse.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace ctl
    {
        const ctl_class_t Controller::metadata = { "Controller", NULL };

        Controller::Controller(ui::IWrapper *wrapper):
            ui::IPortListener()
        {
            pClass          = &metadata;
            pWrapper        = wrapper;
        }

        Controller::~Controller()
        {
        }

        status_t Controller::init()
        {
            return STATUS_OK;
        }

        void Controller::destroy()
        {
        }

        bool Controller::instance_of(const ctl_class_t *wclass) const
        {
            const ctl_class_t *wc = pClass;
            while (wc != NULL)
            {
                if (wc == wclass)
                    return true;
                wc = wc->parent;
            }

            return false;
        }

        bool Controller::link_port(ui::IPort **port, const char *id)
        {
            if (port == NULL)
                return false;

            ui::IPort *oldp = *port;
            ui::IPort *newp = pWrapper->port(id);
            if (oldp == newp)
                return true;

            if (oldp != NULL)
                oldp->unbind(this);
            if (newp != NULL)
                newp->bind(this);

            *port           = newp;

            return true;
        }

        bool Controller::bind_port(ui::IPort **port, const char *param, const char *name, const char *value)
        {
            if (strcmp(param, name))
                return false;

            return link_port(port, value);
        }

        bool Controller::set_value(bool *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;

            if (strcmp(param, name))
                return false;
            PARSE_BOOL(value, *v = __);
            return true;
        }

        bool Controller::set_value(ssize_t *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;

            if (strcmp(param, name))
                return false;
            PARSE_INT(value, *v = __);
            return true;
        }

        bool Controller::set_value(size_t *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;

            if (strcmp(param, name))
                return false;
            PARSE_UINT(value, *v = __);
            return true;
        }

        bool Controller::set_value(float *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;

            if (strcmp(param, name))
                return false;
            PARSE_FLOAT(value, *v = __);
            return true;
        }

        bool Controller::set_value(LSPString *v, const char *param, const char *name, const char *value)
        {
            if (v == NULL)
                return false;

            if (strcmp(param, name))
                return false;

            v->set_utf8(value);
            return true;
        }

        bool Controller::set_expr(ctl::Expression *expr, const char *param, const char *name, const char *value)
        {
            if (expr == NULL)
                return false;

            if (strcmp(name, param))
                return false;

            if (!expr->parse(value))
                lsp_warn("Failed to parse expression for attribute '%s': %s", name, value);
            return true;
        }

    } /* namespace ctl */
} /* namespace lsp */


