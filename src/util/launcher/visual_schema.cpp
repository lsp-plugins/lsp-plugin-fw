/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 14 янв. 2026 г.
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/ui/const.h>
#include <lsp-plug.in/plug-fw/util/launcher/visual_schema.h>

namespace lsp
{
    namespace launcher
    {
        static status_t load_stylesheet(tk::StyleSheet & sheet, resource::ILoader * loader, const LSPString & file)
        {
            if (loader == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Read the resource as sequence
            io::IInSequence *is = loader->read_sequence(&file, "UTF-8");
            if (is == NULL)
                return loader->last_error();

            // Parse the sheet data and close the input sequence
            status_t res    = sheet.parse_data(is);
            if (res != STATUS_OK)
                lsp_warn("Error loading stylesheet '%s': code=%d, %s", file.get_native(), int(res), sheet.error()->get_native());

            res             = update_status(res, is->close());
            delete is;

            return res;
        }

        status_t load_visual_schemas(visual_schemas_t & list, resource::ILoader * loader)
        {
            visual_schemas_t result;
            lsp_finally { destroy_visual_schemas(result); };

            // For that we need to scan all available schemas in the resource directory
            resource::resource_t *items = NULL;
            ssize_t count = loader->enumerate(LSP_BUILTIN_PREFIX "schema", &items);
            if ((count <= 0) || (items == NULL))
            {
                if (items != NULL)
                    free(items);
                return STATUS_OK;
            }
            lsp_finally { free(items); };

            // Load stylesheets for each schema
            status_t res;
            for (ssize_t i=0; i<count; ++i)
            {
                // Create new item
                visual_schema_t *schema = new visual_schema_t;
                lsp_finally {
                    if (schema != NULL)
                        delete schema;
                };

                if (items[i].type != resource::RES_FILE)
                    continue;

                if (!schema->path.fmt_ascii(LSP_BUILTIN_PREFIX SCHEMA_PATH "/%s", items[i].name))
                    return STATUS_NO_MEM;

                // Try to load schema
                if ((res = load_stylesheet(schema->sheet, loader, schema->path)) != STATUS_OK)
                {
                    if (res == STATUS_NO_MEM)
                        return res;
                    continue;
                }

                // Set schema name
                if (!schema->name.set(schema->sheet.title()))
                    return STATUS_NO_MEM;

                // Add schema to list
                if (!result.add(schema))
                    return STATUS_NO_MEM;

                // Do not delete schema
                schema = NULL;
            }

            // Commit changes
            list.swap(result);

            return STATUS_OK;
        }

        void destroy_visual_schemas(visual_schemas_t & list)
        {
            for (lltl::iterator<visual_schema_t> it = list.values(); it; ++it)
            {
                visual_schema_t * const vs = it.get();
                delete vs;
            }
            list.flush();
        }
    } /* namespace launcher */
} /* namespace lsp */
