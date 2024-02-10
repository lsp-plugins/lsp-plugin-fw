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
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/io/charset.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/runtime/LSPString.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/stdlib/string.h>

#include <steinberg/vst3.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/data.h>

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

//            uint32_t refs =
            ptr->addRef();
//            lsp_trace("acquire ptr=%p, refs=%d", ptr, int(refs));
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

//            lsp_trace("release ptr=%p", ptr);
            ptr->release();
            ptr = NULL;
        }

        /**
         * Perform safe acquire of Steinberg::FUnknown object casted to some another interface
         *
         * @tparam V type to which cast the original pointer
         * @param ptr pointer to acquire
         * @return pointer to the object
         */
        template <class T, class V>
        inline T *safe_query_iface(V *ptr)
        {
            if (ptr == NULL)
                return NULL;

            Steinberg::TUID iid;
            T *result   = NULL;
            memcpy(iid, T::iid, sizeof(Steinberg::TUID));

//            IF_TRACE( char dump[36]; );
            if (ptr->queryInterface(iid, reinterpret_cast<void **>(&result)) == Steinberg::kResultOk)
            {
//                lsp_trace("query_interface iid=%s, ptr=%p, result=%p", fmt_tuid(dump, iid, sizeof(dump)), ptr, result);
                return result;
            }

//            lsp_trace("query_interface iid=%s, ptr=%p failed", fmt_tuid(dump, iid, sizeof(dump)), ptr);

            return NULL;
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
         * @return VST3 identifier as a result of hashing
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
         * Write the constant-sized block to the VST3 output streem
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
         * Read the constant-sized block from the VST3 input streem
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
         * Read simple data type from the VST3 input stream
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
                if ((res != Steinberg::kResultOk) || (read <= 0))
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
         * Write floating-point value to the stream
         * @param is output stream
         * @param key parameter name
         * @param value parameter value
         * @return status of operation
         */
        template <class T>
        inline status_t write_value(Steinberg::IBStream *is, const char *key, const T value)
        {
            status_t res = write_string(is, key);
            if (res == STATUS_OK)
            {
                T tmp   = CPU_TO_LE(value);
                res     = write_fully(is, &tmp, sizeof(T));
            }
            return res;
        }

        template <>
        inline status_t write_value(Steinberg::IBStream *is, const char *key, const char *value)
        {
            status_t res = write_string(is, key);
            if (res == STATUS_OK)
                res = write_string(is, value);
            return res;
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
         * Read the string from the VST3 input stream
         * @param is VST3 input stream
         * @param buf pointer to variable to store the pointer to the string. The previous value will be
         *   reallocated if there is not enough capacity. Should be freed by caller after use even if the
         *   execution was unsuccessful.
         * @param capacity the pointer to variable that contains the current capacity of the string
         * @return number of actual bytes read or negative error code
         */
        inline status_t read_string(Steinberg::IBStream *is, char **buf, size_t *capacity)
        {
            // Read variable-sized string length
            size_t len = 0;
            status_t res = read_varint(is, &len);
            if (res != STATUS_OK)
                return res;

            // Reallocate memory if there is not enough space
            char *s     = *buf;
            size_t cap  = *capacity;
            if ((s == NULL) || (cap < (len + 1)))
            {
                cap         = align_size(len + 1, 32);
                char *ns    = static_cast<char *>(realloc(s, sizeof(char *) * cap));
                if (ns == NULL)
                    return STATUS_NO_MEM;

                s           = ns;
                *buf        = ns;
                *capacity   = cap;
            }

            // Read the payload data
            res = read_fully(is, s, len);
            if (res == STATUS_OK)
                s[len]      = '\0';

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

        /**
         * Helper function to allocate a message
         * @param host IHostApplication instance
         */
        inline Steinberg::Vst::IMessage *alloc_message(Steinberg::Vst::IHostApplication *host)
        {
            if (host == NULL)
                return NULL;

            Steinberg::TUID iid;
            Steinberg::Vst::IMessage* m = NULL;
            memcpy(iid, Steinberg::Vst::IMessage::iid, sizeof(Steinberg::TUID));

            if (host->createInstance(iid, iid, reinterpret_cast<void **>(&m)) == Steinberg::kResultOk)
                return m;
            return NULL;
        }

        inline Steinberg::Vst::TChar *to_tchar(lsp_utf16_t *str)
        {
            return reinterpret_cast<Steinberg::Vst::TChar *>(str);
        }

        inline const Steinberg::Vst::TChar *to_tchar(const lsp_utf16_t *str)
        {
            return reinterpret_cast<const Steinberg::Vst::TChar *>(str);
        }

        inline lsp_utf16_t *to_utf16(Steinberg::Vst::TChar *str)
        {
            return reinterpret_cast<lsp_utf16_t *>(str);
        }

        inline const lsp_utf16_t *to_utf16(const Steinberg::Vst::TChar *str)
        {
            return reinterpret_cast<const lsp_utf16_t *>(str);
        }

        inline size_t strnlen_u16(const lsp_utf16_t *str, size_t len)
        {
            for (size_t i=0; i<len; ++i)
                if (str[i] == 0)
                    return i;
            return len;
        }

        inline uint8_t encode_param_type(core::kvt_param_type_t type)
        {
            switch (type)
            {
                case core::KVT_INT32:   return TYPE_INT32;
                case core::KVT_UINT32:  return TYPE_UINT32;
                case core::KVT_INT64:   return TYPE_INT64;
                case core::KVT_UINT64:  return TYPE_UINT64;
                case core::KVT_FLOAT32: return TYPE_FLOAT32;
                case core::KVT_FLOAT64: return TYPE_FLOAT64;
                case core::KVT_STRING:  return TYPE_STRING;
                case core::KVT_BLOB:    return TYPE_BLOB;
                default:                break;
            }
            return TYPE_UNKNOWN;
        }

        inline status_t write_kvt_value(Steinberg::IBStream *os, const core::kvt_param_t *p, uint8_t flags)
        {
            status_t res;

            if ((res = write_fully(os, &flags, sizeof(flags))) != STATUS_OK)
                return res;
            uint8_t type = encode_param_type(p->type);
            if ((res = write_fully(os, &type, sizeof(type))) != STATUS_OK)
                return res;

            // Serialize parameter according to it's type
            switch (p->type)
            {
                case core::KVT_INT32:
                    res = write_fully(os, &p->i32, sizeof(p->i32));
                    break;
                case core::KVT_UINT32:
                    res = write_fully(os, &p->u32, sizeof(p->u32));
                    break;
                case core::KVT_INT64:
                    res = write_fully(os, &p->i64, sizeof(p->i64));
                    break;
                case core::KVT_UINT64:
                    res = write_fully(os, &p->u64, sizeof(p->u64));
                    break;
                case core::KVT_FLOAT32:
                    res = write_fully(os, &p->f32, sizeof(p->f32));
                    break;
                case core::KVT_FLOAT64:
                    res = write_fully(os, &p->f64, sizeof(p->f64));
                    break;
                case core::KVT_STRING:
                {
                    const char *str = (p->str != NULL) ? p->str : "";
                    res = write_string(os, str);
                    break;
                }
                case core::KVT_BLOB:
                {
                    if ((p->blob.size > 0) && (p->blob.data == NULL))
                    {
                        lsp_warn("Non-empty blob with NULL data pointer for KVT parameter");
                        return STATUS_INVALID_VALUE;
                    }

                    const char *ctype = (p->blob.ctype != NULL) ? p->blob.ctype : "";
                    res = write_string(os, ctype);
                    if ((res == STATUS_OK) && (p->blob.size > 0))
                        res = write_fully(os, p->blob.data, p->blob.size);
                    break;
                }

                default:
                    lsp_trace("KVT serialization failed: unknown parameter type");
                    return STATUS_BAD_TYPE;
            }

            return STATUS_OK;
        }

        inline void destroy_kvt_value(core::kvt_param_t *p)
        {
            switch (p->type)
            {
                case core::KVT_STRING:
                {
                    if (p->str != NULL)
                    {
                        free(const_cast<char *>(p->str));
                        p->str          = NULL;
                    }
                    break;
                }
                case core::KVT_BLOB:
                {
                    if (p->blob.ctype != NULL)
                    {
                        free(const_cast<char *>(p->blob.ctype));
                        p->blob.ctype   = NULL;
                    }
                    if (p->blob.data != NULL)
                    {
                        free(const_cast<void *>(p->blob.data));
                        p->blob.data    = NULL;
                    }
                    break;
                }
                default:
                    break;
            }

            p->type = core::KVT_ANY;
        }

        inline status_t read_kvt_value(Steinberg::IBStream *is, const char *name, core::kvt_param_t *p)
        {
            status_t res;
            uint8_t type = 0;

            p->type = core::KVT_ANY;

            // Read the type
            if ((res = read_fully(is, &type)) != STATUS_OK)
            {
                lsp_warn("Failed to read type for port id=%s", name);
                return res;
            }

            lsp_trace("Parameter type: '%c'", char(p->type));

            switch (type)
            {
                case vst3::TYPE_INT32:
                    p->type         = core::KVT_INT32;
                    res             = read_fully(is, &p->i32);
                    break;
                case vst3::TYPE_UINT32:
                    p->type         = core::KVT_UINT32;
                    res             = read_fully(is, &p->u32);
                    break;
                case vst3::TYPE_INT64:
                    p->type         = core::KVT_INT64;
                    res             = read_fully(is, &p->i64);
                    break;
                case vst3::TYPE_UINT64:
                    p->type         = core::KVT_UINT64;
                    res             = read_fully(is, &p->u64);
                    break;
                case vst3::TYPE_FLOAT32:
                    p->type         = core::KVT_FLOAT32;
                    res             = read_fully(is, &p->f32);
                    break;
                case vst3::TYPE_FLOAT64:
                    p->type         = core::KVT_FLOAT64;
                    res             = read_fully(is, &p->f64);
                    break;
                case vst3::TYPE_STRING:
                {
                    char *str       = NULL;
                    size_t cap      = 0;

                    p->type         = core::KVT_STRING;
                    p->str          = NULL;
                    res             = read_string(is, &str, &cap);
                    if (res == STATUS_OK)
                        p->str      = str;
                    break;
                }
                case vst3::TYPE_BLOB:
                {
                    uint32_t size   = 0;
                    char *ctype     = NULL;
                    uint8_t *data   = NULL;
                    size_t cap      = 0;

                    lsp_finally {
                        if (ctype != NULL)
                            free(ctype);
                        if (data != NULL)
                            free(data);
                    };

                    p->type         = core::KVT_BLOB;
                    p->blob.ctype   = NULL;
                    p->blob.data    = NULL;
                    if ((res = read_fully(is, &size)) != STATUS_OK)
                        return res;
                    lsp_trace("BLOB size: %d (0x%x)", int(size), int(size));
                    if ((res = read_string(is, &ctype, &cap)) != STATUS_OK)
                        return res;
                    lsp_trace("BLOB content type: %s", ctype);

                    if (size > 0)
                    {
                        data            = static_cast<uint8_t *>(malloc(size));
                        if (data == NULL)
                            return STATUS_NO_MEM;
                        if ((res = read_fully(is, data, size)) != STATUS_OK)
                            return res;
                    }

                    p->blob.ctype   = ctype;
                    p->blob.data    = data;
                    p->blob.size    = size;
                    ctype           = NULL;
                    data            = NULL;

                    break;
                }
                default:
                    lsp_warn("Unknown KVT parameter type: %d ('%c') for id=%s", type, type, name);
                    break;
            }

            return res;
        }

        inline float to_vst_value(const meta::port_t *meta, float value)
        {
            float min = 0.0f, max = 1.0f, step = 0.0f;
            meta::get_port_parameters(meta, &min, &max, &step);

//            lsp_trace("input=%f, min=%f, max=%f, step=%f", value, min, max, step);

            // Set value as integer or normalized
            if ((meta::is_gain_unit(meta->unit)) || (meta::is_log_rule(meta)))
            {
//                        float p_value   = value;

                float thresh    = (meta->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                float l_step    = log(step + 1.0f) * 0.1f;
                float l_thresh  = log(thresh);

                float l_min     = (fabsf(min)   < thresh) ? (l_thresh - l_step) : (log(min));
                float l_max     = (fabsf(max)   < thresh) ? (l_thresh - l_step) : (log(max));
                float l_value   = (fabsf(value) < thresh) ? (l_thresh - l_step) : (log(value));

                value           = (l_value - l_min) / (l_max - l_min);

                min             = 0.0f;
                max             = 1.0f;
            }
            else if (meta->unit == meta::U_BOOL)
            {
                value           = (value >= (min + max) * 0.5f) ? 1.0f : 0.0f;
                min             = 0.0f;
                max             = 1.0f;
            }
            else
            {
                if ((meta->flags & meta::F_INT) ||
                    (meta->unit == meta::U_ENUM) ||
                    (meta->unit == meta::U_SAMPLES))
                    value  = truncf(value);

                // Normalize value
                value           = (max != min) ? (value - min) / (max - min) : 0.0f;
                min             = 0.0f;
                max             = 1.0f;
            }

//            lsp_trace("result = %f", value);
            return value;
        }

        inline float from_vst_value(const meta::port_t *meta, float value)
        {
//                lsp_trace("input = %.3f", value);
            // Set value as integer or normalized
            float min = 0.0f, max = 1.0f, step = 0.0f;
            meta::get_port_parameters(meta, &min, &max, &step);

            if ((meta::is_gain_unit(meta->unit)) || (meta::is_log_rule(meta)))
            {
//                        float p_value   = value;
                float thresh    = (meta->flags & meta::F_EXT) ? GAIN_AMP_M_140_DB : GAIN_AMP_M_80_DB;
                float l_step    = log(step + 1.0f) * 0.1f;
                float l_thresh  = log(thresh);
                float l_min     = (fabsf(min)  < thresh) ? (l_thresh - l_step) : (log(min));
                float l_max     = (fabsf(max)  < thresh) ? (l_thresh - l_step) : (log(max));

                value           = value * (l_max - l_min) + l_min;
                value           = (value < l_thresh) ? 0.0f : expf(value);

//                        lsp_trace("%s = %f (%f, %f, %f) -> %f (%f, %f, %f)",
//                            pMetadata->id,
//                            p_value,
//                            l_thresh, l_min, l_max,
//                            value,
//                            min, max, step);
            }
            else if (meta->unit == meta::U_BOOL)
            {
                value = (value >= 0.5f) ? max : min;
            }
            else
            {
                value = min + value * (max - min) + 1e-5f;
                if ((meta->flags & meta::F_INT) ||
                    (meta->unit == meta::U_ENUM) ||
                    (meta->unit == meta::U_SAMPLES))
                    value  = truncf(value);
            }

//                lsp_trace("result = %.3f", value);
            return value;
        }

        inline const char *get_unit_name(meta::unit_t unit)
        {
            if (meta::is_gain_unit(unit))
                return "dB";
            const char *res = meta::get_unit_name(unit);
            return (res != NULL) ? res : "";
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_HELPERS_H_ */
