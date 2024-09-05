/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 20 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>

#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/core/KVTDispatcher.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/core/SamplePlayer.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/executor.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/ports.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/sink.h>

namespace lsp
{
    namespace lv2
    {
        class Factory;

        /**
         * LV2 format plugin wrapper
         */
        class Wrapper: public plug::IWrapper
        {
            public:
                enum state_mode_t
                {
                    SM_SYNC,        // State is in sync with host
                    SM_CHANGED,     // State has been changed
                    SM_REPORTED,    // State change has been reported to the host
                    SM_LOADING      // State has been loaded but still not committed
                };

                enum kvt_parse_flags_t
                {
                    KP_KEY      = 1 << 0,       // KVT key has been deserialized
                    KP_VALUE    = 1 << 1,       // KVT value has been deserialized
                    KP_FLAGS    = 1 << 2        // KVT flags have been deserialized
                };

            private:
                class LV2KVTListener: public core::KVTListener
                {
                    private:
                        lv2::Wrapper *pWrapper;

                    public:
                        explicit LV2KVTListener(lv2::Wrapper *wrapper) { pWrapper = wrapper; }

                    public:
                        virtual void created(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending);
                        virtual void changed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *oval, const core::kvt_param_t *nval, size_t pending);
                        virtual void removed(core::KVTStorage *storage, const char *id, const core::kvt_param_t *param, size_t pending);
                };

            protected:
                lltl::parray<lv2::Port>         vExtPorts;
                lltl::parray<lv2::Port>         vAllPorts;      // List of all created ports, for garbage collection
                lltl::parray<lv2::Port>         vPluginPorts;   // All plugin ports sorted in urid order
                lltl::parray<lv2::Port>         vMeshPorts;
                lltl::parray<lv2::Port>         vFrameBufferPorts;
                lltl::parray<lv2::Port>         vStreamPorts;
                lltl::parray<lv2::Port>         vMidiPorts;
                lltl::parray<lv2::Port>         vOscPorts;
                lltl::parray<lv2::AudioPort>    vAudioPorts;
                lltl::parray<lv2::StringPort>   vStringPorts;
                lltl::parray<meta::port_t>      vGenMetadata;   // Generated metadata

                lv2::Factory           *pFactory;       // LV2 plugin factory
                lv2::Extensions        *pExt;           // LV2 plugin extensions
                ipc::IExecutor         *pExecutor;      // Executor service
                void                   *pAtomIn;        // Atom input port
                void                   *pAtomOut;       // Atom output port
                float                  *pLatency;       // Latency output port
                size_t                  nPatchReqs;     // Number of patch requests
                size_t                  nStateReqs;     // Number of state requests
                ssize_t                 nSyncTime;      // Synchronization time
                ssize_t                 nSyncSamples;   // Synchronization counter
                ssize_t                 nClients;       // Number of clients
                ssize_t                 nDirectClients; // Number of direct clients
                bool                    bQueueDraw;     // Queue draw request
                bool                    bUpdateSettings;// Settings update
                bool                    bStateManage;   // State management barrier
                float                   fSampleRate;
                uint8_t                *pOscPacket;     // OSC packet data
                uatomic_t               nStateMode;     // State change flag
                uatomic_t               nDumpReq;
                uatomic_t               nDumpResp;
                meta::package_t        *pPackage;

                core::KVTStorage        sKVT;
                LV2KVTListener          sKVTListener;
                ipc::Mutex              sKVTMutex;
                core::KVTDispatcher    *pKVTDispatcher;

                core::SamplePlayer     *pSamplePlayer;      // Sample player
                wssize_t                nPlayPosition;      // Sample playback position
                wssize_t                nPlayLength;        // Sample playback length

                LV2_Inline_Display_Image_Surface sSurface;  // Canvas surface

            protected:
                lv2::Port                      *create_port(lltl::parray<plug::IPort> *plugin_ports, const meta::port_t *meta, const char *postfix, bool virt);
                void                            clear_midi_ports();
                void                            save_kvt_parameters();
                void                            restore_kvt_parameters();

                void                            parse_kvt_v1(const LV2_Atom_Object_Body *data, size_t size);
                void                            parse_kvt_v2(const LV2_Atom *data, size_t size);
                bool                            parse_kvt_key(char const **key, const LV2_Atom *value);
                bool                            parse_kvt_flags(size_t *flags, const LV2_Atom *value);
                bool                            parse_kvt_value(core::kvt_param_t *param, const LV2_Atom *value);

                void                            transmit_port_data_to_clients(bool sync_req, bool patch_req, bool state_req);
                void                            transmit_time_position_to_clients();
                void                            transmit_play_position_to_clients();
                void                            transmit_midi_events(lv2::Port *p);
                void                            transmit_osc_events(lv2::Port *p);
                void                            transmit_kvt_events();
                void                            transmit_atoms(size_t samples);

                void                            receive_midi_event(const LV2_Atom_Event *ev);
                void                            receive_raw_osc_event(osc::parse_frame_t *frame);
                void                            receive_atom_object(const LV2_Atom_Event *ev);
                void                            receive_atoms(size_t samples);

                void                            do_destroy();

                static ssize_t                  compare_ports_by_urid(const lv2::Port *a, const lv2::Port *b);

            public:
                explicit Wrapper(plug::Module *plugin, lv2::Factory *factory, lv2::Extensions *ext);
                virtual ~Wrapper() override;

                status_t                        init(float srate);
                void                            destroy();

            public:
                inline void                     activate()          { pPlugin->activate(); }
                inline void                     deactivate()        { pPlugin->deactivate(); }

                inline void                     connect(size_t id, void *data);
                inline void                     run(size_t samples);
                bool                            change_state_atomic(state_mode_t from, state_mode_t to);

                // State part
                inline void save_state(
                    LV2_State_Store_Function   store,
                    LV2_State_Handle           handle,
                    uint32_t                   flags,
                    const LV2_Feature *const * features);

                inline void restore_state(
                    LV2_State_Retrieve_Function retrieve,
                    LV2_State_Handle            handle,
                    uint32_t                    flags,
                    const LV2_Feature *const *  features);

                // Job part
                inline void job_run(
                    LV2_Worker_Respond_Handle   handle,
                    LV2_Worker_Respond_Function respond,
                    uint32_t                    size,
                    const void*                 data);

                inline void job_response(size_t size, const void *body) {}
                inline void job_end() {}

                // Inline display part
                LV2_Inline_Display_Image_Surface *render_inline_display(size_t width, size_t height);

                lv2::Port                      *port(const char *id);
                lv2::Port                      *port_by_urid(LV2_URID urid);

                void                            connect_direct_ui();

                void                            disconnect_direct_ui();

                inline float                    get_sample_rate() const { return fSampleRate; }

                inline core::KVTDispatcher     *kvt_dispatcher()        { return pKVTDispatcher; }

            public: // plug::IWrapper
                virtual ipc::IExecutor         *executor() override;
                virtual core::KVTStorage       *kvt_lock() override;
                virtual core::KVTStorage       *kvt_trylock() override;
                virtual bool                    kvt_release() override;
                virtual void                    state_changed() override;
                virtual const meta::package_t  *package() const override;
                virtual void                    query_display_draw() override;
                virtual void                    request_settings_update() override;
                virtual meta::plugin_format_t   plugin_format() const override;
        };
    } /* namespace lv2 */
} /* namespace lsp */

#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_WRAPPER_H_ */
