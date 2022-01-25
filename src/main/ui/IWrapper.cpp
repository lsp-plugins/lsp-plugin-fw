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
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/core/config.h>
#include <lsp-plug.in/io/InFileStream.h>
#include <lsp-plug.in/io/InSequence.h>
#include <lsp-plug.in/io/OutFileStream.h>
#include <lsp-plug.in/io/OutSequence.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/fmt/config/Serializer.h>
#include <lsp-plug.in/runtime/system.h>

#include <private/ui/xml/Handler.h>
#include <private/ui/xml/RootNode.h>
#include <private/ui/BuiltinStyle.h>

static const char *config_separator = "-------------------------------------------------------------------------------";

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
            KNOB(UI_SCALING_PORT_ID, "Manual UI scaling factor", U_PERCENT, 25.0f, 400.0f, 100.0f, 1.0f),
            SWITCH(UI_SCALING_HOST_ID, "Prefer host-reported UI scale factor", 1.0f),
            KNOB(UI_FONT_SCALING_PORT_ID, "Manual UI font scaling factor", U_PERCENT, 50.0f, 200.0f, 100.0f, 1.0f),
            PATH(UI_VISUAL_SCHEMA_FILE_ID, "Current visual schema file used by the UI"),
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
            pDisplay    = NULL;
            wWindow     = NULL;
            pWindow     = NULL;
            pUI         = ui;
            pLoader     = loader;
            nFlags      = 0;

            plug::position_t::init(&sPosition);
        }

        IWrapper::~IWrapper()
        {
            pDisplay    = NULL;
            pUI         = NULL;
            pLoader     = NULL;
            nFlags      = 0;
        }

        void IWrapper::destroy()
        {
            // Flush list of 'Schema reloaded' handlers
            vSchemaListeners.flush();

            // Destroy window controller if present
            if (pWindow != NULL)
            {
                pWindow->destroy();
                delete pWindow;
                pWindow = NULL;
            }

            // Destroy the window widget if present
            if (wWindow != NULL)
            {
                wWindow->destroy();
                delete wWindow;
                wWindow = NULL;
            }

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
                    lsp_trace("Destroy custom port id=%s", p->metadata()->id);
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
            status_t res;

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

            // Load the global configuration file
            io::Path gconfig;
            if ((res = system::get_user_config_path(&gconfig)) == STATUS_OK)
            {
                lsp_trace("User config path: %s", gconfig.as_utf8());
                res = gconfig.append_child("lsp-plugins");
                if (res == STATUS_OK)
                    res = gconfig.append_child("lsp-plugins.cfg");
                if (res == STATUS_OK)
                    res = load_global_config(&gconfig);
            }
            else
                lsp_warn("Failed to obtain plugin configuration: error=%d", int(res));

            return STATUS_OK;
        }

        void IWrapper::notify_all()
        {
            for (size_t i=0, n=vPorts.size(); i<n; ++i)
            {
                ui::IPort *port = vPorts.uget(i);
                if (port != NULL)
                    port->notify_all();
            }
        }

        core::KVTStorage *IWrapper::kvt_lock()
        {
            return NULL;
        }

        core::KVTStorage *IWrapper::kvt_trylock()
        {
            return NULL;
        }

        void IWrapper::kvt_notify_write(core::KVTStorage *storage, const char *id, const core::kvt_param_t *value)
        {
            for (size_t i=0, n=vKvtListeners.size(); i<n; ++i)
            {
                IKVTListener *l = vKvtListeners.uget(i);
                if (l != NULL)
                    l->changed(storage, id, value);
            }
        }

        status_t IWrapper::kvt_subscribe(ui::IKVTListener *listener)
        {
            if (listener == NULL)
                return STATUS_BAD_ARGUMENTS;
            if (vKvtListeners.index_of(listener) >= 0)
                return STATUS_ALREADY_BOUND;

            return (vKvtListeners.add(listener)) ? STATUS_OK : STATUS_NO_MEM;
        }

        status_t IWrapper::kvt_unsubscribe(ui::IKVTListener *listener)
        {
            if (listener == NULL)
                return STATUS_BAD_ARGUMENTS;
            return (vKvtListeners.premove(listener)) ? STATUS_OK : STATUS_NOT_FOUND;
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

            // Check that port name contains "_ui_" prefix
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

            // Check that port name contains "_time_" prefix
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
            if ((nFlags & (F_CONFIG_LOCK | F_CONFIG_DIRTY)) == 0)
                nFlags     |= F_CONFIG_DIRTY;
            else
            {
                if (nFlags & F_CONFIG_LOCK)
                    lsp_trace("CONFIG IS LOCKED");
            }
            if (nFlags & F_CONFIG_DIRTY)
                lsp_trace("CONFIG IS MARKED AS DIRTY");
        }

        void IWrapper::main_iteration()
        {
            // Synchronize meta ports
            for (size_t i=0, count=vTimePorts.size(); i < count; ++i)
            {
                ValuePort *vp = vTimePorts.uget(i);
                if (vp != NULL)
                    vp->sync();
            }

            // Call main iteration for the underlying display
            if (pDisplay != NULL)
                pDisplay->main_iteration();

            if ((nFlags & (F_CONFIG_LOCK | F_CONFIG_DIRTY)) == F_CONFIG_DIRTY)
            {
                // Save global configuration
                io::Path path;
                status_t res = system::get_user_config_path(&path);
                if (res == STATUS_OK)
                    res = path.append_child("lsp-plugins");
                if (res == STATUS_OK)
                    res = path.mkdir(true);
                if (res == STATUS_OK)
                    res = path.append_child("lsp-plugins.cfg");
                if (res == STATUS_OK)
                    res = save_global_config(&path);

                lsp_trace("Save global configuration to %s: result=%d", path.as_native(), int(res));

                // Reset flags
                nFlags     &= ~F_CONFIG_DIRTY;
            }
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

        status_t IWrapper::bind_custom_port(ui::IPort *port)
        {
            if (!vCustomPorts.add(port))
                return STATUS_NO_MEM;

            lsp_trace("added custom port id=%s", port->metadata()->id);
            return STATUS_OK;
        }

        status_t IWrapper::build_ui(const char *path, void *handle, ssize_t screen)
        {
            status_t res;

            // Create window widget
            wWindow     = new tk::Window(pDisplay, handle, screen);
            if (wWindow == NULL)
                return STATUS_NO_MEM;
            if ((res = wWindow->init()) != STATUS_OK)
                return res;

            // Create window controller
            pWindow     = new ctl::PluginWindow(this, wWindow);
            if (pWindow == NULL)
                return STATUS_NO_MEM;
            if ((res = pWindow->init()) != STATUS_OK)
                return res;

            // Form the location of the resource
            LSPString xpath;
            if (xpath.fmt_utf8(LSP_BUILTIN_PREFIX "ui/%s", path) <= 0)
                return STATUS_NO_MEM;

            // Create context
            UIContext ctx(this, pWindow->controllers(), pWindow->widgets());
            if ((res = ctx.init()) != STATUS_OK)
                return res;

            // Parse the XML document
            xml::RootNode root(&ctx, "plugin", pWindow);
            xml::Handler handler(resources());
            return handler.parse_resource(&xpath, &root);
        }

        status_t IWrapper::export_settings(const char *file, bool relative)
        {
            io::Path path;
            status_t res = path.set(file);
            if (res != STATUS_OK)
                return res;

            return export_settings(&path, relative);
        }

        status_t IWrapper::export_settings(const LSPString *file, bool relative)
        {
            io::Path path;
            status_t res = path.set(file);
            if (res != STATUS_OK)
                return res;

            return export_settings(&path, relative);
        }

        status_t IWrapper::export_settings(const io::Path *file, bool relative)
        {
            io::OutFileStream os;
            io::OutSequence o;

            status_t res = os.open(file, io::File::FM_WRITE_NEW);
            if (res != STATUS_OK)
                return res;

            // Wrap
            if ((res = o.wrap(&os, WRAP_CLOSE, "UTF-8")) != STATUS_OK)
            {
                os.close();
                return res;
            }

            // Export settings
            res = export_settings(&o, (relative) ? file : NULL);
            status_t res2 = o.close();

            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::export_settings(io::IOutSequence *os, const char *relative)
        {
            if (relative == NULL)
                return export_settings(os, static_cast<io::Path *>(NULL));

            io::Path path;
            status_t res = path.set(relative);
            if (res != STATUS_OK)
                return res;

            return export_settings(os, &path);
        }

        status_t IWrapper::export_settings(io::IOutSequence *os, const LSPString *relative)
        {
            if (relative == NULL)
                return export_settings(os, static_cast<io::Path *>(NULL));

            io::Path path;
            status_t res = path.set(relative);
            if (res != STATUS_OK)
                return res;

            return export_settings(os, &path);
        }

        void IWrapper::build_config_header(LSPString *c)
        {
            const meta::package_t *pkg = package();
            const meta::plugin_t *meta = pUI->metadata();

            LSPString pkv;
            pkv.fmt_ascii("%d.%d.%d",
                    int(pkg->version.major),
                    int(pkg->version.minor),
                    int(pkg->version.micro)
            );
            if (pkg->version.branch)
                pkv.fmt_append_ascii("-%s", pkg->version.branch);

            c->append_ascii     (config_separator);
            c->append           ('\n');
            c->append_utf8      ("This file contains configuration of the audio plugin.\n");
            c->fmt_append_utf8  ("  Package:             %s (%s)\n", pkg->artifact);
            c->fmt_append_utf8  ("  Package version:     %s\n", pkv.get_utf8());
            c->fmt_append_utf8  ("  Plugin name:         %s (%s)\n", meta->name, meta->description);
            c->fmt_append_utf8  ("  Plugin version:      %d.%d.%d\n",
                    int(LSP_MODULE_VERSION_MAJOR(meta->version)),
                    int(LSP_MODULE_VERSION_MINOR(meta->version)),
                    int(LSP_MODULE_VERSION_MICRO(meta->version))
                );
            if (meta->uid != NULL)
                c->fmt_append_utf8   ("  UID:                 %s\n", meta->uid);
            if (meta->lv2_uri != NULL)
                c->fmt_append_utf8   ("  LV2 URI:             %s\n", meta->lv2_uri);
            if (meta->vst2_uid != NULL)
                c->fmt_append_utf8   ("  VST identifier:      %s\n", meta->vst2_uid);
            if (meta->ladspa_id > 0)
                c->fmt_append_utf8   ("  LADSPA identifier:   %d\n", meta->ladspa_id);
            if (meta->ladspa_lbl > 0)
                c->fmt_append_utf8   ("  LADSPA label:        %s\n", meta->ladspa_lbl);
            c->append           ('\n');
            c->fmt_append_utf8  ("(C) %s\n", pkg->full_name);
            c->fmt_append_utf8  ("  %s\n", pkg->site);
            c->append           ('\n');
            c->append_ascii     (config_separator);
        }

        void IWrapper::build_global_config_header(LSPString *c)
        {
            const meta::package_t *pkg = package();

            c->append_ascii     (config_separator);
            c->append           ('\n');
            c->append           ('\n');
            c->append_utf8      ("This file contains global configuration of plugins.\n");
            c->append           ('\n');
            c->fmt_append_utf8  ("(C) %s\n", pkg->full_name);
            c->fmt_append_utf8  ("  %s\n", pkg->site);
            c->append           ('\n');
            c->append_ascii     (config_separator);
        }

        status_t IWrapper::export_ports(config::Serializer *s, lltl::parray<IPort> *ports, const io::Path *relative)
        {
            status_t res;
            float buf;
            const void *data;
            LSPString name, value, comment;

            // Write port data
            for (size_t i=0, n=ports->size(); i<n; ++i)
            {
                IPort *p    = ports->uget(i);
                if (p == NULL)
                    continue;

                const meta::port_t *meta = p->metadata();
                if (meta == NULL)
                    continue;

                switch (meta->role)
                {
                    case meta::R_CONTROL:
                    case meta::R_METER:
                    case meta::R_PORT_SET:
                    case meta::R_BYPASS:
                        buf     = p->value();
                        data    = &buf;
                        break;
                    default:
                        data    = p->buffer();
                        break;
                }

                // Format the port value
                comment.clear();
                name.clear();
                value.clear();
                res     = core::serialize_port_value(s, meta, data, relative, 0);
                if (res != STATUS_BAD_TYPE)
                {
                    if (res != STATUS_OK)
                        return res;
                    if ((res = s->writeln()) != STATUS_OK)
                        return res;
                }
            }

            return STATUS_OK;
        }

        status_t IWrapper::export_kvt(config::Serializer *s, core::KVTStorage *kvt, const io::Path *relative)
        {
            status_t res;
            core::KVTIterator *iter = kvt->enum_all();

            // Emit the whole list of KVT parameters
            while ((iter != NULL) && (iter->next() == STATUS_OK))
            {
                const core::kvt_param_t *p;

                // Get KVT parameter
                res = iter->get(&p);
                if (res == STATUS_NOT_FOUND)
                    continue;
                else if (res != STATUS_OK)
                {
                    lsp_warn("Could not get parameter: code=%d", int(res));
                    break;
                }

                // Skip transient and private parameters
                if ((iter->is_transient()) || (iter->is_private()))
                    continue;

                // Get parameter name
                const char *pname = iter->name();

                // Serialize state
                switch (p->type)
                {
                    case core::KVT_INT32:
                        res = s->write_i32(pname, p->i32, config::SF_TYPE_SET);
                        break;
                    case core::KVT_UINT32:
                        res = s->write_u32(pname, p->u32, config::SF_TYPE_SET);
                        break;
                    case core::KVT_INT64:
                        res = s->write_i64(pname, p->i64, config::SF_TYPE_SET);
                        break;
                    case core::KVT_UINT64:
                        res = s->write_u64(pname, p->u64, config::SF_TYPE_SET);
                        break;
                    case core::KVT_FLOAT32:
                        res = s->write_f32(pname, p->f32, config::SF_TYPE_SET);
                        break;
                    case core::KVT_FLOAT64:
                        res = s->write_f64(pname, p->f64, config::SF_TYPE_SET);
                        break;
                    case core::KVT_STRING:
                        res = s->write_string(pname, p->str, config::SF_TYPE_STR | config::SF_QUOTED);
                        break;
                    case core::KVT_BLOB:
                    {
                        config::blob_t blob;
                        blob.length = 0;
                        blob.ctype  = const_cast<char *>(p->blob.ctype);
                        blob.data   = NULL;

                        // Encode BLOB to BASE-64
                        if ((p->blob.size > 0) && (p->blob.data != NULL))
                        {
                            size_t dst_size     = 0x10 + ((p->blob.size * 4) / 3);
                            blob.data = static_cast<char *>(::malloc(dst_size));
                            if (blob.data != NULL)
                            {
                                size_t dst_left = dst_size, src_left = p->blob.size;
                                dsp::base64_enc(blob.data, &dst_left, p->blob.data, &src_left);
                                blob.length = p->blob.size;
                            }
                        }

                        res = s->write_blob(pname, &blob, config::SF_TYPE_SET | config::SF_QUOTED);
                        if (blob.data != NULL)
                            free(blob.data);
                        break;
                    }
                    default:
                        res = STATUS_BAD_STATE;
                        break;
                }

                if (res != STATUS_OK)
                {
                    lsp_warn("Error emitting parameter %s: %d", pname, int(res));
                    continue;
                }
            }

            return STATUS_OK;
        }

        status_t IWrapper::export_settings(io::IOutSequence *os, const io::Path *relative)
        {
            // Create configuration serializer
            config::Serializer s;
            status_t res = s.wrap(os, 0);
            if (res != STATUS_OK)
                return res;

            // Write header
            LSPString comment;
            build_config_header(&comment);
            if ((res = s.write_comment(&comment)) != STATUS_OK)
                return res;
            if ((res = s.writeln()) != STATUS_OK)
                return res;

            // Export regular ports
            if ((res = export_ports(&s, &vPorts, relative)) != STATUS_OK)
                return res;

            // Export KVT data
            core::KVTStorage *kvt = kvt_lock();
            if (kvt != NULL)
            {
                // Write comment
                res = s.writeln();
                if (res == STATUS_OK)
                    res = s.write_comment(config_separator);
                if (res == STATUS_OK)
                    res = s.write_comment("KVT parameters");
                if (res == STATUS_OK)
                    res = s.write_comment(config_separator);
                if (res == STATUS_OK)
                    res = s.writeln();
                if (res == STATUS_OK)
                    res = export_kvt(&s, kvt, relative);

                kvt->gc();
                kvt_release();
            }

            if (res == STATUS_OK)
                res = s.writeln();
            if (res == STATUS_OK)
                res = s.write_comment(config_separator);

            return res;
        }

        status_t IWrapper::import_settings(const char *file, bool preset)
        {
            io::Path tmp;
            status_t res = tmp.set(file);
            return (res == STATUS_OK) ? import_settings(&tmp, preset) : res;
        }

        status_t IWrapper::import_settings(const LSPString *file, bool preset)
        {
            io::Path tmp;
            status_t res = tmp.set(file);
            return (res == STATUS_OK) ? import_settings(&tmp, preset) : res;
        }

        status_t IWrapper::import_settings(const io::Path *file, bool preset)
        {
            // Read the resource as sequence
            io::IInSequence *is = pLoader->read_sequence(file, "UTF-8");
            if (is == NULL)
                return pLoader->last_error();
            status_t res = import_settings(is, preset);
            status_t res2 = is->close();
            delete is;
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::import_settings(io::IInSequence *is, bool preset)
        {
            config::PullParser parser;
            status_t res = parser.wrap(is);
            if (res == STATUS_OK)
                res = import_settings(&parser, preset);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::import_settings(config::PullParser *parser, bool preset)
        {
            status_t res;
            config::param_t param;
            core::KVTStorage *kvt = kvt_lock();

            while ((res = parser->next(&param)) == STATUS_OK)
            {
                if ((param.name.starts_with('/')) && (kvt != NULL)) // KVT
                {
                    core::kvt_param_t kp;

                    switch (param.type())
                    {
                        case config::SF_TYPE_I32:
                            kp.type         = core::KVT_INT32;
                            kp.i32          = param.v.i32;
                            break;
                        case config::SF_TYPE_U32:
                            kp.type         = core::KVT_UINT32;
                            kp.u32          = param.v.u32;
                            break;
                        case config::SF_TYPE_I64:
                            kp.type         = core::KVT_INT64;
                            kp.i64          = param.v.i64;
                            break;
                        case config::SF_TYPE_U64:
                            kp.type         = core::KVT_UINT64;
                            kp.u64          = param.v.u64;
                            break;
                        case config::SF_TYPE_F32:
                            kp.type         = core::KVT_FLOAT32;
                            kp.f32          = param.v.f32;
                            break;
                        case config::SF_TYPE_F64:
                            kp.type         = core::KVT_FLOAT64;
                            kp.f64          = param.v.f64;
                            break;
                        case config::SF_TYPE_BOOL:
                            kp.type         = core::KVT_FLOAT32;
                            kp.f32          = (param.v.bval) ? 1.0f : 0.0f;
                            break;
                        case config::SF_TYPE_STR:
                            kp.type         = core::KVT_STRING;
                            kp.str          = param.v.str;
                            break;
                        case config::SF_TYPE_BLOB:
                            kp.type         = core::KVT_BLOB;
                            kp.blob.size    = param.v.blob.length;
                            kp.blob.ctype   = param.v.blob.ctype;
                            kp.blob.data    = NULL;
                            if (param.v.blob.data != NULL)
                            {
                                // Allocate memory
                                size_t src_left = strlen(param.v.blob.data);
                                size_t dst_left = 0x10 + param.v.blob.length;
                                void *blob      = ::malloc(dst_left);
                                if (blob != NULL)
                                {
                                    kp.blob.data    = blob;

                                    // Decode
                                    size_t n = dsp::base64_dec(blob, &dst_left, param.v.blob.data, &src_left);
                                    if ((n != param.v.blob.length) || (src_left != 0))
                                    {
                                        ::free(blob);
                                        kp.type         = core::KVT_ANY;
                                        kp.blob.data    = NULL;
                                    }
                                }
                                else
                                    kp.type         = core::KVT_ANY;
                            }
                            break;
                        default:
                            kp.type         = core::KVT_ANY;
                            break;
                    }

                    if (kp.type != core::KVT_ANY)
                    {
                        const char *id = param.name.get_utf8();
                        kvt->put(id, &kp, core::KVT_RX);
                        kvt_notify_write(kvt, id, &kp);
                    }

                    // Free previously allocated data
                    if ((kp.type == core::KVT_BLOB) && (kp.blob.data != NULL))
                        free(const_cast<void *>(kp.blob.data));
                }
                else
                {
                    size_t flags = (preset) ? plug::PF_PRESET_IMPORT : plug::PF_STATE_IMPORT;

                    for (size_t i=0, n=vPorts.size(); i<n; ++i)
                    {
                        ui::IPort *p = vPorts.uget(i);
                        if (p == NULL)
                            continue;
                        const meta::port_t *meta = p->metadata();
                        if ((meta != NULL) && (param.name.equals_ascii(meta->id)))
                        {
                            if (set_port_value(p, &param, flags, NULL))
                                p->notify_all();
                            break;
                        }
                    }
                }
            }

            // Release KVT
            if (kvt != NULL)
            {
                kvt->gc();
                kvt_release();
            }

            return (res == STATUS_EOF) ? STATUS_OK : res;
        }

        status_t IWrapper::load_global_config(const char *file)
        {
            config::PullParser parser;
            status_t res = parser.open(file);
            if (res == STATUS_OK)
                res = load_global_config(&parser);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::load_global_config(const LSPString *file)
        {
            config::PullParser parser;
            status_t res = parser.open(file);
            if (res == STATUS_OK)
                res = load_global_config(&parser);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::load_global_config(const io::Path *file)
        {
            config::PullParser parser;
            status_t res = parser.open(file);
            if (res == STATUS_OK)
                res = load_global_config(&parser);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::load_global_config(io::IInSequence *is)
        {
            config::PullParser parser;
            status_t res = parser.wrap(is);
            if (res == STATUS_OK)
                res = load_global_config(&parser);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::load_global_config(config::PullParser *parser)
        {
            status_t res;
            config::param_t param;

            // Lock config update
            nFlags |= F_CONFIG_LOCK;

            while ((res = parser->next(&param)) == STATUS_OK)
            {
                for (size_t i=0, n=vConfigPorts.size(); i<n; ++i)
                {
                    ui::IPort *p = vConfigPorts.uget(i);
                    if (p == NULL)
                        continue;
                    const meta::port_t *meta = p->metadata();
                    if ((meta != NULL) && (param.name.equals_ascii(meta->id)))
                    {
                        if (set_port_value(p, &param, plug::PF_STATE_IMPORT, NULL))
                            p->notify_all();
                        break;
                    }
                }
            }

            // Unlock config update
            nFlags &= ~F_CONFIG_LOCK;

            return (res == STATUS_EOF) ? STATUS_OK : res;
        }

        status_t IWrapper::save_global_config(const char *file)
        {
            io::Path path;
            status_t res = path.set(file);
            if (res != STATUS_OK)
                return res;

            return save_global_config(&path);
        }

        status_t IWrapper::save_global_config(const LSPString *file)
        {
            io::Path path;
            status_t res = path.set(file);
            if (res != STATUS_OK)
                return res;

            return save_global_config(&path);
        }

        status_t IWrapper::save_global_config(const io::Path *file)
        {
            io::OutFileStream os;
            io::OutSequence o;

            status_t res = os.open(file, io::File::FM_WRITE_NEW);
            if (res != STATUS_OK)
                return res;

            // Wrap
            if ((res = o.wrap(&os, WRAP_CLOSE, "UTF-8")) != STATUS_OK)
            {
                os.close();
                return res;
            }

            // Export settings
            res = save_global_config(&o);
            status_t res2 = o.close();

            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::save_global_config(io::IOutSequence *os)
        {
            // Create configuration serializer
            config::Serializer s;
            status_t res = s.wrap(os, 0);
            if (res != STATUS_OK)
                return res;

            // Write header
            LSPString comment;
            build_global_config_header(&comment);
            if ((res = s.write_comment(&comment)) != STATUS_OK)
                return res;
            if ((res = s.writeln()) != STATUS_OK)
                return res;

            // Export regular ports
            if ((res = export_ports(&s, &vConfigPorts, NULL)) != STATUS_OK)
                return res;

            if (res == STATUS_OK)
                res = s.write_comment(config_separator);

            return res;
        }

        void IWrapper::position_updated(const plug::position_t *pos)
        {
            size_t i = 0;

            // Actual time position
            sPosition       = *pos;

            vTimePorts[i++]->commit_value(pos->sampleRate);
            vTimePorts[i++]->commit_value(pos->speed);
            vTimePorts[i++]->commit_value(pos->frame);
            vTimePorts[i++]->commit_value(pos->numerator);
            vTimePorts[i++]->commit_value(pos->denominator);
            vTimePorts[i++]->commit_value(pos->beatsPerMinute);
            vTimePorts[i++]->commit_value(pos->tick);
            vTimePorts[i++]->commit_value(pos->ticksPerBeat);

            // Issue callback
            if (pUI != NULL)
                pUI->position_updated(pos);
        }

        bool IWrapper::set_port_value(ui::IPort *port, const config::param_t *param, size_t flags, const io::Path *base)
        {
            // Get metadata
            const meta::port_t *p = (port != NULL) ? port->metadata() : NULL;
            if (p == NULL)
                return false;

            // Check that it's a control port
            if (!meta::is_in_port(p))
                return false;

            // Apply changes
            switch (p->role)
            {
                case meta::R_PORT_SET:
                case meta::R_CONTROL:
                {
                    lsp_trace("  param = %s, value = %f", param->name.get_utf8(), param->to_float());

                    if (meta::is_discrete_unit(p->unit))
                    {
                        if (meta::is_bool_unit(p->unit))
                            port->set_value((param->to_bool()) ? 1.0f : 0.0f, flags);
                        else
                            port->set_value(param->to_int(), flags);
                    }
                    else
                    {
                        float v = param->to_float();

                        // Decode decibels to values
                        if ((meta::is_decibel_unit(p->unit)) && (param->is_decibel()))
                        {
                            if ((p->unit == meta::U_GAIN_AMP) || (p->unit == meta::U_GAIN_POW))
                            {
                                if (v < -250.0f)
                                    v       = 0.0f;
                                else if (v > 250.0f)
                                    v       = (p->unit == meta::U_GAIN_AMP) ? dspu::db_to_gain(250.0f) : dspu::db_to_power(250.0f);
                                else
                                    v       = (p->unit == meta::U_GAIN_AMP) ? dspu::db_to_gain(v) : dspu::db_to_power(v);
                            }
                        }

                        port->set_value(v, flags);
                    }
                    break;
                }
                case meta::R_PATH:
                {
                    // Check type of argument
                    if (!param->is_string())
                        return false;

                    lsp_trace("  param = %s, value = %s", param->name.get_utf8(), param->v.str);

                    const char *value = param->v.str;
                    size_t len      = ::strlen(value);
                    io::Path path;

                    if (core::parse_relative_path(&path, base, value, len))
                    {
                        // Update value and it's length
                        value   = path.as_utf8();
                        len     = strlen(value);
                    }

                    port->write(value, len, flags);
                    break;
                }
                default:
                    return false;
            }
            return true;
        }

        const meta::package_t *IWrapper::package() const
        {
            return NULL;
        }

        float IWrapper::ui_scaling_factor(float scaling)
        {
            return scaling;
        }

        status_t IWrapper::init_visual_schema()
        {
            status_t res;

            // Register builtin styles provided by the framework
            if ((res = BuiltinStyle::init_schema(pDisplay->schema())) != STATUS_OK)
                return res;

            // Try to load selected schema
            ui::IPort *s_port   = port(UI_VISUAL_SCHEMA_PORT);
            const char *schema  = ((s_port != NULL) && (meta::is_path_port(s_port->metadata()))) ?
                                    s_port->buffer<const char>() :
                                    NULL;

            if ((schema != NULL) && (strlen(schema) > 0))
            {
                if ((res = load_visual_schema(schema)) == STATUS_OK)
                    return res;
            }

            // Load fallback schema
            schema          = LSP_BUILTIN_PREFIX "schema/modern.xml";
            if (s_port != NULL)
            {
                s_port->write(schema, strlen(schema));
                s_port->notify_all();
            }

            return load_visual_schema(schema);
        }

        status_t IWrapper::load_visual_schema(const char *file)
        {
            if (pDisplay == NULL)
                return STATUS_BAD_STATE;

            tk::StyleSheet ss;
            status_t res = load_stylesheet(&ss, file);
            return (res == STATUS_OK) ? apply_visual_schema(&ss) : res;
        }

        status_t IWrapper::load_visual_schema(const io::Path *file)
        {
            if (pDisplay == NULL)
                return STATUS_BAD_STATE;

            tk::StyleSheet ss;
            status_t res = load_stylesheet(&ss, file);
            return (res == STATUS_OK) ? apply_visual_schema(&ss) : res;
        }

        status_t IWrapper::load_visual_schema(const LSPString *file)
        {
            if (pDisplay == NULL)
                return STATUS_BAD_STATE;

            // Load style sheet
            tk::StyleSheet ss;
            status_t res = load_stylesheet(&ss, file);
            return (res == STATUS_OK) ? apply_visual_schema(&ss) : res;
        }

        status_t IWrapper::apply_visual_schema(const tk::StyleSheet *sheet)
        {
            status_t res;

            // Apply schema
            if ((res = pDisplay->schema()->apply(sheet, pLoader)) != STATUS_OK)
                return res;

            // Initialize global constants
            if ((res = init_global_constants(sheet)) != STATUS_OK)
                return res;

            // Notify all listeners in reverse order
            for (size_t i=vSchemaListeners.size(); i > 0; )
            {
                ISchemaListener *listener = vSchemaListeners.uget(--i);
                if (listener != NULL)
                    listener->reloaded(sheet);
            }

            return res;
        }

        status_t IWrapper::init_global_constants(const tk::StyleSheet *sheet)
        {
            status_t res;

            // Cleanup variables
            sGlobalVars.clear();

            // Evaluate global constants
            lltl::parray<LSPString> constants;
            if ((res = sheet->enum_constants(&constants)) != STATUS_OK)
            {
                lsp_warn("Error enumerating global constants");
                return res;
            }

            LSPString name, value;
            expr::value_t xvalue;
            expr::init_value(&xvalue);
            expr::Expression ex;

            // Evaluate all constants
            for (size_t i=0, n=constants.size(); i<n; ++i)
            {
                // Get constant name and value
                const LSPString *cname = constants.uget(i);
                if (cname == NULL)
                    continue;

                if ((res = sheet->get_constant(cname, &value)) != STATUS_OK)
                {
                    lsp_warn("Error reading constant value for '%s'", cname->get_native());
                    return res;
                }

                // Evaluate expression
                if ((res = ex.parse(&value, expr::Expression::FLAG_NONE)) != STATUS_OK)
                {
                    lsp_warn("Error parsing expression for '%s': %s", cname->get_native(), value.get_native());
                    return res;
                }
                if ((res = ex.evaluate(&xvalue)) != STATUS_OK)
                {
                    lsp_warn("Error evaluating expression for '%s': %s", cname->get_native(), value.get_native());
                    return res;
                }

                // Form variable name
                if (!name.set_ascii("const_"))
                    return STATUS_NO_MEM;
                if (!name.append(cname))
                    return STATUS_NO_MEM;

                // Set the variable value
                if ((res = sGlobalVars.set(&name, &xvalue)) != STATUS_OK)
                {
                    lsp_warn("Error setting global constant '%s'", name.get_native());
                    return res;
                }

                // Undefine the value
                expr::set_value_undef(&xvalue);
            }

            expr::destroy_value(&xvalue);

            return res;
        }

        status_t IWrapper::load_stylesheet(tk::StyleSheet *sheet, const char *file)
        {
            if ((sheet == NULL) || (file == NULL))
                return STATUS_BAD_ARGUMENTS;

            LSPString path;
            if (!path.set_utf8(file))
                return STATUS_NO_MEM;

            return load_stylesheet(sheet, &path);
        }

        status_t IWrapper::load_stylesheet(tk::StyleSheet *sheet, const io::Path *file)
        {
            if ((sheet == NULL) || (file == NULL))
                return STATUS_BAD_ARGUMENTS;

            return load_stylesheet(sheet, file->as_string());
        }

        status_t IWrapper::load_stylesheet(tk::StyleSheet *sheet, const LSPString *file)
        {
            if ((sheet == NULL) || (file == NULL))
                return STATUS_BAD_ARGUMENTS;

            // Read the resource as sequence
            io::IInSequence *is = pLoader->read_sequence(file, "UTF-8");
            if (is == NULL)
                return pLoader->last_error();

            // Parse the sheet data and close the input sequence
            status_t res    = sheet->parse_data(is);
            if (res != STATUS_OK)
                lsp_warn("Error loading stylesheet '%s': code=%d, %s", file->get_native(), int(res), sheet->error()->get_native());

            status_t res2   = is->close();
            delete is;

            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::add_schema_listener(ui::ISchemaListener *listener)
        {
            if (vSchemaListeners.contains(listener))
                return STATUS_ALREADY_EXISTS;

            return (vSchemaListeners.add(listener)) ? STATUS_OK : STATUS_NO_MEM;
        }

        status_t IWrapper::remove_schema_listener(ui::ISchemaListener *listener)
        {
            return (vSchemaListeners.premove(listener)) ? STATUS_OK : STATUS_NOT_FOUND;
        }

        expr::Variables *IWrapper::global_variables()
        {
            return &sGlobalVars;
        }
    }
} /* namespace lsp */


