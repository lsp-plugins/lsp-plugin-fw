/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 7 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_TYPES_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_TYPES_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/plug.h>

#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/stdlib/string.h>


namespace lsp
{
    namespace vst2
    {
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

                return false;
            }

            virtual bool update()
            {
                if (pending())
                    return false;

                // Check for pending request
                if (!atomic_trylock(nDspRequest))
                    return false;
                lsp_finally { atomic_unlock(nDspRequest); };

                // Update state of the DSP
                if (nDspSerial == nDspCommit)
                    return false;

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

                return true;
            }

            virtual bool accepted()
            {
                return nFlags & F_ACCEPTED;
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
                    sDspRequest[count]  = '\0';

                    if (flags & plug::PF_STATE_RESTORE)
                    {
                        ::memcpy(sPath, path, count);
                        sPath[count]        = '\0';
                    }

                    nXFlagsReq          = flags;
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

    } /* namespace vst2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_TYPES_H_ */
