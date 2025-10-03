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

#include <lsp-plug.in/common/alloc.h>
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

        void destroy_preset_param(preset_param_t *param)
        {
            if (param == NULL)
                return;

            kvt_destroy_parameter(&param->value);
            free(param);
        }

        void destroy_preset_params(lltl::parray<preset_param_t> *list)
        {
            for (lltl::iterator<preset_param_t> it=list->values(); it; ++it)
                destroy_preset_param(it.get());
            list->flush();
        }

        void destroy_preset_data(preset_data_t *data)
        {
            if (data != NULL)
                destroy_preset_params(&data->values);
            data->empty         = true;
            data->dirty         = false;
        }

        status_t add_preset_data_param(preset_data_t *dst, const char *name, const kvt_param_t * value)
        {
            const size_t param_len  = strlen(name) + 1;
            const size_t to_alloc   = sizeof(kvt_param_t) + align_size(param_len, DEFAULT_ALIGN);

            // Allocate and initialize parameter
            preset_param_t *param   = static_cast<preset_param_t * >(malloc(to_alloc));
            if (param == NULL)
                return STATUS_NO_MEM;

            kvt_init_parameter(&param->value);
            memcpy(param->name, name, param_len);
            lsp_finally { destroy_preset_param(param); };

            // Copy value to parameter
            status_t res = kvt_copy_parameter(&param->value, value);
            if (res != STATUS_OK)
                return res;

            // Add parameter to the list
            if (!dst->values.add(param))
                return STATUS_NO_MEM;

            param                   = NULL;
            return STATUS_OK;
        }

        status_t copy_preset_data(preset_data_t *dst, const preset_data_t *src)
        {
            if ((src == NULL) || (dst == NULL))
                return STATUS_BAD_ARGUMENTS;

            // Copy parameters
            preset_data_t data;
            if (!data.values.reserve(src->values.size()))
                return STATUS_NO_MEM;

            lsp_finally { destroy_preset_data(&data); };

            for (lltl::iterator<preset_param_t> it = (const_cast<preset_data_t *>(src))->values.values(); it; ++it)
            {
                const preset_param_t *param     = it.get();
                status_t res = add_preset_data_param(&data, param->name, &param->value);
                if (res != STATUS_OK)
                    return res;
            }

            // Now we are ready to commit result
            dst->values.swap(&data.values);
            dst->empty      = false;

            return STATUS_OK;
        }

        void init_preset_data(preset_data_t *data)
        {
            data->empty         = true;
            data->dirty         = false;
        }

    } /* namespace core */
} /* namespace lsp */
