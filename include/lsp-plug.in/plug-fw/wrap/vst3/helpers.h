/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 28 дек. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_HELPERS_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_HELPERS_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/stdlib/string.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        inline bool read_octet(char & dst, const char *str)
        {
            uint8_t c1 = str[0];
            uint8_t c2 = str[1];

            if ((c1 >= '0') && (c1 <= '9'))
                c1     -= '0';
            else if ((c1 >= 'a') && (c1 <= 'f'))
                c1      = c1 + 10 - 'a';
            else if ((c1 >= 'A') && (c1 <= 'F'))
                c1      = c1 + 10 - 'A';
            else
                return false;

            if ((c2 >= '0') && (c2 <= '9'))
                c2     -= '0';
            else if ((c2 >= 'a') && (c2 <= 'f'))
                c2      = c2 + 10 - 'a';
            else if ((c2 >= 'A') && (c2 <= 'F'))
                c2      = c2 + 10 - 'A';
            else
                return false;

            dst     = (c1 << 4) | c2;

            return true;
        }

        /**
         * Parse TUID from string to the TUID data structure
         *
         * @param tuid TUID data structure
         * @param str string to parse (different forms of TUID available)
         * @return status of operation
         */
        inline status_t parse_tuid(Steinberg::TUID tuid, const char *str)
        {
            size_t len = strlen(str);

            if (len == 16)
            {
                uint32_t v[4];
                v[0]    = CPU_TO_BE(*reinterpret_cast<const uint32_t *>(&str[0]));
                v[1]    = CPU_TO_BE(*reinterpret_cast<const uint32_t *>(&str[4]));
                v[2]    = CPU_TO_BE(*reinterpret_cast<const uint32_t *>(&str[8]));
                v[3]    = CPU_TO_BE(*reinterpret_cast<const uint32_t *>(&str[12]));

                const Steinberg::TUID uid = INLINE_UID(v[0], v[1], v[2], v[3]);

                memcpy(tuid, uid, sizeof(Steinberg::TUID));

                return STATUS_OK;
            }
            else if (len == 32)
            {
                for (size_t i=0; i<16; ++i)
                {
                    if (!read_octet(tuid[i], &str[i*2]))
                        return STATUS_INVALID_UID;
                }

                return STATUS_OK;
            }

            return STATUS_INVALID_UID;
        }

        /**
         * Perform safe acquire of Steinberg::FUnknown object
         *
         * @tparam V type to which cast the original pointer
         * @param ptr pointer to acquire
         * @return pointer to the object
         */
        template <class T>
        inline T *safe_acquire(T *ptr)
        {
            if (ptr == NULL)
                return NULL;

            ptr->addRef();
            return ptr;
        }

        /**
         * Perform safe release of Steinberg::FUnknown object
         * @tparam T type of the class derived from Steinberg::FUnknown
         * @param ptr field that stores the pointer, is automatically reset to NULL.
         */
        template <class T>
        inline void safe_release(T * &ptr)
        {
            if (ptr == NULL)
                return;

            ptr->release();
            ptr = NULL;
        }

        /**
         * Implement the cast operation for the queryInterface call.
         * @tparam V destination type of pointer to cast
         * @param ptr original pointer to cast
         * @param obj destination to store pointer
         * @return Steinberg::kResultOk
         */
        template <class T, class V>
        inline Steinberg::tresult cast_interface(V *ptr, void **obj)
        {
            ptr->addRef();
            *obj = static_cast<T *>(ptr);
            return Steinberg::kResultOk;
        }

        /**
         * Return the value when it is not possible to implement queryInterface call.
         * @param obj destination to store pointer
         * @return Steinberg::kNoInterface
         */
        inline Steinberg::tresult no_interface(void **obj)
        {
            *obj = NULL;
            return Steinberg::kNoInterface;
        }

        /**
         * Hash the string value and return the hash value as a VST parameter identifier
         * @param str string to hash
         * @return clap identifier as a result of hashing
         */
        inline Steinberg::Vst::ParamID gen_parameter_id(const char *str)
        {
            constexpr size_t num_primes = 8;
            static const uint16_t primes[num_primes] = {
                0x80ab, 0x815f, 0x8d41, 0x9161,
                0x9463, 0x9b77, 0xabc1, 0xb567,
            };

            size_t prime_id = 0;
            size_t len      = strlen(str);
            Steinberg::Vst::ParamID res = len * primes[prime_id];

            for (size_t i=0; i<len; ++i)
            {
                prime_id        = (prime_id + 1) % num_primes;
                res             = Steinberg::Vst::ParamID(res << 7) | Steinberg::Vst::ParamID((res >> (sizeof(Steinberg::Vst::ParamID) * 8 - 7)) & 0x7f); // rotate 7 bits left
                res            += uint8_t(str[i]) * Steinberg::Vst::ParamID(primes[prime_id]);
            }

            return res;
        }

        /**
         * Write the constant-sized block to the CLAP output streem
         * @param os VST3 output stream
         * @param buf buffer that should be written
         * @param size size of buffer to write
         * @return status of operation
         */
        inline status_t write_fully(Steinberg::IBStream *os, const void *buf, size_t size)
        {
            uint8_t *ptr = const_cast<uint8_t *>(static_cast<const uint8_t *>(buf));
            Steinberg::int32 written = 0;
            for (size_t offset = 0; offset < size; )
            {
                Steinberg::tresult res = os->write(&ptr[offset], size - offset, &written);
                if (res != Steinberg::kResultOk)
                    return STATUS_IO_ERROR;
                offset         += written;
            }
            return STATUS_OK;
        }

        /**
         * Write simple data type to the CLAP output stream
         * @param os VST3 output stream
         * @param value value to write
         * @return status of operation
         */
        template <class T>
        inline status_t write_fully(Steinberg::IBStream *os, const T &value)
        {
            T tmp   = CPU_TO_LE(value);
            return write_fully(os, &tmp, sizeof(tmp));
        }

        /**
         * Read the constant-sized block from the CLAP input streem
         * @param is VST3 input stream
         * @param buf target buffer to read the data to
         * @param size size of buffer to read
         * @return status of operation
         */
        inline status_t read_fully(Steinberg::IBStream *is, void *buf, size_t size)
        {
            uint8_t *ptr = static_cast<uint8_t *>(buf);
            Steinberg::int32 read = 0;
            for (size_t offset = 0; offset < size; )
            {
                Steinberg::tresult res = is->read(&ptr[offset], size - offset, &read);
                if ((res != Steinberg::kResultOk) || (read <= 0))
                {
                    if (read < 0)
                        return STATUS_IO_ERROR;
                    return (offset > 0) ? STATUS_CORRUPTED : STATUS_EOF;
                }
                offset         += read;
            }
            return STATUS_OK;
        }

        /**
         * Read simple data type from the CLAP input stream
         * @param is VST3 input stream
         * @param value value to write
         * @return status of operation
         */
        template <class T>
        inline status_t read_fully(Steinberg::IBStream *is, T *value)
        {
            T tmp;
            status_t res = read_fully(is, &tmp, sizeof(tmp));
            if (res == STATUS_OK)
                *value      = LE_TO_CPU(tmp);
            return STATUS_OK;
        }

        inline status_t write_varint(Steinberg::IBStream *os, size_t value)
        {
            Steinberg::int32 written = 0;
            do {
                uint8_t b   = (value >= 0x80) ? 0x80 | (value & 0x7f) : value;
                value     >>= 7;

                Steinberg::tresult res = os->write(&b, sizeof(b), &written);
                if ((res != Steinberg::kResultOk) || (written < 0))
                    return STATUS_IO_ERROR;
            } while (value > 0);

            return STATUS_OK;
        }

        /**
         * Write string to VST3 output stream
         * @param os VST3 output stream
         * @param s NULL-terminated string to write
         * @return number of actual bytes written or negative error code
         */
        inline status_t write_string(Steinberg::IBStream *os, const char *s)
        {
            size_t len = strlen(s);

            // Write variable-sized string length
            status_t res = write_varint(os, len);
            if (res != STATUS_OK)
                return res;

            // Write the payload data
            return write_fully(os, s, len);
        }

        /**
         * Read the variable-sized integer
         * @param is input stream to perform read
         * @param value the pointer to store the read value
         * @return status of operation
         */
        inline status_t read_varint(Steinberg::IBStream *is, size_t *value)
        {
            // Read variable-sized string length
            size_t len = 0, shift = 0;
            Steinberg::int32 read;
            while (true)
            {
                uint8_t b;
                Steinberg::tresult res = is->read(&b, sizeof(b), &read);
                if ((res != Steinberg::kResultOk) || (read < 0))
                {
                    if (read < 0)
                        return STATUS_IO_ERROR;
                    return (shift > 0) ? STATUS_CORRUPTED : STATUS_EOF;
                }

                // Commit part of the value to the result variable
                len    |= size_t(b & 0x7f) << shift;
                if (!(b & 0x80)) // Last byte in the sequence?
                    break;
                shift += 7;
                if (shift > ((sizeof(size_t) * 8) - 7))
                    return STATUS_OVERFLOW;
            }

            *value = len;
            return STATUS_OK;
        }

        /**
         * Read the string from the VST3 input stream
         * @param is VST3 input stream
         * @param buf buffer to store the string
         * @param maxlen the maximum available string length. @note The value should consider
         *        that the destination buffer holds at least one more character for NULL-terminating
         *        character
         * @return number of actual bytes read or negative error code
         */
        inline status_t read_string(Steinberg::IBStream *is, char *buf, size_t maxlen)
        {
            // Read variable-sized string length
            size_t len = 0;
            status_t res = read_varint(is, &len);
            if (res != STATUS_OK)
                return res;
            if (len > maxlen)
                return STATUS_OVERFLOW;

            // Read the payload data
            res = read_fully(is, buf, len);
            if (res == STATUS_OK)
                buf[len]  = '\0';
            return STATUS_OK;
        }

        /**
         * Make plugin categories
         *
         * @param dst destination string to store plugin categories
         * @param meta plugin metadata
         *
         * @return status of operation
         */
        inline status_t make_plugin_categories(LSPString *dst, const meta::plugin_t *meta)
        {
            LSPString tmp;
            bool is_instrument = false;
            lltl::phashset<char> visited;

            // Scan features (part 1)
            for (const int *c = meta->classes; (c != NULL) && (*c >= 0); ++c)
            {
                Steinberg::Vst::CString code = NULL;

                switch (*c)
                {
                    case meta::C_DELAY:
                        code = Steinberg::Vst::PlugType::kFxDelay;
                        break;
                    case meta::C_REVERB:
                        code = Steinberg::Vst::PlugType::kFxReverb;
                        break;
                    case meta::C_DISTORTION:
                    case meta::C_WAVESHAPER:
                    case meta::C_AMPLIFIER:
                    case meta::C_SIMULATOR:
                        code = Steinberg::Vst::PlugType::kFxDistortion;
                        break;
                    case meta::C_DYNAMICS:
                    case meta::C_COMPRESSOR:
                    case meta::C_ENVELOPE:
                    case meta::C_EXPANDER:
                    case meta::C_GATE:
                    case meta::C_LIMITER:
                        code = Steinberg::Vst::PlugType::kFxDynamics;
                        break;
                    case meta::C_FILTER:
                    case meta::C_ALLPASS:
                    case meta::C_BANDPASS:
                    case meta::C_COMB:
                    case meta::C_HIGHPASS:
                    case meta::C_LOWPASS:
                        code = Steinberg::Vst::PlugType::kFxFilter;
                        break;
                    case meta::C_EQ:
                    case meta::C_MULTI_EQ:
                    case meta::C_PARA_EQ:
                        code = Steinberg::Vst::PlugType::kFxEQ;
                        break;
                    case meta::C_GENERATOR:
                    case meta::C_OSCILLATOR:
                        code = Steinberg::Vst::PlugType::kFxGenerator;
                        break;
                    case meta::C_CONSTANT:
                    case meta::C_SPECTRAL:
                    case meta::C_UTILITY:
                    case meta::C_CONVERTER:
                    case meta::C_FUNCTION:
                    case meta::C_MIXER:
                        code = Steinberg::Vst::PlugType::kFxTools;
                        break;
                    case meta::C_INSTRUMENT:
                        code = Steinberg::Vst::PlugType::kInstrument;
                        is_instrument   = true;
                        break;
                    case meta::C_DRUM:
                        code = Steinberg::Vst::PlugType::kInstrumentDrum;
                        is_instrument   = true;
                        break;
                    case meta::C_EXTERNAL:
                        code = Steinberg::Vst::PlugType::kInstrumentExternal;
                        is_instrument   = true;
                        break;
                    case meta::C_PIANO:
                        code = Steinberg::Vst::PlugType::kInstrumentPiano;
                        is_instrument   = true;
                        break;
                    case meta::C_SAMPLER:
                        code = Steinberg::Vst::PlugType::kInstrumentSampler;
                        is_instrument   = true;
                        break;
                    case meta::C_SYNTH_SAMPLER:
                        code = Steinberg::Vst::PlugType::kInstrumentSynthSampler;
                        is_instrument   = true;
                        break;
                    case meta::C_MODULATOR:
                    case meta::C_CHORUS:
                    case meta::C_FLANGER:
                    case meta::C_PHASER:
                        code = Steinberg::Vst::PlugType::kFxModulation;
                        break;
                    case meta::C_SPATIAL:
                        code = Steinberg::Vst::PlugType::kFxSpatial;
                        break;
                    case meta::C_PITCH:
                        code = Steinberg::Vst::PlugType::kFxPitchShift;
                        break;
                    case meta::C_ANALYSER:
                        code = Steinberg::Vst::PlugType::kFxAnalyzer;
                        break;
                    default:
                        break;
                }

                if ((code == NULL) || (!visited.create(const_cast<char *>(code))))
                    continue;

                if (!tmp.is_empty())
                {
                    if (!tmp.append('|'))
                        return STATUS_NO_MEM;
                }
                if (!tmp.append_ascii(code))
                    return STATUS_NO_MEM;
            }

            // Scan additional features (part 2)
            for (const int *f=meta->clap_features; (f != NULL) && (*f >= 0); ++f)
            {
                Steinberg::Vst::CString code = NULL;

                switch (*f)
                {
                    case meta::CF_INSTRUMENT:
                        code = Steinberg::Vst::PlugType::kInstrument;
                        is_instrument   = true;
                        break;
                    case meta::CF_SYNTHESIZER:
                        code = Steinberg::Vst::PlugType::kInstrumentSynth;
                        is_instrument   = true;
                        break;
                    case meta::CF_SAMPLER:
                        code = Steinberg::Vst::PlugType::kInstrumentSampler;
                        is_instrument   = true;
                        break;
                    case meta::CF_DRUM:
                    case meta::CF_DRUM_MACHINE:
                        code = Steinberg::Vst::PlugType::kInstrumentDrum;
                        is_instrument   = true;
                        break;
                    case meta::CF_FILTER:
                        code = Steinberg::Vst::PlugType::kFxFilter;
                        break;
                    case meta::CF_MONO:
                        code = Steinberg::Vst::PlugType::kMono;
                        break;
                    case meta::CF_STEREO:
                        code = Steinberg::Vst::PlugType::kStereo;
                        break;
                    case meta::CF_SURROUND:
                        code = Steinberg::Vst::PlugType::kSurround;
                        break;
                    case meta::CF_AMBISONIC:
                        code = Steinberg::Vst::PlugType::kAmbisonics;
                        break;
                    default:
                        break;
                }

                if ((code == NULL) || (!visited.create(const_cast<char *>(code))))
                    continue;

                if (!tmp.is_empty())
                {
                    if (!tmp.append('|'))
                        return STATUS_NO_MEM;
                }
                if (!tmp.append_ascii(code))
                    return STATUS_NO_MEM;
            }

            // Add Fx/Instrument if there is no categorization
            if (tmp.is_empty())
            {
                // Check for MIDI ports present in metadata
                if (!is_instrument)
                {
                    for (const meta::port_t *p = meta->ports; (p != NULL) && (p->id != NULL); ++p)
                    {
                        if (meta::is_midi_out_port(p))
                        {
                            is_instrument   = true;
                            break;
                        }
                    }
                }

                // Generate Fx/Instrument
                if (!tmp.append_ascii((is_instrument) ? Steinberg::Vst::PlugType::kInstrument : Steinberg::Vst::PlugType::kFx))
                    return STATUS_NO_MEM;
            }

            tmp.swap(dst);

            return STATUS_OK;
        }

        inline plug::mesh_t *create_mesh(const meta::port_t *meta)
        {
            size_t buffers      = meta->step;
            size_t buf_size     = meta->start * sizeof(float);
            size_t mesh_size    = sizeof(plug::mesh_t) + sizeof(float *) * buffers;

            // Align values to 64-byte boundaries
            buf_size            = align_size(buf_size, 0x40);
            mesh_size           = align_size(mesh_size, 0x40);

            // Allocate pointer
            uint8_t *ptr        = static_cast<uint8_t *>(malloc(mesh_size + buf_size * buffers));
            if (ptr == NULL)
                return NULL;

            // Initialize references
            plug::mesh_t *mesh  = reinterpret_cast<plug::mesh_t *>(ptr);
            mesh->nState        = plug::M_EMPTY;
            mesh->nBuffers      = 0;
            mesh->nItems        = 0;
            ptr                += mesh_size;
            for (size_t i=0; i<buffers; ++i)
            {
                mesh->pvData[i]     = reinterpret_cast<float *>(ptr);
                ptr                += buf_size;
            }

            return mesh;
        }

        inline void destroy_mesh(plug::mesh_t *mesh)
        {
            if (mesh != NULL)
                free(mesh);
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_HELPERS_H_ */
