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

#include <lsp-plug.in/plug-fw/ui.h>

namespace lsp
{
    namespace ui
    {
        IModule::IModule(const meta::plugin_t *meta, IWrapper *wrapper)
        {
            pMetadata       = meta;
            pWrapper        = wrapper;
        }

        IModule::~IModule()
        {
            destroy();
        }

        void IModule::destroy()
        {
            // Clear ports
            vSortedPorts.clear();
            vConfigPorts.clear();
            vPorts.clear();
            vCustomPorts.clear();
        }

        status_t IModule::init()
        {
            return STATUS_OK;
        }

        void IModule::position_updated(const plug::position_t *pos)
        {
        }

        status_t IModule::add_port(IPort *port)
        {
            if (!vPorts.add(port))
                return STATUS_NO_MEM;

            lsp_trace("added port id=%s", port->metadata()->id);
            return STATUS_OK;
        }

        status_t IModule::add_custom_port(IPort *port)
        {
            if (!vCustomPorts.add(port))
                return STATUS_NO_MEM;

            lsp_trace("added custom port id=%s", port->metadata()->id);
            return STATUS_OK;
        }

        IPort *IModule::port(const char *name)
        {
//            // Check aliases
//            size_t n_aliases = vAliases.size();
//
//            for (size_t i=0; i<n_aliases; ++i)
//            {
//                CtlPortAlias *pa = vAliases.at(i);
//                if ((pa->id() == NULL) || (pa->alias() == NULL))
//                    continue;
//
//                if (!strcmp(name, pa->id()))
//                {
//                    name    = pa->alias();
//                    break;
//                }
//            }
//
//            // Check that port name contains index
//            if (strchr(name, '[') != NULL)
//            {
//                // Try to find switched port
//                size_t count = vSwitched.size();
//                for (size_t i=0; i<count; ++i)
//                {
//                    CtlSwitchedPort *p  = vSwitched.at(i);
//                    if (p == NULL)
//                        continue;
//                    const char *p_id    = p->id();
//                    if (p_id == NULL)
//                        continue;
//                    if (!strcmp(p_id, name))
//                        return p;
//                }
//
//                // Create new switched port
//                CtlSwitchedPort *s   = new CtlSwitchedPort(this);
//                if (s == NULL)
//                    return NULL;
//
//                if (s->compile(name))
//                {
//                    if (vSwitched.add(s))
//                        return s;
//                }
//
//                delete s;
//                return NULL;
//            }
//
//            // Check that port name contains "ui:" prefix
//            if (strstr(name, UI_CONFIG_PORT_PREFIX) == name)
//            {
//                const char *ui_id = &name[strlen(UI_CONFIG_PORT_PREFIX)];
//
//                // Try to find configuration port
//                size_t count = vConfigPorts.size();
//                for (size_t i=0; i<count; ++i)
//                {
//                    CtlPort *p          = vConfigPorts.at(i);
//                    if (p == NULL)
//                        continue;
//                    const char *p_id    = p->metadata()->id;
//                    if (p_id == NULL)
//                        continue;
//                    if (!strcmp(p_id, ui_id))
//                        return p;
//                }
//            }
//
//            // Check that port name contains "time:" prefix
//            if (strstr(name, TIME_PORT_PREFIX) == name)
//            {
//                const char *ui_id = &name[strlen(TIME_PORT_PREFIX)];
//
//                // Try to find configuration port
//                size_t count = vTimePorts.size();
//                for (size_t i=0; i<count; ++i)
//                {
//                    CtlPort *p          = vTimePorts.at(i);
//                    if (p == NULL)
//                        continue;
//                    const char *p_id    = p->metadata()->id;
//                    if (p_id == NULL)
//                        continue;
//                    if (!strcmp(p_id, ui_id))
//                        return p;
//                }
//            }
//
//            // Look to custom ports
//            for (size_t i=0, n=vCustomPorts.size(); i<n; ++i)
//            {
//                CtlPort *p = vCustomPorts.get(i);
//                const port_t *ctl = (p != NULL) ? p->metadata() : NULL;
//                if ((ctl != NULL) && (!strcmp(ctl->id, name)))
//                    return p;
//            }
//
//            // Do usual stuff
//            size_t count = vPorts.size();
//            if (vSortedPorts.size() != count)
//                count = rebuild_sorted_ports();
//
//            // Try to find the corresponding port
//            ssize_t first = 0, last = count - 1;
//            while (first <= last)
//            {
//                size_t center       = (first + last) >> 1;
//                CtlPort *p          = vSortedPorts.at(center);
//                if (p == NULL)
//                    break;
//                const port_t *ctl   = p->metadata();
//                if (ctl == NULL)
//                    break;
//
//                int cmp     = strcmp(name, ctl->id);
//                if (cmp < 0)
//                    last    = center - 1;
//                else if (cmp > 0)
//                    first   = center + 1;
//                else
//                    return p;
//
//            }
            return NULL;
        }

        void IModule::sync_meta_ports()
        {
        }

    }
}


