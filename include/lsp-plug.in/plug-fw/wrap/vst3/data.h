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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DATA_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DATA_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/io/charset.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/defs.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        constexpr const char *STATE_SIGNATURE               = "LSPS";

        constexpr const char *ID_MSG_LATENCY                = "Latency";
        constexpr const char *ID_MSG_STATE_DIRTY            = "StateDirty";
        constexpr const char *ID_MSG_DUMP_STATE             = "DumpState";
        constexpr const char *ID_MSG_MUSIC_POSITION         = "MusicPosition";
        constexpr const char *ID_MSG_PLAY_SAMPLE_POSITION   = "PlaySamplePosition";
        constexpr const char *ID_MSG_VIRTUAL_PARAMETER      = "VParam";
        constexpr const char *ID_MSG_METERS                 = "Meters";
        constexpr const char *ID_MSG_PATH                   = "Path";
        constexpr const char *ID_MSG_STRING                 = "String";
        constexpr const char *ID_MSG_SHM_STATE              = "ShmState";
        constexpr const char *ID_MSG_MESH                   = "Mesh";
        constexpr const char *ID_MSG_FRAMEBUFFER            = "FrameBuffer";
        constexpr const char *ID_MSG_STREAM                 = "Stream";
        constexpr const char *ID_MSG_KVT                    = "KVT";
        constexpr const char *ID_MSG_ACTIVATE_UI            = "UIActivate";
        constexpr const char *ID_MSG_DEACTIVATE_UI          = "UIDeactivate";
        constexpr const char *ID_MSG_PRESET_STATE           = "PresetState";
        constexpr const char *ID_MSG_PLAY_SAMPLE            = "PlaySample";

        constexpr const Steinberg::Vst::ParamID MIDI_MAPPING_PARAM_BASE = 0x40000000;
        constexpr const Steinberg::Vst::ParamID PARAM_ID_MODULO         = 0x3fffffff;

        constexpr const float MIDI_FLOAT_TO_BYTE            = 127.0f;
        constexpr const float MIDI_FLOAT_TO_BEND            = 16383.0f;
        constexpr const float MIDI_BYTE_TO_FLOAT            = 1.0f / MIDI_FLOAT_TO_BYTE;
        constexpr const float MIDI_BEND_TO_FLOAT            = 1.0f / MIDI_FLOAT_TO_BEND;

        constexpr const size_t MIDI_MAPPING_SIZE            = Steinberg::Vst::kCountCtrlNumber * midi::MIDI_CHANNELS;

        enum serial_flags_t
        {
            FLAG_PRIVATE    = 1 << 0,
        };

        enum serial_types_t
        {
            TYPE_UNKNOWN    = '\0',
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

            uint8_t             nFlags;                 // Current state of path primitive
            uint8_t             nAFlags;                // Async request status
            atomic_t            nLock;                  // Atomic lock variable for async access
            uint32_t            nPathFlags;             // Path flags
            uint32_t            nPPathFlags;            // Pending path flags
            uint32_t            nAPathFlags;            // Asynchronous pending path flags

            char                sPath[MAX_PATH_LEN];    // Current path value
            char                sQPath[MAX_PATH_LEN];   // Pending path value (sync request)
            char                sAPath[MAX_PATH_LEN];   // Pending path value (async request)

            virtual void init() override
            {
                nFlags          = 0;
                atomic_store(&nAFlags, 0);
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
                return nPathFlags;
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
                return false;
            }

            bool sync()
            {
                // Check that path is not pending
                if (nFlags & F_PENDING)
                    return false;

                // Check that we have pending sync request
                if (nFlags & F_QPATH)
                {
                    strncpy(sPath, sQPath, MAX_PATH_LEN);
                    sPath[MAX_PATH_LEN-1]   = '\0';
                    sQPath[0]               = '\0';
                    nPathFlags              = nPPathFlags;
                    nFlags                  = F_PENDING;

                    lsp_trace("Pending path=%s", sPath);
                    return true;
                }

                // Check that we have pending async request
                if (!(atomic_load(&nAFlags) & XF_APATH))
                    return false;
                if (atomic_trylock(nLock))
                {
                    lsp_finally {atomic_unlock(nLock); };

                    // Copy data
                    strncpy(sPath, sAPath, MAX_PATH_LEN);
                    sPath[MAX_PATH_LEN-1]   = '\0';
                    sAPath[0]               = '\0';
                    atomic_store(&nAFlags, 0);
                    nPathFlags              = nAPathFlags;
                    nFlags                  = F_PENDING;
                }

                lsp_trace("Pending path=%s", sPath);

                return true;
            }

            virtual void commit() override
            {
                if (nFlags & (F_PENDING | F_ACCEPTED))
                    nFlags             &= ~(F_PENDING | F_ACCEPTED);
            }

            virtual bool accepted() override
            {
                return nFlags & F_ACCEPTED;
            }

            void submit(const char *path, size_t len, size_t flags)
            {
                // Write DSP request
                const size_t count  = lsp_min(len, size_t(MAX_PATH_LEN - 1));
                ::memcpy(sQPath, path, count);
                sQPath[count]       = '\0';

                if (flags & plug::PF_STATE_RESTORE)
                {
                    ::memcpy(sPath, path, count);
                    sPath[count]        = '\0';
                }

                nPathFlags          = flags;
                nFlags             |= F_QPATH;
            }

            void submit_async(const char *path, size_t flags)
            {
                while (true)
                {
                    // Try to acquire critical section
                    if (atomic_trylock(nLock))
                    {
                        lsp_finally { atomic_unlock(nLock); };

                        // Write Async request
                        ::strncpy(sAPath, path, MAX_PATH_LEN);
                        sAPath[MAX_PATH_LEN-1]  = '\0';
                        atomic_store(&nAFlags, XF_APATH);
                        nAPathFlags             = flags;
                        break;
                    }

                    // Wait for a while (10 milliseconds)
                    ipc::Thread::sleep(10);
                }
            }

        } path_t;

    } /* namespace vst3 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DATA_H_ */
