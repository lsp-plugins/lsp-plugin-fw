/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
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
                ProxyPort(const ProxyPort &) = delete;
                ProxyPort(ProxyPort &&) = delete;
                virtual ~ProxyPort() override;

                ProxyPort & operator = (const ProxyPort &) = delete;
                ProxyPort & operator = (ProxyPort &&) = delete;

            public:
                /**
                 * Initialize proxy port
                 * @param id port identifier
                 * @param proxied initially proxied port
                 * @param meta alternative port metadata template
                 * @return status of operation
                 */
                status_t            init(const char *id, ui::IPort *proxied, const meta::port_t *meta = NULL);

                /**
                 * Change currently proxied port to another one
                 * @param proxied proxied port
                 */
                void                set_proxy_port(ui::IPort *proxied, const meta::port_t *meta = NULL);

            public:
                ui::IPort          *proxied();
                const meta::port_t *proxied_metadata() const;

            public: // ui::IPort
                virtual void        write(const void *buffer, size_t size) override;
                virtual void        write(const void *buffer, size_t size, size_t flags) override;
                virtual void       *buffer() override;
                virtual float       value() override;
                virtual float       default_value() override;
                virtual void        set_default() override;
                virtual void        set_value(float value) override;
                virtual void        set_value(float value, size_t flags) override;
                virtual void        notify_all(size_t flags) override;
                virtual bool        begin_edit() override;
                virtual bool        end_edit() override;
                virtual bool        editing() const override;
                virtual void        sync_metadata() override;
                virtual void        sync_metadata(IPort *port) override;
                virtual const char *id() const override;

            public: // ui::IPortListener
                virtual void        notify(IPort *port, size_t flags) override;

            public:
                virtual float       from_value(float value);
                virtual float       to_value(float value);
        };
    } /* namespace ui */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_UI_PROXYPORT_H_ */
