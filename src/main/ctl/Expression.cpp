/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 11 апр. 2021 г.
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
        Expression::ExprResolver::ExprResolver(Expression *expr): ui::PortResolver()
        {
            pExpr = expr;
        }

        status_t Expression::ExprResolver::on_resolved(const LSPString *name, ui::IPort *p)
        {
            return pExpr->on_resolved(name, p);
        }

        status_t Expression::ExprResolver::resolve(expr::value_t *value, const char *name, size_t num_indexes, const ssize_t *indexes)
        {
            status_t res = pExpr->sParams.resolve(value, name, num_indexes, indexes);
            if (res != STATUS_OK)
                res     = PortResolver::resolve(value, name, num_indexes, indexes);
            return res;
        }

        status_t Expression::ExprResolver::resolve(expr::value_t *value, const LSPString *name, size_t num_indexes, const ssize_t *indexes)
        {
            status_t res = pExpr->sParams.resolve(value, name, num_indexes, indexes);
            if (res != STATUS_OK)
                res     = PortResolver::resolve(value, name, num_indexes, indexes);
            return res;
        }

        Expression::Expression(): IPortListener(),
            sResolver(this)
        {
            pWrapper    = NULL;
            pListener   = NULL;
        }

        Expression::~Expression()
        {
            do_destroy();
        }

        void Expression::init(ui::IWrapper *wrapper, ui::IPortListener *listener)
        {
            pWrapper    = wrapper;
            pListener   = listener;

            // Bind expression stuff
            sResolver.init(wrapper);
            sVars.set_resolver(&sResolver);
            sExpr.set_resolver(&sVars);
        }

        void Expression::destroy()
        {
            do_destroy();
        }

        void Expression::do_destroy()
        {
            sExpr.destroy();
            sVars.clear();
            drop_dependencies();
        }

        void Expression::drop_dependencies()
        {
            for (size_t i=0, n=vDependencies.size(); i<n; ++i)
            {
                ui::IPort *p = vDependencies.uget(i);
                if (p != NULL)
                    p->unbind(this);
            }
            vDependencies.clear();
        }

        void Expression::notify(ui::IPort *port)
        {
            if (!depends(port))
                return;
            if (pListener == NULL)
                return;
            pListener->notify(port);
        }

        float Expression::evaluate()
        {
            expr::value_t value;
            expr::init_value(&value);

            sVars.clear();
            drop_dependencies();
            status_t res = sExpr.evaluate(&value);
            if (res != STATUS_OK)
            {
                expr::destroy_value(&value);
                return 0.0f;
            }

            float fval;
            expr::cast_float(&value);
            fval = (value.type == expr::VT_FLOAT) ? value.v_float : 0.0f;
            expr::destroy_value(&value);
            return fval;
        }

        float Expression::evaluate(size_t idx)
        {
            expr::value_t value;
            expr::init_value(&value);

            sVars.clear();
            drop_dependencies();
            status_t res = sExpr.evaluate(idx, &value);
            if (res != STATUS_OK)
            {
                expr::destroy_value(&value);
                return 0.0f;
            }

            float fval;
            expr::cast_float(&value);
            fval = (value.type == expr::VT_FLOAT) ? value.v_float : 0.0f;
            expr::destroy_value(&value);
            return fval;
        }

        float Expression::result(size_t idx)
        {
            expr::value_t value;
            expr::init_value(&value);

            status_t res = sExpr.result(&value, idx);
            if (res != STATUS_OK)
            {
                expr::destroy_value(&value);
                return 0.0f;
            }

            float fval;
            expr::cast_float(&value);
            fval = (value.type == expr::VT_FLOAT) ? value.v_float : 0.0f;
            expr::destroy_value(&value);
            return fval;
        }

        bool Expression::parse(const char *expr, size_t flags)
        {
            sVars.clear();
            drop_dependencies();

            LSPString tmp;
            if (!tmp.set_utf8(expr))
                return false;
            if (sExpr.parse(&tmp, flags) != STATUS_OK)
                return false;

            status_t res = sExpr.evaluate();
            #ifdef LSP_TRACE
                if (res == STATUS_OK)
                    sText.swap(&tmp);
            #endif
            return res;
        }

        bool Expression::parse(const LSPString *expr, size_t flags)
        {
            sVars.clear();
            drop_dependencies();

            if (sExpr.parse(expr, flags) != STATUS_OK)
                return false;

            status_t res = sExpr.evaluate();
            #ifdef LSP_TRACE
                if (res == STATUS_OK)
                    res = (sText.set(expr) ? STATUS_OK : STATUS_NO_MEM);
            #endif
            return res;
        }

        bool Expression::parse(io::IInSequence *expr, size_t flags)
        {
            if (sExpr.parse(expr, flags) != STATUS_OK)
                return false;
            return evaluate() == STATUS_OK;
        }

        status_t Expression::on_resolved(const LSPString *name, ui::IPort *p)
        {
            // lsp_trace("[%s] resolved %s -> %s = %f", sText.get_utf8(), name->get_utf8(), p->id(), p->get_value());
            // Already subscribed?
            if (vDependencies.index_of(p) >= 0)
                return STATUS_OK;

            if (!vDependencies.add(p))
                return STATUS_NO_MEM;
            // lsp_trace("bind to %s", p->id());
            p->bind(this);
            return STATUS_OK;
        }

    } /* namespace tk */
} /* namespace lsp */

