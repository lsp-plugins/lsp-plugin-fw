/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 янв. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_DATA_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_DATA_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <clap/clap.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/clap/helpers.h>
#include <lsp-plug.in/stdlib/string.h>

namespace lsp
{
    namespace clap
    {
        constexpr uint32_t LSP_CLAP_MAGIC   = 0x2050534C;
        constexpr uint32_t LSP_CLAP_VERSION = 0x1;

        enum serial_flags_t
        {
            FLAG_PRIVATE    = 1 << 0,
        };

        enum serial_types_t
        {
            TYPE_INT32      = 'i',
            TYPE_UINT32     = 'u',
            TYPE_INT64      = 'I',
            TYPE_UINT64     = 'U',
            TYPE_FLOAT32    = 'f',
            TYPE_FLOAT64    = 'F',
            TYPE_STRING     = 's',
            TYPE_BLOB       = 'B'
        };

        /**
         * Path data primitive
         */
        typedef struct path_t: public plug::path_t
        {
            enum flags_t
            {
                F_PENDING       = 1 << 0,
                F_ACCEPTED      = 1 << 1
            };

            atomic_t    nDspRequest;
            atomic_t    nDspSerial;
            atomic_t    nDspCommit;
            atomic_t    nUiSerial;
            atomic_t    nUiCommit;

            size_t      nFlags;

            size_t      nXFlags;
            size_t      nXFlagsReq;

            char        sPath[PATH_MAX];
            char        sDspRequest[PATH_MAX];
            char        sUiPath[PATH_MAX];

            virtual void init()
            {
                atomic_init(nDspRequest);
                nDspSerial      = 0;
                nDspCommit      = 0;
                nUiSerial       = 0;
                nUiCommit       = 0;

                nFlags          = 0;
                nXFlags         = 0;
                nXFlagsReq      = 0;

                sPath[0]        = '\0';
                sDspRequest[0]  = '\0';
                sUiPath[0]      = '\0';
            }

            virtual const char *path() const
            {
                return sPath;
            }

            virtual size_t flags() const
            {
                return nXFlags;
            }

            virtual void accept()
            {
                if (nFlags & F_PENDING)
                    nFlags     |= F_ACCEPTED;
            }

            virtual void commit()
            {
                if (nFlags & (F_PENDING | F_ACCEPTED))
                    nFlags      = 0;
            }

            virtual bool pending()
            {
                // Check accepted flags
                if (nFlags & F_PENDING)
                    return !(nFlags & F_ACCEPTED);

                // Check for pending request
                if (!atomic_trylock(nDspRequest))
                    return false;

                // Update state of the DSP
                if (nDspSerial != nDspCommit)
                {
                    // Copy the data
                    nXFlags             = nXFlagsReq;
                    nXFlagsReq          = 0;
                    ::strncpy(sPath, sDspRequest, PATH_MAX-1);
                    sPath[PATH_MAX-1]   = '\0';
                    nFlags              = F_PENDING;

                    lsp_trace("  DSP Request: %s", sDspRequest);
                    lsp_trace("  saved path: %s", sPath);

                    // Update serial(s)
                    atomic_add(&nUiSerial, 1);
                    atomic_add(&nDspCommit, 1);
                }

                atomic_unlock(nDspRequest);

                return (nFlags & F_PENDING);
            }

            virtual bool accepted()
            {
                return nFlags & F_ACCEPTED;
            }

            status_t serialize(const clap_ostream_t *os)
            {
                ssize_t res = write_string(os, sPath);
                return (res < 0) ? -res : STATUS_OK;
            }

            status_t deserialize(const clap_istream_t *is)
            {
                // Deserialize as DSP request
                status_t res = read_string(is, sDspRequest, PATH_MAX-1);
                if (res != STATUS_OK)
                    return res;

                nXFlagsReq          = plug::PF_STATE_RESTORE;
                atomic_add(&nDspSerial, 1);
                return STATUS_OK;
            }

            void submit(const char *path, size_t len, bool ui, size_t flags)
            {
                // Determine size of path
                size_t count = lsp_min(len, size_t(PATH_MAX-1));

                lsp_trace("submit %s, len=%d, ui=%s, flags=%x",
                        path, int(len), (ui) ?  "true" : "false", int(flags));

                // Wait until the queue is empty
                if (ui)
                {
                    while (true)
                    {
                        // Try to acquire critical section
                        if (atomic_trylock(nDspRequest))
                        {
                            // Write DSP request
                            ::memcpy(sDspRequest, path, count);
                            nXFlagsReq          = flags;
                            sDspRequest[count]  = '\0';
                            atomic_add(&nDspSerial, 1);

                            // Release critical section and leave
                            atomic_unlock(nDspRequest);
                            break;
                        }

                        // Wait for a while (10 milliseconds)
                        ipc::Thread::sleep(10);
                    }
                }
                else
                {
                    // Write DSP request
                    ::memcpy(sDspRequest, path, count);
                    nXFlagsReq          = flags;
                    sDspRequest[count]  = '\0';
                    atomic_add(&nDspSerial, 1);
                }
            }

            bool ui_sync()
            {
                if (!atomic_trylock(nDspRequest))
                    return false;
                bool sync = (nUiSerial != nUiCommit);
                if (sync)
                {
                    ::strncpy(sUiPath, sPath, PATH_MAX-1);
                    sUiPath[PATH_MAX-1] = '\0';

                    atomic_add(&nUiCommit, 1);
                }
                atomic_unlock(nDspRequest);

                return sync;
            }

        } path_t;

    } /* namespace clap */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_CLAP_DATA_H_ */

