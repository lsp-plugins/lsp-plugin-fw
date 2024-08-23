/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/common/debug.h>
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
        static const port_item_t filter_point_thickness_modes[]=
        {
            { "Thinnest",       "filter.point_thick.thinnest" },
            { "Thin",           "filter.point_thick.thin" },
            { "Normal",         "filter.point_thick.normal" },
            { "Semibold",       "filter.point_thick.semibold" },
            { "Bold",           "filter.point_thick.bold" },
            { NULL,             NULL }
        };

        static const meta::port_t config_metadata[] =
        {
            PATH(UI_LAST_VERSION_PORT_ID, "Last version of the product installed"),
            PATH(UI_DLG_SAMPLE_PATH_ID, "Dialog path for selecting sample files"),
            INT_CONTROL_RANGE(UI_DLG_SAMPLE_FTYPE_ID, "Dialog file type for selecting sample files", U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_IR_PATH_ID, "Dialog path for selecting impulse response files"),
            INT_CONTROL_RANGE(UI_DLG_IR_FTYPE_ID, "Dialog file type for selecting impulse response files", U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_CONFIG_PATH_ID, "Dialog path for saving/loading configuration files"),
            INT_CONTROL_RANGE(UI_DLG_CONFIG_FTYPE_ID, "Dialog file type for saving/loading configuration files", U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_REW_PATH_ID, "Dialog path for importing REW settings files"),
            INT_CONTROL_RANGE(UI_DLG_REW_FTYPE_ID, "Dialog file type for importing REW settings files", U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_HYDROGEN_PATH_ID, "Dialog path for importing Hydrogen drumkit files"),
            INT_CONTROL_RANGE(UI_DLG_HYDROGEN_FTYPE_ID, "Dialog file type for importing Hydrogen drumkit files", U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_LSPC_BUNDLE_PATH_ID, "Dialog path for exporting/importing LSPC bundles"),
            INT_CONTROL_RANGE(UI_DLG_LSPC_BUNDLE_FTYPE_ID, "Dialog file type for exporting/importing LSPC bundles", U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_SFZ_PATH_ID, "Dialog path for exporting/importing SFZ files"),
            INT_CONTROL_RANGE(UI_DLG_SFZ_FTYPE_ID, "Dialog file type for exporting/importing SFZ files", U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_MODEL3D_PATH_ID, "Dialog for saving/loading 3D model files"),
            INT_CONTROL_RANGE(UI_DLG_MODEL3D_FTYPE_ID, "Dialog file type for saving/loading 3D model files", U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_DEFAULT_PATH_ID, "Dialog default path for other files"),
            INT_CONTROL_RANGE(UI_DLG_DEFAULT_FTYPE_ID, "Dialog default file type for other files", U_NONE, 0, 100, 0, 1),
            PATH(UI_R3D_BACKEND_PORT_ID, "Identifier of selected backend for 3D rendering"),
            PATH(UI_LANGUAGE_PORT_ID, "Selected language identifier for the UI interface"),
            SWITCH(UI_REL_PATHS_PORT_ID, "Use relative paths when exporting configuration file", 0.0f),
            KNOB(UI_SCALING_PORT_ID, "Manual UI scaling factor", U_PERCENT, 25.0f, 400.0f, 100.0f, 1.0f),
            SWITCH(UI_SCALING_HOST_ID, "Prefer host-reported UI scale factor", 1.0f),
            KNOB(UI_FONT_SCALING_PORT_ID, "Manual UI font scaling factor", U_PERCENT, 50.0f, 200.0f, 100.0f, 1.0f),
            PATH(UI_VISUAL_SCHEMA_FILE_ID, "Current visual schema file used by the UI"),
            SWITCH(UI_PREVIEW_AUTO_PLAY_ID, "Enable automatic playback of the audio file in the file preview part of the file open dialog", 0.0f),
            SWITCH(UI_ENABLE_KNOB_SCALE_ACTIONS_ID, "Enable knob scale mouse actions", 1.0f),
            PATH(UI_USER_HYDROGEN_KIT_PATH_ID, "User Hydrogen kits path"),
            PATH(UI_OVERRIDE_HYDROGEN_KIT_PATH_ID, "Override Hydrogen kits path"),
            SWITCH(UI_OVERRIDE_HYDROGEN_KITS_ID, "Override Hydrogen kits", 1.0f),
            SWITCH(UI_INVERT_VSCROLL_ID, "Invert global mouse vertical scroll behaviour", 0.0f),
            SWITCH(UI_GRAPH_DOT_INVERT_VSCROLL_ID, "Invert mouse vertical scroll behaviour for graph dot widget", 0.0f),
            SWITCH(UI_ZOOMABLE_SPECTRUM_GRAPH_ID, "Enables the automatic scaling mode of the frequency graph", 1.0f),
            COMBO(UI_FILTER_POINT_THICK_ID, "Thickness of the filter point", 1.0f, filter_point_thickness_modes),
            PATH(UI_DOCUMENTATION_PATH_ID, "Path to the local documentation installation"),
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
            pDisplay        = NULL;
            wWindow         = NULL;
            pWindow         = NULL;
            pUI             = ui;
            pLoader         = loader;
            nFlags          = 0;
            nPlayPosition   = 0;
            nPlayLength     = 0;

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
            // Flush list of playback listeners
            vPlayListeners.flush();

            // Flush list of 'Schema reloaded' handlers
            vSchemaListeners.flush();

//            // Unbind all listeners for ports
//            for (lltl::iterator<IPort> it=vPorts.values(); it; ++it)
//                it->unbind_all();

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

            // Destroy switched ports in two passes.
            // 1. Disconnect from dependent ports.
            for (size_t i=0, n=vSwitchedPorts.size(); i<n; ++i)
            {
                SwitchedPort *p = vSwitchedPorts.uget(i);
                if (p != NULL)
                    p->destroy();
            }
            // 2. Free memory allocated for the switched port.
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
                ui::IPort *p = vPorts.uget(i);
                p->unbind_all();
                lsp_trace("destroy UI port id=%s", p->metadata()->id);
                delete p;
            }
            vPorts.flush();
        }

        status_t IWrapper::init(void *root_widget)
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
                    port->notify_all(ui::PORT_NONE);
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
            if (pUI != NULL)
                pUI->kvt_changed(storage, id, value);
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
            return (vKvtListeners.qpremove(listener)) ? STATUS_OK : STATUS_NOT_FOUND;
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
            // Check for alias: perform recursive search until alias will be translated into expression
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
            id = key.get_utf8(); // Finally, translate alias value back into port identifier

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

            return port_by_id(id);
        }

        ui::IPort *IWrapper::port_by_id(const char *id)
        {
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

            // Call the nested UI (deliver idle signal)
            if (pUI != NULL)
                pUI->idle();

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

            // Obtain the parent directory if needed
            io::Path dir;
            if (relative)
            {
                if ((res = file->get_parent(&dir)) != STATUS_OK)
                    relative = false;
            }

            // Export settings
            res = export_settings(&o, (relative) ? &dir : NULL);
            status_t res2 = o.close();

            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::export_settings(io::IOutSequence *os, const char *basedir)
        {
            if (basedir == NULL)
                return export_settings(os, static_cast<io::Path *>(NULL));

            io::Path path;
            status_t res = path.set(basedir);
            if (res != STATUS_OK)
                return res;

            return export_settings(os, &path);
        }

        status_t IWrapper::export_settings(io::IOutSequence *os, const LSPString *basedir)
        {
            if (basedir == NULL)
                return export_settings(os, static_cast<io::Path *>(NULL));

            io::Path path;
            status_t res = path.set(basedir);
            if (res != STATUS_OK)
                return res;

            return export_settings(os, &path);
        }

        status_t IWrapper::export_settings(io::IOutSequence *os, const io::Path *basedir)
        {
            // Create configuration serializer
            config::Serializer s;
            status_t res = s.wrap(os, 0);
            if (res != STATUS_OK)
                return res;

            res = export_settings(&s, basedir);
            status_t res2 = s.close();
            return (res != STATUS_OK) ? res : res2;
        }

        status_t IWrapper::export_settings(config::Serializer *s, const char *basedir)
        {
            if (basedir == NULL)
                return export_settings(s, static_cast<io::Path *>(NULL));

            io::Path path;
            status_t res = path.set(basedir);
            if (res != STATUS_OK)
                return res;

            return export_settings(s, &path);
        }

        status_t IWrapper::export_settings(config::Serializer *s, const LSPString *basedir)
        {
            if (basedir == NULL)
                return export_settings(s, static_cast<io::Path *>(NULL));

            io::Path path;
            status_t res = path.set(basedir);
            if (res != STATUS_OK)
                return res;

            return export_settings(s, &path);
        }

        status_t IWrapper::export_settings(config::Serializer *s, const io::Path *basedir)
        {
            // Write header
            status_t res;
            LSPString comment;
            build_config_header(&comment);
            if ((res = s->write_comment(&comment)) != STATUS_OK)
                return res;
            if ((res = s->writeln()) != STATUS_OK)
                return res;

            // Export regular ports
            if ((res = export_ports(s, &vPorts, basedir)) != STATUS_OK)
                return res;

            // Export KVT data
            core::KVTStorage *kvt = kvt_lock();
            if (kvt != NULL)
            {
                // Write comment
                res = s->writeln();
                if (res == STATUS_OK)
                    res = s->write_comment(config_separator);
                if (res == STATUS_OK)
                    res = s->write_comment("KVT parameters");
                if (res == STATUS_OK)
                    res = s->write_comment(config_separator);
                if (res == STATUS_OK)
                    res = s->writeln();
                if (res == STATUS_OK)
                    res = export_kvt(s, kvt, basedir);

                kvt->gc();
                kvt_release();
            }

            if (res == STATUS_OK)
                res = s->writeln();
            if (res == STATUS_OK)
                res = s->write_comment(config_separator);

            return res;
        }

        void IWrapper::build_config_header(LSPString *c)
        {
            const meta::package_t *pkg = package();
            const meta::plugin_t *meta = pUI->metadata();
            char vst3_uid[40];

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
            c->fmt_append_utf8  ("  Package:             %s (%s)\n", pkg->artifact, pkg->artifact_name);
            c->fmt_append_utf8  ("  Package version:     %s\n", pkv.get_utf8());
            c->fmt_append_utf8  ("  Plugin name:         %s (%s)\n", meta->name, meta->description);
            c->fmt_append_utf8  ("  Plugin version:      %d.%d.%d\n",
                    int(LSP_MODULE_VERSION_MAJOR(meta->version)),
                    int(LSP_MODULE_VERSION_MINOR(meta->version)),
                    int(LSP_MODULE_VERSION_MICRO(meta->version))
                );

            char *gst_uid = meta::make_gst_canonical_name(meta->uids.gst);
            lsp_finally {
                if (gst_uid != NULL)
                    free(gst_uid);
            };

            if (meta->uid != NULL)
                c->fmt_append_utf8   ("  UID:                     %s\n", meta->uid);
            if (meta->uids.clap != NULL)
                c->fmt_append_utf8   ("  CLAP URI:                %s\n", meta->uids.clap);
            if (gst_uid != NULL)
                c->fmt_append_utf8   ("  GStreamer identifier:    %s\n", gst_uid);
            if (meta->uids.ladspa_id > 0)
                c->fmt_append_utf8   ("  LADSPA identifier:       %d\n", meta->uids.ladspa_id);
            if (meta->uids.ladspa_lbl != NULL)
                c->fmt_append_utf8   ("  LADSPA label:            %s\n", meta->uids.ladspa_lbl);
            if (meta->uids.lv2 != NULL)
                c->fmt_append_utf8   ("  LV2 URI:                 %s\n", meta->uids.lv2);
            if (meta->uids.vst2 != NULL)
                c->fmt_append_utf8   ("  VST 2.x identifier:      %s\n", meta->uids.vst2);
            if (meta->uids.vst3 != NULL)
                c->fmt_append_utf8   ("  VST 3.x identifier:      %s\n", meta::uid_meta_to_vst3(vst3_uid, meta->uids.vst3));
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

        status_t IWrapper::export_ports(config::Serializer *s, lltl::parray<IPort> *ports, const io::Path *basedir)
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
                if (meta::is_out_port(meta))
                    continue;
                if (!strcmp(meta->id, UI_LAST_VERSION_PORT_ID))
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
                res     = core::serialize_port_value(s, meta, data, basedir, 0);
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

        status_t IWrapper::export_bundle_versions(config::Serializer *s, const lltl::pphash<LSPString, LSPString> *versions)
        {
            // Perform export, replace the bundle version by the actual version parameter
            status_t res;
            lltl::parray<LSPString> kv;
            if (!versions->keys(&kv))
                return STATUS_NO_MEM;

            // Get the actual version of the bundle
            LSPString version_key, version_value;
            get_bundle_version_key(&version_key);

            lsp_trace("bundle version key=%s", version_key.get_native());

            for (size_t i=0, n=vConfigPorts.size(); i<n; ++i)
            {
                ui::IPort *port = vConfigPorts.get(i);
                if (port == NULL)
                    continue;
                const meta::port_t *meta = port->metadata();
                if ((meta == NULL) || (meta->role != meta::R_PATH) ||
                    (meta->id == NULL) || (strcmp(meta->id, UI_LAST_VERSION_PORT_ID) != 0))
                    continue;
                const char *value = static_cast<const char *>(port->buffer());
                if (value != NULL)
                    version_value.set_utf8(value);
                break;
            }
            lsp_trace("bundle version value=%s", version_value.get_native());

            // Add the version to the list if it is missing
            if (!versions->contains(&version_key))
            {
                if (!kv.add(&version_key))
                    return STATUS_NO_MEM;
            }

            // Export all available bundle versions
            for (size_t i=0, n=kv.size(); i<n; ++i)
            {
                const LSPString *name = kv.uget(i);
                if (name == NULL)
                    return STATUS_UNKNOWN_ERR;

                const LSPString *value = (!version_key.equals(name)) ? versions->get(name) : &version_value;
                if (value == NULL)
                    return STATUS_UNKNOWN_ERR;

                lsp_trace("Export version: %s=%s", name->get_native(), value->get_native());

                // The bundle version should be replaced by the actual one in the output configuration
                if ((res = s->write_string(name, value, config::SF_QUOTED)) != STATUS_OK)
                    return res;
            }

            return STATUS_OK;
        }

        status_t IWrapper::import_settings(const char *file, size_t flags)
        {
            io::Path tmp;
            status_t res = tmp.set(file);
            return (res == STATUS_OK) ? import_settings(&tmp, flags) : res;
        }

        status_t IWrapper::import_settings(const LSPString *file, size_t flags)
        {
            io::Path tmp;
            status_t res = tmp.set(file);
            return (res == STATUS_OK) ? import_settings(&tmp, flags) : res;
        }

        status_t IWrapper::import_settings(const io::Path *file, size_t flags)
        {
            // Get parent file
            io::Path basedir;
            bool has_parent = file->get_parent(&basedir) == STATUS_OK;

            // Read the resource as sequence
            io::IInSequence *is = pLoader->read_sequence(file, "UTF-8");
            if (is == NULL)
                return pLoader->last_error();
            status_t res = import_settings(is, flags, (has_parent) ? &basedir : NULL);
            status_t res2 = is->close();
            delete is;
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::import_settings(io::IInSequence *is, size_t flags, const io::Path *basedir)
        {
            config::PullParser parser;
            status_t res = parser.wrap(is);
            if (res == STATUS_OK)
                res = import_settings(&parser, flags, basedir);
            status_t res2 = parser.close();
            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::import_settings(io::IInSequence *is, size_t flags, const char *basedir)
        {
            if (basedir == NULL)
                return import_settings(is, flags, static_cast<io::Path *>(NULL));

            io::Path tmp;
            status_t res = tmp.set(basedir);
            return (res == STATUS_OK) ? import_settings(is, flags, &tmp) : res;
        }

        status_t IWrapper::import_settings(io::IInSequence *is, size_t flags, const LSPString *basedir)
        {
            if (basedir == NULL)
                return import_settings(is, flags, static_cast<io::Path *>(NULL));

            io::Path tmp;
            status_t res = tmp.set(basedir);
            return (res == STATUS_OK) ? import_settings(is, flags, &tmp) : res;
        }

        status_t IWrapper::import_settings(config::PullParser *parser, size_t flags, const char *basedir)
        {
            io::Path tmp;
            status_t res = tmp.set(basedir);
            return (res == STATUS_OK) ? import_settings(parser, flags, &tmp) : res;
        }

        status_t IWrapper::import_settings(config::PullParser *parser, size_t flags, const LSPString *basedir)
        {
            io::Path tmp;
            status_t res = tmp.set(basedir);
            return (res == STATUS_OK) ? import_settings(parser, flags, &tmp) : res;
        }

        status_t IWrapper::import_settings(config::PullParser *parser, size_t flags, const io::Path *basedir)
        {
            status_t res;
            config::param_t param;
            core::KVTStorage *kvt = kvt_lock();
            lsp_finally {
                if (kvt != NULL)
                {
                    kvt->gc();
                    kvt_release();
                }
            };

            // Reset all ports to default values
            if (!(flags & IMPORT_FLAG_PATCH))
            {
                for (size_t i=0, n=vPorts.size(); i<n; ++i)
                {
                    ui::IPort *p = vPorts.uget(i);
                    if (p == NULL)
                        continue;

                    p->set_default();
                    p->notify_all(ui::PORT_NONE);
                }

                if (pUI != NULL)
                    pUI->reset_settings();
            }

            while ((res = parser->next(&param)) == STATUS_OK)
            {
                if (param.name.starts_with('/')) // KVT
                {
                    // Do nothing if there is no KVT
                    if (kvt == NULL)
                    {
                        lsp_warn("Could not apply KVT parameter %s because there is no KVT", param.name.get_utf8());
                        continue;
                    }

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
                    size_t port_flags = (flags & (IMPORT_FLAG_PRESET | IMPORT_FLAG_PATCH)) ?
                                    plug::PF_PRESET_IMPORT : plug::PF_STATE_IMPORT;

                    for (size_t i=0, n=vPorts.size(); i<n; ++i)
                    {
                        ui::IPort *p = vPorts.uget(i);
                        if (p == NULL)
                            continue;
                        const meta::port_t *meta = p->metadata();
                        if ((meta != NULL) && (param.name.equals_ascii(meta->id)))
                        {
                            if (set_port_value(p, &param, port_flags, basedir))
                                p->notify_all(ui::PORT_NONE);
                            break;
                        }
                    }
                }
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

        void IWrapper::get_bundle_version_key(LSPString *key)
        {
            LSPString version_key;
            const meta::package_t *pkg_info = package();
            if (pkg_info != NULL)
            {
                version_key.set_utf8(pkg_info->artifact);
                version_key.replace_all('-', '_');
                version_key.append_ascii("_version");
            }
            else
                version_key.set_ascii(UI_LAST_VERSION_PORT_ID);

            key->swap(&version_key);
        }

        status_t IWrapper::load_global_config(config::PullParser *parser)
        {
            status_t res;
            config::param_t param;
            LSPString version_key;

            // Get the proper name of the version parameter
            get_bundle_version_key(&version_key);

            // Lock config update
            nFlags |= F_CONFIG_LOCK;

            while ((res = parser->next(&param)) == STATUS_OK)
            {
                // Skip raw last version parameter from legacy configuration files
                if (param.name.equals_ascii(UI_LAST_VERSION_PORT_ID))
                    continue;

                const char *param_name = (version_key.equals(&param.name)) ?
                    UI_LAST_VERSION_PORT_ID : param.name.get_utf8();
                for (size_t i=0, n=vConfigPorts.size(); i<n; ++i)
                {
                    ui::IPort *p = vConfigPorts.uget(i);
                    if (p == NULL)
                        continue;
                    const meta::port_t *meta = p->metadata();
                    if ((meta != NULL) && (strcmp(param_name, meta->id) == 0))
                    {
                        if (set_port_value(p, &param, plug::PF_STATE_IMPORT, NULL))
                            p->notify_all(ui::PORT_NONE);
                        break;
                    }
                }
            }

            // Unlock config update
            nFlags &= ~F_CONFIG_LOCK;

            return (res == STATUS_EOF) ? STATUS_OK : res;
        }

        void IWrapper::drop_bundle_versions(lltl::pphash<LSPString, LSPString> *versions)
        {
            lltl::parray<LSPString> vv;
            versions->values(&vv);
            versions->clear();

            for (size_t i=0, n=vv.size(); i<n; ++i)
            {
                LSPString *s = vv.uget(i);
                if (s != NULL)
                    delete s;
            }
        }

        status_t IWrapper::read_bundle_versions(const io::Path *file, lltl::pphash<LSPString, LSPString> *versions)
        {
            config::PullParser parser;
            config::param_t param;
            status_t res;
            lltl::pphash<LSPString, LSPString> tmp;
            LSPString *str = NULL;

            if ((res = parser.open(file)) != STATUS_OK)
                return res;

            // Lock config update
            nFlags |= F_CONFIG_LOCK;

            while ((res = parser.next(&param)) == STATUS_OK)
            {
                if ((param.is_string()) &&
                    (param.name.ends_with_ascii("_version")))
                {
                    // Add new value to hash
                    if ((str = new LSPString()) == NULL)
                    {
                        drop_bundle_versions(&tmp);
                        parser.close();
                        return STATUS_NO_MEM;
                    }
                    if (!str->set_utf8(param.v.str))
                    {
                        delete str;
                        drop_bundle_versions(&tmp);
                        parser.close();
                        return STATUS_NO_MEM;
                    }

                    // Put data to the mapping
                    bool success = tmp.put(&param.name, str, &str);
                    if (str != NULL)
                    {
                        lsp_warn("Duplicate entry in configuration file, assuming parameter %s being %s",
                            param.name.get_utf8(), param.v.str);
                        delete str;
                    }
                    if (!success)
                    {
                        drop_bundle_versions(&tmp);
                        parser.close();
                        return STATUS_NO_MEM;
                    }
                }
            }

            // Unlock config update
            nFlags &= ~F_CONFIG_LOCK;

            // Commit changes
            versions->swap(&tmp);
            drop_bundle_versions(&tmp);

            return STATUS_OK;
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

            // Obtain actual versions of all modules
            lltl::pphash<LSPString, LSPString> versions;
            read_bundle_versions(file, &versions);
            lsp_finally { drop_bundle_versions(&versions); };

            // Write new file
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
            res = save_global_config(&o, &versions);
            status_t res2 = o.close();

            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::save_global_config(io::IOutSequence *os, const lltl::pphash<LSPString, LSPString> *versions)
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

            // Export bundle versions
            res = s.write_comment(config_separator);
            if (res == STATUS_OK)
                res = s.write_comment("Recently used versions of bundles");
            if ((res = export_bundle_versions(&s, versions)) != STATUS_OK)
                return res;
            if ((res = s.writeln()) != STATUS_OK)
                return res;

            // Write footer
            return s.write_comment(config_separator);
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
                case meta::R_BYPASS:
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

                    const char *value   = param->v.str;
                    size_t len          = ::strlen(value);
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
                case meta::R_STRING:
                {
                    // Check type of argument
                    if (!param->is_string())
                        return false;

                    lsp_trace("  param = %s, value = %s", param->name.get_utf8(), param->v.str);

                    // Submit string to the port
                    const char *value   = param->v.str;
                    const size_t len    = ::strlen(value);
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

        const meta::plugin_t *IWrapper::metadata() const
        {
            return (pUI != NULL) ? pUI->metadata() : NULL;
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
                s_port->notify_all(ui::PORT_NONE);
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
            lltl::parray<ISchemaListener> listeners;
            if (vSchemaListeners.values(&listeners))
            {
                for (size_t i=0, n=listeners.size(); i < n; ++i)
                {
                    ISchemaListener *listener = listeners.uget(i);
                    if (listener != NULL)
                        listener->reloaded(sheet);
                }
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

            return (vSchemaListeners.put(listener)) ? STATUS_OK : STATUS_NO_MEM;
        }

        status_t IWrapper::remove_schema_listener(ui::ISchemaListener *listener)
        {
            return (vSchemaListeners.remove(listener)) ? STATUS_OK : STATUS_NOT_FOUND;
        }

        expr::Variables *IWrapper::global_variables()
        {
            return &sGlobalVars;
        }

        status_t IWrapper::reset_settings()
        {
            lsp_trace("Resetting plugin settings");

            for (size_t i=0, n=vPorts.size(); i<n; ++i)
            {
                // Get the port
                ui::IPort *p = vPorts.uget(i);
                if (p == NULL)
                    continue;

                // Skip output ports
                if (meta::is_out_port(p->metadata()))
                    continue;

                // Reset port value to default
                p->set_default();
                p->notify_all(ui::PORT_NONE);
            }

            if (pUI != NULL)
                pUI->reset_settings();

            return STATUS_OK;
        }

        status_t IWrapper::play_file(const char *file, wsize_t position, bool release)
        {
            lsp_trace("file=%s, position=%lld, release=%s",
                file, (long long)(position), (release) ? "true" : "false");
            return STATUS_NOT_IMPLEMENTED;
        }

        status_t IWrapper::play_subscribe(IPlayListener *listener)
        {
            if (listener == NULL)
                return STATUS_BAD_ARGUMENTS;
            if (vPlayListeners.contains(listener))
                return STATUS_ALREADY_BOUND;
            if (!vPlayListeners.add(listener))
                return STATUS_NO_MEM;

            listener->play_position_update(nPlayPosition, nPlayLength);
            return STATUS_OK;
        }

        status_t IWrapper::play_unsubscribe(IPlayListener *listener)
        {
            if (listener == NULL)
                return STATUS_BAD_ARGUMENTS;
            if (!vPlayListeners.contains(listener))
                return STATUS_NOT_BOUND;
            if (!vPlayListeners.qpremove(listener))
                return STATUS_NO_MEM;
            return STATUS_OK;
        }

        void IWrapper::notify_play_position(wssize_t position, wssize_t length)
        {
            if ((nPlayPosition == position) && (nPlayLength == length))
                return;

            lltl::parray<ui::IPlayListener> listeners;
            listeners.add(vPlayListeners);
            for (size_t i=0; i<vPlayListeners.size(); ++i)
            {
                ui::IPlayListener *listener = vPlayListeners.uget(i);
                if (listener != NULL)
                    listener->play_position_update(position, length);
            }

            nPlayPosition   = position;
            nPlayLength     = length;
        }

        bool IWrapper::accept_window_size(tk::Window *wnd, size_t width, size_t height)
        {
            return true;
        }

        bool IWrapper::window_resized(tk::Window *wnd, size_t width, size_t height)
        {
            return true;
        }

        meta::plugin_format_t IWrapper::plugin_format() const
        {
            return meta::PLUGIN_UNKNOWN;
        }

        const core::ShmState *IWrapper::shm_state()
        {
            return NULL;
        }

    } /* namespace ui */
} /* namespace lsp */


