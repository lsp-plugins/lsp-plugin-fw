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

#include <steinberg/vst3.h>

namespace lsp
{
    namespace vst3
    {
        PluginFactory::PluginFactory()
        {
            nRefCounter         = 1;
        }

        PluginFactory::~PluginFactory()
        {
            destroy();
        }

        status_t PluginFactory::init()
        {


            return STATUS_OK;
        }

        void PluginFactory::destroy()
        {
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
