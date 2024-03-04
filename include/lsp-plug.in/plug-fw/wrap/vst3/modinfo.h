/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 февр. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_MODINFO_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_MODINFO_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/fmt/json/Serializer.h>
#include <lsp-plug.in/io/IOutStream.h>
#include <lsp-plug.in/io/IOutSequence.h>
#include <lsp-plug.in/io/OutSequence.h>
#include <lsp-plug.in/io/Path.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        static inline status_t make_moduleinfo(json::Serializer *s, const meta::package_t *package);
        static inline status_t make_moduleinfo(const char *path, const meta::package_t *package);
        static inline status_t make_moduleinfo(const io::Path *path, const meta::package_t *package);
        static inline status_t make_moduleinfo(const LSPString *path, const meta::package_t *package);
        static inline status_t make_moduleinfo(io::IOutStream *os, const meta::package_t *package);
        static inline status_t make_moduleinfo(io::IOutSequence *os, const meta::package_t *package);

        static inline void init_make_moduleinfo_flags(json::serial_flags_t & flags)
        {
            json::init_serial_flags(&flags);
            flags.padding   = 1;
            flags.ident     = '\t';
            flags.separator = true;
            flags.multiline = true;
        }

        static inline status_t make_moduleinfo(io::IOutStream *os, const meta::package_t *package)
        {
            json::serial_flags_t flags;
            json::Serializer s;

            init_make_moduleinfo_flags(flags);
            status_t res = s.wrap(os, &flags, WRAP_NONE, "UTF-8");
            lsp_finally { s.close(); };
            if (res == STATUS_OK)
                res         = make_moduleinfo(&s, package);
            if (res == STATUS_OK)
                res         = s.close();

            return res;
        }

        static inline status_t make_moduleinfo(io::IOutSequence *os, const meta::package_t *package)
        {
            json::serial_flags_t flags;
            json::Serializer s;

            init_make_moduleinfo_flags(flags);
            status_t res = s.wrap(os, &flags, WRAP_NONE);
            lsp_finally { s.close(); };
            if (res == STATUS_OK)
                res         = make_moduleinfo(&s, package);
            if (res == STATUS_OK)
                res         = s.close();

            return res;
        }

        static inline status_t make_moduleinfo(LSPString *path, const meta::package_t *package)
        {
            json::serial_flags_t flags;
            json::Serializer s;

            init_make_moduleinfo_flags(flags);
            status_t res = s.open(path, &flags, "UTF-8");
            lsp_finally { s.close(); };
            if (res == STATUS_OK)
                res         = make_moduleinfo(&s, package);
            if (res == STATUS_OK)
                res         = s.close();

            return res;
        }

        static inline status_t make_moduleinfo(const char *path, const meta::package_t *package)
        {
            json::serial_flags_t flags;
            json::Serializer s;

            init_make_moduleinfo_flags(flags);
            status_t res = s.open(path, &flags, "UTF-8");
            lsp_finally { s.close(); };
            if (res == STATUS_OK)
                res         = make_moduleinfo(&s, package);
            if (res == STATUS_OK)
                res         = s.close();

            return res;
        }

        static inline status_t make_moduleinfo(const io::Path *path, const meta::package_t *package)
        {
            json::serial_flags_t flags;
            json::Serializer s;

            init_make_moduleinfo_flags(flags);
            status_t res = s.open(path, &flags, "UTF-8");
            lsp_finally { s.close(); };
            if (res == STATUS_OK)
                res         = make_moduleinfo(&s, package);
            if (res == STATUS_OK)
                res         = s.close();

            return res;
        }

        static inline status_t make_moduleinfo(const LSPString *path, const meta::package_t *package)
        {
            json::serial_flags_t flags;
            json::Serializer s;

            init_make_moduleinfo_flags(flags);
            status_t res = s.open(path, &flags, "UTF-8");
            lsp_finally { s.close(); };
            if (res == STATUS_OK)
                res         = make_moduleinfo(&s, package);
            if (res == STATUS_OK)
                res         = s.close();

            return res;
        }

        static inline ssize_t cmp_plugin_metadata(const meta::plugin_t *a, const meta::plugin_t *b)
        {
            return strcmp(a->uid, b->uid);
        }

        static inline status_t make_moduleinfo(json::Serializer *s, const meta::package_t *pkg)
        {
            // Enumerate all plugins and sort in ascending order of UID
            lltl::parray<meta::plugin_t> plugins;
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *plug_meta = f->enumerate(i);
                    if (plug_meta == NULL)
                        break;
                    if (!plugins.add(const_cast<meta::plugin_t *>(plug_meta)))
                        return STATUS_NO_MEM;
                }
            }
            plugins.qsort(cmp_plugin_metadata);


        #define SA(x) LSP_STATUS_ASSERT(x)

            SA(s->start_object());
            {
                Steinberg::TUID cid;
                char vst3_uid[40], vst3_legacy_uid[40];
                LSPString pkgver, plugver, vendor;
                pkgver.fmt_ascii("%d.%d.%d",
                    int(pkg->version.major),
                    int(pkg->version.minor),
                    int(pkg->version.micro)
                );
                if (pkg->version.branch)
                    pkgver.fmt_append_utf8("-%s", pkg->version.branch);
                vendor.fmt_utf8("%s VST3", pkg->brand);

                const char *site    = (pkg->site != NULL) ? pkg->site : "";
                const char *email   = (pkg->email != NULL) ? pkg->email : "";

                SA(s->prop_string("Name", pkg->artifact));
                SA(s->prop_string("Version", &pkgver));

                // Factory info
                SA(s->write_property("Factory Info"));
                SA(s->start_object());
                {
                    SA(s->prop_string("Vendor", pkg->brand));
                    SA(s->prop_string("URL", site));
                    SA(s->prop_string("E-Mail", email));
                    SA(s->write_property("Flags"));
                    SA(s->start_object());
                    {
                        constexpr Steinberg::int32 flags = Steinberg::Vst::kDefaultFactoryFlags;

                        SA(s->prop_bool("Classes Discardable", flags & Steinberg::PFactoryInfo::kClassesDiscardable));
                        SA(s->prop_bool("License Check", flags & Steinberg::PFactoryInfo::kLicenseCheck));
                        SA(s->prop_bool("Component Non Discardable", flags & Steinberg::PFactoryInfo::kComponentNonDiscardable));
                        SA(s->prop_bool("Unicode", flags & Steinberg::PFactoryInfo::kUnicode));
                    }
                    SA(s->end_object());
                }
                SA(s->end_object());

                // Classes
                SA(s->write_property("Classes"));
                SA(s->start_array());
                {
                    // Processor classes
                    for (lltl::iterator<meta::plugin_t> it = plugins.values(); it; ++it)
                    {
                        // Enumerate next element
                        const meta::plugin_t *plug_meta = it.get();
                        if (plug_meta->vst3_uid == NULL)
                            continue;

                        // Create plugin record
                        SA(s->start_object());
                        {
                            if (!meta::uid_vst3_to_tuid(cid, plug_meta->vst3_uid))
                                return STATUS_BAD_FORMAT;
                            plugver.fmt_ascii(
                                "%d.%d.%d",
                                int(plug_meta->version.major),
                                int(plug_meta->version.minor),
                                int(plug_meta->version.micro));

                            SA(s->prop_string("CID", meta::uid_tuid_to_vst3(vst3_uid, cid)));
                            SA(s->prop_string("Catetory", kVstAudioEffectClass));
                            SA(s->prop_string("Name", plug_meta->description));
                            SA(s->prop_string("Vendor", &vendor));
                            SA(s->prop_string("Version", &plugver));
                            SA(s->prop_string("SDKVersion", Steinberg::Vst::SDKVersionString));
                            SA(s->write_property("Sub Categories"));
                            SA(s->start_array());
                            {
                                // Form list of plugin categories
                                LSPString cat, tmp;
                                status_t res = make_plugin_categories(&cat, plug_meta);
                                if (res != STATUS_OK)
                                    return res;

                                // Form the list of categories
                                size_t prev = 0, end = cat.length();
                                ssize_t curr;
                                while ((curr = cat.index_of(prev, '|')) > 0)
                                {
                                    if (!tmp.set(&cat, prev, curr))
                                        return STATUS_NO_MEM;
                                    SA(s->write_string(&tmp));
                                    prev    = curr + 1;
                                }
                                if (prev < end)
                                {
                                    if (!tmp.set(&cat, prev, end))
                                        return STATUS_NO_MEM;
                                    SA(s->write_string(&tmp));
                                }
                            }
                            SA(s->end_array());

                            SA(s->prop_int("Class Flags", Steinberg::Vst::kDistributable));
                            SA(s->prop_int("Cardinality", Steinberg::PClassInfo::kManyInstances));
                            SA(s->write_property("Snapshots"));
                            SA(s->start_array());
                            SA(s->end_array());
                        }
                        SA(s->end_object());
                    }

                    // Controller classes
                    for (lltl::iterator<meta::plugin_t> it = plugins.values(); it; ++it)
                    {
                        // Enumerate next element
                        const meta::plugin_t *ui_meta = it.get();
                        if (ui_meta == NULL)
                            break;
                        if ((ui_meta->vst3_uid == NULL)|| (ui_meta->vst3ui_uid == NULL))
                            continue;

                        // Create plugin record
                        SA(s->start_object());
                        {
                            if (!meta::uid_vst3_to_tuid(cid, ui_meta->vst3ui_uid))
                                return STATUS_BAD_FORMAT;
                            plugver.fmt_ascii(
                                "%d.%d.%d",
                                int(ui_meta->version.major),
                                int(ui_meta->version.minor),
                                int(ui_meta->version.micro));

                            SA(s->prop_string("CID", meta::uid_tuid_to_vst3(vst3_uid, cid)));
                            SA(s->prop_string("Catetory", kVstComponentControllerClass));
                            SA(s->prop_string("Name", ui_meta->description));
                            SA(s->prop_string("Vendor", &vendor));
                            SA(s->prop_string("Version", &plugver));
                            SA(s->prop_string("SDKVersion", Steinberg::Vst::SDKVersionString));
                            SA(s->prop_int("Class Flags", Steinberg::Vst::kDistributable));
                            SA(s->prop_int("Cardinality", Steinberg::PClassInfo::kManyInstances));
                            SA(s->write_property("Snapshots"));
                            SA(s->start_array());
                            SA(s->end_array());
                        }
                        SA(s->end_object());
                    }
                }
                SA(s->end_array());

                // Compatibility
                SA(s->write_property("Compatibility"));
                SA(s->start_array());
                {
                    // Processor classes
                    for (lltl::iterator<meta::plugin_t> it = plugins.values(); it; ++it)
                    {
                        // Enumerate next element
                        const meta::plugin_t *plug_meta = it.get();
                        if (plug_meta == NULL)
                            break;
                        if ((plug_meta->vst3_uid == NULL) || (plug_meta->vst2_uid == NULL))
                            continue;
                        const char *plugin_name = (plug_meta->vst2_name != NULL) ? plug_meta->vst2_name : plug_meta->name;

                        if (!meta::uid_meta_to_vst3(vst3_uid, plug_meta->vst3_uid))
                            continue;
                        if (!meta::uid_vst2_to_vst3(vst3_legacy_uid, plug_meta->vst2_uid, plugin_name))
                            continue;
                        if (!strcmp(vst3_uid, vst3_legacy_uid))
                            continue;

                        // Create plugin record
                        SA(s->start_object());
                        {
                            // New
                            SA(s->prop_string("New", vst3_uid));

                            // Old
                            SA(s->write_property("Old"));
                            SA(s->start_array());
                            {
                                SA(s->write_string(vst3_legacy_uid));
                            }
                            SA(s->end_array());
                        }
                        SA(s->end_object());
                    }
                }
                SA(s->end_array());
            }
            SA(s->end_object());

        #undef SA
            return STATUS_OK;
        }

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_MODINFO_H_ */
