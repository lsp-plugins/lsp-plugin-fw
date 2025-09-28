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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        //---------------------------------------------------------------------
        CTL_FACTORY_IMPL_START(PortLink)
            if (!name->equals_ascii("ctl:link"))
                return STATUS_NOT_FOUND;

            ctl::PortLink *c    = new ctl::PortLink(context->wrapper());
            if (c == NULL)
                return STATUS_NO_MEM;

            *ctl = c;
            return STATUS_OK;
        CTL_FACTORY_IMPL_END(PortLink)

        //---------------------------------------------------------------------
        PortLink::PortLink(ui::IWrapper *wrapper):
            DOMController(wrapper)
        {
            bEnabled        = false;
            bChanging       = false;
        }

        PortLink::~PortLink()
        {
            // Drop all bindings
            for (lltl::iterator<binding_t> it = vBindings.values(); it; ++it)
                destroy_binding(it.get());

            vBindings.flush();
        }

        status_t PortLink::init()
        {
            status_t res = DOMController::init();
            if (res != STATUS_OK)
                return res;

            sActivity.init(pWrapper, this);

            return STATUS_OK;
        }

        void PortLink::destroy_binding(binding_t *b)
        {
            if (b == NULL)
                return;

            if (b->pId != NULL)
                free(b->pId);
            delete b;
        }

        PortLink::binding_t *PortLink::get_binding(const char *id)
        {
            binding_t *b;

            // Lookup for matching binding
            for (lltl::iterator<binding_t> it=vBindings.values(); it; ++it)
            {
                b = it.get();
                if ((b != NULL) && (strcmp(b->pId, id) == 0))
                    return b;
            }

            // Create and initialize new binding
            b = new binding_t;
            if (b == NULL)
                return NULL;
            lsp_finally { destroy_binding(b); };

            b->pId          = NULL;
            b->pPort        = NULL;

            b->pId          = strdup(id);
            if (b->pId == NULL)
                return NULL;
            b->sValue.init(pWrapper, this, this);

            // Put binding to the list
            if (!vBindings.add(b))
                return NULL;

            return release_ptr(b);
        }

        void PortLink::set(ui::UIContext *ctx, const char *name, const char *value)
        {
            if (strncmp(name, "id.", 3) == 0)
            {
                binding_t *b = get_binding(&name[3]);
                if (b != NULL)
                    link_port(&b->pPort, value);
            }
            else if (strncmp(name, "value.", 6) == 0)
            {
                binding_t *b = get_binding(&name[6]);
                if (b != NULL)
                    b->sValue.parse(value);
            }
            else if (strcmp(name, "activity") == 0)
                sActivity.parse(value);
        }

        void PortLink::end(ui::UIContext *ctx)
        {
            // Update activity
            if (sActivity.valid())
                bEnabled    = sActivity.evaluate() >= 0.5f;

            // Initialize current values
            for (lltl::iterator<binding_t> it = vBindings.values(); it; ++it)
            {
                binding_t *b = it.get();
                if (b->pPort != NULL)
                {
                    const float value = b->pPort->value();
                    b->fOldValue    = value;
                    b->fNewValue    = value;
                }
            }
        }

        void PortLink::notify(ui::IPort *port, size_t flags)
        {
            // Skip recursive calls.
            if (bChanging)
                return;

            // Update activity and skip other events if not active
            if (sActivity.depends(port))
                bEnabled    = sActivity.evaluate() >= 0.5f;
            if ((!bEnabled) || (!(flags & ui::PORT_USER_EDIT)))
            {
                for (lltl::iterator<binding_t> it = vBindings.values(); it; ++it)
                {
                    binding_t *b = it.get();
                    if (b->pPort == port)
                    {
                        const float value = b->pPort->value();
                        b->fOldValue    = value;
                        b->fNewValue    = value;
                    }
                }
                return;
            }

            // Now we're ready to apply changes to all ports
            bChanging = true;
            lsp_finally { bChanging = false; };

            // Compute new value for each binding depending on it's port's role
            for (lltl::iterator<binding_t> it = vBindings.values(); it; ++it)
            {
                binding_t *b = it.get();
                if ((b->pPort != NULL) && (b->pPort == port))
                    b->fNewValue    = b->pPort->value();
                else if (b->sValue.depends(port))
                    b->fNewValue    = b->sValue.evaluate();
            }

            // Update port values without notification
            for (lltl::iterator<binding_t> it = vBindings.values(); it; ++it)
            {
                binding_t *b = it.get();

                if ((b->pPort != NULL) && (b->pPort != port))
                {
                    b->pPort->set_value(b->fNewValue);
                    b->fNewValue = b->pPort->value();
                    lsp_trace("set %s = %f", b->pPort->id(), b->fNewValue);
                }
                b->fOldValue = b->fNewValue;
            }

            // Notify changed ports
            for (lltl::iterator<binding_t> it = vBindings.values(); it; ++it)
            {
                binding_t *b = it.get();
                if ((b->pPort != NULL) && (b->pPort != port))
                    b->pPort->notify_all(flags);
            }
        }

        status_t PortLink::do_resolve(expr::value_t *value, const char *name, size_t num_indexes, const ssize_t *indexes)
        {
            if ((name == NULL) || (num_indexes > 0))
                return STATUS_NOT_FOUND;
            if (strncmp(name, "_old_", 5) != 0)
                return STATUS_NOT_FOUND;

            // Lookup for previous values
            name += 5;
            for (lltl::iterator<binding_t> it = vBindings.values(); it; ++it)
            {
                binding_t *b = it.get();
                if ((b->pId != NULL) && (strcmp(b->pId, name) == 0))
                {
                    expr::set_value_float(value, b->fOldValue);
                    lsp_trace("resolve _old_%s -> %f", b->pId, b->fOldValue);
                    return STATUS_OK;
                }
            }

            return STATUS_NOT_FOUND;
        }

        status_t PortLink::resolve(expr::value_t *value, const char *name, size_t num_indexes, const ssize_t *indexes)
        {
            status_t res    = do_resolve(value, name, num_indexes, indexes);
            if (res == STATUS_NOT_FOUND)
            {
                expr::Resolver *vars = (pWrapper != NULL) ? pWrapper->global_variables() : NULL;
                if (vars != NULL)
                    res     = vars->resolve(value, name, num_indexes, indexes);
            }

            return res;
        }

        status_t PortLink::resolve(expr::value_t *value, const LSPString *name, size_t num_indexes, const ssize_t *indexes)
        {
            status_t res    = do_resolve(value, name->get_utf8(), num_indexes, indexes);
            if (res == STATUS_NOT_FOUND)
            {
                expr::Resolver *vars = (pWrapper != NULL) ? pWrapper->global_variables() : NULL;
                if (vars != NULL)
                    res     = vars->resolve(value, name, num_indexes, indexes);
            }

            return res;
        }

        status_t PortLink::call(expr::value_t *value, const char *name, size_t num_args, const expr::value_t *args)
        {
            status_t res = Resolver::call(value, name, num_args, args);
            if (res == STATUS_NOT_FOUND)
            {
                expr::Resolver *vars = (pWrapper != NULL) ? pWrapper->global_variables() : NULL;
                if (vars != NULL)
                    res     = vars->call(value, name, num_args, args);
            }
            return res;
        }

        status_t PortLink::call(expr::value_t *value, const LSPString *name, size_t num_args, const expr::value_t *args)
        {
            status_t res = Resolver::call(value, name, num_args, args);
            if (res == STATUS_NOT_FOUND)
            {
                expr::Resolver *vars = (pWrapper != NULL) ? pWrapper->global_variables() : NULL;
                if (vars != NULL)
                    res     = vars->call(value, name, num_args, args);
            }
            return res;
        }

    } /* namespace ctl */
} /* namespace lsp */



