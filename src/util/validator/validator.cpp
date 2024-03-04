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

#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/io/IInStream.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/util/validator/validator.h>
#include <lsp-plug.in/stdlib/stdio.h>

#include <stdarg.h>

namespace lsp
{
    namespace validator
    {
        static void get_manifest(context_t *ctx, meta::package_t **pkg)
        {
            // Obtain the manifest
            status_t res;

            resource::ILoader *loader = core::create_resource_loader();
            if (loader != NULL)
            {
                io::IInStream *is = loader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
                if (is != NULL)
                {
                    res = meta::load_manifest(pkg, is);
                    res = update_status(res, is->close());
                    delete is;
                }
                else
                    res = loader->last_error();
                delete loader;
            }
            else
                res = STATUS_NOT_FOUND;

            if (res != STATUS_OK)
                validation_error(ctx, "Error loading manifest file, error=%d", int(res));
        }

        void allocation_error(context_t *ctx)
        {
            fprintf(stderr, "Failed to allocate memory\n");
            ++ctx->errors;
        }

        void validation_error(context_t *ctx, const char *fmt, ...)
        {
            va_list vl;
            va_start(vl, fmt);
            vfprintf(stderr, fmt, vl);
            fprintf(stderr, "\n");
            ++ctx->errors;
            va_end(vl);
        }

        /**
         * Main method
         * @param argc number of arguments
         * @param argv list of arguments
         * @return status of operation
         */
        int main(int argc, const char **argv)
        {
            context_t ctx;
            ctx.plugins     = 0;
            ctx.warnings    = 0;
            ctx.errors      = 0;
            ctx.midi_in     = 0;
            ctx.midi_out    = 0;
            ctx.osc_in      = 0;
            ctx.osc_out     = 0;

            // Load the manifest
            meta::package_t *pkg = NULL;
            get_manifest(&ctx, &pkg);
            lsp_finally {
                if (pkg != NULL)
                    meta::free_manifest(pkg);
            };
            validate_package(&ctx, pkg);

            // Iterate over all plugin factories and validate their metadata
            // Generate descriptors
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *meta = f->enumerate(i);
                    if (meta == NULL)
                        break;

                    ++ctx.plugins;
                    validate_plugin(&ctx, meta);
                }
            }

            printf("Overall validation statistics: plugins=%d, warnings=%d, errors=%d\n\n",
                int(ctx.plugins), int(ctx.warnings), int(ctx.errors));

            return (ctx.errors > 0) ? STATUS_FAILED : STATUS_OK;
        }
    } /* namespace validator */
} /* namespace lsp */

