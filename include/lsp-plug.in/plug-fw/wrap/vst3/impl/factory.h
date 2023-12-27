/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-comp-delay
 * Created on: 27 дек. 2023 г.
 *
 * lsp-plugins-comp-delay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-comp-delay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-comp-delay. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_FACTORY_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
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
            sFactoryInfo.flags = Steinberg::PFactoryInfo::kUnicode;
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

                    // We have new plugin record, create class info for this plugin
                    if ((res = create_class_info(manifest, plug_meta)) != STATUS_OK)
                        return res;
                    if ((res = create_class_info2(manifest, plug_meta)) != STATUS_OK)
                        return res;
                    if ((res = create_class_infow(manifest, plug_meta)) != STATUS_OK)
                        return res;
                }
            }

            return STATUS_OK;
        }

        status_t PluginFactory::create_class_info(const meta::package_t *manifest, const meta::plugin_t *meta)
        {
            Steinberg::PClassInfo *ci = vClassInfo.add();
            if (ci == NULL)
                return STATUS_NO_MEM;

            status_t res = parse_tuid(ci->cid, meta->vst3_uid);
            if (res != STATUS_OK)
                return res;

            ci->cardinality = Steinberg::PClassInfo::kManyInstances;
            Steinberg::strncpy8(ci->category, "", Steinberg::PClassInfo::kCategorySize); // TODO: insert category
            Steinberg::strncpy8(ci->name, meta->description, Steinberg::PClassInfo::kNameSize);

            return STATUS_OK;
        }

        status_t PluginFactory::create_class_info2(const meta::package_t *manifest, const meta::plugin_t *meta)
        {
            Steinberg::PClassInfo2 *ci = vClassInfo2.add();
            if (ci == NULL)
                return STATUS_NO_MEM;

            char version_str[32];
            snprintf(
                version_str,
                sizeof(version_str),
                "%d.%d.%d",
                int(meta->version.major),
                int(meta->version.minor),
                int(meta->version.micro));

            status_t res = parse_tuid(ci->cid, meta->vst3_uid);
            if (res != STATUS_OK)
                return res;

            ci->cardinality = Steinberg::PClassInfo::kManyInstances;
            Steinberg::strncpy8(ci->category, "", Steinberg::PClassInfo::kCategorySize); // TODO: insert category
            Steinberg::strncpy8(ci->name, meta->description, Steinberg::PClassInfo::kNameSize);

            ci->classFlags = 0; // TODO: find out more about flags
            Steinberg::strncpy8(ci->subCategories, "", Steinberg::PClassInfo2::kSubCategoriesSize); // TODO: insert subcategories
            Steinberg::strncpy8(ci->vendor, manifest->brand, Steinberg::PClassInfo2::kVendorSize);
            Steinberg::strncpy8(ci->version, version_str, Steinberg::PClassInfo2::kVersionSize);
            Steinberg::strncpy8(ci->sdkVersion, VST3_SDK_VERSION, Steinberg::PClassInfo2::kVersionSize);

            return STATUS_OK;
        }

        status_t PluginFactory::create_class_infow(const meta::package_t *manifest, const meta::plugin_t *meta)
        {
            Steinberg::PClassInfoW *ci = vClassInfoW.add();
            if (ci == NULL)
                return STATUS_NO_MEM;

            LSPString tmp;
            status_t res = parse_tuid(ci->cid, meta->vst3_uid);
            if (res != STATUS_OK)
                return res;

            ci->cardinality = Steinberg::PClassInfo::kManyInstances;
            Steinberg::strncpy8(ci->category, "", Steinberg::PClassInfo::kCategorySize); // TODO: insert category
            if (!tmp.set_utf8(meta->description))
                return STATUS_NO_MEM;
            Steinberg::strncpy16(
                ci->name,
                reinterpret_cast<const Steinberg::char16 *>(tmp.get_utf16()),
                Steinberg::PClassInfo::kNameSize);

            ci->classFlags = 0; // TODO: find out more about flags
            Steinberg::strncpy8(ci->subCategories, "", Steinberg::PClassInfoW::kSubCategoriesSize); // TODO: insert subcategories
            if (!tmp.set_utf8(manifest->brand))
                return STATUS_NO_MEM;
            Steinberg::strncpy16(
                ci->vendor,
                reinterpret_cast<const Steinberg::char16 *>(tmp.get_utf16()),
                Steinberg::PClassInfoW::kVendorSize);
            if (tmp.fmt_ascii("%d.%d.%d", int(meta->version.major), int(meta->version.minor),int(meta->version.micro)) <= 0)
                return STATUS_NO_MEM;
            Steinberg::strncpy16(
                ci->version,
                reinterpret_cast<const Steinberg::char16 *>(tmp.get_utf16()),
                Steinberg::PClassInfoW::kVersionSize);
            Steinberg::str8ToStr16(
                ci->sdkVersion, VST3_SDK_VERSION,
                Steinberg::PClassInfoW::kVersionSize);

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
            void *res = NULL;

            // Cast to the requested interface
            if (Steinberg::iidEqual(_iid, Steinberg::FUnknown_iid))
                res = static_cast<Steinberg::FUnknown *>(this);
            else if (Steinberg::iidEqual(_iid, Steinberg::IPluginFactory_iid))
                res = static_cast<Steinberg::IPluginFactory *>(this);
            else if (Steinberg::iidEqual(_iid, Steinberg::IPluginFactory2_iid))
                res = static_cast<Steinberg::IPluginFactory2 *>(this);
            else if (Steinberg::iidEqual(_iid, Steinberg::IPluginFactory3_iid))
                res = static_cast<Steinberg::IPluginFactory3 *>(this);

            // Return result
            if (res != NULL)
            {
                addRef();
                *obj    = res;
                return Steinberg::kResultOk;
            }

            return Steinberg::kNoInterface;
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
            // TODO: create plugin instance
            return Steinberg::kNotImplemented;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_FACTORY_H_ */
