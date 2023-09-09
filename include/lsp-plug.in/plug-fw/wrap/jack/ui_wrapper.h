/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/plug-fw/core/KVTStorage.h>

#include <lsp-plug.in/plug-fw/wrap/jack/ui_ports.h>
#include <lsp-plug.in/plug-fw/wrap/jack/wrapper.h>
#include <lsp-plug.in/lltl/parray.h>

#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace jack
    {
        /**
         * UI wrapper for JACK
         */
        class UIWrapper: public ui::IWrapper
        {
            protected:
                plug::Module                   *pPlugin;
                jack::Wrapper                  *pWrapper;

                atomic_t                        nPosition;          // Position counter
                tk::Label                      *pJackStatus;        // Jack status
                bool                            bJackConnected;     // Jack is connected

                lltl::parray<jack::UIPort>      vSyncPorts;         // Ports for synchronization
                lltl::parray<meta::port_t>      vGenMetadata;       // Generated metadata for virtual ports

            protected:
                void                        do_destroy();

            public:
                explicit UIWrapper(jack::Wrapper *wrapper, resource::ILoader *loader, ui::Module *ui);
                virtual ~UIWrapper() override;

                virtual status_t            init(void *root_widget) override;
                virtual void                destroy() override;

            protected:
                status_t        create_port(const meta::port_t *port, const char *postfix);
                static ssize_t  compare_ports(const jack::UIPort *a, const jack::UIPort *b);
                size_t          rebuild_sorted_ports();
                void            sync_kvt(core::KVTStorage *kvt);
                void            ui_activated();
                void            ui_deactivated();
                void            set_connection_status(bool connected);

            protected:
                static status_t             slot_ui_hide(tk::Widget *sender, void *ptr, void *data);
                static status_t             slot_ui_show(tk::Widget *sender, void *ptr, void *data);

            public:
                virtual core::KVTStorage   *kvt_lock() override;
                virtual core::KVTStorage   *kvt_trylock() override;
                virtual bool                kvt_release() override;

                virtual void                dump_state_request() override;

                virtual void                main_iteration() override;

                virtual const meta::package_t      *package() const override;

                virtual status_t            play_file(const char *file, wsize_t position, bool release) override;

            public:
                /**
                 * Transfer all desired data from DSP to UI
                 * @param ts current execution timestamp
                 */
                bool                    sync(ws::timestamp_t ts);

                /**
                 * Synchronize application icon
                 */
                void                    sync_inline_display();

                /**
                 * The JACK connection has been lost
                 */
                void                    connection_lost();
        };

    } /* namespace jack */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_JACK_UI_WRAPPER_H_ */
