/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 5 мар. 2023 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_UI_PROXYPORT_H_
#define LSP_PLUG_IN_PLUG_FW_UI_PROXYPORT_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

namespace lsp
{
    namespace ui
    {
        class ProxyPort: public ui::IPort, public ui::IPortListener
        {
            protected:
                ui::IPort      *pPort;      // Wrapped/proxied port
                char           *sID;        // Identifier of the port
                meta::port_t    sMetadata;  // Port metadata

            public:
                explicit ProxyPort();
                virtual ~ProxyPort() override;

            public:
                /**
                 * Initialize proxy port
                 * @param id port identifier
                 * @param proxied initially proxied port
                 * @return status of operation
                 */
                status_t            init(const char *id, ui::IPort *proxied);

                /**
                 * Change currently proxied port to another one
                 * @param proxied proxied port
                 */
                void                set_proxy_port(ui::IPort *proxied);

            public:
                virtual void        write(const void *buffer, size_t size) override;
                virtual void        write(const void *buffer, size_t size, size_t flags) override;
                virtual void       *buffer() override;
                virtual float       value() override;
                virtual float       default_value() override;
                virtual void        set_default() override;
                virtual void        set_value(float value) override;
                virtual void        set_value(float value, size_t flags) override;
                virtual void        notify_all(size_t flags) override;
                virtual void        sync_metadata() override;
                virtual void        sync_metadata(IPort *port) override;
                virtual const char *id() const override;

            public:
                virtual void        notify(IPort *port, size_t flags) override;
        };
    } /* namespace ui */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_UI_PROXYPORT_H_ */
