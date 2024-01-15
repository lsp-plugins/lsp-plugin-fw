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

#include <lsp-plug.in/common/endian.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
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
    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_HELPERS_H_ */
