/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 7 сент. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_SHMSTATEBUILDER_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_SHMSTATEBUILDER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/runtime/LSPString.h>

namespace lsp
{
    namespace core
    {
        struct ShmRecord;
        class ShmState;

        class ShmStateBuilder
        {
            private:
                lltl::darray<ShmRecord>     vItems;
                io::OutMemoryStream         sOS;

            public:
                ShmStateBuilder();
                ShmStateBuilder(const ShmStateBuilder &) = delete;
                ShmStateBuilder(ShmStateBuilder &&) = delete;
                ~ShmStateBuilder();

                ShmStateBuilder & operator = (const ShmStateBuilder &) = delete;
                ShmStateBuilder & operator = (ShmStateBuilder &&) = delete;

            public:
                status_t    append(const ShmRecord * record);
                status_t    append(const ShmRecord & record);
                status_t    append(const char *name, const char *id, uint32_t index, uint32_t magic);
                status_t    append(const LSPString *name, const char *id, uint32_t index, uint32_t magic);
                status_t    append(const char *name, const LSPString *id, uint32_t index, uint32_t magic);
                status_t    append(const LSPString *name, const LSPString *id, uint32_t index, uint32_t magic);
                status_t    append(const LSPString &name, const char *id, uint32_t index, uint32_t magic);
                status_t    append(const char *name, const LSPString &id, uint32_t index, uint32_t magic);
                status_t    append(const LSPString &name, const LSPString &id, uint32_t index, uint32_t magic);

            public:
                ShmState   *build();
        };

    } /* namespace core */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_CORE_SHMSTATEBUILDER_H_ */
