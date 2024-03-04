/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/fmt/json/Serializer.h>

namespace lsp
{
    namespace pluglist_gen
    {
        static status_t json_out_property(json::Serializer *s, const char *field, const char *value)
        {
            status_t res;
            if ((res = s->write_property(field)) != STATUS_OK)
                return res;
            return s->write_string(value);
        }

        static status_t json_out_property(json::Serializer *s, const char *field, const LSPString *value)
        {
            status_t res;
            if ((res = s->write_property(field)) != STATUS_OK)
                return res;
            return s->write_string(value);
        }

        status_t json_write_plugin_groups(json::Serializer *s, const int *c)
        {
            status_t res;

            if ((res = s->start_array()) != STATUS_OK)
                return res;

            for (; (c != NULL) && ((*c) >= 0); ++c)
            {
                const char *grp = get_enumeration(*c, plugin_groups);
                if (grp != NULL)
                {
                    if ((res = s->write_string(grp)) != STATUS_OK)
                        return res;
                }
            }

            if ((res = s->end_array()) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        static status_t json_write_package(json::Serializer *s, const meta::package_t *package)
        {
            status_t res;
            LSPString tmp;

            tmp.fmt_utf8("%d.%d.%d",
                package->version.major,
                package->version.minor,
                package->version.micro
            );
            if (package->version.branch)
                tmp.fmt_append_utf8("-%s", package->version.branch);


            // artifact
            if ((res = json_out_property(s, "artifact", package->artifact)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "name", package->artifact_name)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "version", &tmp)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "brand", package->brand)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "short", package->short_name)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "full", package->full_name)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "license", package->license)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "copyright", package->copyright)) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        static status_t json_write_bundle(json::Serializer *s, const meta::bundle_t *bundle)
        {
            status_t res;

            // artifact
            if ((res = json_out_property(s, "id", bundle->uid)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "name", bundle->name)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "group", get_enumeration(bundle->group, bundle_groups))) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "video", bundle->video_id)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "description", bundle->description)) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        status_t json_write_plugin(json::Serializer *s, const meta::plugin_t *m)
        {
            status_t res;
            LSPString tmp;
            char vst3_uid[40];

            tmp.fmt_utf8("%d.%d.%d",
                int(LSP_MODULE_VERSION_MAJOR(m->version)),
                int(LSP_MODULE_VERSION_MINOR(m->version)),
                int(LSP_MODULE_VERSION_MICRO(m->version))
            );

            if ((res = json_out_property(s, "id", m->uid)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "name", m->name)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "author", m->developer->name)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "version", &tmp)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "bundle", (m->bundle) ? m->bundle->uid : NULL)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "description", m->description)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "acronym", m->acronym)) != STATUS_OK)
                return res;

            if (m->ladspa_id > 0)
            {
                if ((res = s->write_property("ladspa_uid")) != STATUS_OK)
                    return res;
                if ((res = s->write_int(m->ladspa_id)) != STATUS_OK)
                    return res;
                if ((res = json_out_property(s, "ladspa_label", m->ladspa_lbl)) != STATUS_OK)
                    return res;
            }
            else
            {
                if ((res = s->write_property("ladspa_uid")) != STATUS_OK)
                    return res;
                if ((res = s->write_null()) != STATUS_OK)
                    return res;
                if ((res = json_out_property(s, "ladspa_label", (const char *)NULL)) != STATUS_OK)
                    return res;
            }

            if ((res = json_out_property(s, "lv2_uri", m->lv2_uri)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "lv2ui_uri", m->lv2ui_uri)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "vst2_uid", m->vst2_uid)) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "vst3_uid", meta::uid_meta_to_vst3(vst3_uid, m->vst3_uid))) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "vst3ui_uid", meta::uid_meta_to_vst3(vst3_uid, m->vst3ui_uid))) != STATUS_OK)
                return res;
            if ((res = json_out_property(s, "clap_uid", m->clap_uid)) != STATUS_OK)
                return res;
            if ((res = s->write_property("jack")) != STATUS_OK)
                return res;
            if ((res = s->write_bool(true)) != STATUS_OK)
                return res;
            if ((res = s->write_property("groups")) != STATUS_OK)
                return res;
            if ((res = json_write_plugin_groups(s, m->classes)) != STATUS_OK)
                return res;


            return STATUS_OK;
        }

        status_t json_write_document(
            json::Serializer *s,
            const meta::package_t *package,
            const meta::plugin_t * const *plugins,
            size_t count)
        {
            status_t res;
            if ((res = s->start_object()) != STATUS_OK)
                return res;
            {
                // Emit package
                if ((res = s->write_property("package")) != STATUS_OK)
                    return res;
                if ((res = s->start_object()) != STATUS_OK)
                    return res;
                {
                    if ((res = json_write_package(s, package)) != STATUS_OK)
                        return res;
                }
                if ((res = s->end_object()) != STATUS_OK)
                    return res;

                // Emit bundle list
                if ((res = s->write_property("bundles")) != STATUS_OK)
                    return res;
                if ((res = s->start_array()) != STATUS_OK)
                    return res;
                {
                    lltl::parray<meta::bundle_t> bundles;
                    if ((res = enum_bundles(&bundles, plugins, count)) != STATUS_OK)
                        return res;

                    for (size_t i=0; i<bundles.size(); ++i)
                    {
                        if ((res = s->start_object()) != STATUS_OK)
                            return res;
                        {
                            if ((res = json_write_bundle(s, bundles.uget(i))) != STATUS_OK)
                                return res;
                        }
                        if ((res = s->end_object()) != STATUS_OK)
                            return res;
                    }
                }
                if ((res = s->end_array()) != STATUS_OK)
                    return res;

                // Emit plugin list
                if ((res = s->write_property("plugins")) != STATUS_OK)
                    return res;
                if ((res = s->start_array()) != STATUS_OK)
                    return res;
                {
                    for (size_t i=0; i<count; ++i)
                    {
                        if ((res = s->start_object()) != STATUS_OK)
                            return res;
                        {
                            if ((res = json_write_plugin(s, plugins[i])) != STATUS_OK)
                                return res;
                        }
                        if ((res = s->end_object()) != STATUS_OK)
                            return res;
                    }
                }
                if ((res = s->end_array()) != STATUS_OK)
                    return res;
            }
            if ((res = s->end_object()) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        status_t generate_json(
            const char *file,
            const meta::package_t *package,
            const meta::plugin_t * const *plugins,
            size_t count)
        {
            status_t res;
            json::Serializer s;
            json::serial_flags_t flags;

            json::init_serial_flags(&flags);
            flags.ident     = '\t';
            flags.multiline = true;
            flags.padding   = 1;

            // Generate PHP file
            if ((res = s.open(file, &flags, "UTF-8")) != STATUS_OK)
            {
                fprintf(stderr, "Error writing file: %s\n", file);
                return res;
            }

            printf("Writing file %s\n", file);

            res = json_write_document(&s, package, plugins, count);
            res = update_status(res, s.close());

            return res;
        }

    } /* namespace pluglist_gen */
} /* namespace lsp */






