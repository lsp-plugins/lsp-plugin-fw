/*
 * Copyright (C) 2022 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2022 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 7 февр. 2022 г.
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


#include <lsp-plug.in/plug-fw/util/pluglist_gen/pluglist_gen.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>
#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace pluglist_gen
    {
        static const char *php_escape(LSPString &buf, const LSPString *value)
        {
            if (value == NULL)
                return "null";

            buf.clear();
            for (size_t i=0, n=value->length(); i<n; ++i)
            {
                lsp_wchar_t ch = value->at(i);
                switch (ch)
                {
                    case '\n':
                        buf.append('\\');
                        ch = 'n';
                        break;
                    case '\r':
                        buf.append('\\');
                        ch = 'r';
                        break;
                    case '\t':
                        buf.append('\\');
                        ch = 't';
                        break;

                    case '\'':
                    case '\"':
                    case '$':
                    case '\\':
                        buf.append('\\');
                        break;
                }
                buf.append(ch);
            }

            return buf.get_utf8();
        }

        static const char *php_escape(LSPString &buf, const char *value)
        {
            if (value == NULL)
                return "null";

            LSPString tmp;
            if (!tmp.set_utf8(value))
                return NULL;

            return php_escape(buf, &tmp);
        }

        void php_print_plugin_groups(FILE *out, const int *c)
        {
            fprintf(out, "array(");
            size_t items = 0;

            for ( ; (c != NULL) && ((*c) >= 0) ; ++c)
            {
                const char *grp = get_enumeration(*c, plugin_groups);
                if (grp != NULL)
                {
                    if ((items++) > 0)
                        fprintf(out, ", ");
                    fprintf(out, "\"%s\"", grp);
                }
            }

            fprintf(out, ")");
        }

        void php_write_plugin_descriptor(FILE *out, const meta::plugin_t *m)
        {
            LSPString tmp, buf;

            tmp.fmt_utf8("%d.%d.%d",
                int(LSP_MODULE_VERSION_MAJOR(m->version)),
                int(LSP_MODULE_VERSION_MINOR(m->version)),
                int(LSP_MODULE_VERSION_MICRO(m->version))
            );

            fprintf(out, "\t\t\t'id' => \"%s\",\n", php_escape(buf, m->uid));
            fprintf(out, "\t\t\t'name' => \"%s\",\n", php_escape(buf, m->name));
            fprintf(out, "\t\t\t'author' => \"%s\",\n", php_escape(buf, m->developer->name));
            fprintf(out, "\t\t\t'version' => \"%s\",\n", php_escape(buf, &tmp));
            if ((m->bundle) && (m->bundle->uid))
                fprintf(out, "\t\t\t'bundle' => \"%s\",\n", php_escape(buf, m->bundle->uid));
            else
                fprintf(out, "\t\t\t'bundle' => null,\n");
            fprintf(out, "\t\t\t'description' => \"%s\",\n", php_escape(buf, m->description));
            fprintf(out, "\t\t\t'acronym' => \"%s\",\n", php_escape(buf, m->acronym));

            if (m->ladspa_id > 0)
            {
                fprintf(out, "\t\t\t'ladspa_uid' => '%ld',\n", long(m->ladspa_id));
                fprintf(out, "\t\t\t'ladspa_label' => \"%s\",\n", php_escape(buf, m->ladspa_lbl));
            }
            else
            {
                fprintf(out, "\t\t\t'ladspa_uid' => null,\n");
                fprintf(out, "\t\t\t'ladspa_label' => null,\n");
            }

            fprintf(out, "\t\t\t'lv2_uri' => \"%s\",\n", php_escape(buf, m->lv2_uri));
            fprintf(out, "\t\t\t'lv2ui_uri' => \"%s\",\n", php_escape(buf, m->lv2ui_uri));
            fprintf(out, "\t\t\t'vst2_uid' => \"%s\",\n", php_escape(buf, m->vst2_uid));
            fprintf(out, "\t\t\t'jack' => true,\n");
            fprintf(out, "\t\t\t'groups' => ");
            php_print_plugin_groups(out, m->classes);
            fprintf(out, "\n");
        }

        static void php_write_bundle(FILE *out, const meta::bundle_t *bundle)
        {
            LSPString buf;
            fprintf(out, "\t\t\t'id' => \"%s\",\n", php_escape(buf, bundle->uid));
            fprintf(out, "\t\t\t'name' => \"%s\",\n", php_escape(buf, bundle->name));
            fprintf(out, "\t\t\t'group' => \"%s\",\n", get_enumeration(bundle->group, bundle_groups));
            fprintf(out, "\t\t\t'video' => \"%s\",\n", php_escape(buf, bundle->video_id));
            fprintf(out, "\t\t\t'description' => \"%s\"\n", php_escape(buf, bundle->description));
        }

        static void php_write_package_info(FILE *out, const meta::package_t *package)
        {
            LSPString tmp, buf;

            tmp.fmt_utf8("%d.%d.%d",
                package->version.major,
                package->version.minor,
                package->version.micro
            );
            if (package->version.branch)
                tmp.fmt_append_utf8("-%s", package->version.branch);

            fprintf(out, "\t\t'artifact' => \"%s\",\n", php_escape(buf, package->artifact));
            fprintf(out, "\t\t'name' => \"%s\",\n", php_escape(buf, package->artifact_name));
            fprintf(out, "\t\t'version' => \"%s\",\n", php_escape(buf, &tmp));
            fprintf(out, "\t\t'brand' => \"%s\",\n", php_escape(buf, package->brand));
            fprintf(out, "\t\t'short' => \"%s\",\n", php_escape(buf, package->short_name));
            fprintf(out, "\t\t'full' => \"%s\",\n", php_escape(buf, package->full_name));
            fprintf(out, "\t\t'license' => \"%s\",\n", php_escape(buf, package->license));
            fprintf(out, "\t\t'copyright' => \"%s\"\n", php_escape(buf, package->copyright));
        }

        status_t generate_php(
            const char *file,
            const meta::package_t *package,
            const meta::plugin_t * const *plugins,
            size_t count)
        {
            status_t res;

            // Generate PHP file
            FILE *out;
            if (!(out = fopen(file, "w+"))) {
                fprintf(stderr, "Error writing file %s\n", file);
                return STATUS_IO_ERROR;
            }
            printf("Writing file %s\n", file);

            // Write PHP header
            fprintf(out, "<?php\n");
            fprintf(out, "\n");

            // Write package information
            fprintf(out, "\t$PACKAGE = array(\n");
            php_write_package_info(out, package);
            fprintf(out, "\t);\n\n");

            // Write bundle information
            fprintf(out, "\t$BUNDLES = array(\n");
            lltl::parray<meta::bundle_t> bundles;
            if ((res = enum_bundles(&bundles, plugins, count)) != STATUS_OK)
            {
                fclose(out);
                return res;
            }
            for (size_t i=0; i<bundles.size(); ++i)
            {
                if (i > 0)
                    fprintf(out, ",\n");
                fprintf(out, "\t\tarray(\n");
                php_write_bundle(out, bundles.uget(i));
                fprintf(out, "\t\t)");
            }

            fprintf(out, "\t);\n\n");

            // Output plugins
            fprintf(out, "\t$PLUGINS = array(\n");
            for (size_t i=0; i<count; ++i)
            {
                if (i > 0)
                    fprintf(out, ",\n");
                fprintf(out, "\t\tarray(\n");
                php_write_plugin_descriptor(out, plugins[i]);
                fprintf(out, "\t\t)");
            }

            fprintf(out, "\n\t);\n");

            // Write PHP footer
            fprintf(out, "?>\n");

            fclose(out);

            return STATUS_OK;
        }

    } /* namespace pluglist_gen */
} /* namespace lsp */



