/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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
#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/ipc/Thread.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/lltl/phashset.h>
#include <lsp-plug.in/plug-fw/const.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/helpers.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/ibstreamout.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/modinfo.h>
#include <lsp-plug.in/plug-fw/wrap/vst3/timer.h>
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
            lsp_trace("this=%p", this);

            nRefCounter         = 1;
            nRefExecutor        = 0;
            pLoader             = NULL;
            pExecutor           = NULL;
            pDataSync           = NULL;
            pPackage            = NULL;
            pActiveSync         = NULL;

        #ifdef VST_USE_RUNLOOP_IFACE
            pRunLoop            = NULL;
        #endif /* VST_USE_RUNLOOP_IFACE */
        }

        PluginFactory::~PluginFactory()
        {
            lsp_trace("this=%p", this);
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

            // Remember the package
            pPackage    = release_ptr(manifest);

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

        status_t PluginFactory::create_class_info(const meta::package_t *manifest, const meta::plugin_t *meta)
        {
            // Generate class info for processor
            Steinberg::PClassInfo *ci = vClassInfo.add();
            if (ci == NULL)
                return STATUS_NO_MEM;

            if (!meta::uid_vst3_to_tuid(ci->cid, meta->vst3_uid))
                return STATUS_BAD_FORMAT;
            ci->cardinality = Steinberg::PClassInfo::kManyInstances;
            Steinberg::strncpy8(ci->category, kVstAudioEffectClass, Steinberg::PClassInfo::kCategorySize);
            Steinberg::strncpy8(ci->name, meta->description, Steinberg::PClassInfo::kNameSize);

            // Generate class info for controller
            if (meta->vst3ui_uid != NULL)
            {
                ci = vClassInfo.add();
                if (ci == NULL)
                    return STATUS_NO_MEM;

                if (!meta::uid_vst3_to_tuid(ci->cid, meta->vst3ui_uid))
                    return STATUS_BAD_FORMAT;

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

            if (!meta::uid_vst3_to_tuid(ci->cid, meta->vst3_uid))
                return STATUS_BAD_FORMAT;
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

                if (!meta::uid_vst3_to_tuid(ci->cid, meta->vst3ui_uid))
                    return STATUS_BAD_FORMAT;

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

            if (!meta::uid_vst3_to_tuid(ci->cid, meta->vst3_uid))
                return STATUS_BAD_FORMAT;

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

                if (!meta::uid_vst3_to_tuid(ci->cid, meta->vst3ui_uid))
                    return STATUS_BAD_FORMAT;

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
            lsp_trace("this=%p", this);

        #ifdef VST_USE_RUNLOOP_IFACE
            safe_release(pRunLoop);
        #endif /* VST_USE_RUNLOOP_IFACE */

            if (pLoader != NULL)
            {
                delete pLoader;
                pLoader = NULL;
            }

            free_manifest(pPackage);

            vClassInfo.flush();
            vClassInfo2.flush();
            vClassInfoW.flush();
        }

        Steinberg::tresult PLUGIN_API PluginFactory::queryInterface(const Steinberg::TUID _iid, void **obj)
        {
            IF_TRACE(
                char dump[36];
                lsp_trace("this=%p, _iid=%s",
                    this,
                    meta::uid_tuid_to_vst3(dump, _iid));
            );

            // Cast to the requested interface
            if (Steinberg::iidEqual(_iid, Steinberg::FUnknown::iid))
                return cast_interface<Steinberg::FUnknown>(static_cast<Steinberg::IPluginFactory *>(this), obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPluginFactory::iid))
                return cast_interface<Steinberg::IPluginFactory>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPluginFactory2::iid))
                return cast_interface<Steinberg::IPluginFactory2>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPluginFactory3::iid))
                return cast_interface<Steinberg::IPluginFactory3>(this, obj);
            if (Steinberg::iidEqual(_iid, Steinberg::IPluginCompatibility::iid))
                return cast_interface<Steinberg::IPluginCompatibility>(this, obj);

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
            lsp_trace("this=%p, info=%p", this, info);

            if (info != NULL)
                *info       = sFactoryInfo;
            return Steinberg::kResultOk;
        }

        Steinberg::int32 PLUGIN_API PluginFactory::countClasses()
        {
            lsp_trace("this=%p", this);
            return vClassInfo.size();
        }

        Steinberg::tresult PLUGIN_API PluginFactory::getClassInfo(Steinberg::int32 index, Steinberg::PClassInfo *info)
        {
            lsp_trace("this=%p, index=%d, info=%p", this, int(index), info);

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
            lsp_trace("this=%p, index=%d, info=%p", this, index, info);

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
            lsp_trace("this=%p, index=%d, info=%p", this, int(index), info);

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
            lsp_trace("this=%p, context=%p", this, context);

        #ifdef VST_USE_RUNLOOP_IFACE
            // Acquire new pointer to the run loop
            pRunLoop = safe_query_iface<Steinberg::Linux::IRunLoop>(context);

            lsp_trace("RUN LOOP object=%p", pRunLoop);
        #endif /* VST_USE_RUNLOOP_IFACE */

            return Steinberg::kResultOk;
        }

        Steinberg::tresult PLUGIN_API PluginFactory::createInstance(Steinberg::FIDString cid, Steinberg::FIDString _iid, void **obj)
        {
            IF_TRACE(
                char dump1[36], dump2[36];
                lsp_trace("this=%p, cid=%s, _iid=%s, obj=%p",
                    this,
                    meta::uid_tuid_to_vst3(dump1, cid),
                    meta::uid_tuid_to_vst3(dump2, _iid),
                    obj);
            );

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
                    Wrapper *w  = new Wrapper(this, module, pLoader, pPackage);
                    if (w == NULL)
                        return Steinberg::kOutOfMemory;
                    lsp_trace("Created Wrapper w=%p", w);
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
                    UIWrapper *w  = new UIWrapper(this, module, pLoader, pPackage);
                    if (w == NULL)
                        return Steinberg::kOutOfMemory;
                    lsp_trace("Created UIWrapper w=%p", w);
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

        ipc::IExecutor *PluginFactory::acquire_executor()
        {
            lsp_trace("this=%p", this);

            if (!sMutex.lock())
                return NULL;
            lsp_finally { sMutex.unlock(); };

            // Try to perform quick access
            if (pExecutor != NULL)
            {
                ++nRefExecutor;
                return pExecutor;
            }

            // Create executor
            ipc::NativeExecutor *executor = new ipc::NativeExecutor();
            if (executor == NULL)
                return NULL;
            lsp_trace("Allocated executor=%p", executor);

            // Launch executor
            status_t res = executor->start();
            if (res != STATUS_OK)
            {
                lsp_trace("Failed to start executor=%p, code=%d", executor, int(res));
                delete executor;
                return NULL;
            }

            // Update status
            ++nRefExecutor;
            return pExecutor = executor;
        }

        void PluginFactory::release_executor()
        {
            lsp_trace("this=%p", this);

            if (!sMutex.lock())
                return;
            lsp_finally { sMutex.unlock(); };

            if ((--nRefExecutor) > 0)
                return;
            if (pExecutor == NULL)
                return;

            lsp_trace("Destroying executor pExecutor=%p", pExecutor);
            pExecutor->shutdown();
            delete pExecutor;
            pExecutor   = NULL;
        }

        status_t PluginFactory::register_data_sync(IDataSync *sync)
        {
            lsp_trace("this=%p, sync=%p", this, sync);

            if (sync == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Add record to the queue
            {
                sDataMutex.lock();
                lsp_finally { sDataMutex.unlock(); };

                // Try to perform quick addition
                if (!vDataSync.put(sync))
                {
                    lsp_trace("failed to register data_sync %p", sync);
                    return STATUS_NO_MEM;
                }

                lsp_trace("Number of clients: %d", int(vDataSync.size()));
            }

            // Remove the record from queue if data sync thread fails to start
            lsp_finally {
                if (sync != NULL)
                {
                    sDataMutex.lock();
                    lsp_finally { sDataMutex.unlock(); };
                    vDataSync.remove(sync);

                    lsp_trace("Roll-back to number of clients: %d", int(vDataSync.size()));
                }
            };

            // Ensure that data synchronization thread is already running
            {
                sMutex.lock();
                lsp_finally { sMutex.unlock(); };
                if (pDataSync != NULL)
                {
                    lsp_trace("data_sync thread %p is already running, clients=%d", pDataSync);
                    sync        = NULL;
                    return STATUS_OK;
                }

                // Start data synchronization thread
                pDataSync       = new ipc::Thread(this);
                if (pDataSync == NULL)
                    return STATUS_NO_MEM;

                lsp_trace("Starting data sync thread %p", pDataSync);
                status_t res    = pDataSync->start();
                if (res != STATUS_OK)
                {
                    delete pDataSync;
                    pDataSync       = NULL;
                    return STATUS_UNKNOWN_ERR;
                }

                lsp_trace("Data sync thread %p started", pDataSync);
                sync            = NULL;
            }

            return STATUS_OK;
        }

        status_t PluginFactory::unregister_data_sync(IDataSync *sync)
        {
            lsp_trace("this=%p, sync=%p", this, sync);

            if (sync == NULL)
                return STATUS_BAD_ARGUMENTS;

            // Try to remove data from queue
            {
                sDataMutex.lock();
                lsp_finally { sDataMutex.unlock(); };

                if (!vDataSync.remove(sync))
                {
                    lsp_warn("Non-existing client=%p", sync);
                    return STATUS_NOT_FOUND;
                }

                while (pActiveSync == sync)
                {
                    // TODO: replace this with proper solution (add conditional wait)
                    // sDataMutex.wait();
                    ipc::Thread::sleep(1);
                }

                lsp_trace("Number of clients: %d", int(vDataSync.size()));

                if (vDataSync.size() > 0)
                    return STATUS_OK;
            }

            // Need to stop synchronization thread
            sMutex.lock();
            lsp_finally { sMutex.unlock(); };
            if (pDataSync == NULL)
            {
                lsp_trace("no data sync thread is running");
                return STATUS_OK;
            }

            lsp_trace("terminating data_sync thread %p", pDataSync);
            pDataSync->cancel();
            pDataSync->join();
            delete pDataSync;

            lsp_trace("terminated data_sync thread %p", pDataSync);
            pDataSync       = NULL;

            return STATUS_OK;
        }

        status_t PluginFactory::run()
        {
            lsp_trace("enter main loop this=%p", this);

            lltl::parray<IDataSync> list;

            while (!ipc::Thread::is_cancelled())
            {
                // Measure the start time
                system::time_millis_t time = system::get_time_millis();

                // Form the list of items for processing
                {
                    sDataMutex.lock();
                    lsp_finally { sDataMutex.unlock(); };
                    vDataSync.values(&list);
                }

                // Process each item
                for (lltl::iterator<IDataSync> it=list.values(); it; ++it)
                {
                    // Obtain the sync object
                    IDataSync *dsync    = it.get();
                    if (dsync == NULL)
                        continue;

                    // Ensure that item is still valid and lock it
                    {
                        sDataMutex.lock();
                        lsp_finally { sDataMutex.unlock(); };
                        if (!vDataSync.contains(dsync))
                            continue;
                        pActiveSync     = dsync;
                    }

                    // Now dsync object is locked, perform processing
                    dsync->sync_data();

                    // Finally, unlock the data sync
                    pActiveSync     = NULL;
                }

                // Wait for a while
                const system::time_millis_t delay = lsp_min(system::get_time_millis() - time, 40u);
                ipc::Thread::sleep(delay);
            }

            lsp_trace("leave main loop this=%p", this);

            return STATUS_OK;
        }

    #ifdef VST_USE_RUNLOOP_IFACE
        Steinberg::Linux::IRunLoop *PluginFactory::acquire_run_loop()
        {
            return safe_acquire(pRunLoop);
        }
    #endif /* VST_USE_RUNLOOP_IFACE */

        Steinberg::tresult PLUGIN_API PluginFactory::getCompatibilityJSON(Steinberg::IBStream *stream)
        {
            vst3::IBStreamOut out(stream);
            lsp_finally { out.close(); };

            status_t res = vst3::make_moduleinfo(&out, pPackage);
            return (res == STATUS_OK) ? Steinberg::kResultOk : Steinberg::kInternalError;
        }

    } /* namespace vst3 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_IMPL_FACTORY_H_ */
