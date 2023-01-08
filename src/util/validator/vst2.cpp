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
            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {
                // Validate VST 2.x identifier
                if (meta->vst2_uid == NULL)
                    return;
                if (strlen(meta->vst2_uid) != 4)
                    validation_error(ctx, "Plugin uid='%s' has invalid VST 2.x identifier '%s', should be 4 characters", meta->vst2_uid);

                // Check conflicts
                const meta::plugin_t *clash = ctx->vst2_ids.get(meta->vst2_uid);
                if (clash != NULL)
                    validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate VST 2.x identifier '%s'",
                        meta->uid, clash->uid, meta->vst2_uid);
                else if (!ctx->vst2_ids.create(meta->vst2_uid, const_cast<meta::plugin_t *>(meta)))
                    allocation_error(ctx);
            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
                if (meta::is_control_port(port) && meta::is_in_port(port))
                {
                    if (strlen(port->id) >= kVstMaxParamStrLen)
                        validation_error(ctx, "Plugin uid='%s', parameter='%s': VST 2.x restrictions do not allow the parameter name to be not larger %d characters",
                            meta->uid, port->id, int(kVstMaxParamStrLen-1));
                }
            }

        } /* namespace vst2 */
    } /* namespace validator */
} /* namespace lsp */



