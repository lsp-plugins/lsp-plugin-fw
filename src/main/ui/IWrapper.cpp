/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/plug-fw/core/config.h>
#include <lsp-plug.in/plug-fw/core/presets.h>
#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/io/Dir.h>
#include <lsp-plug.in/io/InFileStream.h>
#include <lsp-plug.in/io/InSequence.h>
#include <lsp-plug.in/io/OutFileStream.h>
#include <lsp-plug.in/io/OutSequence.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/new.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/fmt/config/Serializer.h>
#include <lsp-plug.in/fmt/json/dom.h>
#include <lsp-plug.in/fmt/json/Serializer.h>
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
            INT_CONTROL_ALL(UI_DLG_SAMPLE_FTYPE_ID, "Dialog file type for selecting sample files", NULL, U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_IR_PATH_ID, "Dialog path for selecting impulse response files"),
            INT_CONTROL_ALL(UI_DLG_IR_FTYPE_ID, "Dialog file type for selecting impulse response files", NULL, U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_CONFIG_PATH_ID, "Dialog path for saving/loading configuration files"),
            INT_CONTROL_ALL(UI_DLG_CONFIG_FTYPE_ID, "Dialog file type for saving/loading configuration files", NULL, U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_REW_PATH_ID, "Dialog path for importing REW settings files"),
            INT_CONTROL_ALL(UI_DLG_REW_FTYPE_ID, "Dialog file type for importing REW settings files", NULL, U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_HYDROGEN_PATH_ID, "Dialog path for importing Hydrogen drumkit files"),
            INT_CONTROL_ALL(UI_DLG_HYDROGEN_FTYPE_ID, "Dialog file type for importing Hydrogen drumkit files", NULL, U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_LSPC_BUNDLE_PATH_ID, "Dialog path for exporting/importing LSPC bundles"),
            INT_CONTROL_ALL(UI_DLG_LSPC_BUNDLE_FTYPE_ID, "Dialog file type for exporting/importing LSPC bundles", NULL, U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_SFZ_PATH_ID, "Dialog path for exporting/importing SFZ files"),
            INT_CONTROL_ALL(UI_DLG_SFZ_FTYPE_ID, "Dialog file type for exporting/importing SFZ files", NULL, U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_MODEL3D_PATH_ID, "Dialog for saving/loading 3D model files"),
            INT_CONTROL_ALL(UI_DLG_MODEL3D_FTYPE_ID, "Dialog file type for saving/loading 3D model files", NULL, U_NONE, 0, 100, 0, 1),
            PATH(UI_DLG_DEFAULT_PATH_ID, "Dialog default path for other files"),
            INT_CONTROL_ALL(UI_DLG_DEFAULT_FTYPE_ID, "Dialog default file type for other files", NULL, U_NONE, 0, 100, 0, 1),
            PATH(UI_R3D_BACKEND_PORT_ID, "Identifier of selected backend for 3D rendering"),
            PATH(UI_LANGUAGE_PORT_ID, "Selected language identifier for the UI interface"),
            SWITCH(UI_REL_PATHS_PORT_ID, "Use relative paths when exporting configuration file", NULL, 0.0f),
            CONTROL_ALL(UI_SCALING_PORT_ID, "Manual UI scaling factor", NULL, U_PERCENT, 25.0f, 400.0f, 100.0f, 1.0f),
            SWITCH(UI_SCALING_HOST_PORT_ID, "Prefer host-reported UI scale factor", NULL, 1.0f),
            CONTROL_ALL(UI_FONT_SCALING_PORT_ID, "Manual UI font scaling factor", NULL, U_PERCENT, 50.0f, 400.0f, 100.0f, 1.0f),
            CONTROL_ALL(UI_BUNDLE_SCALING_PORT_ID, "Manual Bundle UI scaling factor", NULL, U_PERCENT, 0.0f, 400.0f, 0.0f, 1.0f),
            PATH(UI_VISUAL_SCHEMA_FILE_ID, "Current visual schema file used by the UI"),
            SWITCH(UI_PREVIEW_AUTO_PLAY_ID, "Enable automatic playback of the audio file in the file preview part of the file open dialog", NULL, 0.0f),
            SWITCH(UI_ENABLE_KNOB_SCALE_ACTIONS_ID, "Enable setting knob value by clicking on its scale feature", NULL, 1.0f),
            PATH(UI_USER_HYDROGEN_KIT_PATH_ID, "User Hydrogen kits path"),
            PATH(UI_OVERRIDE_HYDROGEN_KIT_PATH_ID, "Override Hydrogen kits path"),
            SWITCH(UI_OVERRIDE_HYDROGEN_KITS_ID, "Override Hydrogen kits", NULL, 1.0f),
            SWITCH(UI_INVERT_VSCROLL_ID, "Invert global mouse vertical scroll behaviour", NULL, 0.0f),
            SWITCH(UI_GRAPH_DOT_INVERT_VSCROLL_ID, "Invert mouse vertical scroll behaviour for graph dot widget", NULL, 0.0f),
            SWITCH(UI_ZOOMABLE_SPECTRUM_GRAPH_ID, "Zoom slider affects the frequency graph", NULL, 1.0f),
            COMBO(UI_FILTER_POINT_THICK_ID, "Thickness of the filter point", NULL, 1.0f, filter_point_thickness_modes),
            PATH(UI_DOCUMENTATION_PATH_ID, "Path to the local documentation installation"),
            SWITCH(UI_FILELIST_NAVIGATION_AUTOLOAD_ID, "Automatically load files when navigating over file list", NULL, 1.0f),
            SWITCH(UI_FILELIST_NAVIGATION_AUTOPLAY_ID, "Enable automatic playback of the audio file in the file navigator when selected", NULL, 0.0f),
            SWITCH(UI_TAKE_INST_NAME_FROM_FILE_ID, "Take instrument name from the name of loaded file", NULL, 0.0f),
            SWITCH(UI_SHOW_PIANO_LAYOUT_ON_GRAPH_ID, "Show piano keyboard layout on frequency graph", NULL, 1.0f),
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
        static constexpr ssize_t INVALID_PRESET_INDEX       = -1;

        static void mark_presets_as_favourite(lltl::darray<preset_t> *list, json::Array array, bool user)
        {
            const uint32_t mask = (user) ? PRESET_FLAG_USER : PRESET_FLAG_NONE;
            LSPString prest_name;

            for (size_t i=0, n=array.size(); i<n; ++i)
            {
                json::String node = array.get(i);
                if (node.get(&prest_name) != STATUS_OK)
                    continue;

                // Set the favourite flag for the associated preset
                for (size_t j=0, m=list->size(); j<m; ++j)
                {
                    preset_t *preset    = list->uget(j);
                    if (preset == NULL)
                        continue;
                    if ((preset->flags & PRESET_FLAG_USER) != mask)
                        continue;

                    if (preset->name.equals_nocase(&prest_name))
                    {
                        preset->flags      |= PRESET_FLAG_FAVOURITE;
                        break;
                    }
                }
            }
        }

        static status_t save_favourites_list(json::Serializer & json, const char *key, lltl::darray<preset_t> *list, size_t flags)
        {
            flags |= ui::PRESET_FLAG_FAVOURITE;

            status_t res = json.write_property(key);
            if (res != STATUS_OK)
                return res;
            if ((res = json.start_array()) != STATUS_OK)
                return res;
            {
                for (size_t i=0, n=list->size(); i<n; ++i)
                {
                    const preset_t *preset = list->uget(i);
                    if (preset == NULL)
                        continue;
                    if ((preset->flags & (ui::PRESET_FLAG_FAVOURITE | ui::PRESET_FLAG_USER)) != flags)
                        continue;

                    if ((res = json.write_string(&preset->name)) != STATUS_OK)
                        return res;
                }
            }

            return json.end_array();
        }

        IWrapper::IWrapper(Module *ui, resource::ILoader *loader)
        {
            pDisplay            = NULL;
            wWindow             = NULL;
            pWindow             = NULL;
            pUI                 = ui;
            pLoader             = loader;
            nFlags              = 0;
            nPlayPosition       = 0;
            nPlayLength         = 0;
            nActivePreset       = INVALID_PRESET_INDEX;
            nActivePresetData   = 0;
            enPresetTab         = PRESET_TAB_ALL;

            plug::position_t::init(&sPosition);

            for (size_t i=0; i<2; ++i)
                core::init_preset_data(&vPresetData[i]);
        }

        IWrapper::~IWrapper()
        {
            pDisplay            = NULL;
            pUI                 = NULL;
            pLoader             = NULL;
            nFlags              = 0;
        }

        void IWrapper::destroy()
        {
            // Destroy preset data
            for (size_t i=0; i<2; ++i)
                core::destroy_preset_data(&vPresetData[i]);

            // Flush list of playback listeners
            vPlayListeners.flush();

            // Flush list of preset listeners
            vPresetListeners.flush();
            destroy_presets(&vPresets);

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

            // Clear all evaluated ports
            lltl::parray<ui::IPort> evaluated;
            vEvaluated.values(&evaluated);
            vEvaluated.flush();

            for (size_t i=0, n=evaluated.size(); i<n; ++i)
            {
                ui::IPort *port = evaluated.uget(i);
                if (port != NULL)
                    delete port;
            }
            evaluated.flush();

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
            if ((res = get_user_config_path(&gconfig)) == STATUS_OK)
            {
                lsp_trace("User config path: %s", gconfig.as_utf8());
                if (res == STATUS_OK)
                    res = gconfig.append_child("lsp-plugins.cfg");
                if (res == STATUS_OK)
                    res = load_global_config(&gconfig);
            }
            else
                lsp_warn("Failed to obtain plugin configuration: error=%d", int(res));

            // Bind custom functions
            ctl::bind_functions(&sGlobalVars);

            return STATUS_OK;
        }

        status_t IWrapper::get_user_config_path(io::Path *path)
        {
            io::Path tmp;
            status_t res    = system::get_user_config_path(&tmp);
            if (res == STATUS_OK)
                res             = tmp.append_child("lsp-plugins");
            if (res == STATUS_OK)
                tmp.swap(path);

            return res;
        }

        status_t IWrapper::get_user_presets_path(io::Path *path)
        {
            io::Path tmp;
            status_t res    = get_user_config_path(&tmp);
            if (res == STATUS_OK)
                res             = tmp.append_child("presets");
            if (res == STATUS_OK)
                tmp.swap(path);

            return res;
        }

        status_t IWrapper::get_plugin_presets_path(io::Path *path)
        {
            const meta::plugin_t *meta = (pUI != NULL) ? pUI->metadata() : NULL;
            if (meta == NULL)
                return STATUS_BAD_STATE;

            // File name format: <config>/presets/<plugin-uid>/<preset-name>.[preset|patch]
            io::Path tmp;
            status_t res    = get_user_presets_path(&tmp);
            if (res == STATUS_OK)
                res             = tmp.append_child(meta->uid);

            if (res == STATUS_OK)
                tmp.swap(path);

            return res;
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

        void IWrapper::notify_write_to_kvt(core::KVTStorage *storage, const char *id, const core::kvt_param_t *value)
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

        void IWrapper::kvt_notify_write(core::KVTStorage *storage, const char *id, const core::kvt_param_t *value)
        {
            notify_write_to_kvt(storage, id, value);
            mark_active_preset_dirty();
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

            // Check for evaluated ports
            ui::IPort *port = vEvaluated.get(&key);
            if (port != NULL)
                return port;

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

        status_t IWrapper::add_evaluated_port(const char *id, ui::EvaluatedPort *port)
        {
            if ((id == NULL) || (port== NULL))
                return STATUS_BAD_ARGUMENTS;

            LSPString pid;
            if (!pid.set_utf8(id))
                return STATUS_NO_MEM;

            return register_evaluated_port(&pid, port);
        }

        status_t IWrapper::add_evaluated_port(const LSPString *id, ui::EvaluatedPort *port)
        {
            if ((id == NULL) || (port== NULL))
                return STATUS_BAD_ARGUMENTS;

            return register_evaluated_port(id, port);
        }

        status_t IWrapper::register_evaluated_port(const LSPString *id, ui::IPort *port)
        {
            if (!vEvaluated.create(id, port))
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
                status_t res = get_user_config_path(&path);
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

            if (nFlags & F_FAVOURITES_DIRTY)
            {
                // Save favourites list
                io::Path path;
                status_t res = get_plugin_presets_path(&path);
                if (res == STATUS_OK)
                    res = path.mkdir(true);
                if (res == STATUS_OK)
                    res = path.append_child("favourites.json");
                if (res == STATUS_OK)
                    res = save_favourites(&path);

                lsp_trace("Save favourites to %s: result=%d", path.as_native(), int(res));

                // Reset flags
                nFlags     &= ~F_FAVOURITES_DIRTY;
            }

            if (nFlags & F_PRESET_SYNC)
            {
                lsp_trace("Synchronizing preset state with backend");
                // Send new state to DSP
                core::preset_state_t state;
                init_preset_state(&state);

                state.flags     = core::PRESET_FLAG_NONE;
                state.tab       = enPresetTab;

                const preset_t *preset  = active_preset();
                const char *name        = (preset != NULL) ? preset->name.get_utf8() : NULL;
                if (name != NULL)
                {
                    const size_t bytes      = lsp_min(strlen(name), core::PRESET_NAME_BYTES - 1);
                    memcpy(state.name, name, bytes);
                    state.name[bytes]       = '\0';

                    if (preset->flags & ui::PRESET_FLAG_USER)
                        state.flags            |= core::PRESET_FLAG_USER;
                    if (nFlags & F_PRESET_DIRTY)
                        state.flags            |= core::PRESET_FLAG_DIRTY;
                }
                else
                    state.name[0]           = '\0';

                // Commit preset state
                send_preset_state(&state);
                nFlags     &= ~F_PRESET_SYNC;
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
            ctl::PluginWindow *wnd  = new ctl::PluginWindow(this, wWindow);
            if (wnd == NULL)
                return STATUS_NO_MEM;
            if ((res = wnd->init()) != STATUS_OK)
                return res;

            // Form the location of the resource
            LSPString xpath;
            if (xpath.fmt_utf8(LSP_BUILTIN_PREFIX "ui/%s", path) <= 0)
                return STATUS_NO_MEM;

            // Create context
            UIContext ctx(this, wnd->controllers(), wnd->widgets());
            if ((res = ctx.init()) != STATUS_OK)
                return res;

            // Parse the XML document
            xml::RootNode root(&ctx, "plugin", wnd);
            xml::Handler handler(resources());
            if ((res = handler.parse_resource(&xpath, &root)) != STATUS_OK)
                return res;

            // Append overlays to the window
            lltl::parray<ctl::Overlay> *overlays = ctx.overlays();
            for (size_t i=0, n=overlays->size(); i<n; ++i)
            {
                ctl::Overlay *ov = overlays->uget(i);
                if (ov == NULL)
                    continue;

                if ((res = wnd->add(&ctx, ov)) != STATUS_OK)
                    return res;
            }

            // Call post-initialization
            if ((res = wnd->post_init()) != STATUS_OK)
                return res;

            pWindow     = wnd;
            return STATUS_OK;
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
            if ((res = export_ports(s, NULL, &vPorts, basedir)) != STATUS_OK)
                return res;

            // Export KVT data
            core::KVTStorage *kvt = kvt_lock();
            if (kvt != NULL)
            {
                lsp_finally {
                    kvt->gc();
                    kvt_release();
                };
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
            c->fmt_append_utf8  ("  Package:                 %s (%s)\n", pkg->artifact, pkg->artifact_name);
            c->fmt_append_utf8  ("  Package version:         %s\n", pkv.get_utf8());
            c->fmt_append_utf8  ("  Plugin name:             %s (%s)\n", meta->name, meta->description);
            c->fmt_append_utf8  ("  Plugin version:          %d.%d.%d\n",
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

        bool IWrapper::update_parameters(lltl::pphash<LSPString, config::param_t> *parameters, ui::IPort *port)
        {
            if (parameters == NULL)
                return false;

            const meta::port_t *meta = port->metadata();
            if (meta == NULL)
                return false;

            // Version port
            LSPString key;
            if (!strcmp(meta->id, UI_LAST_VERSION_PORT_ID))
            {
                if (!meta::is_path_port(meta))
                    return false;

                // Get key and value
                get_bundle_version_key(&key);
                const char *value = static_cast<const char *>(port->buffer());
                if (value == NULL)
                    return false;

                // Update parameter
                config::param_t *p = new config::param_t();
                if (p == NULL)
                    return false;
                lsp_finally {
                    if (p != NULL)
                        delete p;
                };
                if (!p->set_string(value))
                    return false;

                // Replace value
                return parameters->put(&key, p, &p);
            }

            // Bundle scaling port
            if (!strcmp(meta->id, UI_BUNDLE_SCALING_PORT_ID))
            {
                if (!meta::is_control_port(meta))
                    return false;

                // Get key and value
                get_bundle_scaling_key(&key);
                const float value = port->value();

                // Update parameter
                config::param_t *p = new config::param_t();
                if (p == NULL)
                    return false;
                lsp_finally {
                    if (p != NULL)
                        delete p;
                };
                p->set_float(value);

                // Replace value
                return parameters->put(&key, p, &p);
            }

            return false;
        }

        status_t IWrapper::export_ports(
            config::Serializer *s,
            lltl::pphash<LSPString, config::param_t> *parameters,
            lltl::parray<IPort> *ports,
            const io::Path *basedir)
        {
            status_t res;
            float buf;
            const void *data;
            LSPString key;

            // Write port data
            for (size_t i=0, n=ports->size(); i<n; ++i)
            {
                IPort *p    = ports->uget(i);
                if (p == NULL)
                    continue;

                const meta::port_t *meta = p->metadata();
                if (meta == NULL)
                    continue;
                if (!meta::is_in_port(meta))
                    continue;

                if (update_parameters(parameters, p))
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
                res     = core::serialize_port_value(s, meta, data, basedir, 0);
                if (res != STATUS_BAD_TYPE)
                {
                    if (res != STATUS_OK)
                        return res;
                    if ((res = s->writeln()) != STATUS_OK)
                        return res;
                }

                // Remove serialized port from configuration
                if (parameters != NULL)
                {
                    if (!key.set_ascii(meta->id))
                        return STATUS_NO_MEM;
                    config::param_t *param = NULL;
                    if (parameters->remove(&key, &param))
                    {
                        if (param != NULL)
                            delete param;
                    }
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

        status_t IWrapper::export_parameters(config::Serializer *s, lltl::pphash<LSPString, config::param_t> *parameters)
        {
            status_t res;
            // Export all available bundle versions
            for (lltl::iterator<lltl::pair<LSPString, config::param_t>> it = parameters->items(); it; ++it)
            {
                const LSPString *key = it->key;
                const config::param_t *p = it->value;
                switch (p->type())
                {
                    case config::SF_TYPE_I32:
                        res     = s->write_i32(key, p->v.i32, p->flags);
                        break;
                    case config::SF_TYPE_U32:
                        res     = s->write_u32(key, p->v.u32, p->flags);
                        break;
                    case config::SF_TYPE_I64:
                        res     = s->write_i64(key, p->v.i64, p->flags);
                        break;
                    case config::SF_TYPE_U64:
                        res     = s->write_u64(key, p->v.u64, p->flags);
                        break;
                    case config::SF_TYPE_F32:
                        res     = s->write_f32(key, p->v.f32, p->flags);
                        break;
                    case config::SF_TYPE_F64:
                        res     = s->write_f64(key, p->v.f64, p->flags);
                        break;
                    case config::SF_TYPE_BOOL:
                        res     = s->write_bool(key, p->v.bval, p->flags);
                        break;
                    case config::SF_TYPE_STR:
                        res     = s->write_string(key, p->v.str, p->flags);
                        break;
                    case config::SF_TYPE_BLOB:
                        res     = s->write_blob(key, &p->v.blob, p->flags);
                        break;
                    default:
                        res     = STATUS_UNKNOWN_ERR;
                        break;
                }

                if (res != STATUS_OK)
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
                reset_settings();

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
                        notify_write_to_kvt(kvt, id, &kp);
                    }

                    // Free previously allocated data
                    if ((kp.type == core::KVT_BLOB) && (kp.blob.data != NULL))
                        free(const_cast<void *>(kp.blob.data));
                }
                else
                {
                    size_t port_flags = (flags & (IMPORT_FLAG_PRESET | IMPORT_FLAG_PATCH)) ?
                                    plug::PF_PRESET_IMPORT : plug::PF_STATE_IMPORT;

                    const char *pname = param.name.get_utf8();
                    ui::IPort *p = port_by_id(pname);
                    if (p != NULL)
                    {
                        if (set_port_value(p, &param, port_flags, basedir))
                            p->notify_all(ui::PORT_NONE);
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

        void IWrapper::get_bundle_scaling_key(LSPString *key)
        {
            LSPString scaling_key;

            const meta::plugin_t *meta = (pUI != NULL) ? pUI->metadata() : NULL;
            const meta::bundle_t *bundle = (meta != NULL) ? meta->bundle : NULL;
            const char *bundle_uid = (bundle != NULL) ? bundle->uid : NULL;

            if (bundle_uid != NULL)
            {
                scaling_key.set_utf8(bundle_uid);
                scaling_key.replace_all('-', '_');
                scaling_key.append_ascii("_ui_scaling");
            }
            else
                scaling_key.set_ascii(UI_BUNDLE_SCALING_PORT_ID);

            key->swap(&scaling_key);
        }

        status_t IWrapper::load_global_config(config::PullParser *parser)
        {
            status_t res;
            config::param_t param;
            LSPString version_key, scaling_key;

            // Get the proper name of the version parameter
            get_bundle_version_key(&version_key);
            get_bundle_scaling_key(&scaling_key);

            // Lock config update
            nFlags |= F_CONFIG_LOCK;

            while ((res = parser->next(&param)) == STATUS_OK)
            {
                // Skip raw last version parameter from legacy configuration files
                if (param.name.equals_ascii(UI_LAST_VERSION_PORT_ID))
                    continue;
                if (param.name.equals_ascii(UI_BUNDLE_SCALING_PORT_ID))
                    continue;

                // Determine parameter name
                const char *param_name =
                    (version_key.equals(&param.name)) ? UI_LAST_VERSION_PORT_ID :
                    (scaling_key.equals(&param.name)) ? UI_BUNDLE_SCALING_PORT_ID :
                    param.name.get_utf8();

                // Apply configuration if we found matching port
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

        void IWrapper::drop_parameters(lltl::pphash<LSPString, config::param_t> *versions)
        {
            lltl::parray<config::param_t> vv;
            versions->values(&vv);
            versions->clear();

            for (size_t i=0, n=vv.size(); i<n; ++i)
            {
                config::param_t *value = vv.uget(i);
                if (value != NULL)
                    delete value;
            }
        }

        status_t IWrapper::read_parameters(const io::Path *file, lltl::pphash<LSPString, config::param_t> *params)
        {
            config::PullParser parser;
            config::param_t param;
            status_t res;
            lltl::pphash<LSPString, config::param_t> tmp;
            lsp_finally { drop_parameters(&tmp); };

            if ((res = parser.open(file)) != STATUS_OK)
                return res;
            lsp_finally { parser.close(); };

            // Lock config update
            nFlags |= F_CONFIG_LOCK;

            while ((res = parser.next(&param)) == STATUS_OK)
            {
                // Add new value to hash
                config::param_t *value = new config::param_t();
                if (value == NULL)
                    return STATUS_NO_MEM;
                lsp_finally { delete value; };
                if (!value->copy(&param))
                    return STATUS_NO_MEM;

                // Put data to the mapping
                if (!tmp.put(&param.name, value, &value))
                    return STATUS_NO_MEM;
                if (value != NULL)
                    lsp_warn("Duplicate entry '%s' in configuration file", param.name.get_utf8());
            }
            if (res != STATUS_EOF)
                return res;

            // Unlock config update
            nFlags &= ~F_CONFIG_LOCK;

            // Commit changes
            params->swap(&tmp);

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
            lltl::pphash<LSPString, config::param_t> parameters;
            status_t res = read_parameters(file, &parameters);
            if ((res != STATUS_OK) && (res != STATUS_NOT_FOUND))
                return res;
            lsp_finally { drop_parameters(&parameters); };

            // Write new file
            if ((res = os.open(file, io::File::FM_WRITE_NEW)) != STATUS_OK)
                return res;

            // Wrap
            if ((res = o.wrap(&os, WRAP_CLOSE, "UTF-8")) != STATUS_OK)
            {
                os.close();
                return res;
            }

            // Export settings
            res = save_global_config(&o, &parameters);
            status_t res2 = o.close();

            return (res == STATUS_OK) ? res2 : res;
        }

        status_t IWrapper::save_global_config(io::IOutSequence *os, lltl::pphash<LSPString, config::param_t> *parameters)
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
            if ((res = export_ports(&s, parameters, &vConfigPorts, NULL)) != STATUS_OK)
                return res;

            // Export bundle versions
            res = s.write_comment(config_separator);
            if (res == STATUS_OK)
                res = s.write_comment("Recently used versions of bundles");
            if ((res = export_parameters(&s, parameters)) != STATUS_OK)
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

        void IWrapper::visual_schema_reloaded(const tk::StyleSheet *sheet)
        {
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
                case meta::R_SEND_NAME:
                case meta::R_RETURN_NAME:
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
            visual_schema_reloaded(sheet);

            return res;
        }

        status_t IWrapper::init_global_constants(const tk::StyleSheet *sheet)
        {
            status_t res;

            // Cleanup variables
            sGlobalVars.clear_vars();

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

            // Update preset settings
            nActivePreset       = -1;
            nFlags              = (nFlags & ~(F_PRESET_DIRTY)) | F_PRESET_SYNC;
            notify_presets_updated();

            // Notify UI
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

        /**
         * Get name of graphics backend
         * @return name of graphics backend
         */
        const char *IWrapper::graphics_backend() const
        {
            const ws::surface_type_t stype = (wWindow != NULL) ? wWindow->surface_type() : ws::ST_UNKNOWN;

            switch (stype)
            {
                case ws::ST_UNKNOWN:
                case ws::ST_IMAGE:
                case ws::ST_XLIB:
                case ws::ST_SIMILAR:
                    return "Cairo";

                case ws::ST_DDRAW:
                    return "Direct2D";

                case ws::ST_OPENGL:
                    return "OpenGL";

                default:
                    break;
            }

            return "Unknown";
        }

        status_t IWrapper::add_preset_listener(IPresetListener *listener)
        {
            if (listener == NULL)
                return STATUS_BAD_ARGUMENTS;

            if (vPresetListeners.contains(listener))
                return STATUS_ALREADY_EXISTS;

            const bool first_listener = vPresetListeners.is_empty();
            if (!vPresetListeners.add(listener))
                return STATUS_NO_MEM;

            if (first_listener)
                update_preset_list();

            // Notify listener about containing presets
            listener->presets_updated();

            return STATUS_OK;
        }

        status_t IWrapper::remove_preset_listener(IPresetListener *listener)
        {
            if (listener == NULL)
                return STATUS_BAD_ARGUMENTS;

            return (vPresetListeners.qpremove(listener)) ? STATUS_OK : STATUS_NOT_FOUND;
        }

        status_t IWrapper::select_active_preset(const preset_t *preset, bool force)
        {
            const ssize_t preset_id = vPresets.index_of(preset);
            if ((preset_id < 0) && (nActivePreset < 0))
            {
                nFlags              = (nFlags & (~F_PRESET_DIRTY));
                return STATUS_OK;
            }

            // Change current preset
            preset_t *pold  = (nActivePreset >= 0) ? vPresets.get(nActivePreset) : NULL;
            preset_t *pnew  = const_cast<ui::preset_t *>(preset);
            if ((pold == pnew) && (!force))
                return STATUS_OK;

            // Import preset settings
            status_t res        = STATUS_OK;
            if (preset != NULL)
            {
                const size_t flags  = (preset->flags & ui::PRESET_FLAG_PATCH) ? IMPORT_FLAG_PATCH : IMPORT_FLAG_PRESET;
                res                 = import_settings(&pnew->path, flags);
            }

            nActivePreset       = (pnew != NULL) ? preset_id : INVALID_PRESET_INDEX;
            nFlags              = (nFlags & (~F_PRESET_DIRTY)) | F_PRESET_SYNC;

            if (pold != NULL)
                notify_preset_deactivated(pold);
            if (pnew != NULL)
                notify_preset_activated(pnew);
            return res;
        }

        const preset_t *IWrapper::active_preset() const
        {
            return (nActivePreset >= 0) ? vPresets.get(nActivePreset) : NULL;
        }

        const preset_t *IWrapper::all_presets() const
        {
            return vPresets.array();
        }

        size_t IWrapper::num_presets() const
        {
            return vPresets.size();
        }

        void IWrapper::destroy_presets(lltl::darray<preset_t> *list)
        {
            for (size_t i=0, n=list->size(); i<n; ++i)
            {
                preset_t *preset = list->uget(i);
                if (preset != NULL)
                {
                    preset->name.~LSPString();
                    preset->path.~LSPString();
                }
            }
            list->flush();
        }

        preset_t *IWrapper::add_preset(lltl::darray<preset_t> *list)
        {
            preset_t *preset    = list->add();
            if (preset == NULL)
                return NULL;

            new (&preset->name, inplace_new_tag_t()) LSPString();
            new (&preset->path, inplace_new_tag_t()) LSPString();
            preset->flags       = 0;

            return preset;
        }

        void IWrapper::scan_factory_presets(lltl::darray<preset_t> *list)
        {
            const meta::plugin_t *meta = (pUI != NULL) ? pUI->metadata() : NULL;
            if ((meta == NULL) || (meta->ui_presets == NULL))
                return;

            lltl::darray<resource::resource_t> presets;
            if (core::scan_presets(&presets, resources(), meta->ui_presets) != STATUS_OK)
                return;

            io::Path path;
            LSPString tmp;
            preset_t tmp_preset;

            for (size_t i=0, n=presets.size(); i<n; ++i)
            {
                // Enumerate next backend information
                const resource::resource_t *preset = presets.uget(i);
                if (preset == NULL)
                    continue;

                if (path.set(preset->name) != STATUS_OK)
                    continue;

                // Get name of the preset/patch without an extension
                if (path.get_last_noext(&tmp_preset.name) != STATUS_OK)
                    continue;
                if (tmp_preset.path.fmt_utf8(LSP_BUILTIN_PREFIX "presets/%s/%s", meta->ui_presets, preset->name) <= 0)
                    continue;
                if (path.get_ext(&tmp) != STATUS_OK)
                    continue;

                // Fill flags
                tmp_preset.flags    = PRESET_FLAG_NONE;
                if (tmp.equals_ascii_nocase("patch"))
                    tmp_preset.flags   |= PRESET_FLAG_PATCH;
                else if (!tmp.equals_ascii_nocase("preset"))
                    continue;

                // Create preset item
                preset_t *item      = add_preset(list);
                if (item == NULL)
                    continue;

                item->name.swap(tmp_preset.name);
                item->path.swap(tmp_preset.path);
                item->flags     = tmp_preset.flags;
            }
        }

        void IWrapper::scan_user_presets(lltl::darray<preset_t> *list)
        {
            // File name format: <config>/presets/<plugin-uid>/<preset-name>.[preset|patch]
            io::Path path;
            if (get_plugin_presets_path(&path) != STATUS_OK)
                return;

            io::Dir dir;
            if (dir.open(&path) != STATUS_OK)
                return;
            lsp_finally { dir.close(); };

            io::Path preset;
            io::fattr_t fattr;
            LSPString tmp;
            preset_t tmp_preset;

            while (dir.reads(&preset, &fattr, true) == STATUS_OK)
            {
                // We process only regular files
                if (fattr.type != io::fattr_t::FT_REGULAR)
                    continue;

                // Get name of the preset/patch without an extension
                if (preset.get_last_noext(&tmp_preset.name) != STATUS_OK)
                    continue;
                if (preset.get(&tmp_preset.path) != STATUS_OK)
                    continue;
                if (preset.get_ext(&tmp) != STATUS_OK)
                    continue;

                // Fill flags
                tmp_preset.flags    = PRESET_FLAG_USER;
                if (tmp.equals_ascii_nocase("patch"))
                    tmp_preset.flags   |= PRESET_FLAG_PATCH;
                else if (!tmp.equals_ascii_nocase("preset"))
                    continue;

                // Create preset item
                preset_t *item      = add_preset(list);
                if (item == NULL)
                    continue;

                item->name.swap(tmp_preset.name);
                item->path.swap(tmp_preset.path);
                item->flags     = tmp_preset.flags;
            }
        }

        void IWrapper::scan_favourite_presets(lltl::darray<preset_t> *list)
        {
            const meta::plugin_t *meta = (pUI != NULL) ? pUI->metadata() : NULL;
            if ((meta == NULL) || (meta->ui_presets == NULL))
                return;

            // File name format: <config>/presets/<plugin-uid>/favourites.json
            io::Path path;
            if (get_plugin_presets_path(&path) != STATUS_OK)
                return;
            if (path.append_child("favourites.json") != STATUS_OK)
                return;

            // Load configuration file
            json::Object config;
            if (json::dom_load(&path, &config, json::JSON_VERSION5) != STATUS_OK)
                return;
            if (!config.valid())
                return;

            json::Array factory = config.get("factory");
            if (factory.valid())
                mark_presets_as_favourite(list, factory, false);

            json::Array user = config.get("user");
            if (user.valid())
                mark_presets_as_favourite(list, user, true);
        }

        status_t IWrapper::save_favourites(const io::Path *path)
        {
            json::Serializer json;
            json::serial_flags_t flags;

            flags.version       = json::JSON_VERSION5;
            flags.identifiers   = false;
            flags.ident         = ' ';
            flags.padding       = 4;
            flags.separator     = true;
            flags.multiline     = true;

            status_t res = json.open(path, &flags);
            if (res != STATUS_OK)
                return res;
            lsp_finally {
                json.close();
            };

            if ((res = json.start_object()) != STATUS_OK)
                return res;
            {
                if ((res = save_favourites_list(json, "factory", &vPresets, ui::PRESET_FLAG_NONE)) != STATUS_OK)
                    return res;
                if ((res = save_favourites_list(json, "user", &vPresets, ui::PRESET_FLAG_USER)) != STATUS_OK)
                    return res;
            }
            if ((res = json.end_object()) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        void IWrapper::select_presets(lltl::darray<preset_t> *list, const preset_t *active)
        {
            list->qsort(preset_compare_function);
            nActivePreset       = INVALID_PRESET_INDEX;

            for (size_t i=0, n=list->size(); i<n; ++i)
            {
                preset_t *preset    = list->uget(i);
                if ((active != NULL) && (preset_compare_function(preset, active) == 0))
                    nActivePreset       = i;
            }
        }

        void IWrapper::update_preset_list()
        {
            const preset_t *active = active_preset();

            lltl::darray<preset_t> presets;
            lsp_finally { destroy_presets(&presets); };

            scan_factory_presets(&presets);
            scan_user_presets(&presets);
            scan_favourite_presets(&presets);
            select_presets(&presets, active);

            presets.swap(&vPresets);
        }

        void IWrapper::scan_presets()
        {
            // First, update the list of presets
            update_preset_list();
            notify_presets_updated();
        }

        void IWrapper::notify_presets_updated()
        {
            // Notify listeners about the change of whole list of presets
            lltl::parray<IPresetListener> listeners;
            if (listeners.add(vPresetListeners))
            {
                for (size_t i=0, n=listeners.size(); i<n; ++i)
                {
                    IPresetListener *listener = listeners.uget(i);
                    if (listener != NULL)
                        listener->presets_updated();
                }
            }
        }

        void IWrapper::notify_preset_deactivated(const ui::preset_t *preset)
        {
            // Notify listeners about the activation of the preset
            lltl::parray<IPresetListener> listeners;
            if (listeners.add(vPresetListeners))
            {
                for (size_t i=0, n=listeners.size(); i<n; ++i)
                {
                    IPresetListener *listener = listeners.uget(i);
                    if (listener != NULL)
                        listener->preset_deactivated(preset);
                }
            }
        }

        void IWrapper::notify_preset_activated(const ui::preset_t *preset)
        {
            // Notify listeners about the activation of the preset
            lltl::parray<IPresetListener> listeners;
            if (listeners.add(vPresetListeners))
            {
                for (size_t i=0, n=listeners.size(); i<n; ++i)
                {
                    IPresetListener *listener = listeners.uget(i);
                    if (listener != NULL)
                        listener->preset_activated(preset);
                }
            }
        }

        void IWrapper::mark_active_preset_dirty()
        {
            // Set flag only when there is an active preset
            const preset_t *preset = active_preset();
            if ((preset != NULL) && (!(nFlags & F_PRESET_DIRTY)))
            {
                nFlags         |= F_PRESET_DIRTY | F_PRESET_SYNC;
                notify_presets_updated();
            }

            // Mark currently selected A/B preset state as dirty
            vPresetData[nActivePresetData].dirty    = true;
        }

        bool IWrapper::active_preset_dirty() const
        {
            if (!(nFlags & F_PRESET_DIRTY))
                return false;
            return active_preset() != NULL;
        }

        preset_tab_t IWrapper::preset_tab() const
        {
            return enPresetTab;
        }

        void IWrapper::set_preset_tab(preset_tab_t tab)
        {
            enPresetTab         = tab;
            nFlags             |= F_PRESET_SYNC;
        }

        status_t IWrapper::mark_preset_favourite(const preset_t *src_preset, bool favourite)
        {
            if (!vPresets.contains(src_preset))
                return STATUS_NOT_FOUND;
            preset_t *preset = const_cast<preset_t *>(src_preset);
            if (preset == NULL)
                return STATUS_NOT_FOUND;

            const uint32_t new_flags    = lsp_setflag(preset->flags, PRESET_FLAG_FAVOURITE, favourite);
            if (new_flags == preset->flags)
                return STATUS_OK;

            preset->flags   = new_flags;
            nFlags         |= F_FAVOURITES_DIRTY; // Favourites list needs to be synchronized

            notify_presets_updated();

            return STATUS_OK;
        }

        status_t IWrapper::remove_preset(const preset_t *src_preset)
        {
            ssize_t preset_id = vPresets.index_of(src_preset);
            if (preset_id < 0)
                return STATUS_NOT_FOUND;

            preset_t *preset = const_cast<preset_t *>(src_preset);
            if (preset == NULL)
                return STATUS_NOT_FOUND;

            if (!(preset->flags & PRESET_FLAG_USER))
                return STATUS_INVALID_VALUE;

            // Delete original file
            io::Path path;
            status_t res    = path.set(&preset->path);
            if (res == STATUS_OK)
                res         = path.remove();
            if (res != STATUS_OK)
                return res;

            // Remove preset from list
            const uint32_t flags = preset->flags;
            if (!vPresets.remove(preset_id))
                return STATUS_NO_MEM;

            // Update related state
            if (flags & PRESET_FLAG_FAVOURITE)
                nFlags             |= F_FAVOURITES_DIRTY; // Favourites list needs to be synchronized

            if (nActivePreset == ssize_t(preset_id))
            {
                nActivePreset       = INVALID_PRESET_INDEX;
                nFlags             |= F_PRESET_SYNC;
            }

            // Notify listeners
            notify_presets_updated();
            return STATUS_OK;
        }

        status_t IWrapper::allocate_temp_file(io::Path *dst, const io::Path *src)
        {
            const char *spath = src->as_utf8();
            for (int i=0; ; ++i)
            {
                if (dst->fmt("%s.%d", spath, i) <= 0)
                    return STATUS_NO_MEM;
                if (!dst->exists())
                    break;
            }

            return STATUS_OK;
        }

        preset_t *IWrapper::find_preset(const LSPString *name, bool user)
        {
            const bool mask = (user) ? ui::PRESET_FLAG_USER : ui::PRESET_FLAG_NONE;
            for (size_t i=0, n=vPresets.size(); i<n; ++i)
            {
                preset_t *p = vPresets.uget(i);
                if (p == NULL)
                    continue;
                if (((p->flags & ui::PRESET_FLAG_USER) == mask) && (p->name.equals_nocase(name)))
                    return p;
            }

            return NULL;
        }

        status_t IWrapper::save_preset(const LSPString *name, size_t flags)
        {
            // Check preset name
            if (!(flags & ui::PRESET_FLAG_USER))
                return STATUS_INVALID_VALUE;

            // Get location of presets
            io::Path base;
            status_t res = get_plugin_presets_path(&base);
            if (res != STATUS_OK)
                return res;

            res = base.mkdir(true);
            if ((res != STATUS_OK) && (res != STATUS_ALREADY_EXISTS))
                return res;

            // Find the previous preset
            io::Path old_file, new_file, temp_file;

            preset_t *preset = find_preset(name, true);
            bool was_favourite = false;
            if (preset != NULL)
            {
                if ((res = old_file.set(&preset->path)) != STATUS_OK)
                    return res;
                was_favourite   = preset->flags & ui::PRESET_FLAG_FAVOURITE;
            }

            // Create new preset item
            if (new_file.fmt("%s/%s.preset", base.as_utf8(), name->get_utf8()) <= 0)
                return STATUS_NO_MEM;
            if (!old_file.is_empty())
            {
                res = allocate_temp_file(&temp_file, &new_file);
                if (res != STATUS_OK)
                    return res;
            }

            ui::preset_t create;
            if (!create.name.set(name))
                return STATUS_NO_MEM;
            if ((res = new_file.get(&create.path)) != STATUS_OK)
                return res;
            create.flags        = (flags & ui::PRESET_FLAG_FAVOURITE) | ui::PRESET_FLAG_USER;

            // Now we are ready to export configuration
            if (!old_file.is_empty())
            {
                // Export settings
                lsp_trace("Write preset to temporary file %s", temp_file.as_utf8());
                if ((res = export_settings(&temp_file, true)) != STATUS_OK)
                    return res;

                // Replace previous file with new
                lsp_trace("Remove old preset file %s", old_file.as_utf8());
                if ((res = old_file.remove()) != STATUS_OK)
                    return res;

                lsp_trace("Rename temporary file %s to %s", temp_file.as_utf8(), new_file.as_utf8());
                if ((res = temp_file.rename(&new_file)) != STATUS_OK)
                    return res;
            }
            else
            {
                // Export settings
                lsp_trace("Write preset to new file %s", new_file.as_utf8());
                if ((res = export_settings(&new_file, true)) != STATUS_OK)
                    return res;

                // Allocate new record
                const preset_t *base = vPresets.array();
                preset = add_preset(&vPresets);
                if (preset == NULL)
                {
                    // The internal data structure of vPresets may change address,
                    // so we need to notify clients
                    if (base != vPresets.array())
                        notify_presets_updated();
                    return STATUS_NO_MEM;
                }
            }

            // Fill data structure
            preset->flags   = create.flags;
            preset->name.swap(create.name);
            preset->path.swap(create.path);

            const bool is_favourite = flags & ui::PRESET_FLAG_FAVOURITE;
            if (was_favourite != is_favourite)
                nFlags         |= F_FAVOURITES_DIRTY;       // Favourites list needs to be synchronized

            // Sort list of presets
            vPresets.qsort(preset_compare_function);
            preset = find_preset(name, true);
            if (preset == NULL)
                return STATUS_UNKNOWN_ERR;

            nActivePreset   = vPresets.index_of(preset);
            nFlags          = (nFlags & (~F_PRESET_DIRTY)) | F_PRESET_SYNC;

            // Notify listeners about presets changes
            notify_presets_updated();

            return STATUS_OK;
        }

        void IWrapper::send_preset_state(const core::preset_state_t *state)
        {
            // Implement for wrapper
        }

        void IWrapper::receive_preset_state(const core::preset_state_t *state)
        {
            LSPString name;
            if (!name.set_utf8(state->name))
                return;

            size_t flags    = nFlags & ~(F_PRESET_DIRTY | F_PRESET_SYNC);
            if (state->flags & core::PRESET_FLAG_DIRTY)
                flags          |= F_PRESET_DIRTY;

            const preset_t *preset  = (name.is_empty()) ? NULL : find_preset(&name, state->flags & core::PRESET_FLAG_USER);
            const ssize_t active    = (preset != NULL) ? vPresets.index_of(preset) : INVALID_PRESET_INDEX;

            if ((flags == nFlags) && (active == nActivePreset) && (state->tab == uint32_t(enPresetTab)))
                return;

            nFlags          = flags;
            nActivePreset   = active;
            enPresetTab     = preset_tab_t(lsp_min(state->tab, uint32_t(ui::PRESET_TAB_TOTAL - 1)));

            // Notify listeners about presets changes
            notify_presets_updated();
        }

        size_t IWrapper::active_preset_data() const
        {
            return nActivePresetData;
        }

        status_t IWrapper::copy_preset_data()
        {
            core::preset_data_t *inactive   = &vPresetData[(nActivePresetData + 1) % 2];
            return serialize_state(inactive);
        }

        status_t IWrapper::switch_preset_data()
        {
            status_t res;
            core::preset_data_t *active     = &vPresetData[nActivePresetData];
            core::preset_data_t *inactive   = &vPresetData[(nActivePresetData + 1) % 2];

            // Check if we need to serialize current state
            if ((active->empty) || (active->dirty))
            {
                if ((res = serialize_state(active)) != STATUS_OK)
                    return res;
            }

            // Now load new state
            if (!inactive->empty)
            {
                if ((res = deserialize_state(inactive)) != STATUS_OK)
                    return res;
            }
            else
            {
                if ((res = reset_settings()) != STATUS_OK)
                    return res;
            }

            // Switch preset state
            nActivePresetData   = (nActivePresetData + 1) % 2;

            // Set preset dirty flag only when there is an active preset
            const preset_t *preset = active_preset();
            if ((preset != NULL) && (!(nFlags & F_PRESET_DIRTY)))
            {
                nFlags         |= F_PRESET_DIRTY | F_PRESET_SYNC;
                notify_presets_updated();
            }

            return STATUS_OK;
        }

        status_t IWrapper::serialize_state(core::preset_data_t *dst)
        {
            // Write header
            status_t res;
            core::preset_data_t data;
            core::init_preset_data(&data);
            lsp_finally {
                core::destroy_preset_data(&data);
            };

            // Serialize regular ports
            for (lltl::iterator<ui::IPort> it=vPorts.values(); it; ++it)
            {
                ui::IPort *p    = it.get();
                if (p == NULL)
                    continue;

                const meta::port_t *meta = p->metadata();
                if ((meta == NULL) || (!meta::is_in_port(meta)))
                    continue;

                core::kvt_param_t param;
                switch (meta->role)
                {
                    case meta::R_CONTROL:
                    case meta::R_PORT_SET:
                    case meta::R_BYPASS:
                        param.type      = core::KVT_FLOAT32;
                        param.f32       = p->value();
                        break;

                    case meta::R_SEND_NAME:
                    case meta::R_RETURN_NAME:
                    case meta::R_STRING:
                    case meta::R_PATH:
                        param.type      = core::KVT_STRING;
                        param.str       = p->buffer<const char>();
                        break;

                    default:
                        param.type      = core::KVT_ANY;
                        break;
                }
                if (param.type == core::KVT_ANY)
                    continue;

                if ((res = core::add_preset_data_param(&data, meta->id, &param)) != STATUS_OK)
                    return res;
            }

            // Serialize KVT data
            core::KVTStorage *kvt = kvt_lock();
            if (kvt != NULL)
            {
                lsp_finally {
                    kvt->gc();
                    kvt_release();
                };

                // Emit the whole list of KVT parameters
                for (core::KVTIterator *iter = kvt->enum_all();
                    (iter != NULL) && (iter->next() == STATUS_OK);)
                {
                    const core::kvt_param_t *kvt_param = NULL;

                    // Get KVT parameter
                    res = iter->get(&kvt_param);
                    if (res == STATUS_NOT_FOUND)
                        continue;
                    else if (res != STATUS_OK)
                    {
                        lsp_warn("Could not get KVT parameter: code=%d", int(res));
                        break;
                    }

                    // Skip transient and private parameters
                    if ((iter->is_transient()) || (iter->is_private()))
                        continue;


                    if ((res = core::add_preset_data_param(&data, iter->name(), kvt_param)) != STATUS_OK)
                        return res;
                }
            }

            data.values.swap(dst->values);
            dst->empty  = false;
            dst->dirty  = false;

            return res;
        }

        status_t IWrapper::deserialize_state(const core::preset_data_t *src)
        {
            // Apply regular parameters
            lltl::parray<ui::IPort> notify;

            for (lltl::iterator<const core::preset_param_t> it=src->values.values(); it; ++it)
            {
                const core::preset_param_t *param = it.get();
                if (param->name[0] == '/')
                    continue;

                ui::IPort *p    = port_by_id(param->name);
                if (p == NULL)
                    continue;

                const meta::port_t *meta = p->metadata();
                if ((meta == NULL) || (!meta::is_in_port(meta)))
                    continue;

                switch (meta->role)
                {
                    case meta::R_CONTROL:
                    case meta::R_PORT_SET:
                    case meta::R_BYPASS:
                        if (param->value.type != core::KVT_FLOAT32)
                            continue;
                        p->set_value(param->value.f32);
                        p->notify_all(ui::PORT_NONE);
                        break;

                    case meta::R_SEND_NAME:
                    case meta::R_RETURN_NAME:
                    case meta::R_STRING:
                    case meta::R_PATH:
                        if (param->value.type != core::KVT_STRING)
                            continue;

                        p->write(param->value.str, strlen(param->value.str));
                        p->notify_all(ui::PORT_NONE);
                        break;

                    default:
                        break;
                }
            }

            // Deserialize KVT data
            core::KVTStorage *kvt = kvt_lock();
            if (kvt != NULL)
            {
                lsp_finally {
                    kvt->gc();
                    kvt_release();
                };

                for (lltl::iterator<const core::preset_param_t> it=src->values.values(); it; ++it)
                {
                    const core::preset_param_t *param = it.get();
                    if (param->name[0] != '/')
                        continue;

                    if (param->value.type != core::KVT_ANY)
                    {
                        kvt->put(param->name, &param->value, core::KVT_RX);
                        notify_write_to_kvt(kvt, param->name, &param->value);
                    }
                }
            }

            return STATUS_OK;
        }

    } /* namespace ui */
} /* namespace lsp */


