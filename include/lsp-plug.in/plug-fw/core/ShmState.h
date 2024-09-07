/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 авг. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CORE_SHMSTATE_H_
#define LSP_PLUG_IN_PLUG_FW_CORE_SHMSTATE_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace core
    {
        typedef struct ShmRecord
        {
            const char     *id;
            const char     *name;
            uint32_t        index;
            uint32_t        magic;
        } ShmRecord;

        class ShmState
        {
            private:
                friend class ShmStateBuilder;

            private:
                ShmRecord                  *vItems;
                size_t                      nItems;
                char                       *vStrings;

            protected:
                ShmState(ShmRecord *items, char *strings, size_t count);

            public:
                ShmState() = delete;
                ShmState(const ShmState &) = delete;
                ShmState(ShmState &&) = delete;
                ~ShmState();

                ShmState & operator = (const ShmState &) = delete;
                ShmState & operator = (ShmState &&) = delete;

            public:
                /**
                 * Get number of records
                 * @return number of records
                 */
                size_t              size() const;

                /**
                 * Get recored
                 * @param index index of the record
                 * @return record
                 */
                const ShmRecord    *get(size_t index) const;
        };

    } /* namespace core */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CORE_SHMSTATE_H_ */
