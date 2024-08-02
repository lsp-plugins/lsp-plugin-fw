/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 мая 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_TYPES_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_TYPES_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/plug-fw/plug.h>

namespace lsp
{
    namespace gst
    {

        struct Path: public plug::path_t
        {
            enum flags_t
            {
                S_EMPTY,
                S_PENDING,
                S_ACCEPTED
            };

            atomic_t    nRequest;
            uint32_t    nState;
            size_t      nFlags;
            size_t      nReqFlags;
            bool        bRequest;
            char        sPath[PATH_MAX];
            char        sValue[PATH_MAX];

            virtual void init() override
            {
                atomic_init(nRequest);
                nState      = S_EMPTY;
                nFlags      = 0;
                bRequest    = false;
                nReqFlags   = 0;
                sPath[0]    = '\0';
                sValue[0]   = '\0';
            }

            virtual const char *path() const override
            {
                return sPath;
            }

            virtual size_t flags() const override
            {
                return nFlags;
            }

            virtual void accept() override
            {
                if (nState != S_PENDING)
                    return;
                nState  = S_ACCEPTED;
            }

            virtual void commit() override
            {
                nState  = S_EMPTY;
            }

            virtual bool pending() override
            {
                // Check accepted state
                if (nState == S_PENDING)
                    return true;
                else if ((nState != S_EMPTY) || (!bRequest))
                    return false;

                // Move pending request to path if present,
                // do it in spin-lock synchronized mode
                if (atomic_trylock(nRequest))
                {
                    // Copy the data
                    ::strncpy(sPath, sValue, PATH_MAX);
                    sPath[PATH_MAX-1]   = '\0';
                    sValue[0]           = '\0';
                    nFlags              = nReqFlags;
                    nReqFlags           = 0;
                    bRequest            = false;
                    nState              = S_PENDING;

                    atomic_unlock(nRequest);
                }

                return nState == S_PENDING;
            }

            virtual bool accepted() override
            {
                return (nState == S_ACCEPTED);
            }

            /**
             * This is non-RT-safe method to submit new path value to the RT thread
             * @param path path string to submit
             * @param len length of the path string
             * @param flags additional flags
             */
            void submit(const char *path, size_t len, size_t flags = 0)
            {
                // Determine size of path
                const size_t count = lsp_min(len, size_t(PATH_MAX - 1));

                // Wait until the queue is empty
                while (true)
                {
                    // Try to acquire critical section, this will always be true when using LV2 atom transport
                    if (atomic_trylock(nRequest))
                    {
                        // Copy data to request
                        ::memcpy(sValue, path, count);
                        sValue[count]       = '\0';
                        nReqFlags           = flags;
                        bRequest            = true; // Mark request pending

                        // Release critical section and leave the cycle
                        atomic_unlock(nRequest);
                        break;
                    }

                    // Wait for a while
                    ipc::Thread::sleep(10);
                }
            }
        };

    } /* namespace gst */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_GSTREAMER_TYPES_H_ */
