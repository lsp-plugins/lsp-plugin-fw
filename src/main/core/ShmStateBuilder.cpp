/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-send
 * Created on: 7 сент. 2024 г.
 *
 * lsp-plugins-send is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-send is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-send. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/core/ShmState.h>
#include <lsp-plug.in/plug-fw/core/ShmStateBuilder.h>

namespace lsp
{
    namespace core
    {
        ShmStateBuilder::ShmStateBuilder()
        {
        }

        ShmStateBuilder::~ShmStateBuilder()
        {
            vItems.flush();
            sOS.close();
        }

        status_t ShmStateBuilder::append(const ShmRecord * record)
        {
            if (record == NULL)
                return STATUS_BAD_ARGUMENTS;
            return append(*record);
        }

        status_t ShmStateBuilder::append(const ShmRecord & record)
        {
            return append(record.name, record.id, record.index, record.magic);
        }

        status_t ShmStateBuilder::append(const LSPString *name, const char *id, uint32_t index, uint32_t magic)
        {
            if (name == NULL)
                return STATUS_BAD_ARGUMENTS;
            return append(name->get_utf8(), id, index, magic);
        }

        status_t ShmStateBuilder::append(const char *name, const LSPString *id, uint32_t index, uint32_t magic)
        {
            if (id == NULL)
                return STATUS_BAD_ARGUMENTS;
            return append(name, id->get_utf8(), index, magic);
        }

        status_t ShmStateBuilder::append(const LSPString *name, const LSPString *id, uint32_t index, uint32_t magic)
        {
            if ((name == NULL) || (id == NULL))
                return STATUS_BAD_ARGUMENTS;
            return append(name->get_utf8(), id->get_utf8(), index, magic);
        }

        status_t ShmStateBuilder::append(const LSPString &name, const char *id, uint32_t index, uint32_t magic)
        {
            return append(name.get_utf8(), id, index, magic);
        }

        status_t ShmStateBuilder::append(const char *name, const LSPString &id, uint32_t index, uint32_t magic)
        {
            return append(name, id.get_utf8(), index, magic);
        }

        status_t ShmStateBuilder::append(const LSPString &name, const LSPString &id, uint32_t index, uint32_t magic)
        {
            return append(name.get_utf8(), id.get_utf8(), index, magic);
        }

        status_t ShmStateBuilder::append(const char *name, const char *id, uint32_t index, uint32_t magic)
        {
            wssize_t position = sOS.position();
            lsp_finally {
                if (position >= 0)
                    sOS.seek(position);
            };

            // Emit string records
            const ptrdiff_t id_offset = sOS.position();
            ssize_t written = sOS.write(id, strlen(id) + 1);
            if (written < 0)
                return -written;

            const ptrdiff_t name_offset = sOS.position();
            written = sOS.write(name, strlen(name) + 1);
            if (written < 0)
                return -written;

            // Create record
            ShmRecord *dst = vItems.add();
            if (dst == NULL)
                return STATUS_NO_MEM;

            dst->id             = reinterpret_cast<const char *>(id_offset);
            dst->name           = reinterpret_cast<const char *>(name_offset);
            dst->index          = index;
            dst->magic          = magic;
            position            = -1;

            return STATUS_OK;
        }

        ShmState *ShmStateBuilder::build()
        {
            // Complete the data structures and patch pointers
            char *strings       = reinterpret_cast<char *>(sOS.release());
            lsp_finally {
                if (strings != NULL)
                    free(strings);
            };

            size_t count        = vItems.size();
            ShmRecord *items    = vItems.release();
            lsp_finally {
                if (items != NULL)
                    free(items);
            };

            for (size_t i=0; i<count; ++i)
            {
                ShmRecord *dst      = &items[i];
                dst->id             = strings + reinterpret_cast<ptrdiff_t>(dst->id);
                dst->name           = strings + reinterpret_cast<ptrdiff_t>(dst->name);
            }

            // Create shared memory state
            ShmState *state     = new ShmState(items, strings, count);
            if (state != NULL)
            {
                items           = NULL;
                strings         = NULL;
            }

            return state;
        }

    } /* namespace core */
} /* namespace lsp */
