/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 9 апр. 2021 г.
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

#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/plug-fw/ctl.h>

namespace lsp
{
    namespace ui
    {
        UIContext::UIContext(ui::IWrapper *wrapper, ctl::Registry *controllers, tk::Registry *widgets)
        {
            pWrapper        = wrapper;
            pControllers    = controllers;
            pWidgets        = widgets;
            pResolver       = NULL;
        }

        UIContext::~UIContext()
        {
            // Cleanup list of overlay widgets
            vOverlays.flush();

            // Destroy the stack
            for (size_t i=0, n=vStack.size(); i<n; ++i)
            {
                expr::Resolver *r = vStack.uget(i);
                if (r != NULL)
                    delete r;
            }
            vStack.flush();

            // Clear the resolver
            vRoot.set_resolver(NULL);
            if (pResolver != NULL)
            {
                delete pResolver;
                pResolver   = NULL;
            }
        }

        status_t UIContext::init()
        {
            PortResolver *r     = new PortResolver(pWrapper);
            if (r == NULL)
                return STATUS_NO_MEM;

            pResolver           = r;
            vRoot.set_resolver(r);

            return STATUS_OK;
        }

        status_t UIContext::evaluate(expr::Expression *eval, const LSPString *expr, size_t flags)
        {
            status_t res;

            // Parse expression
            if ((res = eval->parse(expr, flags)) != STATUS_OK)
            {
                lsp_error("Could not parse expression: %s", expr->get_utf8());
                return res;
            }

            // Update resolver
            expr::Resolver *resolver = eval->resolver();
            eval->set_resolver(vars());

            // Evaluate expression
            res = eval->evaluate();
            if (res != STATUS_OK)
                lsp_error("Could not evaluate expression: %s", expr->get_utf8());

            // Restore resolver
            eval->set_resolver(resolver);

            return res;
        }

        status_t UIContext::evaluate(expr::value_t *value, const LSPString *expr, size_t flags)
        {
            status_t res;
            expr::Expression e;

            // Parse expression
            if ((res = e.parse(expr, flags)) != STATUS_OK)
            {
                lsp_error("Could not parse expression: %s", expr->get_utf8());
                return res;
            }
            e.set_resolver(vars());

            // Evaluate expression
            res = e.evaluate(value);
            if (res != STATUS_OK)
                lsp_error("Could not evaluate expression: %s", expr->get_utf8());

            return res;
        }

        status_t UIContext::push_scope()
        {
            // Create variables
            expr::Variables *v = new expr::Variables();
            if (v == NULL)
                return STATUS_NO_MEM;

            // Bind resolver, push to stack and quit
            v->set_resolver(vars());
            if (!vStack.push(v))
            {
                delete v;
                return STATUS_NO_MEM;
            }

            return STATUS_OK;
        }

        status_t UIContext::pop_scope()
        {
            expr::Variables *r = NULL;
            if (!vStack.pop(&r))
                return STATUS_BAD_STATE;
            if (r != NULL)
                delete r;
            return STATUS_OK;
        }

        status_t UIContext::eval_string(LSPString *value, const LSPString *expr)
        {
            expr::value_t v;

            expr::init_value(&v);
            status_t res = evaluate(&v, expr, expr::Expression::FLAG_STRING);
            if (res != STATUS_OK)
                return res;

            if ((res = expr::cast_string(&v)) == STATUS_OK)
            {
                if (v.type == expr::VT_STRING)
                    value->swap(v.v_str);
                else
                {
                    lsp_error("Evaluation error: bad return type of expression %s", expr->get_utf8());
                    res = STATUS_BAD_TYPE;
                }
            }
            expr::destroy_value(&v);
            return res;
        }

        status_t UIContext::eval_bool(bool *value, const LSPString *expr)
        {
            expr::value_t v;
            expr::init_value(&v);

            status_t res = evaluate(&v, expr, expr::Expression::FLAG_NONE);
            if (res != STATUS_OK)
                return res;

            if ((res = expr::cast_bool(&v)) == STATUS_OK)
            {
                if (v.type == expr::VT_BOOL)
                    *value  = v.v_bool;
                else
                {
                    lsp_error("Evaluation error: bad return type of expression %s", expr->get_utf8());
                    res = STATUS_BAD_TYPE;
                }
            }
            expr::destroy_value(&v);
            return res;
        }

        status_t UIContext::eval_int(ssize_t *value, const LSPString *expr)
        {
            expr::value_t v;
            expr::init_value(&v);

            status_t res = evaluate(&v, expr, expr::Expression::FLAG_NONE);
            if (res != STATUS_OK)
                return res;

            if ((res = expr::cast_int(&v)) == STATUS_OK)
            {
                if (v.type == expr::VT_INT)
                    *value  = v.v_int;
                else
                {
                    lsp_error("Evaluation error: bad return type of expression %s", expr->get_utf8());
                    res = STATUS_BAD_TYPE;
                }
            }
            expr::destroy_value(&v);
            return res;
        }

        ctl::Widget *UIContext::create_controller(const LSPString *name)
        {
            if (name == NULL)
                return NULL;

            // Instantiate the widget
            ctl::Widget *c = NULL;

            for (ctl::Factory *f = ctl::Factory::root(); f != NULL; f = f->next())
            {
                status_t res = f->create(&c, this, name);
                if (res == STATUS_OK)
                    break;
                if (res != STATUS_NOT_FOUND)
                    return NULL;
            }

            if (c == NULL)
                return NULL;

            // Add to controller
            if (pControllers->add(c) != STATUS_OK)
            {
                delete c;
                return NULL;
            }

            // Initialize wiget
            if ((c->init()) != STATUS_OK)
                return NULL;

            return c;
        }

        status_t UIContext::set_attributes(ctl::Widget *widget, const LSPString * const *atts)
        {
            status_t res;

            // Initialize widget attributes
            for ( ; *atts != NULL; atts += 2)
            {
                // Evaluate attribute value
                LSPString avalue;
                if ((res = eval_string(&avalue, atts[1])) != STATUS_OK)
                    return res;

                // Set widget attribute
                widget->set(this, atts[0]->get_utf8(), avalue.get_utf8());
            }

            return STATUS_OK;
        }
    }
}



