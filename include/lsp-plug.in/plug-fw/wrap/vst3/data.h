/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 янв. 2024 г.
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

#ifndef _LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DATA_H_
#define _LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DATA_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/io/charset.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
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
                F_ACCEPTED      = 1 << 1,
                F_QPATH         = 1 << 2
            };

            enum xflags_t
            {
                XF_APATH        = 1 << 0
            };

            uint8_t             nFlags;
            uint8_t             nXFlags;
            atomic_t            nLock;

            char                sPath[PATH_MAX*2];
            char                sQPath[PATH_MAX*2];
            lsp_utf16_t         sAPath[PATH_MAX];

            virtual void init() override
            {
                nFlags          = 0;
                nXFlags         = 0;
                atomic_init(nLock);
                sPath[0]        = '\0';
                sQPath[0]       = '\0';
                sAPath[0]       = 0;
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
                if (nFlags & F_PENDING)
                    nFlags     |= F_ACCEPTED;
            }

            virtual bool pending() override
            {
                // Check accepted flags
                if (nFlags & F_PENDING)
                    return !(nFlags & F_ACCEPTED);

                if (nFlags & F_QPATH)
                {
                    strncpy(sPath, sQPath, PATH_MAX*2);
                    sPath[PATH_MAX*2-1] = '\0';
                    sQPath[0]           = '\0';
                    nFlags              = F_PENDING;
                    return true;
                }

                // Check for pending request
                if (!atomic_trylock(nLock))
                    return false;
                lsp_finally {atomic_unlock(nLock); };

                // Update state of the DSP
                if (!(nXFlags & XF_APATH))
                    return false;

                // Transform string and update flags
                if (!lsp::utf16le_to_utf8(sPath, sAPath, PATH_MAX*2))
                    sPath[0]            = '\0';

                nXFlags             = 0;
                nFlags              = F_PENDING;

                return true;
            }

            virtual void commit() override
            {
                if (nFlags & (F_PENDING | F_ACCEPTED))
                    nFlags             &= ~(F_PENDING | F_ACCEPTED);
            }

            virtual bool accepted()
            {
                return nFlags & F_ACCEPTED;
            }

            status_t serialize(Steinberg::IBStream *os)
            {
                ssize_t res = write_string(os, sPath);
                return (res < 0) ? -res : STATUS_OK;
            }

            status_t deserialize(Steinberg::IBStream *is)
            {
                char *dst           = ((nFlags & (F_PENDING | F_ACCEPTED)) == (F_PENDING | F_ACCEPTED)) ? sQPath : sPath;

                // Deserialize as DSP request
                status_t res = read_string(is, dst, PATH_MAX*2-1);
                if (res != STATUS_OK)
                    return res;

                return STATUS_OK;
            }

            void submit(const char *path, size_t len, size_t flags)
            {
                const size_t count  = lsp_min(len, size_t(PATH_MAX*2-1));

                // Write DSP request
                ::strncpy(sPath, path, count);
                sPath[count]        = '\0';
                nFlags             |= F_QPATH;
            }

            void submit(Steinberg::Vst::IAttributeList *atts)
            {
                while (true)
                {
                    // Try to acquire critical section
                    if (atomic_trylock(nLock))
                    {
                        lsp_finally { atomic_unlock(nLock); };

                        // Write DSP request
                        Steinberg::tresult res = atts->getString(
                            "value",
                            reinterpret_cast<Steinberg::Vst::TChar *>(sAPath),
                            PATH_MAX * sizeof(lsp_utf16_t));
                        if (res != Steinberg::kResultOk)
                            sAPath[0]       = 0;
                        nXFlags = XF_APATH;
                        break;
                    }

                    // Wait for a while (10 milliseconds)
                    ipc::Thread::sleep(10);
                }
            }

        } path_t;

    } /* namespace vst3 */
} /* namespace lsp */




#endif /* _LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DATA_H_ */
