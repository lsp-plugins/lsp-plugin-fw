/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-pluginfw
 * Created on: 2 апр. 2024 г.
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

#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/core/presets.h>

namespace lsp
{
    namespace core
    {
        static ssize_t compare_presets_asc(const resource::resource_t *a, const resource::resource_t *b)
        {
            return strcmp(a->name, b->name);
        }

        static ssize_t compare_presets_desc(const resource::resource_t *a, const resource::resource_t *b)
        {
            return strcmp(b->name, a->name);
        }

        status_t scan_presets(lltl::darray<resource::resource_t> *presets, resource::ILoader *loader, const char *location)
        {
            if ((presets == NULL) || (loader == NULL) || (location == NULL))
                return STATUS_BAD_ARGUMENTS;

            io::Path path;
            LSPString tmp;
            resource::resource_t *resources = NULL;

            if (!location)
                return STATUS_NOT_FOUND;

            if (tmp.fmt_utf8(LSP_BUILTIN_PREFIX "presets/%s", location) < 0)
                return STATUS_BAD_STATE;

            ssize_t count = loader->enumerate(&tmp, &resources);
            if (!resources)
                return STATUS_NOT_FOUND;

            // Process all resources and form the final list of preset files
            for (ssize_t i=0; i<count; ++i)
            {
                resource::resource_t *item = &resources[i];

                // Filter the preset file
                if (item->type != resource::RES_FILE)
                    continue;
                if (path.set(item->name) != STATUS_OK)
                {
                    free(resources);
                    return STATUS_NO_MEM;
                }
                if (path.get_ext(&tmp) != STATUS_OK)
                {
                    free(resources);
                    return STATUS_BAD_STATE;
                }

                if ((!tmp.equals_ascii("patch")) && (!tmp.equals_ascii("preset")))
                    continue;

                // Add preset file to result
                strncpy(item->name, path.get(), resource::RESOURCE_NAME_MAX);
                item->name[resource::RESOURCE_NAME_MAX-1] = '\0';
                if (!presets->add(item))
                {
                    free(resources);
                    return STATUS_NO_MEM;
                }
            }

            free(resources);

            return STATUS_OK;
        }

        void sort_presets(lltl::darray<resource::resource_t> *presets, bool ascending)
        {
            if (presets == NULL)
                return;

            presets->qsort(
                (ascending) ?
                    compare_presets_asc :
                    compare_presets_desc);
        }

        void init_preset_state(preset_state_t *state)
        {
            state->flags        = PRESET_FLAG_NONE;
            state->tab          = 0;
            state->name[0]      = '\0';
        }

        void copy_preset_state(preset_state_t *dst, const preset_state_t *src)
        {
            dst->flags          = src->flags;
            dst->tab            = src->tab;
            strcpy(dst->name, src->name);
        }

    } /* namespace core */
} /* namespace lsp */
