/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 авг. 2021 г.
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
        Enum::Enum()
        {
            pProp       = NULL;
        }

        void Enum::on_updated(ui::IPort *port)
        {
            apply_changes();
        }

        void Enum::apply_changes()
        {
            if (pProp == NULL)
                return;

            expr::value_t value;
            expr::init_value(&value);

            if (evaluate(&value) == STATUS_OK)
            {
                if (value.type == expr::VT_STRING)
                    pProp->parse(value.v_str);
                else if (expr::cast_int(&value) == STATUS_OK)
                    pProp->set_index(value.v_int);
            }

            expr::destroy_value(&value);
        }

        void Enum::init(ui::IWrapper *wrapper, tk::Enum *prop)
        {
            Property::init(wrapper);
            pProp       = prop;
        }

        bool Enum::set(const char *prop, const char *name, const char *value)
        {
            if (strcmp(prop, name))
                return false;

            if (!Property::parse(value))
            {
                if (!Property::parse(value, expr::Expression::FLAG_STRING))
                    return false;
            }

            apply_changes();
            return true;
        }

    }
}


