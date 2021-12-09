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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_DEFS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_DEFS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/3rdparty/steinberg/vst2.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/types.h>


namespace lsp
{
    namespace vst2
    {
        // This routine should be defined in the linked library
        typedef AEffect * (* create_instance_t) (VstInt32 uid, audioMasterCallback callback);

        typedef const char * (* get_version_t) ();

    #pragma pack(push, 1)
        typedef struct state_t
        {
            uint32_t        nItems;             // Number of elements
            uint8_t         vData[];            // Binary data
        } state_t;

        typedef struct state_buffer_t
        {
            size_t          nDataSize;          // Size of variable part in bytes
            fxBank          sHeader;            // Program header
            vst2::state_t   sState;             // VST state
        } state_buffer_t;

        typedef struct state_header_t
        {
            VstInt32        nMagic1;            // LSP_VST_USER_MAGIC
            VstInt32        nSize;              // Size of contents, again...
            VstInt32        nVersion;           // Current format version
            VstInt32        nMagic2;            // LSP_VST_USER_MAGIC
        } state_header_t;
    #pragma pack(pop)

        #define VST_MAIN_FUNCTION               vst_create_instance
        #define VST_MAIN_FUNCTION_STR           LSP_STRINGIFY(VST_MAIN_FUNCTION)

        #define LSP_VST_USER_MAGIC              CCONST('L', 'S', 'P', 'U')
        #define VST_BANK_HDR_SKIP               (2*sizeof(VstInt32))
        #define VST_PROGRAM_HDR_SKIP            (2*sizeof(VstInt32))
        #define VST_BANK_HDR_SIZE               (sizeof(fxBank) - VST_BANK_HDR_SKIP)
        #define VST_PROGRAM_HDR_SIZE            (sizeof(fxProgram) - VST_PROGRAM_HDR_SKIP)
        #define VST_STATE_BUFFER_SIZE           (VST_BANK_HDR_SIZE + sizeof(vst2::state_t))

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

        enum serial_flags_t
        {
            FLAG_PRIVATE    = 1 << 0
        };

        #define VST_FX_VERSION_KVT_SUPPORT      2000
        #define VST_FX_VERSION_JUCE_FIX         3000

        inline VstInt32 cconst(const char *vst_id)
        {
            if (vst_id == NULL)
            {
                lsp_error("Not defined cconst");
                return 0;
            }
            if (strlen(vst_id) != 4)
            {
                lsp_error("Invalid cconst: %s", vst_id);
                return 0;
            }
            return CCONST(vst_id[0], vst_id[1], vst_id[2], vst_id[3]);
        }

        inline char *cconst_to_str(char *buf, VstInt32 cconst)
        {
            buf[0] = char(cconst >> 24);
            buf[1] = char(cconst >> 16);
            buf[2] = char(cconst >> 8);
            buf[3] = char(cconst);
            buf[4] = '\0';
            return buf;
        }

        inline VstInt32 version(uint32_t lsp_version)
        {
            size_t major = LSP_MODULE_VERSION_MAJOR(lsp_version);
            size_t minor = LSP_MODULE_VERSION_MINOR(lsp_version);
            size_t micro = LSP_MODULE_VERSION_MICRO(lsp_version);

            // Limit version elemnts for VST
            if (minor >= 10)
                minor   = 9;
            if (micro >= 100)
                micro = 99;

            // The VST versioning is too dumb, make micro version extended
            return (major * 1000) + (minor * 100) + micro;
        }

    #ifdef LSP_TRACE
        inline void dump_vst_bank(const void *bank, size_t ck_size)
        {
            const uint8_t *ddump        = reinterpret_cast<const uint8_t *>(bank);
            lsp_trace("Chunk dump:");

            for (size_t offset=0; offset < ck_size; offset += 16)
            {
                // Print HEX dump
                lsp_nprintf("%08x: ", int(offset));
                for (size_t i=0; i<0x10; ++i)
                {
                    if ((offset + i) < ck_size)
                        lsp_nprintf("%02x ", int(ddump[i]));
                    else
                        lsp_nprintf("   ");
                }
                lsp_nprintf("   ");

                // Print character dump
                for (size_t i=0; i<0x10; ++i)
                {
                    if ((offset + i) < ck_size)
                    {
                        uint8_t c   = ddump[i];
                        if ((c < 0x20) || (c >= 0x7f))
                            c           = '.';
                        lsp_nprintf("%c", c);
                    }
                    else
                        lsp_nprintf(" ");
                }
                lsp_printf("");

                // Move pointer
                ddump       += 0x10;
            }
        }
    #else
        inline void dump_vst_bank(const void *bank, size_t ck_size)
        {
        }
    #endif /* LSP_TRACE */
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_DEFS_H_ */
