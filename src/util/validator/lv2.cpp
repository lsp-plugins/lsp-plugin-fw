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
            }

            void validate_port(context_t *ctx, const meta::plugin_t *meta, const meta::port_t *port)
            {
            }
        } /* namespace lv2 */
    } /* namespace validator */
} /* namespace lsp */





