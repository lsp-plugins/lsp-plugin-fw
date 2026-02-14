/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 февр. 2026 г.
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
#include <lsp-plug.in/common/debug.h>

namespace lsp
{
    namespace ctl
    {
        //-----------------------------------------------------------------
        static const char * manual_prefixes[] =
        {
        #ifdef LSP_LIB_PREFIX
            LSP_LIB_PREFIX("/share"),
            LSP_LIB_PREFIX("/local/share"),
            LSP_LIB_PREFIX("/usr/local/share"),
        #endif /*  LSP_LIB_PREFIX */
            "/usr/share",
            "/usr/local/share",
            "/share",
            NULL
        };

        Documentation::Documentation(ui::IWrapper *wrapper)
        {
            pWrapper        = wrapper;
        }

        Documentation::~Documentation()
        {
        }

        status_t Documentation::init()
        {
            return STATUS_OK;
        }

        void Documentation::destroy()
        {
        }

        void Documentation::read_path_param(LSPString *value, const char *port_id)
        {
            ui::IPort *p = pWrapper->port(port_id);
            const char *path = NULL;
            if ((p != NULL) && (meta::is_path_port(p->metadata())))
                path = p->buffer<const char>();
            value->set_utf8((path != NULL) ? path : "");
        }

        bool Documentation::open_manual_file(const char *fmt...)
        {
            va_list vl;
            va_start(vl, fmt);
            lsp_finally{ va_end(vl); };

            // Form the path
            io::Path path;
            LSPString spath;
            ssize_t res = path.vfmt(fmt, vl);
            if (res <= 0)
                return false;

            if (path.is_empty())
                return false;

            lsp_trace("Checking path: %s", path.as_utf8());

            if (!path.exists())
                return false;

            if (!spath.fmt_utf8("file://%s", path.as_utf8()))
                return false;

            return system::follow_url(&spath) == STATUS_OK;
        }

        status_t Documentation::show_plugin_manual(const meta::plugin_t *plugin)
        {
            const meta::plugin_t *meta = (plugin != NULL) ? plugin : pWrapper->metadata();
            if (meta == NULL)
                return STATUS_NOT_FOUND;

            io::Path path;
            LSPString spath;
            status_t res;

            // Check that documentation path is present
            read_path_param(&spath, UI_DOCUMENTATION_PORT);
            if (!spath.is_empty())
            {
                if (open_manual_file("%s/html/plugins/%s.html", spath.get_utf8(), meta->uid))
                    return STATUS_OK;
            }

            // Try to open local documentation
            for (const char **prefix = manual_prefixes; *prefix != NULL; ++prefix)
            {
                if (open_manual_file("%s/doc/%s/html/plugins/%s.html", *prefix, "lsp-plugins", meta->uid))
                    return STATUS_OK;
            }

            // Follow the online documentation
            if (spath.fmt_utf8("%s?page=manuals&section=%s", "https://lsp-plug.in/", meta->uid))
            {
                if ((res = system::follow_url(&spath)) == STATUS_OK)
                    return res;
            }

            return STATUS_NOT_FOUND;
        }

        status_t Documentation::show_ui_manual()
        {
            io::Path path;
            LSPString spath;
            status_t res;

            // Check that documentation path is present
            read_path_param(&spath, UI_DOCUMENTATION_PORT);
            if (!spath.is_empty())
            {
                if (open_manual_file("%s/html/controls.html", spath.get_utf8()))
                    return STATUS_OK;
            }

            // Try to open local documentation
            for (const char **prefix = manual_prefixes; *prefix != NULL; ++prefix)
            {
                if (open_manual_file("%s/doc/%s/html/controls.html", *prefix, "lsp-plugins"))
                    return STATUS_OK;
            }

            // Follow the online documentation
            if (spath.fmt_utf8("%s?page=manuals&section=controls", "https://lsp-plug.in/"))
            {
                if ((res = system::follow_url(&spath)) == STATUS_OK)
                    return res;
            }

            return STATUS_NOT_FOUND;
        }
    } /* namespace ctl */
} /* namespace lsp */


