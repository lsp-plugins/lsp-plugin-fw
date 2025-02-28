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

#include <lsp-plug.in/plug-fw/ctl.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/meta/types.h>

namespace lsp
{
    namespace ui
    {
        static const meta::port_t fake_metadata =
        {
            "", "",
            meta::U_NONE, meta::R_CONTROL, meta::F_LOWER | meta::F_UPPER | meta::F_STEP,
            0.0f, 0.0f, 0.0f, 0.0f,
            NULL, NULL, NULL
        };

        EvaluatedPort::EvaluatedPort(IWrapper *wrapper): IPort(NULL)
        {
            pWrapper        = wrapper;
            pPort           = NULL;
            pExpression     = NULL;
        }

        EvaluatedPort::~EvaluatedPort()
        {
            destroy();
        }

        void EvaluatedPort::evaluate()
        {
            if (pExpression == NULL)
                return;

            LSPString port_id;
            status_t res    = pExpression->evaluate_string(&port_id);

            if (res != STATUS_OK)
            {
                if (pPort != NULL)
                {
                    pPort->unbind(this);
                    pPort       = NULL;
                }
                return;
            }

            // Successfully evaluated
            ui::IPort *port     = pWrapper->port(&port_id);
            if (port == pPort)
                return;

            // Change binding to the port
            if (pPort != NULL)
            {
                pPort->unbind(this);
                pPort       = NULL;
            }

            if (port != NULL)
            {
                port->bind(this);

                // Update port pointer and notify listeners for metadata change
                pPort           = port;
                pMetadata       = pPort->metadata();
                pPort->sync_metadata();
            }
            else
                pMetadata       = &fake_metadata;
        }

        void EvaluatedPort::destroy()
        {
            if (pPort != NULL)
            {
                pPort->unbind(this);
                pPort  = NULL;
            }
            if (pExpression != NULL)
            {
                pExpression->destroy();
                delete pExpression;
                pExpression = NULL;
            }

            pMetadata       = NULL;
        }

        status_t EvaluatedPort::compile(const char *expression)
        {
            // Create expression if needed
            if (pExpression == NULL)
            {
                pExpression         = new ctl::Expression();
                if (pExpression == NULL)
                    return STATUS_NO_MEM;
                pExpression->init(pWrapper, this);
            }

            // Parse result
            if (!pExpression->parse(expression))
                return STATUS_INVALID_VALUE;

            evaluate();
            return STATUS_OK;
        }

        status_t EvaluatedPort::compile(const LSPString *expression)
        {
            // Create expression if needed
            if (pExpression == NULL)
            {
                pExpression         = new ctl::Expression();
                if (pExpression == NULL)
                    return STATUS_NO_MEM;
                pExpression->init(pWrapper, this);
            }

            // Parse result
            if (!pExpression->parse(expression))
                return STATUS_INVALID_VALUE;

            evaluate();
            return STATUS_OK;
        }

        void EvaluatedPort::write(const void *buffer, size_t size)
        {
            if (pPort != NULL)
                pPort->write(buffer, size);
        }

        void *EvaluatedPort::buffer()
        {
            return (pPort != NULL) ? pPort->buffer() : NULL;
        }

        float EvaluatedPort::value()
        {
            return (pPort != NULL) ? pPort->value() : 0.0f;
        }

        float EvaluatedPort::default_value()
        {
            return (pPort != NULL) ? pPort->default_value() : 0.0f;
        }

        void EvaluatedPort::set_value(float value)
        {
            if (pPort != NULL)
                pPort->set_value(value);
        }

        void EvaluatedPort::notify_all(size_t flags)
        {
            if (pPort != NULL)
                pPort->notify_all(flags); // We will receive notify() as subscribers
            else
                IPort::notify_all(flags);
        }

        const char *EvaluatedPort::id() const
        {
            return (pPort != NULL) ? pPort->id() : "";
        }

        void EvaluatedPort::notify(IPort *port, size_t flags)
        {
            if ((pExpression != NULL) && (pExpression->depends(port)))
            {
                evaluate();
                IPort::notify_all(flags);
                return;
            }

            // Proxy notify() event only for active port
            if ((port == NULL) || (port != pPort))
                return;

            // Notify all subscribers
            IPort::notify_all(flags);
        }
    } /* namespace ui */
} /* namespace lsp */


