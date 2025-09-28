/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 25 мая 2025 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_HEADLESS_PORTLINK_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_HEADLESS_PORTLINK_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * Port link: defines the group of ports and dependency between ports in a group
         */
        class PortLink: public DOMController, public expr::Resolver
        {
            public:
                static const ctl_class_t metadata;

            protected:
                typedef struct binding_t
                {
                    char                   *pId;            // Binding identifier
                    float                   fOldValue;      // Old value
                    float                   fNewValue;      // New value
                    ui::IPort              *pPort;          // Associated port
                    ctl::Expression         sValue;         // Expression for computing the value
                } binding_t;

            protected:
                lltl::parray<binding_t>         vBindings;      // List of port bindings
                ctl::Expression                 sActivity;      // Activity expression
                bool                            bEnabled;       // Enabled flag
                bool                            bChanging;      // Changing mode (avoid recursive calls)

            protected:
                static void         destroy_binding(binding_t *b);
                status_t            do_resolve(expr::value_t *value, const char *name, size_t num_indexes, const ssize_t *indexes);

            protected:
                binding_t          *get_binding(const char *id);

            public:
                explicit PortLink(ui::IWrapper *wrapper);
                PortLink(const PortLink &) = delete;
                PortLink(PortLink &&) = delete;
                virtual ~PortLink() override;

                PortLink & operator = (const PortLink &) = delete;
                PortLink & operator = (PortLink &&) = delete;

            public: // expr::Resolver
                virtual status_t    resolve(expr::value_t *value, const char *name, size_t num_indexes, const ssize_t *indexes) override;
                virtual status_t    resolve(expr::value_t *value, const LSPString *name, size_t num_indexes, const ssize_t *indexes) override;
                virtual status_t    call(expr::value_t *value, const char *name, size_t num_args, const expr::value_t *args) override;
                virtual status_t    call(expr::value_t *value, const LSPString *name, size_t num_args, const expr::value_t *args) override;

            public: // ctl::Controller
                virtual status_t    init() override;

            public: // ctl::DOMController
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value) override;
                virtual void        end(ui::UIContext *ctx) override;

            public: // ui::IPortListener
                virtual void        notify(ui::IPort *port, size_t flags) override;
        };

    } /* namespace ctl */
} /* namespace lsp */


#endif /* LSP_PLUG_IN_PLUG_FW_CTL_HEADLESS_PORTLINK_H_ */
