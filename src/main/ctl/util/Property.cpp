/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 26 апр. 2021 г.
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

#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ctl
    {
        Property::PropResolver::PropResolver(Property *prop): ui::PortResolver()
        {
            pProp = prop;
        }

        status_t Property::PropResolver::on_resolved(const LSPString *name, ui::IPort *p)
        {
            return pProp->on_resolved(name, p);
        }

        status_t Property::PropResolver::resolve(expr::value_t *value, const char *name, size_t num_indexes, const ssize_t *indexes)
        {
            status_t res = pProp->sParams.resolve(value, name, num_indexes, indexes);
            if (res != STATUS_OK)
                res     = PortResolver::resolve(value, name, num_indexes, indexes);
            if (res != STATUS_OK)
            {
                expr::Resolver *vars = (pProp->pWrapper != NULL) ? pProp->pWrapper->global_variables() : NULL;
                if (vars != NULL)
                    res     = vars->resolve(value, name, num_indexes, indexes);
            }
            return res;
        }

        status_t Property::PropResolver::resolve(expr::value_t *value, const LSPString *name, size_t num_indexes, const ssize_t *indexes)
        {
            status_t res = pProp->sParams.resolve(value, name, num_indexes, indexes);
            if (res != STATUS_OK)
                res     = PortResolver::resolve(value, name, num_indexes, indexes);
            if (res != STATUS_OK)
            {
                expr::Resolver *vars = (pProp->pWrapper != NULL) ? pProp->pWrapper->global_variables() : NULL;
                if (vars != NULL)
                    res     = vars->resolve(value, name, num_indexes, indexes);
            }
            return res;
        }

        Property::Property(): IPortListener(),
            sResolver(this)
        {
            pWrapper    = NULL;
        }

        Property::~Property()
        {
            do_destroy();
        }

        void Property::init(ui::IWrapper *wrapper)
        {
            pWrapper    = wrapper;

            // Bind expression stuff
            sResolver.init(wrapper);
            sVars.set_resolver(&sResolver);
            sExpr.set_resolver(&sVars);
        }

        void Property::destroy()
        {
            do_destroy();
        }

        void Property::do_destroy()
        {
            sExpr.destroy();
            sVars.clear();
            drop_dependencies();
        }

        void Property::drop_dependencies()
        {
            for (size_t i=0, n=vDependencies.size(); i<n; ++i)
            {
                ui::IPort *p = vDependencies.uget(i);
                if (p != NULL)
                    p->unbind(this);
            }
            vDependencies.clear();
        }

        void Property::notify(ui::IPort *port, size_t flags)
        {
            if (!depends(port))
                return;

            on_updated(port);
        }

        status_t Property::evaluate(expr::value_t *value)
        {
            sVars.clear();
            drop_dependencies();
            return sExpr.evaluate(value);
        }

        status_t Property::evaluate(size_t idx, expr::value_t *value)
        {
            sVars.clear();
            drop_dependencies();
            return sExpr.evaluate(idx, value);
        }

        bool Property::parse(const char *expr, size_t flags)
        {
            sVars.clear();
            drop_dependencies();

            LSPString tmp;
            if (!tmp.set_utf8(expr))
                return false;
            if (sExpr.parse(&tmp, flags) != STATUS_OK)
                return false;

            return sExpr.evaluate() == STATUS_OK;
        }

        bool Property::parse(const LSPString *expr, size_t flags)
        {
            sVars.clear();
            drop_dependencies();

            if (sExpr.parse(expr, flags) != STATUS_OK)
                return false;

            return sExpr.evaluate() == STATUS_OK;
        }

        bool Property::parse(io::IInSequence *expr, size_t flags)
        {
            sVars.clear();
            drop_dependencies();

            if (sExpr.parse(expr, flags) != STATUS_OK)
                return false;

            return sExpr.evaluate() == STATUS_OK;
        }

        status_t Property::on_resolved(const LSPString *name, ui::IPort *p)
        {
//            lsp_trace("resolved %s -> %s = %f", name->get_utf8(), p->id(), p->value());
            // Already subscribed?
            if (vDependencies.index_of(p) >= 0)
                return STATUS_OK;

            if (!vDependencies.add(p))
                return STATUS_NO_MEM;
//            lsp_trace("bind to %s", p->id());
            p->bind(this);
            return STATUS_OK;
        }

        void Property::on_updated(ui::IPort *port)
        {
            // Nothing
        }

    } /* namespace ctl */
} /* namespace lsp */




