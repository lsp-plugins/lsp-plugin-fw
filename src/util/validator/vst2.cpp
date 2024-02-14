/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 янв. 2023 г.
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
#include <lsp-plug.in/plug-fw/util/validator/validator.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>
#include <steinberg/vst2.h>

namespace lsp
{
    namespace validator
    {
        namespace vst2
        {
            void validate_package(context_t *ctx, const meta::package_t *pkg)
            {
                // Validate vendor string
                const size_t vendor_len = strlen(pkg->brand) + strlen(" VST");
                if (vendor_len >= kVstMaxVendorStrLen)
                    validation_error(ctx, "Manifest has too long VST 2.x vendor name '%s VST' generated from '%s', of %d characters, but only %d characters are permitted",
                        pkg->brand, pkg->brand, int(vendor_len), int(kVstMaxVendorStrLen-1));
            }

            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {
                // Validate VST 2.x identifier
                if (meta->vst2_uid == NULL)
                    return;
                if (strlen(meta->vst2_uid) != 4)
                    validation_error(ctx, "Plugin uid='%s' has invalid VST 2.x identifier '%s', should be 4 characters",
                        meta->uid, meta->vst2_uid);

                // Validate VST 2.x plugin name
                const char *plugin_name = (meta->vst2_name != NULL) ? meta->vst2_name : meta->name;
                const size_t name_len = strlen(plugin_name);
                if (name_len >= kVstMaxEffectNameLen)
                    validation_error(ctx, "Plugin uid='%s' has too long VST 2.x name '%s', of %d characters, but only %d characters are permitted",
                        meta->uid, meta->vst2_name, int(name_len), int(kVstMaxEffectNameLen-1));

                // Check conflicts
                const meta::plugin_t *clash = ctx->vst2_ids.get(meta->vst2_uid);
                if (clash != NULL)
                    validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate VST 2.x identifier '%s'",
                        meta->uid, clash->uid, meta->vst2_uid);
                else if (!ctx->vst2_ids.create(meta->vst2_uid, const_cast<meta::plugin_t *>(meta)))
                    allocation_error(ctx);

                // Validate version
                size_t micro = LSP_MODULE_VERSION_MICRO(meta->version);
                if (micro > VST_VERSION_MICRO_MAX)
                    validation_error(ctx,
                        "Micro version value=%d of plugin uid='%s' is greater than maximum possible value %d",
                        int(micro), meta->uid, int(VST_VERSION_MICRO_MAX));

                size_t minor = LSP_MODULE_VERSION_MINOR(meta->version);
                if (minor > VST_VERSION_MINOR_MAX)
                    validation_error(ctx,
                        "Minor version value=%d of plugin uid='%s' is greater than maximum possible value %d",
                        int(micro), meta->uid, int(VST_VERSION_MINOR_MAX));
            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
                const bool is_parameter =
                    meta::is_control_port(port) ||
                    meta::is_bypass_port(port) ||
                    meta::is_port_set_port(port);

                if (is_parameter && meta::is_in_port(port))
                {
                    if (strlen(port->id) >= kVstMaxParamStrLen)
                        validation_error(ctx, "Plugin uid='%s', parameter='%s': VST 2.x restrictions do not allow the parameter name to be not larger %d characters",
                            meta->uid, port->id, int(kVstMaxParamStrLen-1));
                }
            }

        } /* namespace vst2 */
    } /* namespace validator */
} /* namespace lsp */



