/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 4 янв. 2023 г.
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


#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_DEBUG_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_DEBUG_H_

#include <clap/clap.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/io/InMemoryStream.h>
#include <lsp-plug.in/io/OutMemoryStream.h>
#include <lsp-plug.in/plug-fw/wrap/clap/helpers.h>

#ifdef LSP_TRACE

namespace lsp
{
    namespace clap
    {
        struct debug_ostream_t: public clap_ostream_t
        {
            const clap_ostream_t *cos;
            mutable io::OutMemoryStream os;

            explicit debug_ostream_t(const clap_ostream_t *os)
            {
                this->ctx   = NULL;
                this->write = write_data;
                this->cos   = os;
            }

            static inline int64_t CLAP_ABI write_data(const clap_ostream_t *stream, const void *buffer, uint64_t size)
            {
                const debug_ostream_t *this_ = static_cast<const debug_ostream_t *>(stream);
                return this_->os.write(buffer, size);
            }

            inline status_t flush()
            {
                return write_fully(cos, os.data(), os.size());
            }

            inline const uint8_t *data() const
            {
                return os.data();
            }

            inline size_t size() const
            {
                return os.size();
            }
        };

        struct debug_istream_t: public clap_istream_t
        {
            const clap_istream_t *cis;
            mutable io::InMemoryStream is;

            explicit debug_istream_t(const clap_istream_t *cis)
            {
                this->ctx   = NULL;
                this->read  = read_data;
                this->cis   = cis;
            }

            inline status_t fill()
            {
                uint8_t tmp[128];
                io::OutMemoryStream os;

                while (true)
                {
                    ssize_t n = cis->read(cis, tmp, sizeof(tmp));
                    if (n < 0)
                        return STATUS_IO_ERROR;
                    else if (n == 0)
                        break;

                    ssize_t nw = os.write(tmp, n);
                    if (nw < 0)
                        return -nw;
                    else if (nw != n)
                        return STATUS_NO_MEM;
                }

                size_t size = os.size();
                is.wrap(os.release(), size, MEMDROP_FREE);

                return STATUS_OK;
            }

            static inline int64_t CLAP_ABI read_data(const clap_istream_t *stream, void *buffer, uint64_t size)
            {
                const debug_istream_t *this_ = static_cast<const debug_istream_t *>(stream);
                ssize_t n = this_->is.read(buffer, size);
                if (n > 0)
                    return n;
                return (n == -STATUS_EOF) ? 0 : n;
            }

            inline const uint8_t *data() const
            {
                return is.data();
            }

            inline size_t size() const
            {
                return is.size();
            }
        };

    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_DEBUG */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_DEBUG_H_ */
