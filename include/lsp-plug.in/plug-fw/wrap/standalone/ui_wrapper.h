/*
 * Copyright (C) 2026 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2026 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 29 янв. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>

#include <lsp-plug.in/lltl/parray.h>
#include <lsp-plug.in/plug-fw/wrap/standalone/ui_ports.h>
#include <lsp-plug.in/plug-fw/wrap/standalone/wrapper.h>

#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace standalone
    {
        /**
         * UI wrapper for Standalone versions of plugins
         */
        class UIWrapper: public ui::IWrapper
        {
            protected:
                plug::Module                   *pPlugin;
                standalone::Wrapper            *pWrapper;

                atomic_t                        nPosition;          // Position counter
                const core::AudioBackendInfo   *pConnBackend;       // Currently used audio backend
                bool                            bConnConnected;     // Audio backend is connected
                tk::Label                      *pConnStatus;        // Audio backend connection status
                tk::Label                      *pConnName;          // Audio backend name
                tk::Widget                     *pConnIndicatorPanel;// Audio backend indicator panel

                lltl::parray<standalone::UIPort>vSyncPorts;         // Ports for synchronization
                lltl::parray<meta::port_t>      vGenMetadata;       // Generated metadata for virtual ports

            protected:
                void                        do_destroy();

            public:
                explicit UIWrapper(standalone::Wrapper *wrapper, resource::ILoader *loader, ui::Module *ui);
                virtual ~UIWrapper() override;

                virtual status_t                    init(void *root_widget) override;
                virtual void                        destroy() override;

            protected:
                status_t                            create_port(const meta::port_t *port, const char *postfix);
                static ssize_t                      compare_ports(const standalone::UIPort *a, const standalone::UIPort *b);
                size_t                              rebuild_sorted_ports();
                void                                sync_kvt(core::KVTStorage *kvt);
                void                                ui_activated();
                void                                ui_deactivated();
                void                                sync_connection_status(bool connected);
                void                                update_connection_name();

            protected:
                virtual void                        visual_schema_reloaded(const tk::StyleSheet *sheet) override;

            protected:
                static status_t                     slot_ui_hide(tk::Widget *sender, void *ptr, void *data);
                static status_t                     slot_ui_show(tk::Widget *sender, void *ptr, void *data);

            public: // ui::IWrapper
                virtual core::KVTStorage           *kvt_lock() override;
                virtual core::KVTStorage           *kvt_trylock() override;
                virtual bool                        kvt_release() override;
                virtual void                        dump_state_request() override;
                virtual void                        main_iteration() override;
                virtual const meta::package_t      *package() const override;
                virtual status_t                    play_file(const char *file, wsize_t position, bool release) override;
                virtual meta::plugin_format_t       plugin_format() const override;
                virtual status_t                    export_settings(config::Serializer *s, size_t flags, const io::Path *basedir = NULL) override;
                virtual status_t                    import_settings(config::PullParser *parser, size_t flags, const io::Path *basedir = NULL) override;
                virtual const core::ShmState       *shm_state() override;
                virtual status_t                    enumerate_backends(core::AudioBackendInfoList & list) override;
                virtual status_t                    select_backend(const LSPString & name) override;

                using ui::IWrapper::export_settings;
                using ui::IWrapper::import_settings;
                using ui::IWrapper::select_backend;

            public:
                /**
                 * Transfer all desired data from DSP to UI
                 * @param ts current execution timestamp
                 */
                bool                                sync(ws::timestamp_t ts);

                /**
                 * Synchronize application icon
                 */
                void                                sync_inline_display();

                /**
                 * Change the status of connection
                 * @param backend currently used audio backend
                 * @param connected connection established flag
                 */
                void                                set_connection_status(const core::AudioBackendInfo * backend, bool connected);

                /**
                 * Select UI schema
                 * @param name schema name
                 * @return status of operation
                 */
                status_t                            select_ui_schema(const char *name);

                /**
                 * Select UI schema
                 * @param name schema name
                 * @return status of operation
                 */
                status_t                            select_ui_schema(const LSPString *name);

                /**
                 * Select UI schema
                 * @param name schema name
                 * @return status of operation
                 */
                status_t                            select_ui_schema(const LSPString & name);

        };

    } /* namespace standalone */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_STANDALONE_UI_WRAPPER_H_ */
