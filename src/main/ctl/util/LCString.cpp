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
            bEvaluate   = false;
        }

        LCString::~LCString()
        {
            do_destroy();
        }

        void LCString::do_destroy()
        {
            for (lltl::iterator<param_t> it=vParams.values(); it; ++it)
            {
                param_t *p = *it;
                if (p != NULL)
                    delete p;
            }
            vParams.flush();
        }

        void LCString::init(ui::IWrapper *wrapper, tk::String *prop)
        {
            pWrapper    = wrapper;
            pProp       = prop;
        }

        void LCString::destroy()
        {
            do_destroy();
        }

        bool LCString::add_parameter(const char *name, const char *expr)
        {
            // If we do not evaluate parameters, just add the parameter as string
            param_t *p      = new param_t;
            if (p == NULL)
                return false;

            // Remember the expression
            if (!vParams.create(name, p))
            {
                delete p;
                pProp->params()->add_cstring(name, expr);
                return false;
            }

            p->sValue.set_utf8(expr);
            p->bInitialized = false;
            if (!bEvaluate)
            {
                pProp->params()->set_string(name, &p->sValue);
                return true;
            }

            // Initialize and parse the expression
            p->sExpr.init(pWrapper, this);
            p->bInitialized = true;
            if (!p->sExpr.parse(&p->sValue))
            {
                pProp->params()->add_string(name, &p->sValue);
                return false;
            }

            // Evaluate the value
            expr::value_t value;
            expr::init_value(&value);
            lsp_finally { expr::destroy_value(&value); };

            // Set the value
            if (p->sExpr.evaluate(&value) == STATUS_OK)
                pProp->params()->set(name, &value);
            else
                pProp->params()->set_string(name, &p->sValue);

            return true;
        }

        bool LCString::init_expressions()
        {
            // Process all bound expressions
            size_t updated = 0;

            // Evaluated value
            expr::value_t value;
            expr::init_value(&value);
            lsp_finally { expr::destroy_value(&value); };

            for (lltl::iterator<lltl::pair<char, param_t>> it=vParams.items(); it; ++it)
            {
                param_t *p = it->value;

                // Initialize and parse the expression
                if (p->bInitialized)
                    continue;

                p->sExpr.init(pWrapper, this);
                if (!p->sExpr.parse(&p->sValue))
                    continue;
                p->bInitialized = true;

                // Evaluate the expression
                if (p->sExpr.evaluate(&value) == STATUS_OK)
                    pProp->params()->set(it->key, &value);
                else
                    pProp->params()->set_string(it->key, &p->sValue);
                ++updated;
            }

            return updated > 0;
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
            {
                const char *param_name = &name[1];
                if (strlen(param_name) <= 0)
                    return false;

                return add_parameter(param_name, value);
            }
            else if (name[0] == '\0')  // Key?
            {
                if (strchr(value, '.') == NULL) // Raw value with high probability?
                    pProp->set_raw(value);
                else
                    pProp->set_key(value);
            }
            else if ((!strcmp(name, ".meta")) || (!strcmp(name, ".metadata")))
            {
                if (!strcasecmp(value, "true"))
                    bind_metadata(pProp->params());
            }
            else if ((!strcmp(name, ".eval")) || (!strcmp(name, ".evaluate")))
            {
                if (!strcasecmp(value, "true"))
                {
                    bEvaluate = true;
                    init_expressions();
                }
            }
            else
                return false;

            return true;
        }

        void LCString::update_text(ui::IPort *port)
        {
            // Evaluated value
            expr::value_t value;
            expr::init_value(&value);
            lsp_finally { expr::destroy_value(&value); };

            // Process all bound expressions
            for (lltl::iterator<lltl::pair<char, param_t>> it=vParams.items(); it; ++it)
            {
                param_t *p = it->value;

                if (!p->sExpr.depends(port))
                    continue;

                if (p->sExpr.evaluate(&value) == STATUS_OK)
                    pProp->params()->set(it->key, &value);
                else
                    pProp->params()->set_string(it->key, &p->sValue);
            }
        }

        void LCString::bind_metadata(expr::Parameters *params)
        {
            LSPString tmp;
            const meta::package_t *package = pWrapper->package();
            const meta::plugin_t *plugin = pWrapper->ui()->metadata();

            // Bind package meta information
            params->set_cstring("meta_pkg_artifact", package->artifact);
            params->set_cstring("meta_pkg_artifact_name", package->artifact_name);
            params->set_cstring("meta_pkg_brand", package->brand);
            params->set_cstring("meta_pkg_copyright", package->copyright);
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

        void LCString::notify(ui::IPort *port, size_t flags)
        {
            update_text(port);
        }

        void LCString::sync_metadata(ui::IPort *port)
        {
            update_text(port);
        }

    } /* namespace ctl */
} /* namespace lsp */


