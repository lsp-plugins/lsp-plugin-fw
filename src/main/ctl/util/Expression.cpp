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
        Expression::Expression(): Property()
        {
            pListener   = NULL;
        }

        void Expression::init(ui::IWrapper *wrapper, ui::IPortListener *listener)
        {
            Property::init(wrapper);
            pListener   = listener;
        }

        void Expression::on_updated(ui::IPort *port)
        {
            if (pListener != NULL)
                pListener->notify(port);
        }
        
        float Expression::evaluate()
        {
            expr::value_t value;
            expr::init_value(&value);

            status_t res = evaluate(&value);
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

            status_t res = evaluate(idx, &value);
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

    } /* namespace tk */
} /* namespace lsp */

