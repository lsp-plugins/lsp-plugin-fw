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

#include <lsp-plug.in/plug-fw/util/validator/validator.h>
#include <lsp-plug.in/stdlib/stdio.h>

namespace lsp
{
    namespace validator
    {
        namespace ladspa
        {
            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {
                // Validate LADSPA identifier
                if (meta->ladspa_id <= 0)
                {
                    if (meta->ladspa_lbl != NULL)
                        validation_error(ctx, "Plugin uid='%s' has no integral LADSPA identifier but has provided LADSPA label='%s'",
                            meta->uid, meta->ladspa_lbl);

                    return;
                }

                // Check conflicts within ladspa_id
                const meta::plugin_t *clash = ctx->ladspa_ids.get(&meta->ladspa_id);
                if (clash != NULL)
                    validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate LADSPA identifier %d (0x%x)",
                        meta->uid, clash->uid, int(meta->ladspa_id), int(meta->ladspa_id));
                else if (!ctx->ladspa_ids.create(&meta->ladspa_id, const_cast<meta::plugin_t *>(meta)))
                    allocation_error(ctx);

                // Check conflicts within ladspa_label
                if (meta->ladspa_lbl != NULL)
                {
                    clash = ctx->ladspa_labels.get(meta->ladspa_lbl);
                    if (clash != NULL)
                        validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate LADSPA label '%s'",
                            meta->uid, clash->uid, meta->ladspa_lbl);
                    else if (!ctx->ladspa_labels.create(meta->ladspa_lbl, const_cast<meta::plugin_t *>(meta)))
                        allocation_error(ctx);
                }
                else
                    validation_error(ctx, "Plugin uid='%s' provides integral LADSPA identifier %d (0x%x) but does not provide LADSPA label",
                        meta->uid, int(meta->ladspa_id), int(meta->ladspa_id));
            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
            }
        } /* namespace ladspa */
    } /* namespace validator */
} /* namespace lsp */




