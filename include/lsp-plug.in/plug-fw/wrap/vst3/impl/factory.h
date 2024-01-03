/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 дек. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ui_wrapper.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/string.h>

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        PluginFactory::PluginFactory()
        {
            nRefCounter         = 1;
            pLoader             = NULL;
        }

        PluginFactory::~PluginFactory()
        {
            destroy();
        }

        status_t PluginFactory::init()
        {
            // Create resource loader
            pLoader             = core::create_resource_loader();
            if (pLoader == NULL)
            {
                lsp_error("No resource loader available");
                return STATUS_BAD_STATE;
            }

            // Obtain the manifest
            lsp_trace("Obtaining manifest...");

            status_t res;
            meta::package_t *manifest = NULL;
            io::IInStream *is = pLoader->read_stream(LSP_BUILTIN_PREFIX "manifest.json");
            if (is != NULL)
            {
                lsp_finally {
                    is->close();
                    delete is;
                };

                if ((res = meta::load_manifest(&manifest, is)) != STATUS_OK)
                {
                    lsp_warn("Error loading manifest file, error=%d", int(res));
                    manifest = NULL;
                }
            }
            if (manifest == NULL)
            {
                lsp_trace("No manifest file found");
                return STATUS_BAD_STATE;
            }
            lsp_finally {
                meta::free_manifest(manifest);
            };

            // Fill the factory info
            fill_factory_info(manifest);
            if ((res = fill_plugin_info(manifest)) != STATUS_OK)
                return res;

            return STATUS_OK;
        }

        void PluginFactory::fill_factory_info(const meta::package_t *manifest)
        {
            Steinberg::strncpy8(sFactoryInfo.vendor, manifest->brand, Steinberg::PFactoryInfo::kNameSize);
            Steinberg::strncpy8(sFactoryInfo.url, manifest->site, Steinberg::PFactoryInfo::kURLSize);
            Steinberg::strncpy8(sFactoryInfo.email, manifest->email, Steinberg::PFactoryInfo::kEmailSize);
            sFactoryInfo.flags = Steinberg::Vst::kDefaultFactoryFlags;
        }

        status_t PluginFactory::fill_plugin_info(const meta::package_t *manifest)
        {
            status_t res;

            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *plug_meta = f->enumerate(i);
                    if (plug_meta == NULL)
                        break;

                    if (plug_meta->vst3_uid != NULL)
                    {
                        // We have new plugin record, create class info for this plugin
                        if ((res = create_class_info(manifest, plug_meta)) != STATUS_OK)
                            return res;
                        if ((res = create_class_info2(manifest, plug_meta)) != STATUS_OK)
                            return res;
                        if ((res = create_class_infow(manifest, plug_meta)) != STATUS_OK)
                            return res;
                    }
                }
            }

            return STATUS_OK;
        }

        status_t PluginFactory::make_plugin_categories(LSPString *dst, const meta::plugin_t *meta)
        {
            LSPString tmp;
            bool is_instrument = false;
            lltl::phashset<char> visited;

            // Scan features (part 1)
            for (const int *c = meta->classes; (c != NULL) && (c >= 0); ++c)
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
            for (const int *f=meta->clap_features; *f >= 0; ++f)
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

        status_t PluginFactory::create_class_info(const meta::package_t *manifest, const meta::plugin_t *meta)
        {
            status_t res;

            // Generate class info for processor
            Steinberg::PClassInfo *ci = vClassInfo.add();
            if (ci == NULL)
                return STATUS_NO_MEM;

            if ((res = parse_tuid(ci->cid, meta->vst3_uid)) != STATUS_OK)
                return res;
            ci->cardinality = Steinberg::PClassInfo::kManyInstances;
            Steinberg::strncpy8(ci->category, kVstAudioEffectClass, Steinberg::PClassInfo::kCategorySize);
            Steinberg::strncpy8(ci->name, meta->description, Steinberg::PClassInfo::kNameSize);

            // Generate class info for controller
            if (meta->vst3ui_uid != NULL)
            {
                ci = vClassInfo.add();
                if (ci == NULL)
                    return STATUS_NO_MEM;

                if ((res = parse_tuid(ci->cid, meta->vst3ui_uid)) != STATUS_OK)
                    return res;

                ci->cardinality = Steinberg::PClassInfo::kManyInstances;
                Steinberg::strncpy8(ci->category, kVstComponentControllerClass, Steinberg::PClassInfo::kCategorySize);
                Steinberg::strncpy8(ci->name, meta->description, Steinberg::PClassInfo::kNameSize);
            }

            return STATUS_OK;
        }

        status_t PluginFactory::create_class_info2(const meta::package_t *manifest, const meta::plugin_t *meta)
        {
            status_t res;
            LSPString tmp;
            char version_str[32];
            snprintf(
                version_str,
                sizeof(version_str),
                "%d.%d.%d",
                int(meta->version.major),
                int(meta->version.minor),
                int(meta->version.micro));

            // Generate class info for processor
            Steinberg::PClassInfo2 *ci = vClassInfo2.add();
            if (ci == NULL)
                return STATUS_NO_MEM;

            if ((res = parse_tuid(ci->cid, meta->vst3_uid)) != STATUS_OK)
                return res;
            if ((res = make_plugin_categories(&tmp, meta)) != STATUS_OK)
                return res;

            ci->cardinality = Steinberg::PClassInfo::kManyInstances;
            Steinberg::strncpy8(ci->category, kVstAudioEffectClass, Steinberg::PClassInfo::kCategorySize);
            Steinberg::strncpy8(ci->name, meta->description, Steinberg::PClassInfo::kNameSize);

            ci->classFlags = Steinberg::Vst::kDistributable;
            Steinberg::strncpy8(ci->subCategories, tmp.get_ascii(), Steinberg::PClassInfo2::kSubCategoriesSize);
            Steinberg::strncpy8(ci->vendor, manifest->brand, Steinberg::PClassInfo2::kVendorSize);
            Steinberg::strncpy8(ci->version, version_str, Steinberg::PClassInfo2::kVersionSize);
            Steinberg::strncpy8(ci->sdkVersion, Steinberg::Vst::SDKVersionString, Steinberg::PClassInfo2::kVersionSize);

            // Generate class info for controller
            if (meta->vst3ui_uid != NULL)
            {
                ci = vClassInfo2.add();
                if (ci == NULL)
                    return STATUS_NO_MEM;

                if ((res = parse_tuid(ci->cid, meta->vst3ui_uid)) != STATUS_OK)
                    return res;

                ci->cardinality = Steinberg::PClassInfo::kManyInstances;
                Steinberg::strncpy8(ci->category, kVstComponentControllerClass, Steinberg::PClassInfo::kCategorySize);
                Steinberg::strncpy8(ci->name, meta->description, Steinberg::PClassInfo::kNameSize);

                ci->classFlags = 0;
                Steinberg::strncpy8(ci->subCategories, "", Steinberg::PClassInfo2::kSubCategoriesSize);
                Steinberg::strncpy8(ci->vendor, manifest->brand, Steinberg::PClassInfo2::kVendorSize);
                Steinberg::strncpy8(ci->version, version_str, Steinberg::PClassInfo2::kVersionSize);
                Steinberg::strncpy8(ci->sdkVersion, Steinberg::Vst::SDKVersionString, Steinberg::PClassInfo2::kVersionSize);
            }

            return STATUS_OK;
        }

        status_t PluginFactory::create_class_infow(const meta::package_t *manifest, const meta::plugin_t *meta)
        {
            status_t res;
            LSPString tmp;
            char version_str[32];
            snprintf(
                version_str,
                sizeof(version_str),
                "%d.%d.%d",
                int(meta->version.major),
                int(meta->version.minor),
                int(meta->version.micro));

            // Generate class info for processor
            Steinberg::PClassInfoW *ci = vClassInfoW.add();
            if (ci == NULL)
                return STATUS_NO_MEM;

            if ((res = parse_tuid(ci->cid, meta->vst3_uid)) != STATUS_OK)
                return res;

            ci->cardinality = Steinberg::PClassInfo::kManyInstances;
            Steinberg::strncpy8(ci->category, kVstAudioEffectClass, Steinberg::PClassInfo::kCategorySize);
            if (!tmp.set_utf8(meta->description))
                return STATUS_NO_MEM;
            Steinberg::strncpy16(
                ci->name,
                reinterpret_cast<const Steinberg::char16 *>(tmp.get_utf16()),
                Steinberg::PClassInfo::kNameSize);

            ci->classFlags = Steinberg::Vst::kDistributable;
            if ((res = make_plugin_categories(&tmp, meta)) != STATUS_OK)
                return res;
            Steinberg::strncpy8(ci->subCategories, tmp.get_ascii(), Steinberg::PClassInfo2::kSubCategoriesSize);
            if (!tmp.set_utf8(manifest->brand))
                return STATUS_NO_MEM;
            Steinberg::strncpy16(
                ci->vendor,
                reinterpret_cast<const Steinberg::char16 *>(tmp.get_utf16()),
                Steinberg::PClassInfoW::kVendorSize);
            Steinberg::str8ToStr16(ci->version, version_str, Steinberg::PClassInfoW::kVersionSize);
            Steinberg::str8ToStr16(ci->sdkVersion, Steinberg::Vst::SDKVersionString, Steinberg::PClassInfoW::kVersionSize);

            // Generate class info for controller
            if (meta->vst3ui_uid != NULL)
            {
                ci = vClassInfoW.add();
                if (ci == NULL)
                    return STATUS_NO_MEM;

                if ((res = parse_tuid(ci->cid, meta->vst3ui_uid)) != STATUS_OK)
                    return res;

                ci->cardinality = Steinberg::PClassInfo::kManyInstances;
                Steinberg::strncpy8(ci->category, kVstComponentControllerClass, Steinberg::PClassInfo::kCategorySize);
                if (!tmp.set_utf8(meta->description))
                    return STATUS_NO_MEM;
                Steinberg::strncpy16(
                    ci->name,
                    reinterpret_cast<const Steinberg::char16 *>(tmp.get_utf16()),
                    Steinberg::PClassInfo::kNameSize);

                ci->classFlags = 0;
                Steinberg::strncpy8(ci->subCategories, "", Steinberg::PClassInfo2::kSubCategoriesSize);
                if (!tmp.set_utf8(manifest->brand))
                    return STATUS_NO_MEM;
                Steinberg::strncpy16(
                    ci->vendor,
                    reinterpret_cast<const Steinberg::char16 *>(tmp.get_utf16()),
                    Steinberg::PClassInfoW::kVendorSize);
                Steinberg::str8ToStr16(ci->version, version_str, Steinberg::PClassInfoW::kVersionSize);
                Steinberg::str8ToStr16(ci->sdkVersion, Steinberg::Vst::SDKVersionString, Steinberg::PClassInfoW::kVersionSize);
            }

            return STATUS_OK;
        }

        void PluginFactory::destroy()
        {
            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader = NULL;
            }

            vClassInfo.flush();
            vClassInfo2.flush();
            vClassInfoW.flush();
        }

        Steinberg::tresult PLUGIN_API PluginFactory::queryInterface(const Steinberg::TUID _iid, void **obj)
        {
            // Cast to the requested interface
            if (Steinberg::iidEqual(_iid, Steinberg::FUnknown::iid))
                return cast_interface<Steinberg::FUnknown>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPluginFactory::iid))
                return cast_interface<Steinberg::IPluginFactory>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPluginFactory2::iid))
                return cast_interface<Steinberg::IPluginFactory2>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPluginFactory3::iid))
                return cast_interface<Steinberg::IPluginFactory3>(this, obj);

            return no_interface(obj);
        }

        Steinberg::uint32 PLUGIN_API PluginFactory::addRef()
        {
            return atomic_add(&nRefCounter, 1) + 1;
        }

        Steinberg::uint32 PLUGIN_API PluginFactory::release()
        {
            atomic_t ref_count = atomic_add(&nRefCounter, -1) - 1;
            if (ref_count == 0)
                delete this;

            return ref_count;
        }

        Steinberg::tresult PLUGIN_API PluginFactory::getFactoryInfo(Steinberg::PFactoryInfo *info)
        {
            if (info != NULL)
                *info       = sFactoryInfo;
            return Steinberg::kResultOk;
        }

        Steinberg::int32 PLUGIN_API PluginFactory::countClasses()
        {
            return vClassInfo.size();
        }

        Steinberg::tresult PLUGIN_API PluginFactory::getClassInfo(Steinberg::int32 index, Steinberg::PClassInfo *info)
        {
            if ((index < 0) || (info == NULL))
                return Steinberg::kInvalidArgument;

            const Steinberg::PClassInfo *result = vClassInfo.get(index);
            if (result == NULL)
                return Steinberg::kInvalidArgument;

            *info   = *result;

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginFactory::getClassInfo2(Steinberg::int32 index, Steinberg::PClassInfo2 *info)
        {
            if ((index < 0) || (info == NULL))
                return Steinberg::kInvalidArgument;

            const Steinberg::PClassInfo2 *result = vClassInfo2.get(index);
            if (result == NULL)
                return Steinberg::kInvalidArgument;

            *info   = *result;

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginFactory::getClassInfoUnicode(Steinberg::int32 index, Steinberg::PClassInfoW *info)
        {
            if ((index < 0) || (info == NULL))
                return Steinberg::kInvalidArgument;

            const Steinberg::PClassInfoW *result = vClassInfoW.get(index);
            if (result == NULL)
                return Steinberg::kInvalidArgument;

            *info   = *result;

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginFactory::setHostContext(Steinberg::FUnknown *context)
        {
            return Steinberg::kNotImplemented;
        }

        Steinberg::tresult PLUGIN_API PluginFactory::createInstance(Steinberg::FIDString cid, Steinberg::FIDString _iid, void **obj)
        {
            // Watch plugin factories first
            for (plug::Factory *f = plug::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *plug_meta = f->enumerate(i);
                    if (plug_meta == NULL)
                        break;
                    if (memcmp(plug_meta->vst3_uid, cid, sizeof(Steinberg::TUID)) != 0)
                        continue;

                    // UID matched, allocate plugin module
                    plug::Module *module = f->create(plug_meta);
                    if (module == NULL)
                        return Steinberg::kOutOfMemory;
                    lsp_finally {
                        if (module != NULL)
                            delete module;
                    };

                    // Allocate wrapper
                    Wrapper *w  = new Wrapper(this, module, pLoader);
                    if (w == NULL)
                        return Steinberg::kOutOfMemory;
                    module      = NULL; // Force module to be destroyed by the wrapper
                    lsp_finally {
                        safe_release(w);
                    };

                    // Query interface and return
                    return w->queryInterface(_iid, obj);
                }
            }

            // Watch UI factories next
            for (ui::Factory *f = ui::Factory::root(); f != NULL; f = f->next())
            {
                for (size_t i=0; ; ++i)
                {
                    // Enumerate next element
                    const meta::plugin_t *plug_meta = f->enumerate(i);
                    if (plug_meta == NULL)
                        break;
                    if (memcmp(plug_meta->vst3ui_uid, cid, sizeof(Steinberg::TUID)) != 0)
                        continue;

                    // UID matched, allocate plugin module
                    ui::Module *module = f->create(plug_meta);
                    if (module == NULL)
                        return Steinberg::kOutOfMemory;
                    lsp_finally {
                        if (module != NULL)
                            delete module;
                    };

                    // Allocate wrapper
                    UIWrapper *w  = new UIWrapper(this, module, pLoader);
                    if (w == NULL)
                        return Steinberg::kOutOfMemory;
                    module      = NULL; // Force module to be destroyed by the wrapper
                    lsp_finally {
                        safe_release(w);
                    };

                    // Query interface and return
                    return w->queryInterface(_iid, obj);
                }
            }

            *obj = NULL;
            return Steinberg::kNoInterface;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_FACTORY_H_ */
