/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 1 янв. 2024 г.
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
#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST3_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST3_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/common/atomic.h>
#include <lsp-plug.in/common/status.h>
#include <lsp-plug.in/lltl/darray.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/core/Resources.h>
#include <lsp-plug.in/plug-fw/plug.h>

#include <steinberg/vst3.h>

#include <lsp-plug.in/plug-fw/wrap/vst3/factory.h>

namespace lsp
{
    namespace vst3
    {
        class PluginFactory;

        #include <steinberg/vst3/base/WarningsPush.h>
        class Wrapper:
            public plug::IWrapper,
            public Steinberg::IDependent,
            public Steinberg::Vst::IComponent,
            public Steinberg::Vst::IConnectionPoint,
            public Steinberg::Vst::IAudioProcessor,
            public Steinberg::Vst::IProcessContextRequirements
        {
            protected:
                volatile uatomic_t      nRefCounter;    // Reference counter
                PluginFactory          *pFactory;       // Reference to the factory

            public:
                explicit Wrapper(PluginFactory *factory, plug::Module *plugin, resource::ILoader *loader);
                Wrapper(const IWrapper &) = delete;
                Wrapper(IWrapper &&) = delete;
                virtual ~Wrapper() override;

                Wrapper & operator = (const Wrapper &) = delete;
                Wrapper & operator = (Wrapper &&) = delete;

            public: // Steinberg::FUnknown
                virtual Steinberg::tresult  PLUGIN_API queryInterface(const Steinberg::TUID _iid, void **obj) override;
                virtual Steinberg::uint32   PLUGIN_API addRef() override;
                virtual Steinberg::uint32   PLUGIN_API release() override;

            public: // Steinberg::IDependent
                virtual void                PLUGIN_API update(FUnknown* changedUnknown, Steinberg::int32 message) override;

            public: // Steinberg::IPluginBase
                virtual Steinberg::tresult  PLUGIN_API initialize(Steinberg::FUnknown *context) override;
                virtual Steinberg::tresult  PLUGIN_API terminate() override;

            public: // Steinberg::Vst::IComponent
                virtual Steinberg::tresult  PLUGIN_API getControllerClassId(Steinberg::TUID classId) override;
                virtual Steinberg::tresult  PLUGIN_API setIoMode(Steinberg::Vst::IoMode mode) override;
                virtual Steinberg::int32    PLUGIN_API getBusCount(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir) override;
                virtual Steinberg::tresult  PLUGIN_API getBusInfo(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::BusInfo & bus /*out*/) override;
                virtual Steinberg::tresult  PLUGIN_API getRoutingInfo(Steinberg::Vst::RoutingInfo & inInfo, Steinberg::Vst::RoutingInfo & outInfo /*out*/) override;
                virtual Steinberg::tresult  PLUGIN_API activateBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::TBool state) override;
                virtual Steinberg::tresult  PLUGIN_API setActive(Steinberg::TBool state) override;
                virtual Steinberg::tresult  PLUGIN_API setState(Steinberg::IBStream *state) override;
                virtual Steinberg::tresult  PLUGIN_API getState(Steinberg::IBStream *state) override;

            public: // Steinberg::vst::IConnectionPoint
                virtual Steinberg::tresult  PLUGIN_API connect(Steinberg::Vst::IConnectionPoint *other) override;
                virtual Steinberg::tresult  PLUGIN_API disconnect(Steinberg::Vst::IConnectionPoint *other) override;
                virtual Steinberg::tresult  PLUGIN_API notify(Steinberg::Vst::IMessage *message) override;

            public: // Steinberg::vst::IAudioProcessor
                virtual Steinberg::tresult  PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
                virtual Steinberg::tresult  PLUGIN_API getBusArrangement(Steinberg::Vst::BusDirection dir, Steinberg::int32 index, Steinberg::Vst::SpeakerArrangement& arr) override;
                virtual Steinberg::tresult  PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
                virtual Steinberg::uint32   PLUGIN_API getLatencySamples() override;
                virtual Steinberg::tresult  PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup & setup) override;
                virtual Steinberg::tresult  PLUGIN_API setProcessing(Steinberg::TBool state) override;
                virtual Steinberg::tresult  PLUGIN_API process(Steinberg::Vst::ProcessData & data) override;
                virtual Steinberg::uint32   PLUGIN_API getTailSamples() override;

            public: // Steinberg::Vst::IProcessContextRequirements
                virtual Steinberg::uint32   PLUGIN_API getProcessContextRequirements() override;
        };
        #include <steinberg/vst3/base/WarningsPop.h>

    } /* namespace vst3 */
} /* namespace lsp */




#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST3_WRAPPER_H_ */
