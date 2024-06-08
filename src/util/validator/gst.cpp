/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 июн. 2024 г.
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
#include <lsp-plug.in/plug-fw/meta/func.h>

namespace lsp
{
    namespace validator
    {
        namespace gst
        {
            void validate_package(context_t *ctx, const meta::package_t *pkg)
            {
            }

            void validate_plugin(context_t *ctx, const meta::plugin_t *meta)
            {
                // Validate GST identifier
                const meta::plugin_fmt_uids_t *uids = &meta->uids;

                if (uids->gst == NULL)
                    return;

                // Check conflicts
                const meta::plugin_t *clash = ctx->gst_ids.get(uids->gst);
                if (clash != NULL)
                    validation_error(ctx, "Plugin uid='%s' clashes plugin uid='%s': duplicate GStreamer identifier '%s'",
                        meta->uid, clash->uid, uids->gst);
                else if (!ctx->gst_ids.create(uids->gst, const_cast<meta::plugin_t *>(meta)))
                    allocation_error(ctx);
            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
            }
        } /* namespace gst */
    } /* namespace validator */
} /* namespace lsp */


