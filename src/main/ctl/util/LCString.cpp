/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 23 мая 2021 г.
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
#include <lsp-plug.in/plug-fw/meta/func.h>

namespace lsp
{
    namespace ctl
    {
        LCString::LCString()
        {
            pWrapper    = NULL;
            pProp       = NULL;
        }

        void LCString::init(ui::IWrapper *wrapper, tk::String *prop)
        {
            pWrapper    = wrapper;
            pProp       = prop;
        }

        bool LCString::set(const char *param, const char *name, const char *value)
        {
            if ((pWrapper == NULL) || (pProp == NULL))
                return false;

            // Does the prefix match?
            size_t len = ::strlen(param);
            if (strncmp(name, param, len))
                return false;
            name += len;

            // Analyze next character
            if (name[0] == ':') // Parameter ("prefix:")?
                pProp->params()->add_cstring(&name[1], value);
            else if (name[0] == '\0')  // Key?
            {
                if (strchr(value, '.') == NULL) // Raw value with high probability?
                    pProp->set_raw(value);
                else
                    pProp->set_key(value);
            }
            else if ((!strcmp(name, ".meta")) || (!strcmp(name, ".metadata")))
            {
                float flag = 0.0f;
                if (meta::parse_bool(&flag, value) == STATUS_OK)
                {
                    if (flag >= 0.5f)
                        bind_metadata(pProp->params());
                }
            }
            else
                return false;

            return true;
        }

        void LCString::bind_metadata(expr::Parameters *params)
        {
            LSPString tmp;
            const meta::package_t *package = pWrapper->package();
            const meta::plugin_t *plugin = pWrapper->ui()->metadata();

            // Bind package meta information
            params->set_cstring("meta_pkg_artifact", package->artifact);
            params->set_cstring("meta_pkg_brand", package->brand);
            params->set_cstring("meta_pkg_short_name", package->short_name);
            params->set_cstring("meta_pkg_full_name", package->full_name);
            params->set_cstring("meta_pkg_site", package->site);
            params->set_cstring("meta_pkg_license", package->license);
            tmp.fmt_utf8("%d.%d.%d",
                package->version.major,
                package->version.minor,
                package->version.micro
            );
            if (package->version.branch)
                tmp.fmt_append_utf8("-%s", package->version.branch);
            params->set_string ("meta_pkg_version", &tmp);

            // Bind plugin meta information
            params->set_cstring("meta_plugin_name", plugin->name);
            params->set_cstring("meta_plugin_description", plugin->description);
            params->set_cstring("meta_plugin_acronym", plugin->acronym);
            params->set_cstring("meta_plugin_developer_name", plugin->developer->name);
            params->set_cstring("meta_plugin_developer_nick", plugin->developer->nick);
            params->set_cstring("meta_plugin_developer_site", plugin->developer->homepage);
            params->set_cstring("meta_plugin_developer_mail", plugin->developer->mailbox);
            params->set_cstring("meta_plugin_uid", plugin->uid);
            params->set_cstring("meta_plugin_lv2_uri", plugin->lv2_uri);
            params->set_cstring("meta_plugin_lv2ui_uri", plugin->lv2ui_uri);
            params->set_cstring("meta_plugin_vst2_uid", plugin->vst2_uid);
            params->set_int    ("meta_plugin_ladspa_id", plugin->ladspa_id);
            params->set_cstring("meta_plugin_ladspa_lbl", plugin->ladspa_lbl);

            tmp.fmt_utf8("%d.%d.%d",
                int(LSP_MODULE_VERSION_MAJOR(plugin->version)),
                int(LSP_MODULE_VERSION_MINOR(plugin->version)),
                int(LSP_MODULE_VERSION_MICRO(plugin->version))
            );
            params->set_string ("meta_plugin_version", &tmp);
        }
    }
}


