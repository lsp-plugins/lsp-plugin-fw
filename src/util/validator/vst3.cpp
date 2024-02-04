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

#include <steinberg/vst3.h>

namespace lsp
{
    namespace validator
    {
        namespace vst3
        {
            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {
                status_t res;

                // Does plugin support VST 3.x?
                if (meta->vst3_uid == NULL)
                    return;

                // Validate IDs of processor and controller
                Steinberg::TUID tuid;
                if (!meta::uid_vst3_to_tuid(tuid, meta->vst3_uid))
                    validation_error(ctx, "Plugin uid='%s', failed to parse VST3 processor (DSP) TUID '%s': should be string of 16 ASCII characters or string of 32 hexadecimal characters",
                        meta->uid, meta->vst3_uid);

                if (meta->vst3ui_uid != NULL)
                {
                    if (!meta::uid_vst3_to_tuid(tuid, meta->vst3ui_uid))
                        validation_error(ctx, "Plugin uid='%s', failed to parse VST3 controller (UI) TUID '%s': should be string of 16 ASCII characters or string of 32 hexadecimal characters",
                            meta->uid, meta->vst3ui_uid);
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
            }

        } /* namespace vst3 */
    } /* namespace validator */
} /* namespace lsp */


