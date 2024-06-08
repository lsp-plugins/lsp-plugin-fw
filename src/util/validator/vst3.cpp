/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 янв. 2024 г.
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

#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/util/validator/validator.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>
#include <ctype.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace validator
    {
        namespace vst3
        {
//            static void make_vst3_id(char *dst, const char *prefix, const char *model, const char *vst2_code)
//            {
//                size_t i;
//                for (i=0; (i<4) && (prefix[i] != 0); ++i)
//                    *(dst++)    = prefix[i];
//                for (; i<4; ++i)
//                    *(dst++)    = ' ';
//
//                for (i=0; (i<8) && (model[i] != 0); ++i)
//                    *(dst++)    = tolower(model[i]);
//                for (; i<8; ++i)
//                    *(dst++)    = ' ';
//
//                for (i=0; (i<4) && (vst2_code[i] != 0); ++i)
//                    *(dst++)    = vst2_code[i];
//                for (; i<4; ++i)
//                    *(dst++)    = ' ';
//
//                *dst        = '\0';
//            }

            void validate_package(context_t *ctx, const meta::package_t *pkg)
            {
                // Validate vendor string ASCII
                const size_t vendor_len = strlen(pkg->brand) + strlen(" VST3");
                if (vendor_len >= Steinberg::PFactoryInfo::kNameSize)
                    validation_error(ctx, "Manifest has too long VST 3 vendor name '%s VST' generated from '%s', of %d characters, but only PFactoryInfo::kNameSize=%d characters are permitted",
                        pkg->brand, pkg->brand, int(vendor_len), int(Steinberg::PFactoryInfo::kNameSize-1));
                if (vendor_len >= Steinberg::PClassInfo2::kVendorSize)
                    validation_error(ctx, "Manifest has too long VST 3 vendor name '%s VST' generated from '%s', of %d characters, but only PClassInfo2::kVendorSize=%d characters are permitted",
                        pkg->brand, pkg->brand, int(vendor_len), int(Steinberg::PClassInfo2::kVendorSize-1));

                // Validate vendor string UTF-16
                char tmp[Steinberg::PClassInfo2::kVendorSize];
                snprintf(tmp, sizeof(tmp), "%s VST3", pkg->brand);
                Steinberg::char16 u16_vendor[Steinberg::PClassInfoW::kVendorSize];
                if (!utf8_to_utf16(lsp::vst3::to_utf16(u16_vendor), tmp, sizeof(u16_vendor)/sizeof(Steinberg::char16)))
                    validation_error(ctx, "Manifest has too long VST 3 UTF-16 encoded vendor name '%s VST' generated from '%s', only PClassInfoW::kVendorSize=%d characters are permitted",
                        pkg->brand, pkg->brand, int(Steinberg::PClassInfoW::kVendorSize-1));

                // Validate URL
                if (pkg->site != NULL)
                {
                    const size_t url_len = strlen(pkg->site);
                    if (url_len >= Steinberg::PFactoryInfo::kURLSize)
                        validation_error(ctx, "Manifest has too long VST 3 site URL '%s' of %d characters, only PFactoryInfo::kURLSize=%d characters are permitted",
                            pkg->site, int(url_len), int(Steinberg::PFactoryInfo::kURLSize-1));
                }

                // Validate email
                if (pkg->email != NULL)
                {
                    const size_t url_len = strlen(pkg->email);
                    if (url_len >= Steinberg::PFactoryInfo::kEmailSize)
                        validation_error(ctx, "Manifest has too long VST 3 EMail address '%s' of %d characters, only PFactoryInfo::kEmailSize=%d characters are permitted",
                            pkg->email, int(url_len), int(Steinberg::PFactoryInfo::kEmailSize-1));
                }
            }

            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {
                status_t res;
                const meta::plugin_fmt_uids_t *uids = &meta->uids;

                // Does plugin support VST 3.x?
                if (uids->vst3 == NULL)
                {
                    if (uids->vst3ui != NULL)
                        validation_error(ctx, "Plugin uid='%s', VST3 UI identifier is defined but VST3 identifier is missing", meta->uid);
                    return;
                }

                // Validate IDs of processor and controller
                Steinberg::TUID tuid;
                char vst3_iid[40], vst3_legacy_iid[40];
                const char *vst3_id = NULL, *vst3_legacy_id = NULL;
                const char *plugin_name = (meta->vst2_name != NULL) ? meta->vst2_name : meta->name;

//                make_vst3_id(vst3_iid, "dsp", meta->acronym, meta->vst2_uid);
//                if (strcmp(vst3_iid, meta->vst3_uid) != 0)
//                    validation_error(ctx, "Plugin uid='%s', vst3_uid='%s' but expected to be vst3_uid='%s'",
//                        meta->uid, meta->vst3_uid, vst3_iid);
//
//                make_vst3_id(vst3_iid, "ui", meta->acronym, meta->vst2_uid);
//                if (strcmp(vst3_iid, meta->vst3ui_uid) != 0)
//                    validation_error(ctx, "Plugin UI uid='%s', vst3_uid='%s' but expected to be vst3_uid='%s'",
//                        meta->uid, meta->vst3ui_uid, vst3_iid);

                // Process VST3 identifier for processor (DSP)
                if (!meta::uid_vst3_to_tuid(tuid, uids->vst3))
                    validation_error(ctx, "Plugin uid='%s', failed to parse VST3 processor (DSP) TUID '%s': should be string of 16 ASCII characters or string of 32 hexadecimal characters",
                        meta->uid, uids->vst3);

                if ((vst3_id = meta::uid_tuid_to_vst3(vst3_iid, tuid)) != NULL)
                {
                    // Check VST3 processor identifier conflicts
                    const meta::plugin_t *clash = ctx->vst3_ids.get(vst3_id);
                    if (clash != NULL)
                    {
                        validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate VST 3 processor (DSP) identifier '%s' ('%s')",
                            meta->uid, clash->uid, uids->vst3, vst3_id);
                    }
                    else if (!ctx->vst3_ids.create(vst3_id, const_cast<meta::plugin_t *>(meta)))
                        allocation_error(ctx);

                    // Check VST3 legacy processor identifier
                    if (uids->vst2 != NULL)
                    {
                        if ((vst3_legacy_id = meta::uid_vst2_to_vst3(vst3_legacy_iid, uids->vst2, plugin_name, false)) != NULL)
                        {
                            const meta::plugin_t *clash = ctx->vst3_ids.get(vst3_id);
                            if (clash != NULL)
                            {
                                if (clash != meta)
                                    validation_error(ctx, "Plugin uid='%s' legacy VST3 processor (DSP) identifier '%s' clashes plugin uid='%s' VST3 identifier",
                                        meta->uid, vst3_legacy_id, clash->uid);
                            }
                            else if (!ctx->vst3_ids.create(vst3_legacy_id, const_cast<meta::plugin_t *>(meta)))
                                allocation_error(ctx);
                        }
                        else
                            validation_error(ctx, "Plugin uid='%s', failed to generate legacy VST2 processor (DSP) identifier hexadecimal string from '%s'",
                                meta->uid, uids->vst2);
                    }
                }
                else
                    validation_error(ctx, "Plugin uid='%s', failed to convert VST3 processor (DSP) TUID '%s' to VST3 hexadecimal string",
                        meta->uid, uids->vst3);

                // Process VST3 identifier for controller (UI)
                if (uids->vst3ui != NULL)
                {
                    if (!meta::uid_vst3_to_tuid(tuid, uids->vst3ui))
                        validation_error(ctx, "Plugin uid='%s', failed to parse VST3 controller (UI) TUID '%s': should be string of 16 ASCII characters or string of 32 hexadecimal characters",
                            meta->uid, uids->vst3ui);

                    if ((vst3_id = meta::uid_tuid_to_vst3(vst3_iid, tuid)) != NULL)
                    {
                        // Check VST3 controller identifier conflicts
                        const meta::plugin_t *clash = ctx->vst3_ids.get(vst3_id);
                        if (clash != NULL)
                        {
                            validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate VST 3 controller (DSP) identifier '%s' ('%s')",
                                meta->uid, clash->uid, uids->vst3ui, vst3_id);
                        }
                        else if (!ctx->vst3_ids.create(vst3_id, const_cast<meta::plugin_t *>(meta)))
                            allocation_error(ctx);

                        // Check VST3 legacy controller identifier
                        if (uids->vst2 != NULL)
                        {
                            if ((vst3_legacy_id = meta::uid_vst2_to_vst3(vst3_legacy_iid, uids->vst2, plugin_name, true)) != NULL)
                            {
                                const meta::plugin_t *clash = ctx->vst3_ids.get(vst3_id);
                                if (clash != NULL)
                                {
                                    if (clash != meta)
                                        validation_error(ctx, "Plugin uid='%s' legacy VST3 controller (UI) identifier '%s' clashes plugin uid='%s' VST3 identifier",
                                            meta->uid, vst3_legacy_id, clash->uid);
                                }
                                else if (!ctx->vst3_ids.create(vst3_legacy_id, const_cast<meta::plugin_t *>(meta)))
                                    allocation_error(ctx);
                            }
                            else
                                validation_error(ctx, "Plugin uid='%s', failed to generate legacy VST3 controller (UI) identifier hexadecimal string from '%s'",
                                    meta->uid, uids->vst2);
                        }
                    }
                    else
                        validation_error(ctx, "Plugin uid='%s', failed to convert VST3 processor (DSP) TUID '%s' to VST3 hexadecimal string",
                            meta->uid, uids->vst3);
                }

                // Validate length of the version string
                LSPString version;
                ssize_t n = version.fmt_ascii(
                    "%d.%d.%d",
                    int(meta->version.major),
                    int(meta->version.minor),
                    int(meta->version.micro));
                if (n > 0)
                {
                    Steinberg::CStringW u16_version = reinterpret_cast<Steinberg::CStringW>(version.get_utf16());
                    size_t version_len = Steinberg::strlen16(u16_version);
                    if (version_len >= Steinberg::PClassInfo2::kVersionSize)
                        validation_error(ctx, "Plugin uid='%s', version string '%s' length %d is out of allowed size of %d UTF-16 characters",
                            meta->uid, version.get_utf8(), int(version_len), int(Steinberg::PClassInfo2::kVersionSize - 1));
                }
                else
                    validation_error(ctx, "Plugin uid='%s', parameter='%s': could not obtain plugin version for VST 3 format", meta->uid);

                // Validate the name of the plugin
                size_t name_len = strlen(meta->description);
                if (name_len >= Steinberg::PClassInfo::kNameSize)
                {
                    validation_error(ctx, "Plugin uid='%s', description string '%s' length %d is out of allowed size of %d ASCII characters",
                        meta->uid, meta->description, int(name_len), int(Steinberg::PClassInfo::kNameSize - 1));
                }

                LSPString name;
                if (name.set_utf8(meta->description))
                {
                    Steinberg::CStringW u16_name= reinterpret_cast<Steinberg::CStringW>(name.get_utf16());
                    name_len = Steinberg::strlen16(u16_name);
                    if (name_len >= Steinberg::PClassInfo::kNameSize)
                        validation_error(ctx, "Plugin uid='%s', description string '%s' length %d is out of allowed size of %d UTF-16 characters",
                            meta->uid, name.get_utf8(), int(name_len), int(Steinberg::PClassInfo::kNameSize - 1));
                }
                else
                    validation_error(ctx, "Plugin uid='%s', can not set UTF-8 name to temporary string", meta->uid);

                // Validate the length of categories field
                LSPString categories;
                res = lsp::vst3::make_plugin_categories(&categories, meta);
                if (res != STATUS_OK)
                    validation_error(ctx, "Plugin uid='%s': failed to generate list of plugin categories, error=%d", meta->uid, int(res));

                const char *u8_categories = categories.get_ascii();
                size_t categories_length = strlen(u8_categories);
                if (categories_length >= Steinberg::PClassInfo2::kSubCategoriesSize)
                    validation_error(ctx, "Plugin uid='%s', generated subcategories string '%s' length %d is out of allowed size of %d ASCII characters",
                        meta->uid, u8_categories, int(categories_length), int(Steinberg::PClassInfo2::kSubCategoriesSize - 1));
            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
                const bool is_parameter =
                    meta::is_control_port(port) ||
                    meta::is_bypass_port(port) ||
                    meta::is_port_set_port(port);

                constexpr size_t str128_count = sizeof(Steinberg::Vst::String128)/sizeof(Steinberg::Vst::TChar);
                Steinberg::Vst::String128 tmp;

                if (is_parameter && meta::is_in_port(port))
                {
                    const char *units   = lsp::vst3::get_unit_name(port->unit);

                    if (!lsp::utf8_to_utf16(lsp::vst3::to_utf16(tmp), port->name, str128_count))
                    {
                        validation_error(ctx, "Plugin uid='%s', id='%s': The port name '%s' converted to UTF-16 exceeds VST3 title limits of %d characters",
                            meta->uid, port->id, port->name, int(str128_count-1));
                    }
                    if (!lsp::utf8_to_utf16(lsp::vst3::to_utf16(tmp), port->id, str128_count))
                    {
                        validation_error(ctx, "Plugin uid='%s', id='%s': The port id '%s' converted to UTF-16 exceeds VST3 title limits of %d characters",
                            meta->uid, port->id, port->id, int(str128_count-1));
                    }
                    if (!lsp::utf8_to_utf16(lsp::vst3::to_utf16(tmp), units, str128_count))
                    {
                        validation_error(ctx, "Plugin uid='%s', id='%s': The port units string '%s' converted to UTF-16 exceeds VST3 title limits of %d characters",
                            meta->uid, port->id, units, int(str128_count-1));
                    }

                    Steinberg::Vst::ParamID uid = lsp::vst3::gen_parameter_id(port->id);
                    const meta::port_t *clash = ctx->vst3_port_ids.get(&uid);
                    if (clash != NULL)
                        validation_error(ctx, "Port id='%s' VST3 parameter id 0x%x clashes port id='%s' for plugin uid='%s'",
                            port->id, int(uid), clash->id, meta->uid);
                    else if (!ctx->vst3_port_ids.create(&uid, const_cast<meta::port_t *>(port)))
                        allocation_error(ctx);
                }
            }

        } /* namespace vst3 */
    } /* namespace validator */
} /* namespace lsp */


