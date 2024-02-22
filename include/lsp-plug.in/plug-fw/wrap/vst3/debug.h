/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 янв. 2024 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DEBUG_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DEBUG_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/io/InMemoryStream.h>
#include <lsp-plug.in/io/OutMemoryStream.h>

#include <steinberg/vst3.h>

#if defined(LSP_TRACE) || defined(LSP_DEBUG)

#define VST3_DECODE(key) case Steinberg::Vst::key: return LSP_STRINGIFY(key)
#define VST3_DEFAULT return "unknown"

namespace lsp
{
    namespace vst3
    {
        class DbgOutStream: public Steinberg::IBStream
        {
            protected:
                io::OutMemoryStream sOS;
                Steinberg::IBStream *pOS;

            public:
                DbgOutStream(Steinberg::IBStream *out)
                {
                    pOS     = safe_acquire(out);
                }

                virtual ~DbgOutStream()
                {
                    safe_release(pOS);
                }

            public: // Steinberg::IBStream
                virtual Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override
                {
                    return Steinberg::kNotImplemented;
                }

                virtual Steinberg::uint32 PLUGIN_API addRef() override
                {
                    return 0;
                }

                virtual Steinberg::uint32 PLUGIN_API release() override
                {
                    return 0;
                }

            public: // Steinberg::IBStream
                virtual Steinberg::tresult PLUGIN_API read(void* buffer, Steinberg::int32 numBytes, Steinberg::int32 *numBytesRead) override
                {
                    if (numBytesRead != NULL)
                        *numBytesRead = 0;
                    return Steinberg::kResultOk;
                }

                virtual Steinberg::tresult PLUGIN_API seek(Steinberg::int64 pos, Steinberg::int32 mode, Steinberg::int64 *result) override
                {
                    return Steinberg::kNotImplemented;
                }

                virtual Steinberg::tresult PLUGIN_API write(void *buffer, Steinberg::int32 numBytes, Steinberg::int32 *numBytesWritten) override
                {
                    Steinberg::int32 written = 0;
                    Steinberg::tresult res = pOS->write(buffer, numBytes, &written);
                    if (res == Steinberg::kResultOk)
                    {
                        sOS.write(buffer, written);
                        if (numBytesWritten != NULL)
                            *numBytesWritten = written;
                    }

                    return res;
                }

                virtual Steinberg::tresult PLUGIN_API tell(Steinberg::int64 *pos) override
                {
                    return Steinberg::kNotImplemented;
                }

            public:
                inline size_t size() const              { return sOS.size(); }
                inline const uint8_t *data() const      { return sOS.data(); }
        };

        class DbgInStream: public Steinberg::IBStream
        {
            protected:
                io::InMemoryStream sIS;
                Steinberg::IBStream *pIS;
                Steinberg::tresult nResult;

                static constexpr size_t BUF_SIZE = 0x1000;

            public:
                DbgInStream(Steinberg::IBStream *out)
                {
                    uint8_t *buf = new uint8_t[BUF_SIZE];
                    lsp_finally { delete [] buf; };

                    io::OutMemoryStream os;
                    pIS     = safe_acquire(out);

                    // Read data to internal buffer
                    Steinberg::int32 num_read = 0;
                    while (true)
                    {
                        nResult = pIS->read(buf, BUF_SIZE, &num_read);
                        if (nResult != Steinberg::kResultOk)
                            break;
                        if (num_read <= 0)
                            break;
                        os.write(buf, num_read);
                    }

                    sIS.take(os);
                }

                virtual ~DbgInStream()
                {
                    safe_release(pIS);
                }

            public: // Steinberg::IBStream
                virtual Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override
                {
                    return Steinberg::kNotImplemented;
                }

                virtual Steinberg::uint32 PLUGIN_API addRef() override
                {
                    return 0;
                }

                virtual Steinberg::uint32 PLUGIN_API release() override
                {
                    return 0;
                }

            public: // Steinberg::IBStream
                virtual Steinberg::tresult PLUGIN_API read(void* buffer, Steinberg::int32 numBytes, Steinberg::int32 *numBytesRead) override
                {
                    ssize_t read = sIS.read(buffer, numBytes);
                    if (read > 0)
                    {
                        if (numBytesRead != NULL)
                            *numBytesRead = read;
                        return Steinberg::kResultOk;
                    }

                    if (numBytesRead != NULL)
                        *numBytesRead = 0;
                    return nResult;
                }

                virtual Steinberg::tresult PLUGIN_API seek(Steinberg::int64 pos, Steinberg::int32 mode, Steinberg::int64 *result) override
                {
                    return Steinberg::kNotImplemented;
                }

                virtual Steinberg::tresult PLUGIN_API write(void *buffer, Steinberg::int32 numBytes, Steinberg::int32 *numBytesWritten) override
                {
                    return Steinberg::kNotImplemented;
                }

                virtual Steinberg::tresult PLUGIN_API tell(Steinberg::int64 *pos) override
                {
                    return Steinberg::kNotImplemented;
                }

            public:
                inline size_t size() const              { return sIS.size(); }
                inline const uint8_t *data() const      { return sIS.data(); }
        };

        inline void append_option(LSPString *out, bool condition, const char *value)
        {
            if (!condition)
                return;
            if (out->length() > 0)
                out->append_ascii(", ");
            out->append_ascii(value);
        }

        inline const char *media_type_to_str(Steinberg::Vst::MediaType type)
        {
            switch (type)
            {
                VST3_DECODE(kAudio);
                VST3_DECODE(kEvent);
            }
            VST3_DEFAULT;
        }

        inline const char *bus_direction_to_str(Steinberg::Vst::MediaType type)
        {
            switch (type)
            {
                VST3_DECODE(kInput);
                VST3_DECODE(kOutput);
            }
            VST3_DEFAULT;
        }

        inline const char *bus_type_to_str(Steinberg::Vst::BusType type)
        {
            switch (type)
            {
                VST3_DECODE(kMain);
                VST3_DECODE(kAux);
            }
            VST3_DEFAULT;
        }


        inline const char *bus_flags(LSPString *tmp, Steinberg::uint32 flags)
        {
            tmp->clear();
            append_option(tmp, flags & Steinberg::Vst::BusInfo::BusFlags::kDefaultActive, "kDefaultActive");
            append_option(tmp, flags & Steinberg::Vst::BusInfo::BusFlags::kIsControlVoltage, "kIsControlVoltage");

            return tmp->get_native();
        }

        inline const char *speaker_to_str(Steinberg::Vst::Speaker speaker)
        {
            switch (speaker)
            {
                VST3_DECODE(kSpeakerL);
                VST3_DECODE(kSpeakerR);
                VST3_DECODE(kSpeakerC);
                VST3_DECODE(kSpeakerLfe);
                VST3_DECODE(kSpeakerLs);
                VST3_DECODE(kSpeakerRs);
                VST3_DECODE(kSpeakerLc);
                VST3_DECODE(kSpeakerRc);
                VST3_DECODE(kSpeakerS);
                VST3_DECODE(kSpeakerSl);
                VST3_DECODE(kSpeakerSr);
                VST3_DECODE(kSpeakerTc);
                VST3_DECODE(kSpeakerTfl);
                VST3_DECODE(kSpeakerTfc);
                VST3_DECODE(kSpeakerTfr);
                VST3_DECODE(kSpeakerTrl);
                VST3_DECODE(kSpeakerTrc);
                VST3_DECODE(kSpeakerTrr);
                VST3_DECODE(kSpeakerLfe2);
                VST3_DECODE(kSpeakerM);
                VST3_DECODE(kSpeakerACN0);
                VST3_DECODE(kSpeakerACN1);
                VST3_DECODE(kSpeakerACN2);
                VST3_DECODE(kSpeakerACN3);
                VST3_DECODE(kSpeakerACN4);
                VST3_DECODE(kSpeakerACN5);
                VST3_DECODE(kSpeakerACN6);
                VST3_DECODE(kSpeakerACN7);
                VST3_DECODE(kSpeakerACN8);
                VST3_DECODE(kSpeakerACN9);
                VST3_DECODE(kSpeakerACN10);
                VST3_DECODE(kSpeakerACN11);
                VST3_DECODE(kSpeakerACN12);
                VST3_DECODE(kSpeakerACN13);
                VST3_DECODE(kSpeakerACN14);
                VST3_DECODE(kSpeakerACN15);
                VST3_DECODE(kSpeakerACN16);
                VST3_DECODE(kSpeakerACN17);
                VST3_DECODE(kSpeakerACN18);
                VST3_DECODE(kSpeakerACN19);
                VST3_DECODE(kSpeakerACN20);
                VST3_DECODE(kSpeakerACN21);
                VST3_DECODE(kSpeakerACN22);
                VST3_DECODE(kSpeakerACN23);
                VST3_DECODE(kSpeakerACN24);
                VST3_DECODE(kSpeakerTsl);
                VST3_DECODE(kSpeakerTsr);
                VST3_DECODE(kSpeakerLcs);
                VST3_DECODE(kSpeakerRcs);
                VST3_DECODE(kSpeakerBfl);
                VST3_DECODE(kSpeakerBfc);
                VST3_DECODE(kSpeakerBfr);
                VST3_DECODE(kSpeakerPl);
                VST3_DECODE(kSpeakerPr);
                VST3_DECODE(kSpeakerBsl);
                VST3_DECODE(kSpeakerBsr);
                VST3_DECODE(kSpeakerBrl);
                VST3_DECODE(kSpeakerBrc);
                VST3_DECODE(kSpeakerBrr);
                VST3_DECODE(kSpeakerLw);
                VST3_DECODE(kSpeakerRw);
            }
            VST3_DEFAULT;
        }

        inline void log_bus_info(const Steinberg::Vst::BusInfo *bus)
        {
            LSPString tmp;
            tmp.set_utf16(vst3::to_utf16(bus->name));

            lsp_trace("  name       : %s", tmp.get_utf8());
            lsp_trace("  type       : %s", bus_type_to_str(bus->busType));
            lsp_trace("  media type : %s", media_type_to_str(bus->mediaType));
            lsp_trace("  direction  : %s", bus_direction_to_str(bus->direction));
            lsp_trace("  channels   : %d", int(bus->channelCount));
            lsp_trace("  flags      : %s", bus_flags(&tmp, bus->flags));
        }

        inline const char *parameter_flags(LSPString *tmp, Steinberg::uint32 flags)
        {
            tmp->clear();
            append_option(tmp, flags & Steinberg::Vst::ParameterInfo::ParameterFlags::kCanAutomate, "kCanAutomate");
            append_option(tmp, flags & Steinberg::Vst::ParameterInfo::ParameterFlags::kIsReadOnly, "kIsReadOnly");
            append_option(tmp, flags & Steinberg::Vst::ParameterInfo::ParameterFlags::kIsWrapAround, "kIsWrapAround");
            append_option(tmp, flags & Steinberg::Vst::ParameterInfo::ParameterFlags::kIsList, "kIsList");
            append_option(tmp, flags & Steinberg::Vst::ParameterInfo::ParameterFlags::kIsHidden, "kIsHidden");
            append_option(tmp, flags & Steinberg::Vst::ParameterInfo::ParameterFlags::kIsProgramChange, "kIsProgramChange");
            append_option(tmp, flags & Steinberg::Vst::ParameterInfo::ParameterFlags::kIsBypass, "kIsBypass");

            return tmp->get_native();
        }

        inline void log_parameter_info(const Steinberg::Vst::ParameterInfo *info)
        {
            LSPString tmp;

            lsp_trace("  id         : 0x%08x", int(info->id));
            tmp.set_utf16(vst3::to_utf16(info->title));
            lsp_trace("  title      : %s", tmp.get_utf8());
            tmp.set_utf16(vst3::to_utf16(info->shortTitle));
            lsp_trace("  shortTitle : %s", tmp.get_utf8());
            tmp.set_utf16(vst3::to_utf16(info->units));
            lsp_trace("  units      : %s", tmp.get_utf8());
            lsp_trace("  stepCount  : %d", int(info->stepCount));
            lsp_trace("  default    : %f (normalized)", info->defaultNormalizedValue);
            lsp_trace("  unitId     : %d", int(info->unitId));
            lsp_trace("  flags      : %s", parameter_flags(&tmp, info->flags));
        }

        inline const char *speaker_arrangement_to_str(LSPString *tmp, Steinberg::Vst::SpeakerArrangement arr)
        {
            tmp->clear();
            for (size_t i=0; i<64; ++i)
            {
                Steinberg::Vst::Speaker spk = Steinberg::Vst::Speaker(1) << i;
                append_option(tmp, arr & spk, speaker_to_str(spk));
            }

            return tmp->get_native();
        }

    } /* namespace vst3 */
} /* namespace lsp */

#undef VST3_DECODE
#undef VST3_DEFAULT

#endif /* defined(LSP_TRACE) || defined(LSP_DEBUG) */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_DEBUG_H_ */
