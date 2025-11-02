/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace validator
    {
        namespace ladspa
        {
            inline bool port_supported(const meta::port_t *port)
            {
                switch (port->role)
                {
                    case meta::R_AUDIO_IN:
                    case meta::R_AUDIO_OUT:
                    case meta::R_CONTROL:
                    case meta::R_METER:
                    case meta::R_BYPASS:
                        return true;
                    default:
                        break;
                }
                return false;
            }

            inline bool is_ladspa_optional_port(const meta::port_t *port)
            {
                switch (port->role)
                {
                    case meta::R_MESH:
                    case meta::R_STREAM:
                    case meta::R_FBUFFER:
                        return true;
                    default:
                        break;
                }
                return false;
            }

            void validate_package(context_t *ctx, const meta::package_t *pkg)
            {
            }

            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {
                // Validate LADSPA identifier
                const meta::plugin_fmt_uids_t *uids = &meta->uids;

                if (uids->ladspa_id <= 0)
                {
                    if (uids->ladspa_lbl != NULL)
                        validation_error(ctx, "Plugin uid='%s' has no integral LADSPA identifier but has provided LADSPA label='%s'",
                            meta->uid, uids->ladspa_lbl);

                    return;
                }

                // Check conflicts within ladspa_id
                const meta::plugin_t *clash = ctx->ladspa_ids.get(&uids->ladspa_id);
                if (clash != NULL)
                    validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate LADSPA identifier %d (0x%x)",
                        meta->uid, clash->uid, int(uids->ladspa_id), int(uids->ladspa_id));
                else if (!ctx->ladspa_ids.create(&uids->ladspa_id, const_cast<meta::plugin_t *>(meta)))
                    allocation_error(ctx);

                // Check conflicts within ladspa_label
                if (uids->ladspa_lbl != NULL)
                {
                    clash = ctx->ladspa_labels.get(uids->ladspa_lbl);
                    if (clash != NULL)
                        validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate LADSPA label '%s'",
                            meta->uid, clash->uid, uids->ladspa_lbl);
                    else if (!ctx->ladspa_labels.create(uids->ladspa_lbl, const_cast<meta::plugin_t *>(meta)))
                        allocation_error(ctx);
                }
                else
                    validation_error(ctx, "Plugin uid='%s' provides integral LADSPA identifier %d (0x%x) but does not provide LADSPA label",
                        meta->uid, int(uids->ladspa_id), int(uids->ladspa_id));
            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
                // Skip plugins that do not introduce LADSPA support
                if ((meta->uids.ladspa_id <= 0) || (meta->uids.ladspa_lbl == NULL))
                    return;

                // Check that port is supported by LADSPA
                if (!port_supported(port))
                {
                    if ((!is_ladspa_optional_port(port)) && (!meta::is_optional_port(port)))
                    {
                        validation_error(ctx, "LADSPA support problem for plugin uid='%s' port='%s': LADSPA does not support port of type '%s', "
                            "to make plugin compatible with LADSPA, the port='%s' should be marked and handled as optional",
                            meta->uid, port->id,
                            meta::port_role_name(port->role),
                            port->id);
                    }
                }
            }
        } /* namespace ladspa */
    } /* namespace validator */
} /* namespace lsp */




