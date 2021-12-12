/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 8 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/KVTDispatcher.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/chunk.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/defs.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ports.h>

#include <lsp-plug.in/ipc/Mutex.h>
#include <lsp-plug.in/lltl/parray.h>

namespace lsp
{
    namespace vst2
    {
        class UIWrapper;

        class Wrapper: public plug::IWrapper
        {
            private:
                Wrapper & operator = (const Wrapper &);
                Wrapper(const Wrapper &);

                friend class UIWrapper;

            private:
                AEffect                            *pEffect;
                audioMasterCallback                 pMaster;
                ipc::IExecutor                     *pExecutor;
                vst2::chunk_t                       sChunk;
                bool                                bUpdateSettings;
                UIWrapper                          *pUIWrapper;
                float                               fLatency;
                uatomic_t                           nDumpReq;
                uatomic_t                           nDumpResp;
                vst2::Port                         *pBypass;

                lltl::parray<vst2::AudioPort>       vAudioPorts;    // List of audio ports
                lltl::parray<vst2::ParameterPort>   vParams;        // List of controllable parameters
                lltl::parray<vst2::Port>            vPorts;         // List of all created VST ports
                lltl::parray<vst2::Port>            vSortedPorts;   // List of all created VST ports ordered by unique id
                lltl::parray<vst2::Port>            vProxyPorts;    // List of all created VST proxy ports
                lltl::parray<meta::port_t>          vGenMetadata;   // Generated metadata

                plug::position_t                    sPosition;

                core::KVTStorage                    sKVT;
                ipc::Mutex                          sKVTMutex;

                meta::package_t                    *pPackage;

            private:
                vst2::Port                 *create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *port, const char *postfix);

            protected:
                status_t                    check_vst_bank_header(const fxBank *bank, size_t size);
                status_t                    check_vst_program_header(const fxProgram *prog, size_t size);
                void                        deserialize_v1(const fxBank *bank);
                void                        deserialize_v2_v3(const uint8_t *data, size_t bytes);
                void                        deserialize_new_chunk_format(const uint8_t *data, size_t bytes);
                void                        sync_position();
                status_t                    serialize_port_data();

            public:
                Wrapper(
                    plug::Module *plugin,
                    resource::ILoader *loader,
                    AEffect *effect,
                    audioMasterCallback callback
                );
                ~Wrapper();

                status_t                        init();
                void                            destroy();

            public:
                inline vst2::ParameterPort     *parameter_port(size_t index);
                vst2::Port                     *find_by_id(const char *id);

                inline void                     open();
                void                            run(float** inputs, float** outputs, size_t samples);
                void                            run_legacy(float** inputs, float** outputs, size_t samples);
                void                            process_events(const VstEvents *e);
                inline void                     set_sample_rate(float sr);
                inline void                     set_block_size(size_t size);
                inline void                     mains_changed(VstIntPtr value);
                inline bool                     has_bypass() const;
                inline void                     set_bypass(bool bypass);

                inline void                     set_ui_wrapper(UIWrapper *ui);

                inline UIWrapper               *ui_wrapper();

                size_t                          serialize_state(const void **dst, bool program);

                void                            deserialize_state(const void *data, size_t size);

                void                            request_state_dump();

            public:
                virtual ipc::IExecutor         *executor();

                virtual const plug::position_t *position();

                virtual core::KVTStorage       *kvt_lock();

                virtual core::KVTStorage       *kvt_trylock();

                virtual bool                    kvt_release();

                virtual const meta::package_t  *package() const;
        };
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_WRAPPER_H_ */
