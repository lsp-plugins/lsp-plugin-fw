/*
 * Copyright (C) 2025 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2025 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins
 * Created on: 18 февр. 2025 г.
 *
 * lsp-plugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_UI_EVALUATEDPORT_H_
#define LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_UI_EVALUATEDPORT_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/ui/IPort.h>
#include <lsp-plug.in/plug-fw/ui/IPortListener.h>

namespace lsp
{
    namespace ctl
    {
        class Expression;
    } /* namespace ctl */

    namespace ui
    {
        class EvaluatedPort: public IPort, public IPortListener
        {
            protected:
                IWrapper           *pWrapper;
                IPort              *pPort;
                ctl::Expression    *pExpression;

            protected:
                void                evaluate();

            public:
                explicit EvaluatedPort(IWrapper *wrapper);
                EvaluatedPort(const EvaluatedPort &) = delete;
                EvaluatedPort(EvaluatedPort &&) = delete;
                virtual ~EvaluatedPort() override;
                EvaluatedPort & operator = (const EvaluatedPort &) = delete;
                EvaluatedPort & operator = (EvaluatedPort &&) = delete;

                status_t            compile(const char *expression);
                status_t            compile(const LSPString *expression);
                void                destroy();

            public:
                virtual const char *id() const override;

            public:
                virtual void        write(const void *buffer, size_t size) override;
                virtual void       *buffer() override;
                virtual float       value() override;
                virtual float       default_value() override;
                virtual void        set_value(float value) override;
                virtual void        notify_all(size_t flags) override;
                virtual void        notify(IPort *port, size_t flags) override;
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUGIN_FW_INCLUDE_LSP_PLUG_IN_PLUG_FW_UI_EVALUATEDPORT_H_ */
