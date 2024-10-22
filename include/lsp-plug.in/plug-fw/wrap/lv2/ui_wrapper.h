/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_LV2_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_LV2_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/core/KVTDispatcher.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>
#include <lsp-plug.in/plug-fw/core/ShmState.h>
#include <lsp-plug.in/plug-fw/meta/manifest.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/executor.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/extensions.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/ports.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/ui_ports.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/sink.h>
#include <lsp-plug.in/plug-fw/wrap/lv2/wrapper.h>

#include <lsp-plug.in/ipc/NativeExecutor.h>
#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/lltl/state.h>

namespace lsp
{
    namespace lv2
    {
        /**
         * UI wrapper for LV2 plugin format
         */
        class UIWrapper: public ui::IWrapper
        {
            private:
                lltl::parray<lv2::UIPort>   vExtPorts;
                lltl::parray<lv2::UIPort>   vMeshPorts;
                lltl::parray<lv2::UIPort>   vStreamPorts;
                lltl::parray<lv2::UIPort>   vFrameBufferPorts;
                lltl::parray<lv2::UIPort>   vOscInPorts;
                lltl::parray<lv2::UIPort>   vOscOutPorts;
                lltl::parray<meta::port_t>  vGenMetadata;   // Generated metadata

                lv2::Extensions            *pExt;           // LV2 extensions holder
                size_t                      nLatencyID;     // ID of latency port
                lv2::UIPort                *pLatency;       // Latency report port
                bool                        bConnected;     // Flag that indicates that UI is connected to DSP backend
                core::KVTStorage            sKVT;           // KVT storage
                ipc::Mutex                  sKVTMutex;      // KVT mutex
                uint8_t                    *pOscBuffer;     // OSC packet data
                meta::package_t            *pPackage;       // Package metadata

                size_t                      nPlayReq;       // Playback request
                char                       *sPlayFileName;  // Playback file name
                wsize_t                     nPlayPosition;  // Playback position
                bool                        bPlayRelease;   // Playback release flag

                lltl::state<core::ShmState> sShmState;

            protected:
                lv2::UIPort                *create_port(const meta::port_t *p, const char *postfix);

                void                        receive_atom(const LV2_Atom_Object * atom);
                void                        receive_raw_osc_packet(const void *data, size_t size);
                void                        do_destroy();

                static ssize_t              compare_ports_by_urid(const lv2::UIPort *a, const lv2::UIPort *b);
                static ssize_t              compare_abstract_ports_by_urid(const ui::IPort *a, const ui::IPort *b);
                static lv2::UIPort         *find_by_urid(lltl::parray<lv2::UIPort> &v, LV2_URID urid);
                static lv2::UIPort         *find_by_urid(lltl::parray<ui::IPort> &v, LV2_URID urid);

                static void                 shm_state_deleter(core::ShmState *state);

            protected:
                static status_t             slot_ui_hide(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_ui_show(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_ui_resize(tk::Widget *sender, void *ptr, void *data);

            public:
                explicit UIWrapper(ui::Module *ui, resource::ILoader *loader, lv2::Extensions *ext);
                virtual ~UIWrapper() override;

                virtual status_t            init(void *root_widget) override;
                virtual void                destroy() override;

            public:
                int                         resize_ui(ssize_t width, ssize_t height);

                void                        ui_activated();
                void                        ui_deactivated();

                void                        parse_raw_osc_event(osc::parse_frame_t *frame);
                void                        notify(size_t id, size_t size, size_t format, const void *buf);

                void                        send_kvt_state();
                void                        receive_kvt_state();
                void                        sync_kvt_state();

                void                        send_play_event();

            public: // ui::IWrapper
                virtual float                   ui_scaling_factor(float scaling) override;
                virtual void                    main_iteration() override;
                virtual core::KVTStorage       *kvt_lock() override;
                virtual core::KVTStorage       *kvt_trylock() override;
                virtual bool                    kvt_release() override;
                virtual const meta::package_t  *package() const override;
                virtual void                    dump_state_request() override;
                virtual status_t                play_file(const char *file, wsize_t position, bool release) override;
                virtual meta::plugin_format_t   plugin_format() const override;
                virtual bool                    window_resized(tk::Window *wnd, size_t width, size_t height) override;
                virtual const core::ShmState   *shm_state() override;
        };
    } /* namespace lv2 */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_LV2_UI_WRAPPER_H_ */
