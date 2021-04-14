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
#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/plug-fw/ctl.h>

#include <private/ui/xml/Handler.h>
#include <private/ui/xml/RootNode.h>

namespace lsp
{
    // Metadata for the wrapper
    namespace meta
    {
        static const meta::port_t config_metadata[] =
        {
            SWITCH(UI_MOUNT_STUD_PORT_ID, "Visibility of mount studs in the UI", 1.0f),
            PATH(UI_LAST_VERSION_PORT_ID, "Last version of the product installed"),
            PATH(UI_DLG_SAMPLE_PATH_ID, "Dialog path for selecting sample files"),
            PATH(UI_DLG_IR_PATH_ID, "Dialog path for selecting impulse response files"),
            PATH(UI_DLG_CONFIG_PATH_ID, "Dialog path for saving/loading configuration files"),
            PATH(UI_DLG_REW_PATH_ID, "Dialog path for importing REW settings files"),
            PATH(UI_DLG_HYDROGEN_PATH_ID, "Dialog path for importing Hydrogen drumkit files"),
            PATH(UI_DLG_MODEL3D_PATH_ID, "Dialog for saving/loading 3D model files"),
            PATH(UI_DLG_DEFAULT_PATH_ID, "Dialog default path for other files"),
            PATH(UI_R3D_BACKEND_PORT_ID, "Identifier of selected backend for 3D rendering"),
            PATH(UI_LANGUAGE_PORT_ID, "Selected language identifier for the UI interface"),
            SWITCH(UI_REL_PATHS_PORT_ID, "Use relative paths when exporting configuration file", 0.0f),
            PORTS_END
        };

        static const port_t time_metadata[] =
        {
            UNLIMITED_METER(TIME_SAMPLE_RATE_PORT, "Sample rate", U_HZ, DEFAULT_SAMPLE_RATE),
            UNLIMITED_METER(TIME_SPEED_PORT, "Playback speed", U_NONE, 0.0f),
            UNLIMITED_METER(TIME_FRAME_PORT, "Current frame", U_NONE, 0.0f),
            UNLIMITED_METER(TIME_NUMERATOR_PORT, "Numerator", U_NONE, 4.0f),
            UNLIMITED_METER(TIME_DENOMINATOR_PORT, "Denominator", U_NONE, 4.0f),
            UNLIMITED_METER(TIME_BEATS_PER_MINUTE_PORT, "Beats per Minute", U_BPM, BPM_DEFAULT),
            UNLIMITED_METER(TIME_TICK_PORT, "Current tick", U_NONE, 0.0f),
            UNLIMITED_METER(TIME_TICKS_PER_BEAT_PORT, "Ticks per Bar", U_NONE, 960.0f),

            PORTS_END
        };
    }

    namespace ui
    {
        IWrapper::IWrapper(Module *ui, resource::ILoader *loader)
        {
            pUI         = ui;
            pLoader     = loader;
            nFlags      = 0;
        }

        IWrapper::~IWrapper()
        {
            pUI         = NULL;
            pLoader     = NULL;
            nFlags      = 0;
        }

        void IWrapper::destroy()
        {
            // Destroy controllers in reverse order
            for (size_t i=vControllers.size(); (i--) > 0; )
            {
                ctl::Widget *w = vControllers.uget(i);
                if (w != NULL)
                {
                    w->destroy();
                    delete w;
                }
            }
            vControllers.flush();

            // Clear all aliases
            lltl::parray<LSPString> aliases;
            vAliases.values(&aliases);
            vAliases.flush();

            for (size_t i=0, n=aliases.size(); i<n; ++i)
            {
                LSPString *alias = aliases.uget(i);
                if (alias != NULL)
                    delete alias;
            }
            aliases.flush();

            // Clear sorted ports
            vSortedPorts.flush();

            // Destroy switched ports
            for (size_t i=0, n=vSwitchedPorts.size(); i<n; ++i)
            {
                IPort *p = vSwitchedPorts.uget(i);
                if (p != NULL)
                {
                    lsp_trace("Destroy switched port id=%s", p->id());
                    delete p;
                }
            }
            vSwitchedPorts.flush();

            // Destroy config ports
            for (size_t i=0, n=vConfigPorts.size(); i<n; ++i)
            {
                IPort *p = vConfigPorts.uget(i);
                if (p != NULL)
                {
                    lsp_trace("Destroy configuration port id=%s", p->metadata()->id);
                    delete p;
                }
            }
            vConfigPorts.flush();

            // Destroy time ports
            for (size_t i=0, n=vTimePorts.size(); i<n; ++i)
            {
                IPort *p = vTimePorts.uget(i);
                if (p != NULL)
                {
                    lsp_trace("Destroy timing port id=%s", p->metadata()->id);
                    delete p;
                }
            }
            vTimePorts.flush();

            // Destroy custom ports
            for (size_t i=0, n=vCustomPorts.size(); i<n; ++i)
            {
                IPort *p = vCustomPorts.uget(i);
                if (p != NULL)
                {
                    lsp_trace("Destroy timing port id=%s", p->metadata()->id);
                    delete p;
                }
            }
            vCustomPorts.flush();

            // Destroy ports
            for (size_t i=0, n=vPorts.size(); i<n; ++i)
            {
                IPort *p = vPorts.uget(i);
                lsp_trace("destroy UI port id=%s", p->metadata()->id);
                delete p;
            }
            vPorts.flush();
        }

        status_t IWrapper::init()
        {
            // Create additional ports (ui)
            for (const meta::port_t *p = meta::config_metadata; p->id != NULL; ++p)
            {
                switch (p->role)
                {
                    case meta::R_CONTROL:
                    {
                        IPort *up = new ControlPort(p, this);
                        if (up != NULL)
                            vConfigPorts.add(up);
                        break;
                    }

                    case meta::R_PATH:
                    {
                        IPort *up = new PathPort(p, this);
                        if (up != NULL)
                            vConfigPorts.add(up);
                        break;
                    }

                    default:
                        lsp_error("Could not instantiate configuration port id=%s", p->id);
                        break;
                }
            }

            // Create additional ports (time)
            for (const meta::port_t *p = meta::time_metadata; p->id != NULL; ++p)
            {
                switch (p->role)
                {
                    case meta::R_METER:
                    {
                        ValuePort *vp = new ValuePort(p);
                        if (vp != NULL)
                            vTimePorts.add(vp);
                        break;
                    }
                    default:
                        lsp_error("Could not instantiate time port id=%s", p->id);
                        break;
                }
            }

            return STATUS_OK;
        }

        void IWrapper::ui_activated()
        {
        }

        void IWrapper::ui_deactivated()
        {
        }

        core::KVTStorage *IWrapper::kvt_lock()
        {
            return NULL;
        }

        core::KVTStorage *IWrapper::kvt_trylock()
        {
            return NULL;
        }

        bool IWrapper::kvt_release()
        {
            return false;
        }

        void IWrapper::dump_state_request()
        {
        }

        IPort *IWrapper::port(const char *id)
        {
            // Check for alias
            lltl::phashset<LSPString> path;
            LSPString key, *name;
            if (!key.set_utf8(id))
                return NULL;

            while ((name = vAliases.get(&key)) != NULL)
            {
                if (path.contains(name))
                {
                    lsp_warn("Loop while walking through aliases: initial port id=%s", id);
                    return NULL;
                }
                // Update key to current name
                if (!key.set(name))
                    return NULL;
            }
            if ((id = key.get_utf8()) == NULL)
                return NULL;

            // Check that port name contains index
            if (strchr(id, '[') != NULL)
            {
                // Try to find switched port
                size_t count = vSwitchedPorts.size();
                for (size_t i=0; i<count; ++i)
                {
                    SwitchedPort *p  = vSwitchedPorts.uget(i);
                    if (p == NULL)
                        continue;
                    const char *p_id    = p->id();
                    if (p_id == NULL)
                        continue;
                    if (!strcmp(id, p_id))
                        return p;
                }

                // Create new switched port (lazy initialization)
                SwitchedPort *s     = new SwitchedPort(this);
                if (s == NULL)
                    return NULL;

                if (s->compile(id))
                {
                    if (vSwitchedPorts.add(s))
                        return s;
                }

                delete s;
                return NULL;
            }

            // Check that port name contains "ui:" prefix
            if (strstr(id, UI_CONFIG_PORT_PREFIX) == id)
            {
                const char *ui_id = &id[strlen(UI_CONFIG_PORT_PREFIX)];

                // Try to find configuration port
                size_t count = vConfigPorts.size();
                for (size_t i=0; i<count; ++i)
                {
                    IPort *p            = vConfigPorts.uget(i);
                    if (p == NULL)
                        continue;
                    const char *p_id    = p->metadata()->id;
                    if (p_id == NULL)
                        continue;
                    if (!strcmp(p_id, ui_id))
                        return p;
                }
            }

            // Check that port name contains "time:" prefix
            if (strstr(id, TIME_PORT_PREFIX) == id)
            {
                const char *ui_id = &id[strlen(TIME_PORT_PREFIX)];

                // Try to find configuration port
                size_t count = vTimePorts.size();
                for (size_t i=0; i<count; ++i)
                {
                    IPort *p            = vTimePorts.uget(i);
                    if (p == NULL)
                        continue;
                    const char *p_id    = p->metadata()->id;
                    if (p_id == NULL)
                        continue;
                    if (!strcmp(p_id, ui_id))
                        return p;
                }
            }

            // Look to custom ports
            for (size_t i=0, n=vCustomPorts.size(); i<n; ++i)
            {
                IPort *p = vCustomPorts.get(i);
                const meta::port_t *ctl = (p != NULL) ? p->metadata() : NULL;
                if ((ctl != NULL) && (!strcmp(id, ctl->id)))
                    return p;
            }

            // Do usual stuff
            size_t count = vPorts.size();
            if (vSortedPorts.size() != count)
                count = rebuild_sorted_ports();

            // Try to find the corresponding port
            ssize_t first = 0, last = count - 1;
            while (first <= last)
            {
                size_t center           = (first + last) >> 1;
                IPort *p                = vSortedPorts.uget(center);
                if (p == NULL)
                    break;
                const meta::port_t *ctl = p->metadata();
                if (ctl == NULL)
                    break;

                int cmp     = strcmp(id, ctl->id);
                if (cmp < 0)
                    last    = center - 1;
                else if (cmp > 0)
                    first   = center + 1;
                else
                    return p;
            }

            return NULL;
        }

        status_t IWrapper::set_port_alias(const char *alias, const char *id)
        {
            if ((alias == NULL) || (id == NULL))
                return STATUS_BAD_ARGUMENTS;

            LSPString ta, ti;
            if (!ta.set_utf8(alias))
                return STATUS_NO_MEM;
            if (!ti.set_utf8(id))
                return STATUS_NO_MEM;

            return create_alias(&ta, &ti);
        }

        status_t IWrapper::set_port_alias(const LSPString *alias, const char *id)
        {
            if ((alias == NULL) || (id == NULL))
                return STATUS_BAD_ARGUMENTS;

            LSPString ti;
            if (!ti.set_utf8(id))
                return STATUS_NO_MEM;

            return create_alias(alias, &ti);
        }

        status_t IWrapper::set_port_alias(const char *alias, const LSPString *id)
        {
            if ((alias == NULL) || (id == NULL))
                return STATUS_BAD_ARGUMENTS;

            LSPString ta;
            if (!ta.set_utf8(alias))
                return STATUS_NO_MEM;

            return create_alias(&ta, id);
        }

        status_t IWrapper::set_port_alias(const LSPString *alias, const LSPString *id)
        {
            if ((alias == NULL) || (id == NULL))
                return STATUS_BAD_ARGUMENTS;

            return create_alias(alias, id);
        }

        status_t IWrapper::create_alias(const LSPString *id, const LSPString *name)
        {
            LSPString *cname = name->clone();
            if (cname == NULL)
                return STATUS_NO_MEM;

            if (!vAliases.create(id, cname))
                return STATUS_ALREADY_EXISTS;

            return STATUS_OK;
        }

        ssize_t IWrapper::compare_ports(const IPort *a, const IPort *b)
        {
            const meta::port_t *pa = a->metadata(), *pb = b->metadata();
            if (pa == NULL)
                return (pb == NULL) ? 0 : -1;
            else if (pb == NULL)
                return 1;

            return strcmp(pa->id, pb->id);
        }

        size_t IWrapper::rebuild_sorted_ports()
        {
            size_t count = vPorts.size();

            if (!vSortedPorts.set(&vPorts))
                return count;
            if (count <= 1)
                return count;

            // Sort by port's ID
            vSortedPorts.qsort(compare_ports);
            return count;
        }

        void IWrapper::global_config_changed(IPort *src)
        {
            nFlags     |= F_SAVE_CONFIG;
        }

        void IWrapper::quit_main_loop()
        {
            nFlags     |= F_QUIT;

            // Notify display to leave main loop
            tk::Display *dpy = (pUI != NULL) ? pUI->display() : NULL;
            if (dpy != NULL)
                dpy->quit_main();
        }

        IPort *IWrapper::port(const LSPString *id)
        {
            return port(id->get_ascii());
        }

        IPort *IWrapper::port(size_t idx)
        {
            return vPorts.get(idx);
        }

        size_t IWrapper::ports() const
        {
            return vPorts.size();
        }

        status_t IWrapper::build_ui(const char *path)
        {
            // Create context
            UIContext ctx(this);
            status_t res = ctx.init();
            if (res != STATUS_OK)
                return res;

            LSPString xpath;
            if (xpath.fmt_utf8("ui/%s", path) <= 0)
                return STATUS_NO_MEM;

            // Parse the XML document
            xml::RootNode root(&ctx);
            xml::Handler handler(pLoader);
            return handler.parse_resource(&xpath, &root);
        }
    }
} /* namespace lsp */


