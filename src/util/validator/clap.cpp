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

#include <lsp-plug.in/plug-fw/wrap/clap/helpers.h>

namespace lsp
{
    namespace validator
    {
        namespace clap
        {
            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {

            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
                // Check that control port does not clash with another control port
                if (port->role == meta::R_CONTROL)
                {
                    clap_id uid = lsp::clap::clap_hash_string(port->id);
                    const meta::port_t *clash = ctx->clap_ids.get(&uid);
                    if (clash != NULL)
                        validation_error(ctx, "Port id='%s' clashes port id='%s' for plugin uid='%s'",
                            port->id, clash->id, meta->uid);
                    else if (!ctx->clap_ids.create(&uid, const_cast<meta::port_t *>(port)))
                        allocation_error(ctx);
                }
            }
        } /* namespace clap */
    } /* namespace validator */
} /* namespace lsp */




