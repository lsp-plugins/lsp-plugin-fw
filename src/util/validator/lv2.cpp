/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/plug-fw/util/validator/validator.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace validator
    {
        namespace lv2
        {
            void validate_package(context_t *ctx, const meta::package_t *pkg)
            {
            }

            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {
                if (meta->uids.lv2 == NULL)
                {
                    if (meta->uids.lv2ui != NULL)
                        validation_error(ctx, "Plugin uid='%s' has not specified LV2 URI but provides LV2 UI URI='%s'",
                            meta->uid, meta->uids.lv2ui);
                    return;
                }

                // Check conflicts for LV2 UID
                const meta::plugin_t *clash = ctx->lv2_ids.get(meta->uids.lv2);
                if (clash != NULL)
                    validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate LV2 URI identifier '%s'",
                        meta->uid, clash->uid, meta->uids.lv2);
                else if (!ctx->lv2_ids.create(meta->uids.lv2, const_cast<meta::plugin_t *>(meta)))
                    allocation_error(ctx);

                if (meta->uids.lv2ui != NULL)
                {
                    // Check conflicts for LV2 UID
                    clash = ctx->lv2_ids.get(meta->uids.lv2ui);
                    if (clash != NULL)
                        validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate LV2 UI URI identifier '%s'",
                            meta->uid, clash->uid, meta->uids.lv2ui);
                    else if (!ctx->lv2_ids.create(meta->uids.lv2ui, const_cast<meta::plugin_t *>(meta)))
                        allocation_error(ctx);
                }
            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
                if (port->short_name != NULL)
                {
                    // LV2: This is the same as name, with the additional requirement that the value is shorter than 16 characters.
                    const size_t len = strlen(port->short_name);
                    if (len >= 16)
                        validation_error(
                            ctx,
                            "Plugin uid='%s' port id='%s' has short name '%s' of length %d characters but should be shorter than 16 characters",
                            meta->uid, port->id, port->short_name, int(len));
                }
            }
        } /* namespace lv2 */
    } /* namespace validator */
} /* namespace lsp */





